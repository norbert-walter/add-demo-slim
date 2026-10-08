/**
 * @file app_logic.h
 * @brief Complete application logic as pure functions (FROZEN CONTRACT §10).
 *
 * Implementation: src/logic/app_logic.cpp (Sprint S2.4).
 *
 * This is the "brain" of the device. The control task (src/app/control_task.cpp)
 * only reads hardware into HwInputs, calls these functions and writes HwOutputs
 * back to hardware. Everything here runs natively in tests, including complete
 * scenarios with a simulated stepper driver.
 *
 * Call order in the control task (1 kHz cycle, Core 1):
 *   1. every 20 ms:  appMeasure(state, cfg, median_mv, now)
 *   2. every cycle:  out = appTick(state, hw, cfg)
 *   3. if out.drv.op == NONE and a command is queued:
 *                    err = appCommand(state, cmd, hw, cfg, out)   (max. ONE per cycle)
 *   4. apply out to hardware in the order: shutter, motor request, HV LED
 *   5. appFillProc(state, cfg, hw, proc) -> publish snapshot
 */
#pragma once

#include "types.h"
#include "safety_logic.h"
#include "measure_logic.h"
#include "motion_seq.h"
#include "params.h"

struct AppConfig {
    SafetyConfig safety;
    MotionConfig motion;
    SensorCurve  sensor;
    uint32_t     comm_timeout_ms = 5000;
};

/// Everything the application remembers between cycles. Owned by the control task.
struct AppState {
    SafetyStatus     safety;
    MotionSeq        seq;
    Params           params;
    SafetyFlags      flags;
    AcqStats         stats;
    StabilityTracker stab;
    Iir              iir;

    float raw_mm      = 0.0f;   ///< filtered, uncalibrated distance
    float dist_mm     = 0.0f;   ///< calibrated distance
    float thick_mm    = 0.0f;
    bool  in_range    = false;
    bool  on_material = false;

    bool  comm_armed  = false;  ///< communication watchdog armed
};

/// Hardware readings for one cycle (filled by the control task from HAL + driver).
struct HwInputs {
    uint32_t now_ms           = 0;
    bool     estop            = false;  ///< decoded: true = E-stop active
    bool     limit_fwd        = false;
    bool     limit_bwd        = false;
    bool     drv_running      = false;
    int32_t  drv_pos_steps    = 0;
    float    drv_speed_mm_min = 0.0f;   ///< magnitude
    uint32_t last_api_rx_ms   = 0;      ///< time of last valid /api packet (written by HTTP task)
};

/// What the control task must do this cycle.
struct HwOutputs {
    bool       hv_on        = false;   ///< HV LED (i2s slot)
    bool       shutter_open = false;   ///< shutter LED (GPIO32)
    DrvRequest drv;                    ///< at most one driver request
    bool       persist      = false;   ///< save Params to NVS (outside the 1 ms budget is ok)
    bool       reboot       = false;   ///< restart after the reply was sent
    bool       beep         = false;   ///< one short beep (fault entry, ACK, homing done)
};

/// Defaults: safetyInitialStatus(), MotionSeq{}, paramsDefaults(), SafetyFlags{}, ...
void appInit(AppState& s);

/**
 * Measurement update (50 Hz). Chain (CONTRACT §7):
 *   mm = sensorMvToMm(median_mv); in_range = sensorInRange(mm)
 *   in_range:  raw = iirUpdate(iir, mm); dist = calibApply(raw, params.calib);
 *              thick = thickness(dist, base_dist); on_material = onMaterial(thick, edge_threshold);
 *              stabilityUpdate(stab, now, dist)
 *   !in_range: iirReset(iir); raw = dist = mm; thick = 0; on_material = false;
 *              stab.initialized = false
 *   if motionAcquiring(seq): statsAdd(stats, thick, on_material, valid = shutter_open && in_range)
 */
void appMeasure(AppState& s, const AppConfig& cfg, float median_mv, uint32_t now_ms);

/**
 * Cyclic logic (every cycle). Steps in this exact order:
 *  1. d = motionTick(seq, mi); ev = motionTakeEvents(seq)
 *  2. build SafetyInputs (estop, flags, ev.motor_fault, comm timeout via
 *     safetyCommTimeout(comm_armed, now, last_api_rx_ms, comm_timeout_ms),
 *     pos from drv_pos_steps, homed)
 *  3. ev.homing_done -> safetyStep(INIT_DONE); ev.homing_failed -> safetyStep(INIT_FAILED)
 *  4. safetyStep(TICK)
 *  5. any step in 3/4 with motor_stop && (seq.phase != IDLE || drv_running)
 *        -> motionEmergencyStop, d = FORCE_STOP (overrides d)
 *  6. state != RUNNING && seq.mode != NONE -> motionStop (d = STOP unless d is FORCE_STOP)
 *  7. ev.acq_started -> statsReset(stats)
 *  8. e = safetyShutterAutomation(motionAcquiring(seq), ev.acq_ended || acq ended in 5/6, ...)
 *     e != TICK -> safetyStep(e)
 *  9. comm_armed && seq.phase == IDLE && !safety.hv_on -> comm_armed = false
 * 10. out.hv_on/shutter_open from safety status; out.drv = d;
 *     out.beep = entered FAULT this cycle || ev.homing_done
 */
HwOutputs appTick(AppState& s, const HwInputs& hw, const AppConfig& cfg);

/**
 * Executes one command (CONTRACT §6.3, §9). Never blocks.
 *  - GET_STATE, GET_CONFIG: OK, no action (answered by the HTTP task from the snapshot).
 *  - gate: safetyCmdAllowed(state, id) != OK -> return it.
 *  - INIT/MOVE/START_*: motion_seq start functions. Defaults: MOVE speed =
 *    approach_speed; START_MEASURE pos = params.measure_pos; START_TRAVERSE
 *    start/end/speed = params.traverse_*.
 *  - STOP: motionStop, OK.
 *  - SET: paramSet; persist -> out.persist.
 *  - CALIB: seq.phase != IDLE -> BUSY.
 *      NEAR/FAR: !stabilityIsStable -> NOT_STABLE; near_raw/far_raw = raw_mm.
 *      BASE:     !in_range -> PRECONDITION; !stable -> NOT_STABLE; base_dist = dist_mm.
 *      SAVE:     !calibValid -> CALIB_INVALID; else out.persist.
 *      RESET:    calib = CalibPoints{} (identity), OK, no persist.
 *  - ACK, HV_ON, HV_OFF, SHUTTER_OPEN, SHUTTER_CLOSE: safetyStep with the event;
 *    return its api; motor_stop handled as in appTick step 5.
 *  - REBOOT: hv_on -> PRECONDITION; else out.reboot.
 *  - Comm watchdog arming on success of INIT, MOVE, START_*, HV_ON:
 *    source API -> comm_armed = true; source LOCAL -> comm_armed = false.
 *  - The command's driver request goes to out.drv (caller guarantees out.drv was NONE).
 *  - out.hv_on/shutter_open are updated to the new safety status.
 *  - Motion events raised here (e.g. acq_ended by STOP) stay pending in seq.ev
 *    and are processed by the next appTick (shutter closes one cycle later).
 */
ApiError appCommand(AppState& s, const Command& cmd, const HwInputs& hw,
                    const AppConfig& cfg, HwOutputs& out);

/// Builds the process image (CONTRACT §5.1). measure.valid = shutter_open;
/// safety.hv_ok = safetyHvConditionsOk(current inputs); temp_ok = temp <= warn.
void appFillProc(const AppState& s, const AppConfig& cfg, const HwInputs& hw, ProcData& out);
