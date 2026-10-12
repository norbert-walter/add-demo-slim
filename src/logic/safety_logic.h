/**
 * @file safety_logic.h
 * @brief Pure safety state machine (FROZEN CONTRACT V1.5.3 §6). No hardware access.
 *
 * Implementation: src/logic/safety_logic.cpp (Sprint S2.1).
 * The machine is a pure function: given the current status, one event and the
 * current inputs it returns the next status. It never reads globals, never
 * blocks, never allocates. The control task calls it once per cycle (TICK)
 * plus once per accepted command event.
 *
 * INVARIANTS (checked by tests after every step):
 *   I1  shutter_open  =>  hv_on
 *   I2  hv_on         =>  state == RUNNING
 *   I3  state in {FAULT, WAIT_ACK, RECOVER}  =>  !hv_on && !shutter_open
 *   I4  err != NONE   <=>  state in {FAULT, WAIT_ACK}
 *   I5  shutter_open  =>  inputs.pos_mm > park_pos_mm
 *   I6  hv_on         =>  safetyHvConditionsOk(inputs)
 *   I7  shutter_auto  =>  shutter_open                                     (V1.5)
 *   I8  shutter_open && shutter_auto  =>  inputs.pos_mm > maint_max_mm     (V1.5)
 *
 * V1.5 zones (axis mm, origin always on the park side = homing switch):
 *   park zone     [0, park_pos_mm]                 shutter always closed
 *   service zone  (park_pos_mm, maint_max_mm]       shutter only manual (SHUTTER_OPEN)
 *   measuring zone (maint_max_mm, axis_max]         shutter only by the automation
 */
#pragma once

#include "types.h"

struct SafetyConfig {
    float temp_warn_c  = 30.0f;  ///< HV allowed only if temp <= temp_warn_c
    float temp_fault_c = 32.0f;  ///< temp > temp_fault_c -> FAULT (TEMP_HIGH)
    float park_pos_mm  = 20.0f;  ///< V1.2: park zone 0..20 mm: shutter only open if pos > park_pos_mm
    float maint_max_mm = 60.0f;  ///< V1.2: service zone (park_pos_mm, maint_max_mm]: manual SHUTTER_OPEN only here;
                                 ///< V1.5: the measuring zone starts above it (automatic shutter only there)
};

/// Snapshot of all safety-relevant inputs for one evaluation.
struct SafetyInputs {
    bool  estop        = false;  ///< E-stop input active (already debounced, fail-safe decoded)
    bool  interlock    = false;  ///< simulated key switch
    bool  cooling_flow = false;  ///< simulated cooling water flow
    float temp_c       = 18.0f;  ///< simulated cooling water temperature
    bool  motor_fault  = false;  ///< one-shot: unexpected limit switch this cycle
    bool  comm_timeout = false;  ///< communication watchdog expired (see safetyCommTimeout)
    float pos_mm       = 0.0f;   ///< current axis position
    bool  homed        = false;  ///< axis has a valid reference
};

enum class SafetyEvent : uint8_t {
    TICK = 0,       ///< cyclic evaluation, no command
    INIT_DONE,      ///< homing finished successfully
    INIT_FAILED,    ///< homing failed
    HV_ON,
    HV_OFF,
    SHUTTER_OPEN,
    SHUTTER_CLOSE,
    ACK,
    SHUTTER_AUTO_OPEN   ///< V1.2: opening by the shutter automation during an acquisition
};

/// Persistent status of the machine (owned by the control task).
struct SafetyStatus {
    SafetyState state        = SafetyState::SAFE_START;
    bool        hv_on        = false;
    bool        shutter_open = false;
    bool        shutter_auto = false;  ///< V1.5: the open shutter belongs to the automation
                                       ///< (opened or taken over by SHUTTER_AUTO_OPEN); false when closed
    ErrorCode   err          = ErrorCode::NONE;
};

struct SafetyResult {
    SafetyStatus next;                ///< new status
    bool         motor_stop = false;  ///< true -> control task must emergency-stop the axis now
    ApiError     api = ApiError::OK;  ///< result for the command that caused the event
};

/// Initial status after boot: SAFE_START, all outputs off, no error.
SafetyStatus safetyInitialStatus();

/// AND logic: cooling_flow && temp_c <= temp_warn_c && interlock && !estop.
bool safetyHvConditionsOk(const SafetyInputs& in, const SafetyConfig& cfg);

/**
 * Fault cause with fixed priority ESTOP > TEMP_HIGH > MOTOR_FAULT > COMM_TIMEOUT.
 * TEMP_HIGH means temp_c > temp_fault_c. Returns NONE if no fault condition.
 * (HOMING_FAILED is never returned here; it comes from the INIT_FAILED event.)
 */
ErrorCode safetyFaultCause(const SafetyInputs& in, const SafetyConfig& cfg);

/**
 * One step of the state machine. Rules, evaluated in this order:
 *
 * SHUTTER_AUTO_OPEN is treated like SHUTTER_OPEN in every rule, except for the
 * position check in R7 (V1.5: automation only in the measuring zone, pos_mm >
 * maint_max_mm; manual only in the service zone).
 * V1.5: whenever the shutter is closed (by any rule), shutter_auto = false.
 *
 * R1 Fault entry: if cur.state is not FAULT or WAIT_ACK and
 *    safetyFaultCause(in) != NONE  ->  FAULT, err = cause, hv off, shutter closed,
 *    motor_stop = true. Same for event INIT_FAILED (err = HOMING_FAILED) in any
 *    state except FAULT/WAIT_ACK.
 *    V1.5.3: R1 is checked FIRST, for EVERY event (including INIT_DONE), before any
 *    other rule. If a fault cause is active together with INIT_FAILED, the fault
 *    cause wins (err = cause, not HOMING_FAILED).
 *    api: OK for TICK, HV_OFF, SHUTTER_CLOSE; STATE for all other events.
 *
 * V1.5.2: in every state except RUNNING the result has hv_on = false,
 *    shutter_open = false and shutter_auto = false, even if cur (inconsistently)
 *    has them set ("outputs forced off outside RUNNING").
 *
 * R2 FAULT:    motor_stop = true on EVERY event (V1.5.2 clarification).
 *              TICK -> WAIT_ACK (err kept).
 *              HV_OFF, SHUTTER_CLOSE -> OK, no change. Other events -> STATE.
 * R3 WAIT_ACK: outputs off, motor_stop = true on EVERY event, including a
 *              rejected ACK (V1.5.2 clarification).
 *              ACK: if safetyFaultCause(in) == NONE -> RECOVER, err = NONE, OK;
 *                   else stay, api = PRECONDITION.
 *              TICK, HV_OFF, SHUTTER_CLOSE -> OK, no change. Other -> STATE.
 * R4 RECOVER:  TICK -> READY if in.homed, else SAFE_START.
 *              HV_OFF, SHUTTER_CLOSE -> OK. Other events -> STATE.
 * R5 SAFE_START: INIT_DONE -> READY (unconditionally; in.homed is NOT checked,
 *              INIT_DONE itself means homing succeeded). TICK, HV_OFF, SHUTTER_CLOSE -> OK.
 *              Other events (HV_ON, SHUTTER_OPEN, ACK) -> STATE.
 * R6 READY:    HV_ON: conditions ok -> RUNNING, hv on, OK; else PRECONDITION.
 *              INIT_DONE, TICK, HV_OFF, SHUTTER_CLOSE -> OK, no change.
 *              SHUTTER_OPEN, ACK -> STATE.
 * R7 RUNNING:  first, on EVERY event: if !safetyHvConditionsOk(in) -> READY,
 *              hv off, shutter closed; api = PRECONDITION for HV_ON,
 *              SHUTTER_OPEN and SHUTTER_AUTO_OPEN, OK for all others
 *              (including INIT_DONE and TICK) except ACK (STATE).
 *              Then (conditions ok):
 *              - if shutter open and pos_mm <= park_pos_mm -> shutter closed.
 *              - V1.5: if shutter open && shutter_auto && pos_mm <= maint_max_mm
 *                -> shutter closed (automatic shutter closes when entering the service zone).
 *              - HV_ON -> OK (idempotent). HV_OFF -> READY, hv off, shutter closed.
 *              - SHUTTER_OPEN (manual, V1.2): park_pos_mm < pos_mm <= maint_max_mm
 *                -> open, shutter_auto = false, OK; else PRECONDITION. Once open, the
 *                shutter stays open when the axis leaves the service zone (closing
 *                rules above apply).
 *              - SHUTTER_AUTO_OPEN (V1.5): pos_mm > maint_max_mm -> open, shutter_auto =
 *                true, OK (also if it was already open: take-over); else PRECONDITION.
 *              - SHUTTER_CLOSE -> closed, OK. INIT_DONE, TICK -> OK. ACK -> STATE.
 */
SafetyResult safetyStep(const SafetyStatus& cur, SafetyEvent ev,
                        const SafetyInputs& in, const SafetyConfig& cfg);

/**
 * State gate for commands (CONTRACT §6.3). Returns OK or STATE.
 *   INIT                     : SAFE_START, READY
 *   MOVE, PARK, SERVICE      : READY, RUNNING
 *   START_MEASURE/TRAVERSE   : RUNNING
 *   CALIB                    : SAFE_START, READY, RUNNING
 *   ACK                      : WAIT_ACK
 *   HV_ON                    : READY, RUNNING
 *   SHUTTER_OPEN             : RUNNING
 *   REBOOT                   : SAFE_START, READY, WAIT_ACK
 *   STOP, SET, HV_OFF, SHUTTER_CLOSE, GET_STATE, GET_CONFIG : all states
 *   NONE                     : never (STATE)
 */
ApiError safetyCmdAllowed(SafetyState s, CmdId cmd);

/**
 * Automatic shutter for measurement (CONTRACT §6.4, V1.5). Returns the event the
 * control task must feed into safetyStep, or TICK if nothing is to be done.
 * The shutter stays open over a whole measuring programme (e.g. a continuous
 * back-and-forth profile scan) and closes only after the hold time without a
 * measuring mode. Evaluated in this order (first match wins):
 *   1. shutter_open && shutter_auto && !program_active && idle_ms >= hold_ms
 *                                                                  -> SHUTTER_CLOSE
 *   2. acquiring && state RUNNING && pos_mm > maint_max_mm &&
 *      (!shutter_open || !shutter_auto)                            -> SHUTTER_AUTO_OPEN
 *   otherwise                                                      -> TICK
 * acquiring      : motion sequencer is in a measuring phase (TRAV_RUN, MEAS_HOLD)
 * program_active : a measuring mode is active (seq.mode != NONE, incl. approach)
 * idle_ms        : time since program_active was last true
 * hold_ms        : shutter hold time (parameter shutter_hold); 0 = close at once
 * Closing when entering the service zone, on STOP (app_logic), HV loss and faults
 * is done elsewhere (safetyStep R1..R7, appCommand).
 */
SafetyEvent safetyShutterAutomation(bool acquiring, bool program_active,
                                    uint32_t idle_ms, uint32_t hold_ms,
                                    const SafetyStatus& s, float pos_mm,
                                    const SafetyConfig& cfg);

/**
 * Communication watchdog. Wrap-around safe for uint32 milliseconds.
 * @return armed && (now_ms - last_rx_ms) > timeout_ms
 */
bool safetyCommTimeout(bool armed, uint32_t now_ms, uint32_t last_rx_ms, uint32_t timeout_ms);
