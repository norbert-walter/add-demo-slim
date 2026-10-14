/**
 * @file app_logic.cpp
 * @brief Complete application logic (Sprint L4).
 */
#include "app_logic.h"
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------
// V1.1 – Priority command identification
// ---------------------------------------------------------------------------

bool appCmdIsPriority(CmdId id) {
    return id == CmdId::STOP || id == CmdId::HV_OFF || id == CmdId::SHUTTER_CLOSE;
}

// ---------------------------------------------------------------------------
// V1.1 – LOCAL blocks-API table
// ---------------------------------------------------------------------------

bool appLocalBlocksApi(CmdId id) {
    switch (id) {
        case CmdId::INIT:
        case CmdId::MOVE:
        case CmdId::PARK:
        case CmdId::SERVICE:
        case CmdId::START_MEASURE:
        case CmdId::START_TRAVERSE:
        case CmdId::HV_ON:
        case CmdId::SHUTTER_OPEN:
        case CmdId::CALIB:
        case CmdId::ACK:
        case CmdId::REBOOT:
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// V1.1 – Overflow latch (CONTRACT §6.7)
// Bit layout per command:  bit = API, bit+1 = LOCAL
//   STOP  : bits 0/3
//   HV_OFF: bits 1/4
//   SHUTTR: bits 2/5
// ---------------------------------------------------------------------------

static int prioBit(CmdId id) {
    switch (id) {
        case CmdId::STOP:      return 0;
        case CmdId::HV_OFF:    return 1;
        case CmdId::SHUTTER_CLOSE: return 2;
        default:               return -1;
    }
}

void appPrioLatchSet(uint32_t& bits, const Command& cmd) {
    if (!appCmdIsPriority(cmd.id)) return;  // ignored for non-priority
    int b = prioBit(cmd.id);
    if (b < 0) return;
    if (cmd.source == CmdSource::API) {
        bits |= (1u << b);
    } else {
        bits |= (1u << (b + 3));
    }
}

bool appPrioLatchTake(uint32_t& bits, Command& out) {
    // Priority order: STOP(0/3) > HV_OFF(1/4) > SHUTTER_CLOSE(2/5)
    static const struct { CmdId id; int api_bit; int local_bit; } table[] = {
        { CmdId::STOP,      0, 3 },
        { CmdId::HV_OFF,    1, 4 },
        { CmdId::SHUTTER_CLOSE, 2, 5 },
    };

    for (auto& e : table) {
        if ((bits & (1u << e.api_bit)) || (bits & (1u << e.local_bit))) {
            out.id       = e.id;
            out.source   = (bits & (1u << e.local_bit)) ? CmdSource::LOCAL : CmdSource::API;
            out.want_reply = false;
            // Clear both bits for this command
            bits &= ~((1u << e.api_bit) | (1u << e.local_bit));
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

static MotionInputs motionIn(const HwInputs& hw) {
    MotionInputs out{};
    out.now_ms         = hw.now_ms;
    out.limit_fwd      = hw.limit_fwd;
    out.limit_bwd      = hw.limit_bwd;
    out.drv_running    = hw.drv_running;
    out.drv_pos_steps  = hw.drv_pos_steps;
    return out;
}

static SafetyInputs safetyIn(const AppState& s, const HwInputs& hw, const AppConfig& cfg,
                             bool motor_fault) {
    SafetyInputs in{};
    in.estop         = hw.estop;
    in.interlock     = s.flags.interlock;
    in.cooling_flow  = s.flags.cooling_flow;
    in.temp_c        = s.flags.temp_c;
    in.motor_fault   = motor_fault;
    in.comm_timeout  = safetyCommTimeout(s.comm_armed, hw.now_ms, hw.last_api_rx_ms,
                                         cfg.comm_timeout_ms);
    in.pos_mm        = stepsToMm(hw.drv_pos_steps, cfg.motion);
    in.homed         = s.seq.homed;
    return in;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

void appInit(AppState& s) {
    s.safety      = safetyInitialStatus();
    s.seq         = MotionSeq{};
    paramsDefaults(s.params);
    s.flags       = SafetyFlags{};
    s.stats       = AcqStats{};
    s.stab        = StabilityTracker{};
    s.iir         = Iir{};

    s.raw_mm      = 0.0f;
    s.dist_mm     = 0.0f;
    s.thick_mm    = 0.0f;
    s.in_range    = false;
    s.on_material = false;

    s.comm_armed  = false;

    s.local       = false;
    s.local_last_ms = 0;

    s.snd         = SoundPlayer{};
    s.prog_active_ms = 0;
    s.boot_sound_done = false;
}

// ---------------------------------------------------------------------------
// Stubs (filled in later sprints)
// ---------------------------------------------------------------------------

void appMeasure(AppState& s, const AppConfig& cfg, float median_mv, uint32_t now_ms) {
    // 1. Convert MV → mm (unfiltered)
    float mm = sensorMvToMm(median_mv, cfg.sensor);

    // 2. Range check on the unfiltered reading
    bool in_range = sensorInRange(mm, cfg.sensor);

    if (!in_range) {
        // Reset filter and stability tracker
        iirReset(s.iir);
        s.raw_mm      = mm;          // out of range: raw = mm (e.g. SENSOR_NO_SIGNAL_MM)
        s.dist_mm     = mm;          // out of range: dist = mm
        s.thick_mm    = 0.0f;
        s.in_range    = false;
        s.on_material = false;
        s.stab.initialized = false;  // only reset initialized flag (window_ms/tol_mm stay)
        return;
    }

    // 3. IIR filter → raw_mm (uncalibrated)
    s.raw_mm = iirUpdate(s.iir, mm);

    // 4. Calibration → dist_mm
    s.dist_mm = calibApply(s.raw_mm, s.params.calib);

    // 5. Range flag stays from step 2 (in_range decided on unfiltered mm only)
    s.in_range = in_range;

    // 6. Thickness and on_material
    s.thick_mm   = thickness(s.dist_mm, s.params.base_dist);
    s.on_material = onMaterial(s.thick_mm, s.params.edge_threshold);

    // 7. Stability update (always when in range)
    stabilityUpdate(s.stab, now_ms, s.dist_mm);

    // 8. Statistics: only while acquiring AND shutter open && in_range
    if (motionAcquiring(s.seq)) {
        statsAdd(s.stats, s.thick_mm, s.on_material, s.safety.shutter_open && in_range);
    }
}

// ---------------------------------------------------------------------------
// V1.3 / safety – Static helpers
// ---------------------------------------------------------------------------

static ApiError applySafety(AppState& s, SafetyEvent ev, const SafetyInputs& in,
                            const HwInputs& hw, const AppConfig& cfg,
                            DrvRequest& d, bool& enteredFault) {
    // Step: safetyStep -> next state
    SafetyResult r = safetyStep(s.safety, ev, in, cfg.safety);
    bool was_fault = (s.safety.state == SafetyState::FAULT);
    s.safety = r.next;

    // Track fault entry (transitioned INTO fault this cycle)
    enteredFault |= (r.next.state == SafetyState::FAULT && !was_fault);

    // Motor stop -> emergency stop the sequencer
    if (r.motor_stop && (s.seq.phase != SeqPhase::IDLE || hw.drv_running)) {
        motionEmergencyStop(s.seq, motionIn(hw), d);
    }

    return r.api;
}

static void startSound(AppState& s, Sound snd, uint32_t now, HwOutputs& out) {
    if (soundStart(s.snd, snd, now)) {
        out.sound = snd;
    }
}

// ---------------------------------------------------------------------------
// appTick – 11 steps from the header comment
// ---------------------------------------------------------------------------

HwOutputs appTick(AppState& s, const HwInputs& hw, const AppConfig& cfg) {
    HwOutputs out{};

    // ---- Step 1: motion tick + events -----------------------------------
    MotionInputs mi = motionIn(hw);
    DrvRequest d = motionTick(s.seq, mi, cfg.motion);
    MotionEvents ev = motionTakeEvents(s.seq);

    // ---- Step 2: build SafetyInputs ------------------------------------
    SafetyInputs si = safetyIn(s, hw, cfg, ev.motor_fault);

    // ---- Steps 3/4: homing events + TICK through safetyStep ------------
    bool enteredFault = false;
    if (ev.homing_done) {
        applySafety(s, SafetyEvent::INIT_DONE, si, hw, cfg, d, enteredFault);
    }
    if (ev.homing_failed) {
        applySafety(s, SafetyEvent::INIT_FAILED, si, hw, cfg, d, enteredFault);
    }
    // TICK on every cycle
    applySafety(s, SafetyEvent::TICK, si, hw, cfg, d, enteredFault);

    // ---- Step 5: motor_stop emergency stop (steps 3/4) ------------------
    // Already handled inside applySafety above.

    // ---- Step 6: state != RUNNING -> motionStop -------------------------
    if (s.safety.state != SafetyState::RUNNING && s.seq.mode != OpMode::NONE) {
        motionStop(s.seq, d);
        if (d.op == DrvOp::NONE) {
            d.op = DrvOp::STOP;
        }
    }

    // ---- Step 7: acq_started -> statsReset ------------------------------
    if (ev.acq_started) {
        statsReset(s.stats);
    }

    // ---- Step 8: active tracking + shutter automation --------------------
    bool active = (s.seq.mode != OpMode::NONE);
    if (active) {
        s.prog_active_ms = hw.now_ms;
    }

    uint32_t idle_ms;
    if (hw.now_ms >= s.prog_active_ms) {
        idle_ms = hw.now_ms - s.prog_active_ms;
    } else {
        // wrap-around safe
        idle_ms = (0xFFFFFFFFu - s.prog_active_ms) + hw.now_ms + 1u;
    }

    SafetyEvent shutter_ev = safetyShutterAutomation(
        motionAcquiring(s.seq), active, idle_ms,
        static_cast<uint32_t>(lroundf(s.params.shutter_hold * 1000.0f)),
        s.safety, si.pos_mm, cfg.safety);

    if (shutter_ev != SafetyEvent::TICK) {
        applySafety(s, shutter_ev, si, hw, cfg, d, enteredFault);
    }

    // ---- Step 9: comm_armed release ------------------------------------
    if (s.comm_armed && s.seq.phase == SeqPhase::IDLE && !s.safety.hv_on) {
        s.comm_armed = false;
    }

    // ---- Step 9b: V1.1 local release -----------------------------------
    if (s.local && s.seq.phase == SeqPhase::IDLE && !s.safety.hv_on) {
        uint32_t elapsed;
        if (hw.now_ms >= s.local_last_ms) {
            elapsed = hw.now_ms - s.local_last_ms;
        } else {
            elapsed = (0xFFFFFFFFu - s.local_last_ms) + hw.now_ms + 1u;
        }
        if (elapsed >= cfg.local_hold_ms) {
            s.local = false;
        }
    }

    // ---- Step 10: outputs from safety status ----------------------------
    out.hv_on       = s.safety.hv_on;
    out.shutter_open = s.safety.shutter_open;
    out.drv         = d;

    // ---- Step 11: V1.3 sounds -------------------------------------------
    // BOOT sound on first appTick after init
    if (!s.boot_sound_done) {
        startSound(s, Sound::BOOT, hw.now_ms, out);
        s.boot_sound_done = true;
    }

    // ESTOP / FAULT sound when entering FAULT this cycle
    if (enteredFault) {
        Sound fault_snd = (s.safety.err == ErrorCode::ESTOP) ? Sound::ESTOP : Sound::FAULT;
        startSound(s, fault_snd, hw.now_ms, out);
    }

    // HOMED sound when homing just completed
    if (ev.homing_done) {
        startSound(s, Sound::HOMED, hw.now_ms, out);
    }

    // Buzzer output and heartbeat
    out.buzzer  = soundOutput(s.snd, hw.now_ms);
    out.heartbeat = ((hw.now_ms / 1000u) % 2u) == 0u;

    return out;
}

// ---------------------------------------------------------------------------
// Driver request merge helper (V1.1)
// ---------------------------------------------------------------------------
static int drvRank(const DrvRequest& r) {
    switch (r.op) {
        case DrvOp::NONE:           return 0;
        case DrvOp::MOVE_TO:        return 1;
        case DrvOp::RUN_BACKWARD:   return 1;
        case DrvOp::STOP:           return 2;
        case DrvOp::FORCE_STOP:     return 3;
        case DrvOp::ZERO_HERE:      return 3;
        default:                    return 0;
    }
}

ApiError appCommand(AppState& s, const Command& cmd, const HwInputs& hw, const AppConfig& cfg,
                    HwOutputs& out) {
    if (cmd.id == CmdId::GET_STATE || cmd.id == CmdId::GET_CONFIG) return ApiError::OK;
    uint32_t now_ms = hw.now_ms;
    if (cmd.source == CmdSource::LOCAL) {
        s.local = true; s.local_last_ms = now_ms; s.comm_armed = false;
    } else if (s.local && appLocalBlocksApi(cmd.id)) {
        return ApiError::LOCAL_CONTROL;
    }
    ApiError gate = safetyCmdAllowed(s.safety.state, cmd.id);
    if (gate != ApiError::OK) return gate;
    SafetyInputs in = safetyIn(s, hw, cfg, false);
    MotionInputs mi = motionIn(hw);
    DrvRequest d{};
    bool enteredFault = false;
    ApiError r = ApiError::OK;
    bool arm = false;
    switch (cmd.id) {
        case CmdId::INIT:
            r = motionStartHoming(s.seq, mi, cfg.motion, d); arm = true; break;
        case CmdId::MOVE: {
            float spd = cmd.has_speed ? cmd.speed : cfg.motion.approach_speed;
            r = motionStartMove(s.seq, cmd.pos, spd, mi, cfg.motion, d); arm = true; break;
        }
        case CmdId::START_MEASURE: {
            float pos = cmd.has_pos ? cmd.pos : s.params.measure_pos;
            r = motionStartMeasure(s.seq, pos, mi, cfg.motion, d); arm = true; break;
        }
        case CmdId::START_TRAVERSE: {
            float st = cmd.has_start ? cmd.start : s.params.traverse_start;
            float en = cmd.has_end   ? cmd.end   : s.params.traverse_end;
            float sp = cmd.has_speed ? cmd.speed : s.params.traverse_speed;
            r = motionStartTraverse(s.seq, st, en, sp, mi, cfg.motion, d); arm = true; break;
        }
        case CmdId::PARK:
            r = motionStartMove(s.seq, cfg.motion.park_target_mm, cfg.motion.approach_speed, mi, cfg.motion, d); arm = true; break;
        case CmdId::SERVICE:
            r = motionStartMove(s.seq, s.params.maint_pos, cfg.motion.approach_speed, mi, cfg.motion, d); arm = true; break;
        case CmdId::STOP:
            motionStop(s.seq, d);
            if (s.safety.shutter_open && s.safety.shutter_auto)
                applySafety(s, SafetyEvent::SHUTTER_CLOSE, in, hw, cfg, d, enteredFault);
            break;
        case CmdId::SET: {
            bool persist = false;
            if (strcmp(cmd.key, "estop_nc") == 0 && (s.seq.phase != SeqPhase::IDLE || s.safety.hv_on)) {
                r = ApiError::PRECONDITION; break;
            }
            r = paramSet(s.params, s.flags, cmd.key, cmd.vtype, cmd.num, cmd.b, cmd.str, &persist);
            if (r == ApiError::OK && persist) out.persist = true;
            break;
        }
        case CmdId::CALIB:
            if (s.seq.phase != SeqPhase::IDLE) { r = ApiError::BUSY; break; }
            switch (cmd.calib) {
                case CalibAction::NEAR:
                    if (!stabilityIsStable(s.stab, now_ms)) { r = ApiError::NOT_STABLE; break; }
                    s.params.calib.near_raw = s.raw_mm; break;
                case CalibAction::FAR:
                    if (!stabilityIsStable(s.stab, now_ms)) { r = ApiError::NOT_STABLE; break; }
                    s.params.calib.far_raw = s.raw_mm; break;
                case CalibAction::BASE:
                    if (!s.in_range) { r = ApiError::PRECONDITION; break; }
                    if (!stabilityIsStable(s.stab, now_ms)) { r = ApiError::NOT_STABLE; break; }
                    s.params.base_dist = s.dist_mm; break;
                case CalibAction::SAVE:
                    if (!calibValid(s.params.calib)) { r = ApiError::CALIB_INVALID; break; }
                    out.persist = true; break;
                case CalibAction::RESET: s.params.calib = CalibPoints{}; break;
                default: r = ApiError::BAD_PARAM; break;
            } break;
        case CmdId::ACK: r = applySafety(s, SafetyEvent::ACK, in, hw, cfg, d, enteredFault); break;
        case CmdId::HV_ON: r = applySafety(s, SafetyEvent::HV_ON, in, hw, cfg, d, enteredFault); arm = true; break;
        case CmdId::HV_OFF: r = applySafety(s, SafetyEvent::HV_OFF, in, hw, cfg, d, enteredFault); break;
        case CmdId::SHUTTER_OPEN: r = applySafety(s, SafetyEvent::SHUTTER_OPEN, in, hw, cfg, d, enteredFault); break;
        case CmdId::SHUTTER_CLOSE: r = applySafety(s, SafetyEvent::SHUTTER_CLOSE, in, hw, cfg, d, enteredFault); break;
        case CmdId::REBOOT:
            if (s.safety.hv_on) { r = ApiError::PRECONDITION; break; }
            out.reboot = true; startSound(s, Sound::REBOOT, now_ms, out); break;
        default: r = ApiError::UNKNOWN_CMD; break;
    }
    if (r == ApiError::OK && arm) s.comm_armed = (cmd.source == CmdSource::API);
    if (drvRank(d) >= drvRank(out.drv)) out.drv = d;
    if (enteredFault) {
        startSound(s, (s.safety.err == ErrorCode::ESTOP) ? Sound::ESTOP : Sound::FAULT, now_ms, out);
    }
    if (r == ApiError::OK && cmd.source == CmdSource::LOCAL && cmd.id != CmdId::NONE)
        startSound(s, Sound::CLICK, now_ms, out);
    out.hv_on = s.safety.hv_on;
    out.shutter_open = s.safety.shutter_open;
    out.buzzer = soundOutput(s.snd, now_ms);
    return r;
}

void appFillProc(const AppState& s, const AppConfig& cfg, const HwInputs& hw, ProcData& out) {
    // Motion inputs
    MotionInputs m = motionIn(hw);

    // Safety inputs (for hv_ok check)
    SafetyInputs si = safetyIn(s, hw, cfg, false);

    out.ts_ms        = hw.now_ms;
    out.state        = s.safety.state;
    out.err          = s.safety.err;
    out.local        = s.local;

    out.motion.pos         = stepsToMm(hw.drv_pos_steps, cfg.motion);
    out.motion.vel         = hw.drv_speed_mm_s;
    out.motion.state       = motionStateOf(s.seq);
    out.motion.mode        = s.seq.mode;
    out.motion.homed       = s.seq.homed;
    out.motion.limit_fwd   = m.limit_fwd;
    out.motion.limit_bwd   = m.limit_bwd;

    out.measure.dist       = s.dist_mm;
    out.measure.thick      = s.thick_mm;
    out.measure.on_material = s.on_material;
    out.measure.valid      = s.safety.shutter_open;
    out.measure.in_range   = s.in_range;
    out.measure.avg        = statsMean(s.stats);
    out.measure.n          = s.stats.n;

    out.safety.estop       = si.estop;
    out.safety.hv_on       = s.safety.hv_on;
    out.safety.hv_ok       = safetyHvConditionsOk(si, cfg.safety);
    out.safety.shutter_open = s.safety.shutter_open;
    out.safety.interlock   = si.interlock;
    out.safety.cooling_flow = si.cooling_flow;
    out.safety.temp        = si.temp_c;
    out.safety.temp_ok     = (si.temp_c <= cfg.safety.temp_warn_c);
    out.safety.comm_armed  = s.comm_armed;
}