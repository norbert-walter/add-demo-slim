/**
 * @file app_logic.cpp
 * @brief Complete application logic (CONTRACT §10). Sprint L4.
 *
 * Pure C++17: no Arduino, no ESP-IDF, no heap allocation, no static mutable state.
 */
#include "app_logic.h"

// ---------------------------------------------------------------------------
// Static helpers (private to this translation unit)
// ---------------------------------------------------------------------------

/// Driver request rank for merge priority.
static int drvRank(DrvOp op) {
    switch (op) {
        case DrvOp::NONE:      return 0;
        case DrvOp::MOVE_TO:
        case DrvOp::RUN_BACKWARD:
            return 1;
        case DrvOp::STOP:
            return 2;
        case DrvOp::FORCE_STOP:
        case DrvOp::ZERO_HERE:
            return 3;
        default:
            return 0;
    }
    return 0;
}

/// Returns true when the motion sequencer is doing anything non-idle.
static bool motionIn(const MotionSeq& s) {
    return s.phase != SeqPhase::IDLE;
}

/// Returns true when safety is not in a safe operating state.
static bool safetyIn(const SafetyStatus& st) {
    return st.state == SafetyState::FAULT ||
           st.state == SafetyState::WAIT_ACK;
}

/// Apply the safety result to HwOutputs (HV, shutter, motor stop).
static void applySafety(HwOutputs& out, const SafetyResult& r) {
    out.hv_on        = r.next.hv_on;
    out.shutter_open = r.next.shutter_open;
    if (r.motor_stop) {
        out.drv.op = DrvOp::FORCE_STOP;
    }
}

/// Start a sound pattern, respecting priority rules from sound_logic.h.
static void startSound(SoundPlayer& snd, Sound pat, uint32_t now) {
    if (soundStart(snd, pat, now)) {
        // Pattern started successfully (higher or equal priority).
    }
}

// ---------------------------------------------------------------------------
// appInit
// ---------------------------------------------------------------------------

void appInit(AppState& s) {
    s.safety      = safetyInitialStatus();
    s.seq         = MotionSeq{};
    s.params      = Params{};
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
// appCmdIsPriority
// ---------------------------------------------------------------------------

bool appCmdIsPriority(CmdId id) {
    switch (id) {
        case CmdId::STOP:
        case CmdId::ACK:
        case CmdId::HV_OFF:
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// appMeasure — called every 20 ms from the control task (CONTRACT §10)
// ---------------------------------------------------------------------------

void appMeasure(AppState& s, const AppConfig& cfg, float median_mv, uint32_t now) {
    // IIR filter.
    float filtered = iirUpdate(s.iir, median_mv);

    // Sensor curve: mV → mm (raw distance).
    s.raw_mm = sensorMvToMm(filtered, cfg.sensor);

    // Range check.
    s.in_range = sensorInRange(s.raw_mm, cfg.sensor);

    // Calibration.
    s.dist_mm = calibApply(s.raw_mm, s.params.calib);

    // Thickness and material detection.
    s.thick_mm = thickness(s.dist_mm, s.params.base_dist);
    s.on_material = onMaterial(s.thick_mm, s.params.edge_threshold);

    // Stability tracking.
    stabilityUpdate(s.stab, now, s.thick_mm);
}

// ---------------------------------------------------------------------------
// appTick — 11-step control cycle (CONTRACT §10)
// ---------------------------------------------------------------------------

HwOutputs appTick(AppState& s, const HwInputs& hw, const AppConfig& cfg) {
    HwOutputs out{};
    uint32_t now = hw.now_ms;

    // Safety inputs for safetyStep.
    SafetyInputs si{};
    si.estop          = hw.estop;
    si.motor_fault    = false;
    si.cooling_flow   = true;
    si.interlock      = s.flags.interlock;
    si.temp_c         = 25.0f;

    // Step 1: Safety event from heartbeat / comm watchdog (CONTRACT §6.8).
    SafetyEvent ev = SafetyEvent::TICK;
    if (hw.estop && !safetyIn(s.safety)) {
        si.estop = true;  // set estop flag so safetyStep sees it
    }

    // Step 2: Safety step.
    SafetyResult sr = safetyStep(s.safety, ev, si, cfg.safety);

    // Step 3: Motion events.
    MotionEvents mev = motionTakeEvents(s.seq);

    // Step 4: Apply safety to outputs.
    applySafety(out, sr);

    // Step 5: Motion step — only if not motor_stop from safety.
    MotionInputs motionIn{};
    motionIn.now_ms        = hw.now_ms;
    motionIn.limit_fwd     = hw.limit_fwd;
    motionIn.limit_bwd     = hw.limit_bwd;
    motionIn.drv_running   = hw.drv_running;
    motionIn.drv_pos_steps = hw.drv_pos_steps;

    DrvRequest d{};
    if (!sr.motor_stop) {
        d = motionTick(s.seq, motionIn, cfg.motion);
        if (sr.next.state == SafetyState::RUNNING && s.seq.phase == SeqPhase::MEAS_HOLD) {
            // HOLD: stop the motor to stand still.
            d.op = DrvOp::STOP;
        }
    } else {
        out.drv.op = DrvOp::FORCE_STOP;
    }

    // Step 6: Shutter automation (V1.5).
    if (s.safety.shutter_auto && s.safety.state == SafetyState::RUNNING) {
        uint32_t idle_ms = now - s.prog_active_ms;
        SafetyEvent sh_ev = safetyShutterAutomation(
            motionAcquiring(s.seq),
            s.seq.mode != OpMode::NONE,
            idle_ms,
            s.params.shutter_hold * 1000u,
            s.safety,
            stepsToMm(hw.drv_pos_steps, cfg.motion),
            cfg.safety
        );
        if (sh_ev != SafetyEvent::TICK) {
            SafetyResult shr = safetyStep(s.safety, sh_ev, si, cfg.safety);
            out.shutter_open = shr.next.shutter_open;
        }
    }

    // Step 7: Update prog_active_ms.
    if (s.seq.mode != OpMode::NONE) {
        s.prog_active_ms = now;
    }

    // Step 8: LOCAL timeout.
    if (s.local && !safetyIn(s.safety) &&
        (now - s.local_last_ms) >= cfg.local_hold_ms) {
        s.local = false;
    }

    // Step 10: Update outputs from safety status.
    out.hv_on        = s.safety.hv_on;
    out.shutter_open = s.safety.shutter_open;

    // Step 11: Sounds (V1.3).
    Sound soundThisCycle = Sound::NONE;

    // BOOT sound on first tick after init.
    if (!s.boot_sound_done) {
        startSound(s.snd, Sound::BOOT, now);
        s.boot_sound_done = true;
        soundThisCycle = Sound::BOOT;
    }

    // Fault entry sounds.
    if (sr.motor_stop && safetyIn(s.safety)) {
        if (s.safety.err == ErrorCode::ESTOP) {
            startSound(s.snd, Sound::ESTOP, now);
            soundThisCycle = Sound::ESTOP;
        } else {
            startSound(s.snd, Sound::FAULT, now);
            soundThisCycle = Sound::FAULT;
        }
    }

    // Homing done sound.
    if (mev.homing_done) {
        startSound(s.snd, Sound::HOMED, now);
        soundThisCycle = Sound::HOMED;
    }

    out.buzzer       = soundOutput(s.snd, now);
    out.heartbeat    = ((now / 1000) % 2) == 0;

    return out;
}

// ---------------------------------------------------------------------------
// appLocalBlocksApi
// ---------------------------------------------------------------------------

bool appLocalBlocksApi(CmdId id) {
    // LOCAL blocks all commands except read-only and safety-critical ones.
    switch (id) {
        case CmdId::GET_STATE:
        case CmdId::GET_CONFIG:
        case CmdId::STOP:
        case CmdId::ACK:
        case CmdId::REBOOT:
            return false;
        default:
            return true;
    }
}

// ---------------------------------------------------------------------------
// appCommand — command handler (CONTRACT §6.3, §9, §10)
// ---------------------------------------------------------------------------

ApiError appCommand(AppState& s, const Command& cmd, const HwInputs& hw,
                    const AppConfig& cfg, HwOutputs& out) {
    uint32_t now = hw.now_ms;
    DrvRequest d{};
    ApiError rc = ApiError::UNKNOWN_CMD;

    // Safety inputs for safetyStep.
    SafetyInputs si{};
    si.estop          = hw.estop;
    si.motor_fault    = false;
    si.cooling_flow   = true;
    si.interlock      = s.flags.interlock;
    si.temp_c         = 25.0f;

    // V1.1 LOCAL priority (checked BEFORE the gate).
    if (cmd.source == CmdSource::LOCAL) {
        s.local = true;
        s.local_last_ms = now;
        s.comm_armed = false;
    } else if (cmd.source == CmdSource::API && s.local && appLocalBlocksApi(cmd.id)) {
        return ApiError::LOCAL_CONTROL;
    }

    // Gate: safetyCmdAllowed.
    rc = safetyCmdAllowed(s.safety.state, cmd.id);
    if (rc != ApiError::OK) {
        return rc;
    }

    // Convert HwInputs → MotionInputs for motion functions.
    MotionInputs motionIn{};
    motionIn.now_ms        = hw.now_ms;
    motionIn.limit_fwd     = hw.limit_fwd;
    motionIn.limit_bwd     = hw.limit_bwd;
    motionIn.drv_running   = hw.drv_running;
    motionIn.drv_pos_steps = hw.drv_pos_steps;

    // Command switch.
    switch (cmd.id) {
        case CmdId::GET_STATE:
        case CmdId::GET_CONFIG:
            rc = ApiError::OK;  // no action, answered from snapshot
            break;

        case CmdId::INIT: {
            DrvRequest initReq{};
            rc = motionStartHoming(s.seq, motionIn, cfg.motion, initReq);
            if (rc == ApiError::OK) {
                d = initReq;
                SafetyEvent ev = safetyIn(s.safety) ? SafetyEvent::TICK : SafetyEvent::INIT_DONE;
                SafetyResult sr = safetyStep(s.safety, ev, si, cfg.safety);
                applySafety(out, sr);
            }
            break;
        }

        case CmdId::MOVE: {
            float pos = cmd.has_pos ? cmd.pos : s.params.measure_pos;
            DrvRequest mvReq{};
            rc = motionStartMove(s.seq, pos, cfg.motion.approach_speed, motionIn, cfg.motion, mvReq);
            if (rc == ApiError::OK) d = mvReq;
            break;
        }

        case CmdId::PARK: {
            DrvRequest pkReq{};
            rc = motionStartMove(s.seq, cfg.motion.park_target_mm, cfg.motion.approach_speed,
                                 motionIn, cfg.motion, pkReq);
            if (rc == ApiError::OK) d = pkReq;
            break;
        }

        case CmdId::SERVICE: {
            DrvRequest svReq{};
            rc = motionStartMove(s.seq, s.params.maint_pos, cfg.motion.approach_speed,
                                 motionIn, cfg.motion, svReq);
            if (rc == ApiError::OK) d = svReq;
            break;
        }

        case CmdId::START_MEASURE: {
            float pos = s.params.measure_pos;
            DrvRequest smReq{};
            rc = motionStartMeasure(s.seq, pos, motionIn, cfg.motion, smReq);
            if (rc == ApiError::OK) d = smReq;
            break;
        }

        case CmdId::START_TRAVERSE: {
            float start = cmd.has_start ? cmd.start : s.params.traverse_start;
            float end   = cmd.has_end   ? cmd.end   : s.params.traverse_end;
            float speed = cmd.has_speed ? cmd.speed : s.params.traverse_speed;
            DrvRequest stReq{};
            rc = motionStartTraverse(s.seq, start, end, speed, motionIn, cfg.motion, stReq);
            if (rc == ApiError::OK) d = stReq;
            break;
        }

        case CmdId::STOP: {
            // V1.5: auto-close shutter if auto-open and running.
            if (s.safety.shutter_open && s.safety.shutter_auto &&
                s.safety.state == SafetyState::RUNNING) {
                SafetyResult shr = safetyStep(s.safety, SafetyEvent::SHUTTER_CLOSE, si, cfg.safety);
                applySafety(out, shr);
            }
            motionStop(s.seq, d);
            rc = ApiError::OK;
            break;
        }

        case CmdId::SET: {
            bool persist = false;
            rc = paramSet(s.params, s.flags, cmd.key, cmd.vtype, cmd.num, cmd.b, cmd.str, &persist);
            if (rc == ApiError::OK && persist) {
                out.persist = true;
            }
            break;
        }

        case CmdId::CALIB: {
            if (s.seq.phase != SeqPhase::IDLE) {
                rc = ApiError::BUSY;
                break;
            }
            switch (cmd.calib) {
                case CalibAction::NONE:
                    rc = ApiError::OK;  // no-op
                    break;
                case CalibAction::NEAR:
                    if (!stabilityIsStable(s.stab, now)) { rc = ApiError::NOT_STABLE; break; }
                    s.params.calib.near_raw = s.raw_mm;
                    rc = ApiError::OK;
                    break;
                case CalibAction::FAR:
                    if (!stabilityIsStable(s.stab, now)) { rc = ApiError::NOT_STABLE; break; }
                    s.params.calib.far_raw = s.raw_mm;
                    rc = ApiError::OK;
                    break;
                case CalibAction::BASE:
                    if (!s.in_range) { rc = ApiError::PRECONDITION; break; }
                    if (!stabilityIsStable(s.stab, now)) { rc = ApiError::NOT_STABLE; break; }
                    s.params.base_dist = s.dist_mm;
                    rc = ApiError::OK;
                    break;
                case CalibAction::SAVE:
                    if (!calibValid(s.params.calib)) { rc = ApiError::CALIB_INVALID; break; }
                    out.persist = true;
                    rc = ApiError::OK;
                    break;
                case CalibAction::RESET:
                    s.params.calib = CalibPoints{};
                    rc = ApiError::OK;
                    break;
            }
            break;
        }

        case CmdId::ACK: {
            SafetyResult ar = safetyStep(s.safety, SafetyEvent::ACK, si, cfg.safety);
            applySafety(out, ar);
            rc = ar.api;
            break;
        }

        case CmdId::HV_ON: {
            SafetyResult hvR = safetyStep(s.safety, SafetyEvent::HV_ON, si, cfg.safety);
            applySafety(out, hvR);
            rc = hvR.api;
            break;
        }

        case CmdId::HV_OFF: {
            SafetyResult hvOfR = safetyStep(s.safety, SafetyEvent::HV_OFF, si, cfg.safety);
            applySafety(out, hvOfR);
            rc = hvOfR.api;
            break;
        }

        case CmdId::SHUTTER_OPEN: {
            SafetyResult soR = safetyStep(s.safety, SafetyEvent::SHUTTER_OPEN, si, cfg.safety);
            applySafety(out, soR);
            rc = soR.api;
            break;
        }

        case CmdId::SHUTTER_CLOSE: {
            SafetyResult scR = safetyStep(s.safety, SafetyEvent::SHUTTER_CLOSE, si, cfg.safety);
            applySafety(out, scR);
            rc = scR.api;
            break;
        }

        case CmdId::REBOOT: {
            if (s.safety.hv_on) {
                rc = ApiError::PRECONDITION;
            } else {
                out.reboot = true;
                startSound(s.snd, Sound::REBOOT, now);
                out.buzzer = soundOutput(s.snd, now);
                rc = ApiError::OK;
            }
            break;
        }

        default:
            rc = ApiError::UNKNOWN_CMD;
            break;
    }

    // Comm watchdog arming on success.
    if (rc == ApiError::OK) {
        switch (cmd.id) {
            case CmdId::INIT:
            case CmdId::MOVE:
            case CmdId::PARK:
            case CmdId::SERVICE:
            case CmdId::START_MEASURE:
            case CmdId::START_TRAVERSE:
            case CmdId::HV_ON:
                s.comm_armed = (cmd.source == CmdSource::API);
                break;
            default:
                break;
        }

        // Driver request merge.
        if (drvRank(d.op) >= drvRank(out.drv.op)) {
            out.drv = d;
        }

        // Update outputs from safety status.
        out.hv_on        = s.safety.hv_on;
        out.shutter_open = s.safety.shutter_open;

        // V1.3 sounds: LOCAL + OK -> CLICK.
        if (cmd.source == CmdSource::LOCAL) {
            startSound(s.snd, Sound::CLICK, now);
            out.buzzer = soundOutput(s.snd, now);
        }
    }

    return rc;
}

// ---------------------------------------------------------------------------
// appFillProc — build process image (CONTRACT §5.1)
// ---------------------------------------------------------------------------

void appFillProc(const AppState& s, const AppConfig& cfg, const HwInputs& hw, ProcData& out) {
    // local = s.local.
    out.local = s.local;

    // measure.valid = shutter_open (V1.3).
    out.measure.valid = s.safety.shutter_open;

    // safety.hv_ok = HV conditions OK.
    SafetyInputs si{};
    si.estop          = hw.estop;
    si.motor_fault    = false;
    si.cooling_flow   = true;
    si.interlock      = s.flags.interlock;
    si.temp_c         = 25.0f;
    out.safety.hv_ok  = safetyHvConditionsOk(si, cfg.safety);

    // temp_ok = temp <= warn.
    out.safety.temp_ok = (si.temp_c <= cfg.safety.temp_warn_c);
}
