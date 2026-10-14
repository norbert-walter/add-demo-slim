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
    float raw_unfiltered = sensorMvToMm(median_mv, cfg.sensor);

    // 2. Range check on the unfiltered reading
    bool in_range = sensorInRange(raw_unfiltered, cfg.sensor);

    if (!in_range) {
        // Reset filter and stability tracker
        iirReset(s.iir);
        s.stab = StabilityTracker{};
        s.raw_mm      = 0.0f;
        s.dist_mm     = 0.0f;
        s.thick_mm    = 0.0f;
        s.in_range    = false;
        s.on_material = false;
        return;
    }

    // 3. IIR filter → raw_mm (uncalibrated)
    s.raw_mm = iirUpdate(s.iir, raw_unfiltered);

    // 4. Calibration → dist_mm
    s.dist_mm = calibApply(s.raw_mm, s.params.calib);

    // 5. Range flag on the calibrated reading
    s.in_range = sensorInRange(s.dist_mm, cfg.sensor);

    // 6. Thickness and on_material
    s.thick_mm   = thickness(s.dist_mm, s.params.base_dist);
    s.on_material = onMaterial(s.thick_mm, s.params.edge_threshold);

    // 7. Stability update (always when in range)
    stabilityUpdate(s.stab, now_ms, s.dist_mm);

    // 8. Statistics: only while acquiring AND shutter open
    bool acquiring = (s.seq.phase != SeqPhase::IDLE);
    if (acquiring && s.safety.shutter_open) {
        statsAdd(s.stats, s.thick_mm, s.on_material, true);
    }
}

HwOutputs appTick(AppState& s, const HwInputs& hw, const AppConfig& cfg) { return HwOutputs{}; }

ApiError appCommand(AppState& s, const Command& cmd, const HwInputs& hw, const AppConfig& cfg,
                    HwOutputs& out) { return ApiError::UNKNOWN_CMD; }

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