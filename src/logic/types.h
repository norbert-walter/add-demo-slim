/**
 * @file types.h
 * @brief Shared types of the ADD Demo System (FROZEN CONTRACT V1.5).
 *
 * Pure C++ header: no Arduino, no ESP-IDF, no FreeRTOS includes.
 * Every module in src/logic/ must compile on the host (native tests).
 *
 * DO NOT MODIFY. Changes require a contract revision (docs/CONTRACT.md).
 * Implementation of the name functions: src/logic/types.cpp (Sprint L1).
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Safety state machine (CONTRACT §6)
// ---------------------------------------------------------------------------
enum class SafetyState : uint8_t {
    SAFE_START = 0,  ///< Boot / after recovery without valid homing: outputs off, INIT required
    READY      = 1,  ///< Homed, HV off, shutter closed
    RUNNING    = 2,  ///< HV on (shutter may be open)
    FAULT      = 3,  ///< Fault detected this cycle: outputs off, motor stopped (lasts one tick)
    WAIT_ACK   = 4,  ///< Waiting for ACK command; outputs off
    RECOVER    = 5   ///< ACK accepted; next tick -> READY (homed) or SAFE_START (not homed)
};

/// System error code reported as "err" in /proc. Latched until ACK.
enum class ErrorCode : uint8_t {
    NONE          = 0,
    ESTOP         = 1,  ///< E-stop input active
    TEMP_HIGH     = 2,  ///< Simulated cooling water temperature > TEMP_FAULT_C
    COMM_TIMEOUT  = 3,  ///< No /api packet for COMM_TIMEOUT_MS during a remote operation
    MOTOR_FAULT   = 4,  ///< Unexpected limit switch during motion
    HOMING_FAILED = 5   ///< Homing timeout or limit error during homing
};

/// Result code of an /api command, reported as "err" in the /api response.
/// Negative values are distinct from ErrorCode by design.
enum class ApiError : int8_t {
    OK            = 0,
    BAD_CRC       = -1,  ///< CRC32 frame missing or wrong
    BAD_JSON      = -2,  ///< Body is not a JSON object
    UNKNOWN_CMD   = -3,  ///< "cmd" missing or unknown
    BAD_PARAM     = -4,  ///< Parameter missing, wrong type or out of range
    STATE         = -5,  ///< Command not allowed in the current safety state
    PRECONDITION  = -6,  ///< Allowed in this state, but a condition is not met
    BUSY          = -7,  ///< Motion sequence active / queue full
    NOT_STABLE    = -8,  ///< Calibration capture: signal not stable
    CALIB_INVALID = -9,  ///< Calibration points do not form a valid 2-point calibration
    LOCAL_CONTROL = -10  ///< V1.1: operator at the display has priority (CONTRACT §6.6)
};

enum class CmdId : uint8_t {
    NONE = 0,
    INIT,            ///< Homing
    START_MEASURE,   ///< params: pos (opt.)
    START_TRAVERSE,  ///< params: start, end, speed (all opt.)
    STOP,            ///< Controlled stop of any motion
    MOVE,            ///< params: pos (req.), speed (opt.)
    GET_STATE,       ///< Heartbeat + short state
    GET_CONFIG,      ///< Returns all parameters
    SET,             ///< params: key, value
    CALIB,           ///< params: action = near|far|base|save|reset
    ACK,             ///< Acknowledge fault
    HV_ON,
    HV_OFF,
    SHUTTER_OPEN,
    SHUTTER_CLOSE,
    REBOOT,
    PARK,            ///< V1.2: move to the park position (MotionConfig.park_target_mm)
    SERVICE          ///< V1.2: move to the service/maintenance position (params.maint_pos)
};

enum class CmdSource : uint8_t { API = 0, LOCAL = 1 };  ///< LOCAL = touch display

enum class MotionState : uint8_t { IDLE = 0, MOVING = 1, HOMING = 2, ERROR = 3 };
enum class OpMode      : uint8_t { NONE = 0, MEASURE = 1, TRAVERSE = 2 };
enum class CalibAction : uint8_t { NONE = 0, NEAR = 1, FAR = 2, BASE = 3, SAVE = 4, RESET = 5 };
enum class ValueType   : uint8_t { NONE = 0, NUMBER = 1, BOOL = 2, STRING = 3 };

// ---------------------------------------------------------------------------
// Command (produced by api_parse or the touch display, consumed by app_logic)
// ---------------------------------------------------------------------------
struct Command {
    CmdId     id         = CmdId::NONE;
    CmdSource source     = CmdSource::API;
    uint32_t  seq        = 0;      ///< Sequence number for reply matching
    bool      want_reply = false;  ///< HTTP waits for the result, touch does not

    bool  has_pos = false, has_speed = false, has_start = false, has_end = false;
    float pos = 0.0f, speed = 0.0f, start = 0.0f, end = 0.0f;  ///< mm, mm/s (V1.3)

    char      key[24] = {0};       ///< SET: parameter key
    ValueType vtype   = ValueType::NONE;
    double    num     = 0.0;       ///< SET: numeric value
    bool      b       = false;     ///< SET: bool value
    char      str[64] = {0};       ///< SET: string value

    CalibAction calib = CalibAction::NONE;

    uint32_t  t_enq_us = 0;        ///< V1.1: enqueue time (µs, set by cmdQueueSend) for latency measurement
};

/// Result returned to the HTTP task for commands with want_reply = true.
struct CommandResult {
    uint32_t seq = 0;
    CmdId    id  = CmdId::NONE;
    ApiError err = ApiError::OK;
};

// ---------------------------------------------------------------------------
// Process image (single source for /proc, display and web UI) – CONTRACT §5
// ---------------------------------------------------------------------------
struct ProcData {
    uint32_t    ts_ms = 0;
    SafetyState state = SafetyState::SAFE_START;
    ErrorCode   err   = ErrorCode::NONE;
    bool        local = false;   ///< V1.1: LOCAL operator priority active (CONTRACT §6.6)

    struct {
        float       pos       = 0.0f;   ///< mm
        float       vel       = 0.0f;   ///< mm/s (unsigned magnitude, V1.3)
        MotionState state     = MotionState::IDLE;
        OpMode      mode      = OpMode::NONE;
        bool        homed     = false;
        bool        limit_fwd = false;
        bool        limit_bwd = false;
    } motion;

    struct {
        float    dist        = 0.0f;   ///< calibrated distance, mm
        float    thick       = 0.0f;   ///< thickness = base_dist - dist (>= 0), mm
        bool     on_material = false;  ///< thick > edge_threshold
        bool     valid       = false;  ///< true only while shutter is open
        bool     in_range    = false;  ///< sensor within 40..300 mm
        float    avg         = 0.0f;   ///< mean thickness of current/last acquisition
        uint32_t n           = 0;      ///< number of samples in avg
    } measure;

    struct {
        bool  estop        = false;
        bool  hv_on        = false;
        bool  hv_ok        = false;   ///< HV enable conditions currently met
        bool  shutter_open = false;
        bool  interlock    = false;
        bool  cooling_flow = false;
        float temp         = 18.0f;   ///< °C
        bool  temp_ok      = true;    ///< temp <= TEMP_WARN_C
        bool  comm_armed   = false;   ///< communication watchdog active
    } safety;
};

// ---------------------------------------------------------------------------
// Name mapping (types.cpp). Returned strings are static, never NULL.
// Names are the exact strings used in JSON (see CONTRACT §5).
// ---------------------------------------------------------------------------
const char* safetyStateName(SafetyState s);   ///< "SAFE_START", "READY", ...
const char* errorCodeName(ErrorCode e);       ///< "NONE", "ESTOP", "TEMP_HIGH", ...
const char* motionStateName(MotionState m);   ///< "idle", "moving", "homing", "error"
const char* opModeName(OpMode m);             ///< "none", "measure", "traverse"
const char* cmdName(CmdId c);                 ///< "INIT", "START_MEASURE", ..., "PARK", "SERVICE"; NONE -> "NONE"
CmdId       cmdFromName(const char* name);    ///< exact, case-sensitive; unknown/NULL -> NONE
CalibAction calibActionFromName(const char* name);  ///< "near","far","base","save","reset"; else NONE
const char* apiErrorMessage(ApiError e);      ///< short English text, OK -> "ok"
