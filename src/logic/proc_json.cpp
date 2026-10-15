/**
 * @file proc_json.cpp
 * @brief Serialisation of /proc and /api responses (FROZEN CONTRACT V1.5 §5).
 */

#include <cstdio>
#include <cmath>
#include "proc_json.h"
#include "crc32.h"

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/// Sanitize a float: non-finite → 0.0; clamp to [-99999.9, 99999.9]; |v| < 0.05 → 0.0.
static double fz(float v) {
    if (!std::isfinite(v)) return 0.0;
    if (v > 99999.9f) return 99999.9;
    if (v < -99999.9f) return -99999.9;
    if (std::fabs(v) < 0.05f) return 0.0;
    return static_cast<double>(v);
}

/// Boolean as JSON literal.
static const char* tf(bool b) {
    return b ? "true" : "false";
}

// ---------------------------------------------------------------------------
// procToJson – /proc document (one snprintf call)
// ---------------------------------------------------------------------------

int procToJson(const ProcData& d, char* buf, size_t cap) {
    int n = snprintf(buf, cap,
        "{\"ts\":%lu,\"state\":\"%s\",\"err\":%d,\"local\":%s,"
        "\"motion\":{\"pos\":%.1f,\"vel\":%.1f,\"state\":\"%s\",\"mode\":\"%s\","
        "\"homed\":%s,\"limit_fwd\":%s,\"limit_bwd\":%s},"
        "\"measure\":{\"dist\":%.1f,\"thick\":%.1f,\"on_material\":%s,\"valid\":%s,"
        "\"in_range\":%s,\"avg\":%.1f,\"n\":%lu},"
        "\"safety\":{\"estop\":%s,\"hv_on\":%s,\"hv_ok\":%s,\"shutter\":\"%s\","
        "\"interlock\":%s,\"cooling_flow\":%s,\"temp\":%.1f,\"temp_ok\":%s,"
        "\"comm_armed\":%s}}"
        , (unsigned long)d.ts_ms
        , safetyStateName(d.state)
        , static_cast<int>(d.err)
        , tf(d.local)
        , fz(d.motion.pos)
        , fz(d.motion.vel)
        , motionStateName(d.motion.state)
        , opModeName(d.motion.mode)
        , tf(d.motion.homed)
        , tf(d.motion.limit_fwd)
        , tf(d.motion.limit_bwd)
        , fz(d.measure.dist)
        , fz(d.measure.thick)
        , tf(d.measure.on_material)
        , tf(d.measure.valid)
        , tf(d.measure.in_range)
        , fz(d.measure.avg)
        , (unsigned long)d.measure.n
        , tf(d.safety.estop)
        , tf(d.safety.hv_on)
        , tf(d.safety.hv_ok)
        , d.safety.shutter_open ? "open" : "closed"
        , tf(d.safety.interlock)
        , tf(d.safety.cooling_flow)
        , fz(d.safety.temp)
        , tf(d.safety.temp_ok)
        , tf(d.safety.comm_armed)
    );
    if (n < 0 || static_cast<int>(cap) <= n) return -1;
    return crcFrameAppend(buf, static_cast<size_t>(n), cap);
}

// ---------------------------------------------------------------------------
// responseToJson – /api response (one snprintf per OK/error path)
// ---------------------------------------------------------------------------

int responseToJson(CmdId cmd, ApiError err, const char* msg, const char* extra,
                   char* buf, size_t cap) {
    int n;
    if (err == ApiError::OK) {
        // OK path – no "msg" field
        if (extra) {
            n = snprintf(buf, cap,
                "{\"status\":\"ok\",\"err\":%d,\"cmd\":\"%s\",%s}",
                static_cast<int>(err), cmdName(cmd), extra);
        } else {
            n = snprintf(buf, cap,
                "{\"status\":\"ok\",\"err\":%d,\"cmd\":\"%s\"}",
                static_cast<int>(err), cmdName(cmd));
        }
    } else {
        // Error path – "msg" always present
        const char* msgText = msg ? msg : apiErrorMessage(err);
        if (extra) {
            n = snprintf(buf, cap,
                "{\"status\":\"error\",\"err\":%d,\"cmd\":\"%s\",\"msg\":\"%s\",%s}",
                static_cast<int>(err), cmdName(cmd), msgText, extra);
        } else {
            n = snprintf(buf, cap,
                "{\"status\":\"error\",\"err\":%d,\"cmd\":\"%s\",\"msg\":\"%s\"}",
                static_cast<int>(err), cmdName(cmd), msgText);
        }
    }
    if (n < 0 || static_cast<int>(cap) <= n) return -1;
    return crcFrameAppend(buf, static_cast<size_t>(n), cap);
}

// ---------------------------------------------------------------------------
// stateExtraToJson – GET_STATE extras (one snprintf call, no CRC)
// ---------------------------------------------------------------------------

int stateExtraToJson(SafetyState state, ErrorCode err, bool local,
                     const CycleStats& cs, char* buf, size_t cap) {
    int n = snprintf(buf, cap,
        "\"state\":\"%s\",\"sys_err\":%d,\"local\":%s,"
        "\"cycle_max_us\":%lu,\"period_max_us\":%lu,\"overruns\":%lu,\"prio_lat_max_us\":%lu"
        , safetyStateName(state)
        , static_cast<int>(err)
        , tf(local)
        , (unsigned long)cs.cycle_max_us
        , (unsigned long)cs.period_max_us
        , (unsigned long)cs.overruns
        , (unsigned long)cs.prio_lat_max_us
    );
    if (n < 0 || static_cast<int>(cap) <= n) return -1;
    return n;
}
