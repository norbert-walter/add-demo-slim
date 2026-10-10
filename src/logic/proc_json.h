/**
 * @file proc_json.h
 * @brief Serialisation of /proc and /api responses (FROZEN CONTRACT V1.3 §5).
 *
 * Implementation: src/logic/proc_json.cpp (Sprint S3). snprintf only, no
 * heap, no ArduinoJson. Output is framed with crcFrameAppend().
 *
 * Number format: all floats "%.1f". Before printing every float is sanitised:
 * non-finite -> 0.0; clamped to [-99999.9, 99999.9]; |v| < 0.05 -> 0.0 (never "-0.0").
 * Integers "%lu"/"%d".
 */
#pragma once

#include "types.h"

/// Maximum size of a /proc document including the CRC frame and NUL.
constexpr size_t PROC_JSON_MAX = 512;

/**
 * Writes /proc exactly in this member order (one line, no spaces):
 * {"ts":<u32>,"state":"<SafetyState>","err":<ErrorCode int>,"local":<b>,
 *  "motion":{"pos":<f>,"vel":<f>,"state":"<motionState>","mode":"<opMode>",
 *            "homed":<b>,"limit_fwd":<b>,"limit_bwd":<b>},
 *  "measure":{"dist":<f>,"thick":<f>,"on_material":<b>,"valid":<b>,
 *             "in_range":<b>,"avg":<f>,"n":<u32>},
 *  "safety":{"estop":<b>,"hv_on":<b>,"hv_ok":<b>,"shutter":"open"|"closed",
 *            "interlock":<b>,"cooling_flow":<b>,"temp":<f>,"temp_ok":<b>,
 *            "comm_armed":<b>},
 *  "crc":"XXXXXXXX"}
 * @return length (without NUL) or -1 if cap is too small.
 *         With cap = PROC_JSON_MAX it never fails for any ProcData value.
 */
int procToJson(const ProcData& d, char* buf, size_t cap);

/**
 * Writes an /api response:
 *   {"status":"ok"|"error","err":<int>,"cmd":"<cmdName>"[,"msg":"<text>"][,<extra>],"crc":"XXXXXXXX"}
 * - status is "ok" iff err == OK.
 * - "msg" is present iff err != OK; text = msg if non-NULL, else apiErrorMessage(err).
 * - extra: optional pre-formatted JSON member list without leading comma
 *   (e.g. from paramsToJsonMember), or NULL.
 * @return length or -1 if cap is too small.
 */
int responseToJson(CmdId cmd, ApiError err, const char* msg, const char* extra,
                   char* buf, size_t cap);

/// V1.1 – control task timing, measured on the target since boot (CONTRACT §10).
struct CycleStats {
    uint32_t cycle_max_us    = 0;  ///< longest work time of one cycle (steps 1..6), µs
    uint32_t period_max_us   = 0;  ///< longest distance between two cycle starts (jitter), µs
    uint32_t overruns        = 0;  ///< cycles with work time > CONTROL_PERIOD_MS
    uint32_t prio_lat_max_us = 0;  ///< longest priority command latency: cmdQueueSend -> outputs applied, µs
};

/**
 * V1.1 – extra member list of the GET_STATE response (no leading comma, no frame):
 *   "state":"<SafetyState>","sys_err":<int>,"local":<b>,"cycle_max_us":<u32>,
 *   "period_max_us":<u32>,"overruns":<u32>,"prio_lat_max_us":<u32>
 * @return length (without NUL) or -1 if cap is too small (cap >= 192 never fails).
 */
int stateExtraToJson(SafetyState state, ErrorCode err, bool local, const CycleStats& cs,
                     char* buf, size_t cap);
