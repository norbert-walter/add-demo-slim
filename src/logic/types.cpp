/**
 * @file types.cpp
 * @brief Name mapping functions for shared types (Sprint S2.1).
 */
#include "types.h"

// ---------------------------------------------------------------------------
// Safety state names
// ---------------------------------------------------------------------------
const char* safetyStateName(SafetyState s) {
    switch (s) {
        case SafetyState::SAFE_START: return "SAFE_START";
        case SafetyState::READY:      return "READY";
        case SafetyState::RUNNING:    return "RUNNING";
        case SafetyState::FAULT:      return "FAULT";
        case SafetyState::WAIT_ACK:   return "WAIT_ACK";
        case SafetyState::RECOVER:    return "RECOVER";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Error code names
// ---------------------------------------------------------------------------
const char* errorCodeName(ErrorCode e) {
    switch (e) {
        case ErrorCode::NONE:          return "NONE";
        case ErrorCode::ESTOP:         return "ESTOP";
        case ErrorCode::TEMP_HIGH:     return "TEMP_HIGH";
        case ErrorCode::COMM_TIMEOUT:  return "COMM_TIMEOUT";
        case ErrorCode::MOTOR_FAULT:   return "MOTOR_FAULT";
        case ErrorCode::HOMING_FAILED: return "HOMING_FAILED";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Motion state names
// ---------------------------------------------------------------------------
const char* motionStateName(MotionState m) {
    switch (m) {
        case MotionState::IDLE:    return "idle";
        case MotionState::MOVING:  return "moving";
        case MotionState::HOMING:  return "homing";
        case MotionState::ERROR:   return "error";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Op mode names
// ---------------------------------------------------------------------------
const char* opModeName(OpMode m) {
    switch (m) {
        case OpMode::NONE:     return "none";
        case OpMode::MEASURE:  return "measure";
        case OpMode::TRAVERSE: return "traverse";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Command names and reverse lookup
// ---------------------------------------------------------------------------
const char* cmdName(CmdId c) {
    switch (c) {
        case CmdId::NONE:           return "NONE";
        case CmdId::INIT:           return "INIT";
        case CmdId::START_MEASURE:  return "START_MEASURE";
        case CmdId::START_TRAVERSE: return "START_TRAVERSE";
        case CmdId::STOP:           return "STOP";
        case CmdId::MOVE:           return "MOVE";
        case CmdId::GET_STATE:      return "GET_STATE";
        case CmdId::GET_CONFIG:     return "GET_CONFIG";
        case CmdId::SET:            return "SET";
        case CmdId::CALIB:          return "CALIB";
        case CmdId::ACK:            return "ACK";
        case CmdId::HV_ON:          return "HV_ON";
        case CmdId::HV_OFF:         return "HV_OFF";
        case CmdId::SHUTTER_OPEN:   return "SHUTTER_OPEN";
        case CmdId::SHUTTER_CLOSE:  return "SHUTTER_CLOSE";
        case CmdId::REBOOT:         return "REBOOT";
        case CmdId::PARK:           return "PARK";
        case CmdId::SERVICE:        return "SERVICE";
    }
    return "UNKNOWN";
}

static bool namesEqual(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) { if (*a++ != *b++) return false; }
    return *a == '\0' && *b == '\0';
}

CmdId cmdFromName(const char* name) {
    if (!name) return CmdId::NONE;
    if (namesEqual(name, "INIT"))              return CmdId::INIT;
    if (namesEqual(name, "START_MEASURE"))     return CmdId::START_MEASURE;
    if (namesEqual(name, "START_TRAVERSE"))    return CmdId::START_TRAVERSE;
    if (namesEqual(name, "STOP"))              return CmdId::STOP;
    if (namesEqual(name, "MOVE"))              return CmdId::MOVE;
    if (namesEqual(name, "GET_STATE"))         return CmdId::GET_STATE;
    if (namesEqual(name, "GET_CONFIG"))        return CmdId::GET_CONFIG;
    if (namesEqual(name, "SET"))               return CmdId::SET;
    if (namesEqual(name, "CALIB"))             return CmdId::CALIB;
    if (namesEqual(name, "ACK"))               return CmdId::ACK;
    if (namesEqual(name, "HV_ON"))             return CmdId::HV_ON;
    if (namesEqual(name, "HV_OFF"))            return CmdId::HV_OFF;
    if (namesEqual(name, "SHUTTER_OPEN"))      return CmdId::SHUTTER_OPEN;
    if (namesEqual(name, "SHUTTER_CLOSE"))     return CmdId::SHUTTER_CLOSE;
    if (namesEqual(name, "REBOOT"))            return CmdId::REBOOT;
    if (namesEqual(name, "PARK"))              return CmdId::PARK;
    if (namesEqual(name, "SERVICE"))           return CmdId::SERVICE;
    return CmdId::NONE;
}

// ---------------------------------------------------------------------------
// CalibAction reverse lookup (case-sensitive, lowercase only)
// ---------------------------------------------------------------------------
CalibAction calibActionFromName(const char* name) {
    if (!name) return CalibAction::NONE;
    if (namesEqual(name, "near"))  return CalibAction::NEAR;
    if (namesEqual(name, "far"))   return CalibAction::FAR;
    if (namesEqual(name, "base"))  return CalibAction::BASE;
    if (namesEqual(name, "save"))  return CalibAction::SAVE;
    if (namesEqual(name, "reset")) return CalibAction::RESET;
    return CalibAction::NONE;
}

// ---------------------------------------------------------------------------
// ApiError messages (short English text, no quotes — embeddable in JSON)
// ---------------------------------------------------------------------------
const char* apiErrorMessage(ApiError e) {
    switch (e) {
        case ApiError::OK:            return "ok";
        case ApiError::BAD_CRC:       return "bad CRC32";
        case ApiError::BAD_JSON:      return "invalid JSON";
        case ApiError::UNKNOWN_CMD:   return "unknown command";
        case ApiError::BAD_PARAM:     return "bad parameter";
        case ApiError::STATE:         return "not allowed in current state";
        case ApiError::PRECONDITION:  return "precondition not met";
        case ApiError::BUSY:          return "busy";
        case ApiError::NOT_STABLE:    return "signal not stable";
        case ApiError::CALIB_INVALID: return "invalid calibration";
        case ApiError::LOCAL_CONTROL: return "local control active";
    }
    return "unknown error";
}

