/**
 * @file motion_seq.h
 * @brief Pure, non-blocking motion sequencer (FROZEN CONTRACT V1.4 §8).
 *
 * V1.3: ALL speeds are mm/s (API, parameters, /proc, MotionConfig).
 *
 * Implementation: src/logic/motion_seq.cpp (Sprint S2.3).
 *
 * The sequencer never talks to hardware. Every call returns at most ONE
 * DrvRequest which the control task hands to the stepper driver adapter
 * (hal/motion_drv.h). The driver reports back through MotionInputs on the next
 * cycle. All positions inside are in steps; the API works in mm.
 *
 * Phase overview:
 *   IDLE
 *   HOME_SEEK     run backward until limit_bwd       -> request ZERO_HERE
 *   HOME_LATCH    (one tick) after ZERO_HERE          -> request MOVE_TO(+pulloff)
 *   HOME_PULLOFF  wait until arrived                  -> request ZERO_HERE, homed = true
 *   MOVE          MOVE command, wait until arrived    -> IDLE
 *   TRAV_APPROACH move to start at approach speed     -> TRAV_RUN
 *   TRAV_RUN      move to end at traverse speed       -> IDLE (mode ends)
 *   MEAS_APPROACH move to measure position            -> MEAS_HOLD
 *   MEAS_HOLD     stand still, measuring              (until STOP)
 */
#pragma once

#include "types.h"

struct MotionConfig {
    float    steps_per_mm      = 3200.0f / 40.77f; ///< V1.4: 200 steps * 16 microsteps / 40.77 mm (measured)
    float    axis_max_mm       = 242.0f;  ///< V1.4: forward end switch at axis 244.3 mm (measured), 2.3 mm margin
    float    park_pos_mm       = 20.0f;   ///< V1.2: park zone limit; measuring positions must be > park
    float    park_target_mm    = 2.0f;    ///< V1.2: destination of the PARK command
    float    homing_speed      = 5.0f;    ///< mm/s (V1.3), also pull-off
    float    pulloff_mm        = 2.0f;
    uint32_t homing_timeout_ms = 70000;   ///< whole homing sequence (246 mm at 5 mm/s = 49 s + margin)
    float    approach_speed    = 10.0f;   ///< mm/s, normal speed: MOVE/PARK/SERVICE default and approach moves
    float    min_speed         = 1.0f;    ///< mm/s
    float    max_speed         = 20.0f;   ///< mm/s
    uint32_t settle_ms         = 100;     ///< see motionTick "arrival"
};

enum class DrvOp : uint8_t {
    NONE = 0,
    MOVE_TO,       ///< absolute target_steps at speed_hz
    RUN_BACKWARD,  ///< continuous run towards negative at speed_hz
    STOP,          ///< decelerated stop
    FORCE_STOP,    ///< immediate stop, position may be lost
    ZERO_HERE      ///< immediate stop and set current position = 0
};

struct DrvRequest {
    DrvOp    op           = DrvOp::NONE;
    int32_t  target_steps = 0;
    uint32_t speed_hz     = 0;   ///< steps per second
};

/// Feedback from driver and limit switches for this cycle.
struct MotionInputs {
    uint32_t now_ms        = 0;
    bool     limit_fwd     = false;
    bool     limit_bwd     = false;
    bool     drv_running   = false;
    int32_t  drv_pos_steps = 0;
};

enum class SeqPhase : uint8_t {
    IDLE = 0, HOME_SEEK, HOME_LATCH, HOME_PULLOFF, MOVE,
    TRAV_APPROACH, TRAV_RUN, MEAS_APPROACH, MEAS_HOLD
};

/// One-shot events, set by the sequencer, consumed by motionTakeEvents().
struct MotionEvents {
    bool homing_done     = false;
    bool homing_failed   = false;
    bool motor_fault     = false;  ///< unexpected limit switch outside homing
    bool acq_started     = false;  ///< entered TRAV_RUN or MEAS_HOLD
    bool acq_ended       = false;  ///< left TRAV_RUN or MEAS_HOLD (finished, stopped or aborted)
};

struct MotionSeq {
    SeqPhase     phase          = SeqPhase::IDLE;
    OpMode       mode           = OpMode::NONE;
    bool         homed          = false;
    bool         error_latched  = false;  ///< last sequence ended with fault/failure
    uint32_t     phase_start_ms = 0;
    uint32_t     homing_start_ms = 0;
    int32_t      target_steps   = 0;      ///< current move target
    int32_t      trav_end_steps = 0;
    uint32_t     trav_speed_hz  = 0;
    MotionEvents ev;                      ///< pending one-shot events
};

// ---- conversions ----------------------------------------------------------
int32_t  mmToSteps(float mm, const MotionConfig& c);            ///< lroundf(mm * steps_per_mm)
float    stepsToMm(int32_t steps, const MotionConfig& c);       ///< steps / steps_per_mm
uint32_t mmPerSecToHz(float mm_s, const MotionConfig& c);       ///< lroundf(mm_s * steps_per_mm)

// ---- commands (each returns OK or an ApiError and fills `out`) ------------
// `out` is reset to NONE first. On error the sequencer state is unchanged.
// Every successful start clears error_latched.
/**
 * Homing. BUSY if phase != IDLE. Sets homed = false, error_latched = false,
 * homing_start_ms = now. If limit_bwd is already active: phase HOME_LATCH,
 * out = ZERO_HERE. Else phase HOME_SEEK, out = RUN_BACKWARD at homing_speed.
 */
ApiError motionStartHoming(MotionSeq& s, const MotionInputs& in, const MotionConfig& c, DrvRequest& out);

/**
 * Absolute move. Checks in this order: phase != IDLE -> BUSY; !homed -> PRECONDITION;
 * pos outside [0, axis_max_mm] or speed outside [min_speed, max_speed] -> BAD_PARAM.
 * OK: phase MOVE, out = MOVE_TO(pos, speed).
 */
ApiError motionStartMove(MotionSeq& s, float pos_mm, float speed_mm_s,
                         const MotionInputs& in, const MotionConfig& c, DrvRequest& out);

/**
 * Traverse start -> end at speed. Checks: BUSY, PRECONDITION (not homed),
 * BAD_PARAM if start or end not in (park_pos_mm, axis_max_mm], |end - start| < 1 mm,
 * or speed out of range.
 * OK: mode TRAVERSE, phase TRAV_APPROACH, out = MOVE_TO(start, approach_speed).
 */
ApiError motionStartTraverse(MotionSeq& s, float start_mm, float end_mm, float speed_mm_s,
                             const MotionInputs& in, const MotionConfig& c, DrvRequest& out);

/**
 * Stationary measurement at pos. Checks: BUSY, PRECONDITION, BAD_PARAM if pos not
 * in (park_pos_mm, axis_max_mm]. OK: mode MEASURE, phase MEAS_APPROACH,
 * out = MOVE_TO(pos, approach_speed).
 */
ApiError motionStartMeasure(MotionSeq& s, float pos_mm,
                            const MotionInputs& in, const MotionConfig& c, DrvRequest& out);

/**
 * Controlled stop, always OK. Homing phases: homed stays false.
 * TRAV_RUN/MEAS_HOLD: ev.acq_ended = true. phase IDLE, mode NONE.
 * out = STOP if phase was not IDLE, else NONE.
 */
void motionStop(MotionSeq& s, DrvRequest& out);

/**
 * Emergency stop (safety FAULT). out = FORCE_STOP always.
 * If phase != IDLE or drv_running: homed = false (reference lost).
 * TRAV_RUN/MEAS_HOLD: ev.acq_ended = true. phase IDLE, mode NONE.
 */
void motionEmergencyStop(MotionSeq& s, const MotionInputs& in, DrvRequest& out);

/**
 * Cyclic step (call every control cycle BEFORE dispatching commands).
 * "Arrived" means: !drv_running && drv_pos_steps == target_steps, OR
 *                  !drv_running && (now - phase_start_ms) >= settle_ms.
 * Rules:
 *  - Homing timeout: any HOME_* phase and (now - homing_start_ms) > homing_timeout_ms
 *      -> FORCE_STOP, phase IDLE, ev.homing_failed, error_latched.
 *  - Unexpected limit (only while drv_running):
 *      limit_fwd in any phase, or limit_bwd in any phase except HOME_SEEK/HOME_LATCH/HOME_PULLOFF
 *      -> FORCE_STOP, homed = false, phase IDLE, mode NONE, error_latched,
 *         ev.motor_fault (outside homing) or ev.homing_failed (during homing);
 *         acq_ended if acquiring.
 *  - HOME_SEEK: limit_bwd -> ZERO_HERE, phase HOME_LATCH.
 *  - HOME_LATCH: -> MOVE_TO(+pulloff, homing_speed), phase HOME_PULLOFF.
 *  - HOME_PULLOFF: arrived -> ZERO_HERE, homed = true, phase IDLE, ev.homing_done.
 *  - MOVE: arrived -> IDLE.
 *  - TRAV_APPROACH: arrived -> MOVE_TO(trav_end, trav_speed), phase TRAV_RUN, ev.acq_started.
 *  - TRAV_RUN: arrived -> IDLE, mode NONE, ev.acq_ended.
 *  - MEAS_APPROACH: arrived -> phase MEAS_HOLD, ev.acq_started (out NONE).
 *  - MEAS_HOLD: nothing.
 * Every phase change sets phase_start_ms = now; every MOVE_TO sets target_steps.
 */
DrvRequest motionTick(MotionSeq& s, const MotionInputs& in, const MotionConfig& c);

/// Returns pending events and clears them.
MotionEvents motionTakeEvents(MotionSeq& s);

/// TRAV_RUN or MEAS_HOLD
bool motionAcquiring(const MotionSeq& s);

/// HOME_* -> HOMING; other non-IDLE -> MOVING; IDLE && error_latched -> ERROR; else IDLE.
MotionState motionStateOf(const MotionSeq& s);
