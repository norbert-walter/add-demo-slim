/**
 * @file app_logic.h
 * @brief Complete application logic as pure functions (FROZEN CONTRACT V1.5.9 §10).
 *
 * Implementation: src/logic/app_logic.cpp (Sprint L4).
 *
 * This is the "brain" of the device. The control task (src/app/control_task.cpp)
 * only reads hardware into HwInputs, calls these functions and writes HwOutputs
 * back to hardware. Everything here runs natively in tests, including complete
 * scenarios with a simulated stepper driver.
 *
 * Call order in the control task (1 kHz cycle, Core 1):
 *   1. every 20 ms:  appMeasure(state, cfg, median_mv, now)
 *   2. every cycle:  out = appTick(state, hw, cfg)
 *   3a. ALL pending priority commands (cmdQueueReceivePriority: up to
 *       CMD_PRIO_QUEUE_LEN from the queue + the whole overflow latch, max.
 *       CMD_PRIO_MAX_PER_CYCLE) -> appCommand each, in the order received.
 *       They are idempotent and act in the safe direction, so executing them
 *       together is always allowed (CONTRACT §6.7).
 *   3b. if out.drv.op == NONE: at most ONE further command (cmdQueueReceive)
 *       -> appCommand
 *   4. apply out to hardware in the order: shutter, motor request, HV LED
 *   5. reply to the commands (AFTER the outputs: "OK" means accepted AND outputs set)
 *   (V1.3: before step 2 the control task calls halSetEstopActiveHigh(params.estop_nc)
 *    and halUpdateInputs(); outputs include buzzer and heartbeat LED)
 *   6. appFillProc(state, cfg, hw, proc) -> publish snapshot
 */
#pragma once

#include "types.h"
#include "safety_logic.h"
#include "measure_logic.h"
#include "motion_seq.h"
#include "params.h"
#include "sound_logic.h"

struct AppConfig {
    SafetyConfig safety;
    MotionConfig motion;
    SensorCurve  sensor;
    uint32_t     comm_timeout_ms = 5000;
    uint32_t     local_hold_ms   = 30000;  ///< V1.1: LOCAL priority hold time after the last touch
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

    bool     local         = false;  ///< V1.1: LOCAL operator priority active (CONTRACT §6.6)
    uint32_t local_last_ms = 0;      ///< time of the last LOCAL command

    SoundPlayer snd;                 ///< V1.3: buzzer pattern player
    uint32_t    prog_active_ms = 0;  ///< V1.5: last cycle with seq.mode != NONE (shutter hold time)
    bool        boot_sound_done = false;   ///< V1.3: BOOT pattern started on the first appTick
};

/// Hardware readings for one cycle (filled by the control task from HAL + driver).
struct HwInputs {
    uint32_t now_ms           = 0;
    bool     estop            = false;  ///< decoded: true = E-stop active
    bool     limit_fwd        = false;
    bool     limit_bwd        = false;
    bool     drv_running      = false;
    int32_t  drv_pos_steps    = 0;
    float    drv_speed_mm_s   = 0.0f;   ///< magnitude, mm/s (V1.3)
    uint32_t last_api_rx_ms   = 0;      ///< time of last valid /api packet (written by HTTP task)
};

/// What the control task must do this cycle.
struct HwOutputs {
    bool       hv_on        = false;   ///< HV LED (GPIO 4)
    bool       shutter_open = false;   ///< shutter LED (GPIO 32)
    DrvRequest drv;                    ///< at most one driver request
    bool       persist      = false;   ///< save Params to NVS (outside the 1 ms budget is ok)
    bool       reboot       = false;   ///< restart REBOOT_DELAY_MS after the request (REBOOT pattern plays first)
    bool       buzzer       = false;   ///< V1.3: buzzer level this cycle (soundOutput)
    Sound      sound        = Sound::NONE;  ///< V1.3: pattern STARTED this cycle (one-shot, diagnostics/tests)
    bool       heartbeat    = false;   ///< V1.3: blue LED GPIO 2, 0.5 Hz: on while (now_ms / 1000) is even
};

/**
 * V1.1 – Priority commands (CONTRACT §6.7): STOP, HV_OFF, SHUTTER_CLOSE.
 * They are queued at the FRONT (cmdQueueSend) and executed even in a cycle
 * where appTick already produced a driver request.
 */
bool appCmdIsPriority(CmdId id);

/**
 * V1.1 – Commands an API client may NOT send while LOCAL priority is active
 * (CONTRACT §6.6): INIT, MOVE, PARK, SERVICE, START_MEASURE, START_TRAVERSE,
 * HV_ON, SHUTTER_OPEN, CALIB, ACK, REBOOT.  Always allowed: STOP, HV_OFF,
 * SHUTTER_CLOSE, SET, GET_STATE, GET_CONFIG.
 */
bool appLocalBlocksApi(CmdId id);

/**
 * V1.1 – overflow latch for priority commands (CONTRACT §6.7). Used by
 * command_queue when the priority queue is full: the request is stored as a
 * bit, so it can be coalesced with an identical pending one but never dropped.
 * The origin is stored PER COMMAND:
 *   bit 0 STOP/API, bit 1 HV_OFF/API, bit 2 SHUTTER_CLOSE/API,
 *   bit 3 STOP/LOCAL, bit 4 HV_OFF/LOCAL, bit 5 SHUTTER_CLOSE/LOCAL.
 * appPrioLatchSet: non-priority commands are ignored.
 * appPrioLatchTake: picks STOP before HV_OFF before SHUTTER_CLOSE, writes a fresh
 *   Command{id, source, want_reply = false} and clears both bits of that command.
 *   source = LOCAL if that command's LOCAL bit was set (an API and a LOCAL request
 *   of the SAME command coalesce into one LOCAL command – the operator did press
 *   it), else API. Another command's origin never influences it. false if none pending.
 */
void appPrioLatchSet(uint32_t& bits, const Command& cmd);
bool appPrioLatchTake(uint32_t& bits, Command& out);

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
 *     safetyCommTimeout(comm_armed, now, last_api_rx_ms, cfg.comm_timeout_ms),
 *     pos_mm = stepsToMm(drv_pos_steps, cfg.motion), homed)
 *     V1.5.9: always use the cfg fields (cfg.motion, cfg.comm_timeout_ms,
 *     cfg.local_hold_ms, ...), never their default values.
 *  3. ev.homing_done -> safetyStep(INIT_DONE); ev.homing_failed -> safetyStep(INIT_FAILED)
 *  4. safetyStep(TICK)
 *  5. any step in 3/4 with motor_stop && (seq.phase != IDLE || drv_running)
 *        -> motionEmergencyStop, d = FORCE_STOP (overrides d)
 *  6. state != RUNNING && seq.mode != NONE -> motionStop (d = STOP unless d is FORCE_STOP)
 *  7. ev.acq_started -> statsReset(stats)
 *  8. V1.5: active = seq.mode != NONE; active -> prog_active_ms = now;
 *     e = safetyShutterAutomation(motionAcquiring(seq), active,
 *                                 idle_ms = now - prog_active_ms (wrap-around safe),
 *                                 hold_ms = lroundf(params.shutter_hold * 1000), safety, pos, cfg.safety)
 *     e != TICK -> safetyStep(e)
 *     (ev.acq_ended no longer closes the shutter: it stays open over a measuring
 *      programme and closes after the hold time, on STOP, when entering the
 *      service zone, on HV loss or on a fault.)
 *  9. comm_armed && seq.phase == IDLE && !safety.hv_on -> comm_armed = false
 *  9b. V1.1: local && seq.phase == IDLE && !safety.hv_on &&
 *      (now - local_last_ms) >= local_hold_ms  -> local = false   (wrap-around safe)
 * 10. out.hv_on/shutter_open from safety status; out.drv = d
 * 11. V1.3 sounds (soundStart, priority rules of sound_logic.h):
 *     first appTick after appInit -> BOOT (boot_sound_done = true);
 *     entered FAULT this cycle -> ESTOP if safety.err == ESTOP else FAULT;
 *     ev.homing_done -> HOMED. out.sound = the pattern started this cycle.
 *     out.buzzer = soundOutput(snd, now); out.heartbeat = ((now / 1000) % 2) == 0
 */
HwOutputs appTick(AppState& s, const HwInputs& hw, const AppConfig& cfg);

/**
 * Executes one command (CONTRACT §6.3, §9). Never blocks.
 *  - GET_STATE, GET_CONFIG: OK, no action (answered by the HTTP task from the snapshot).
 *  - V1.1 LOCAL priority (checked BEFORE the state gate):
 *      source LOCAL: local = true, local_last_ms = now, comm_armed = false
 *                    (also if the command is then rejected by the gate);
 *      source API && local && appLocalBlocksApi(id) -> LOCAL_CONTROL, no action.
 *  - gate: safetyCmdAllowed(state, id) != OK -> return it.
 *  - PARK (V1.2):    motionStartMove(cfg.motion.park_target_mm, approach_speed).
 *  - SERVICE (V1.2): motionStartMove(params.maint_pos, approach_speed).
 *  - INIT/MOVE/START_*: motion_seq start functions. Defaults: MOVE speed =
 *    approach_speed; START_MEASURE pos = params.measure_pos; START_TRAVERSE
 *    start/end/speed = params.traverse_*.
 *  - STOP: motionStop, OK. V1.5: if safety.shutter_open && safety.shutter_auto
 *          -> safetyStep(SHUTTER_CLOSE) immediately (STOP ends the measuring programme).
 *  - SET: paramSet; persist -> out.persist.
 *    V1.3: key "estop_nc" only if seq.phase == IDLE && !safety.hv_on, else PRECONDITION.
 *  - CALIB: seq.phase != IDLE -> BUSY.
 *      NEAR/FAR: !stabilityIsStable -> NOT_STABLE; near_raw/far_raw = raw_mm.
 *      BASE:     !in_range -> PRECONDITION; !stable -> NOT_STABLE; base_dist = dist_mm.
 *      SAVE:     !calibValid -> CALIB_INVALID; else out.persist.
 *      RESET:    calib = CalibPoints{} (identity 40/40, 65/65), OK, no persist.
 *  - ACK, HV_ON, HV_OFF, SHUTTER_OPEN, SHUTTER_CLOSE: safetyStep with the event;
 *    return its api; motor_stop handled as in appTick step 5.
 *  - REBOOT: hv_on -> PRECONDITION; else out.reboot and sound REBOOT.
 *  - Comm watchdog arming on success of INIT, MOVE, PARK, SERVICE, START_*, HV_ON:
 *    source API -> comm_armed = true; source LOCAL -> comm_armed = false.
 *  - Driver request merge (V1.1): the command's request d replaces out.drv if
 *    rank(d) >= rank(out.drv), rank: NONE 0 < MOVE_TO/RUN_BACKWARD 1 < STOP 2
 *    < FORCE_STOP/ZERO_HERE 3 (both stop immediately). An immediate stop is
 *    never weakened to a decelerated one (e.g. ZERO_HERE at the home switch).
 *    (Non-priority commands are only called with out.drv == NONE.)
 *  - out.hv_on/shutter_open are updated to the new safety status.
 *  - V1.3 sounds: entered FAULT (e.g. HV_ON with E-stop) -> ESTOP/FAULT as in appTick;
 *    source LOCAL and result OK -> CLICK (lowest priority); out.sound / out.buzzer updated.
 *  - Motion events raised here (e.g. acq_ended by STOP) stay pending in seq.ev
 *    and are processed by the next appTick.
 */
ApiError appCommand(AppState& s, const Command& cmd, const HwInputs& hw,
                    const AppConfig& cfg, HwOutputs& out);

/// Builds the process image (CONTRACT §5.1). local = s.local; measure.valid = shutter_open;
/// safety.hv_ok = safetyHvConditionsOk(current inputs); temp_ok = temp <= warn.
void appFillProc(const AppState& s, const AppConfig& cfg, const HwInputs& hw, ProcData& out);
