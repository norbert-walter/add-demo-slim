/**
 * @file safety_logic.h
 * @brief Pure safety state machine (FROZEN CONTRACT V1.2 §6). No hardware access.
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
 */
#pragma once

#include "types.h"

struct SafetyConfig {
    float temp_warn_c  = 30.0f;  ///< HV allowed only if temp <= temp_warn_c
    float temp_fault_c = 32.0f;  ///< temp > temp_fault_c -> FAULT (TEMP_HIGH)
    float park_pos_mm  = 20.0f;  ///< V1.2: park zone 0..20 mm: shutter only open if pos > park_pos_mm
    float maint_max_mm = 60.0f;  ///< V1.2: service zone (park_pos_mm, maint_max_mm]: manual SHUTTER_OPEN only here
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
 * position check in R7 (automation: anywhere outside the park zone; manual: only
 * in the service zone).
 *
 * R1 Fault entry: if cur.state is not FAULT or WAIT_ACK and
 *    safetyFaultCause(in) != NONE  ->  FAULT, err = cause, hv off, shutter closed,
 *    motor_stop = true. Same for event INIT_FAILED (err = HOMING_FAILED) in any
 *    state except FAULT/WAIT_ACK.
 *    api: OK for TICK, HV_OFF, SHUTTER_CLOSE; STATE for all other events.
 *
 * R2 FAULT:    TICK -> WAIT_ACK (err kept, motor_stop = true).
 *              HV_OFF, SHUTTER_CLOSE -> OK, no change. Other events -> STATE.
 * R3 WAIT_ACK: outputs off, motor_stop = true on every step.
 *              ACK: if safetyFaultCause(in) == NONE -> RECOVER, err = NONE, OK;
 *                   else stay, api = PRECONDITION.
 *              TICK, HV_OFF, SHUTTER_CLOSE -> OK, no change. Other -> STATE.
 * R4 RECOVER:  TICK -> READY if in.homed, else SAFE_START.
 *              HV_OFF, SHUTTER_CLOSE -> OK. Other events -> STATE.
 * R5 SAFE_START: INIT_DONE -> READY. TICK, HV_OFF, SHUTTER_CLOSE -> OK.
 *              Other events (HV_ON, SHUTTER_OPEN, ACK) -> STATE.
 * R6 READY:    HV_ON: conditions ok -> RUNNING, hv on, OK; else PRECONDITION.
 *              INIT_DONE, TICK, HV_OFF, SHUTTER_CLOSE -> OK, no change.
 *              SHUTTER_OPEN, ACK -> STATE.
 * R7 RUNNING:  first, on EVERY event: if !safetyHvConditionsOk(in) -> READY,
 *              hv off, shutter closed; api = PRECONDITION for HV_ON,
 *              SHUTTER_OPEN and SHUTTER_AUTO_OPEN, OK for all others except ACK (STATE).
 *              Then (conditions ok):
 *              - if shutter open and pos_mm <= park_pos_mm -> shutter closed.
 *              - HV_ON -> OK (idempotent). HV_OFF -> READY, hv off, shutter closed.
 *              - SHUTTER_OPEN (manual, V1.2): park_pos_mm < pos_mm <= maint_max_mm
 *                -> open, OK; else PRECONDITION. Once open, the shutter stays open
 *                when the axis leaves the service zone (closing rules above apply).
 *              - SHUTTER_AUTO_OPEN: pos_mm > park_pos_mm -> open, OK; else PRECONDITION.
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
 * Automatic shutter for measurement (CONTRACT §6.4). Returns the event the
 * control task must feed into safetyStep, or TICK if nothing is to be done.
 * Evaluated in this order (first match wins):
 *   1. acquisition_ended && shutter_open                                 -> SHUTTER_CLOSE
 *   2. acquiring && state RUNNING && !shutter_open && pos_mm > park_pos_mm -> SHUTTER_AUTO_OPEN
 * acquiring        : motion sequencer is in a measuring phase (TRAV_RUN, MEAS_HOLD)
 * acquisition_ended: one-shot, a measuring mode ended this cycle
 */
SafetyEvent safetyShutterAutomation(bool acquiring, bool acquisition_ended,
                                    const SafetyStatus& s, float pos_mm,
                                    const SafetyConfig& cfg);

/**
 * Communication watchdog. Wrap-around safe for uint32 milliseconds.
 * @return armed && (now_ms - last_rx_ms) > timeout_ms
 */
bool safetyCommTimeout(bool armed, uint32_t now_ms, uint32_t last_rx_ms, uint32_t timeout_ms);
