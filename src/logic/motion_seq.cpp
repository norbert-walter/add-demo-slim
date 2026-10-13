/**
 * @file motion_seq.cpp
 * @brief Pure, non-blocking motion sequencer (FROZEN CONTRACT V1.5.7 §8).
 *
 * Sprint L3 implementation: every function declared in motion_seq.h.
 * Pure C++17: no Arduino, no ESP-IDF, no heap, no static mutable state.
 */
#include "motion_seq.h"

#include <cmath>      // std::isfinite
#include <cstdint>

// ---------------------------------------------------------------------------
// Static helpers (private to this translation unit)
// ---------------------------------------------------------------------------

static void setPhase(MotionSeq& s, SeqPhase phase, uint32_t now) {
    s.phase          = phase;
    s.phase_start_ms = now;
}

static DrvRequest moveTo(MotionSeq& s, int32_t target_steps, uint32_t speed_hz) {
    s.target_steps = target_steps;
    DrvRequest r{};
    r.op           = DrvOp::MOVE_TO;
    r.target_steps = target_steps;
    r.speed_hz     = speed_hz;
    return r;
}

static bool isHoming(SeqPhase p) {
    return p == SeqPhase::HOME_SEEK ||
           p == SeqPhase::HOME_LATCH  ||
           p == SeqPhase::HOME_PULLOFF;
}

// ---------------------------------------------------------------------------
// Conversions
// ---------------------------------------------------------------------------

int32_t mmToSteps(float mm, const MotionConfig& c) {
    return (int32_t)lroundf(mm * c.steps_per_mm);
}

float stepsToMm(int32_t steps, const MotionConfig& c) {
    return static_cast<float>(static_cast<double>(steps) / static_cast<double>(c.steps_per_mm));
}

uint32_t mmPerSecToHz(float mm_s, const MotionConfig& c) {
    return (uint32_t)lroundf(mm_s * c.steps_per_mm);
}

// ---------------------------------------------------------------------------
// motionStartHoming
// ---------------------------------------------------------------------------

ApiError motionStartHoming(MotionSeq& s, const MotionInputs& in,
                           const MotionConfig& c, DrvRequest& out) {
    out.op = DrvOp::NONE;

    if (s.phase != SeqPhase::IDLE) return ApiError::BUSY;

    /* Always clear homed when starting a new homing sequence. */
    s.homed          = false;
    s.error_latched  = false;
    s.homing_start_ms = in.now_ms;

    if (in.limit_bwd) {
        setPhase(s, SeqPhase::HOME_LATCH, in.now_ms);
        out.op     = DrvOp::ZERO_HERE;
    } else {
        setPhase(s, SeqPhase::HOME_SEEK, in.now_ms);
        out.op         = DrvOp::RUN_BACKWARD;
        out.speed_hz   = mmPerSecToHz(c.homing_speed, c);
    }

    return ApiError::OK;
}

// ---------------------------------------------------------------------------
// motionStartMove
// ---------------------------------------------------------------------------

ApiError motionStartMove(MotionSeq& s, float pos_mm, float speed_mm_s,
                         const MotionInputs& in, const MotionConfig& c,
                         DrvRequest& out) {
    out.op = DrvOp::NONE;

    if (s.phase != SeqPhase::IDLE) return ApiError::BUSY;
    if (!s.homed) return ApiError::PRECONDITION;

    /* Range checks using !(lo <= x && x <= hi) — NaN-safe. */
    if (!(0.0f <= pos_mm && pos_mm <= c.axis_max_mm))  return ApiError::BAD_PARAM;
    if (!(c.min_speed <= speed_mm_s && speed_mm_s <= c.max_speed))
        return ApiError::BAD_PARAM;

    s.error_latched = false;

    int32_t target = mmToSteps(pos_mm, c);
    setPhase(s, SeqPhase::MOVE, in.now_ms);
    out = moveTo(s, target, mmPerSecToHz(speed_mm_s, c));
    return ApiError::OK;
}
// ---------------------------------------------------------------------------
// motionStartTraverse
// ---------------------------------------------------------------------------

ApiError motionStartTraverse(MotionSeq& s, float start_mm, float end_mm,
                             float speed_mm_s, const MotionInputs& in,
                             const MotionConfig& c, DrvRequest& out) {
    out.op = DrvOp::NONE;

    if (s.phase != SeqPhase::IDLE) return ApiError::BUSY;
    if (!s.homed) return ApiError::PRECONDITION;

    /* V1.5: start and end must lie in the measuring zone (> measure_min_mm,
       axis_max_mm]. */
    if (!(c.measure_min_mm < start_mm && start_mm <= c.axis_max_mm))
        return ApiError::BAD_PARAM;
    if (!(c.measure_min_mm < end_mm   && end_mm   <= c.axis_max_mm))
        return ApiError::BAD_PARAM;

    /* Distance must be at least 1 mm. */
    float dist = end_mm - start_mm;
    if (dist < 0) dist = -dist;
    if (!(1.0f <= dist)) return ApiError::BAD_PARAM;

    /* Speed range check (NaN-safe). */
    if (!(c.min_speed <= speed_mm_s && speed_mm_s <= c.max_speed))
        return ApiError::BAD_PARAM;

    s.error_latched = false;
    s.mode          = OpMode::TRAVERSE;

    int32_t start_steps = mmToSteps(start_mm, c);
    setPhase(s, SeqPhase::TRAV_APPROACH, in.now_ms);
    out = moveTo(s, start_steps, mmPerSecToHz(c.approach_speed, c));

    s.trav_end_steps = mmToSteps(end_mm, c);
    s.trav_speed_hz  = mmPerSecToHz(speed_mm_s, c);

    return ApiError::OK;
}

// ---------------------------------------------------------------------------
// motionStartMeasure
// ---------------------------------------------------------------------------

ApiError motionStartMeasure(MotionSeq& s, float pos_mm,
                            const MotionInputs& in, const MotionConfig& c,
                            DrvRequest& out) {
    out.op = DrvOp::NONE;

    if (s.phase != SeqPhase::IDLE) return ApiError::BUSY;
    if (!s.homed) return ApiError::PRECONDITION;

    /* V1.5: pos must lie in the measuring zone (> measure_min_mm, axis_max_mm]. */
    if (!(c.measure_min_mm < pos_mm && pos_mm <= c.axis_max_mm))
        return ApiError::BAD_PARAM;

    s.error_latched = false;

    int32_t target = mmToSteps(pos_mm, c);
    setPhase(s, SeqPhase::MEAS_APPROACH, in.now_ms);
    out = moveTo(s, target, mmPerSecToHz(c.approach_speed, c));
    s.mode = OpMode::MEASURE;

    return ApiError::OK;
}

// ---------------------------------------------------------------------------
// motionStop
// ---------------------------------------------------------------------------

void motionStop(MotionSeq& s, DrvRequest& out) {
    out.op = DrvOp::NONE;

    if (s.phase == SeqPhase::TRAV_RUN || s.phase == SeqPhase::MEAS_HOLD) {
        s.ev.acq_ended = true;
    }

    if (s.phase != SeqPhase::IDLE) {
        out.op = DrvOp::STOP;
        setPhase(s, SeqPhase::IDLE, 0);
        s.mode = OpMode::NONE;
    }
}

// ---------------------------------------------------------------------------
// motionEmergencyStop
// ---------------------------------------------------------------------------

void motionEmergencyStop(MotionSeq& s, const MotionInputs& in, DrvRequest& out) {
    out.op = DrvOp::FORCE_STOP;

    /* Lose reference if we were moving or the driver was still running. */
    if (s.phase != SeqPhase::IDLE || in.drv_running) {
        s.homed = false;
    }

    if (s.phase == SeqPhase::TRAV_RUN || s.phase == SeqPhase::MEAS_HOLD) {
        s.ev.acq_ended = true;
    }

    setPhase(s, SeqPhase::IDLE, 0);
    s.mode = OpMode::NONE;
}
// ---------------------------------------------------------------------------
// motionTick
// ---------------------------------------------------------------------------

DrvRequest motionTick(MotionSeq& s, const MotionInputs& in, const MotionConfig& c) {
    DrvRequest out{};

    /* "Arrived": driver stopped AND (at target OR settled long enough). */
    bool arrived = !in.drv_running &&
                   (in.drv_pos_steps == s.target_steps ||
                    (uint32_t)(in.now_ms - s.phase_start_ms) >= c.settle_ms);

    /* ---- (1) Homing timeout -------------------------------------------- */
    if (isHoming(s.phase) &&
        (uint32_t)(in.now_ms - s.homing_start_ms) > c.homing_timeout_ms) {
        out.op     = DrvOp::FORCE_STOP;
        setPhase(s, SeqPhase::IDLE, in.now_ms);
        s.ev.homing_failed   = true;
        s.error_latched      = true;
        return out;
    }

    /* ---- (2) Unexpected limit switch (only while drv_running) ----------- */
    bool unexpected = false;
    if (in.drv_running) {
        /* Forward limit is always unexpected. */
        if (in.limit_fwd) unexpected = true;
        /* Backward limit is unexpected only outside homing phases. */
        else if (!isHoming(s.phase) && in.limit_bwd) unexpected = true;
    }

    if (unexpected) {
        out.op     = DrvOp::FORCE_STOP;
        s.homed              = false;
        SeqPhase old_phase   = s.phase;
        setPhase(s, SeqPhase::IDLE, in.now_ms);
        s.mode               = OpMode::NONE;
        s.error_latched      = true;

        if (isHoming(old_phase)) {
            /* Original phase was a HOME_* phase. */
            s.ev.homing_failed   = true;
        } else {
            s.ev.motor_fault     = true;
        }

        if (old_phase == SeqPhase::TRAV_RUN || old_phase == SeqPhase::MEAS_HOLD) {
            s.ev.acq_ended = true;
        }

        return out;
    }

    /* ---- (3) Phase transitions ----------------------------------------- */
    switch (s.phase) {

        /* ===== Homing sequence ========================================== */
        case SeqPhase::HOME_SEEK:
            if (in.limit_bwd) {
                out.op     = DrvOp::ZERO_HERE;
                setPhase(s, SeqPhase::HOME_LATCH, in.now_ms);
            }
            break;

        case SeqPhase::HOME_LATCH:
            out = moveTo(s, mmToSteps(c.pulloff_mm, c),
                                   mmPerSecToHz(c.homing_speed, c));
            setPhase(s, SeqPhase::HOME_PULLOFF, in.now_ms);
            break;

        case SeqPhase::HOME_PULLOFF:
            if (arrived) {
                out.op     = DrvOp::ZERO_HERE;
                s.homed          = true;
                setPhase(s, SeqPhase::IDLE, in.now_ms);
                s.ev.homing_done   = true;
            }
            break;

        /* ===== MOVE ===================================================== */
        case SeqPhase::MOVE:
            if (arrived) {
                setPhase(s, SeqPhase::IDLE, in.now_ms);
            }
            break;

        /* ===== TRAV_APPROACH ============================================ */
        case SeqPhase::TRAV_APPROACH:
            if (arrived) {
                out = moveTo(s, s.trav_end_steps, s.trav_speed_hz);
                setPhase(s, SeqPhase::TRAV_RUN, in.now_ms);
                s.ev.acq_started = true;
            }
            break;

        /* ===== TRAV_RUN ================================================= */
        case SeqPhase::TRAV_RUN:
            if (arrived) {
                setPhase(s, SeqPhase::IDLE, in.now_ms);
                s.mode               = OpMode::NONE;
                s.ev.acq_ended       = true;
            }
            break;

        /* ===== MEAS_APPROACH ============================================ */
        case SeqPhase::MEAS_APPROACH:
            if (arrived) {
                setPhase(s, SeqPhase::MEAS_HOLD, in.now_ms);
                s.ev.acq_started   = true;
            }
            break;

        /* ===== MEAS_HOLD — stand still until STOP ------------------------ */
        case SeqPhase::MEAS_HOLD:
            break;

        /* ===== IDLE — nothing to do ------------------------------------ */
        case SeqPhase::IDLE:
            break;
    }

    return out;
}
// ---------------------------------------------------------------------------
// motionTakeEvents
// ---------------------------------------------------------------------------

MotionEvents motionTakeEvents(MotionSeq& s) {
    MotionEvents ev = s.ev;
    s.ev = MotionEvents{};   /* clear all pending events */
    return ev;
}

// ---------------------------------------------------------------------------
// motionAcquiring
// ---------------------------------------------------------------------------

bool motionAcquiring(const MotionSeq& s) {
    return s.phase == SeqPhase::TRAV_RUN ||
           s.phase == SeqPhase::MEAS_HOLD;
}

// ---------------------------------------------------------------------------
// motionStateOf
// ---------------------------------------------------------------------------

MotionState motionStateOf(const MotionSeq& s) {
    if (isHoming(s.phase)) return MotionState::HOMING;
    if (s.phase != SeqPhase::IDLE) return MotionState::MOVING;
    if (s.error_latched)     return MotionState::ERROR;
    return MotionState::IDLE;
}