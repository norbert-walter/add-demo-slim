/**
 * @file params.cpp
 * @brief Parameter defaults, SET handling and JSON serialization (CONTRACT V1.5.5 §9).
 *
 * Pure C++17: only <cmath>, <cstddef>, <cstring> — no Arduino, no ESP-IDF.
 */
#include "params.h"

#include <cmath>      // isfinite
#include <cstddef>    // offsetof
#include <cstring>    // strlen, strcpy, strcmp
#include <cstdio>     // snprintf

// ---------------------------------------------------------------------------
// 1. paramsDefaults
// ---------------------------------------------------------------------------
void paramsDefaults(Params& p) {
    p.measure_pos     = 155.0f;
    p.maint_pos       = 40.0f;
    p.traverse_start  = 80.0f;
    p.traverse_end    = 240.0f;
    p.traverse_speed  = 10.0f;
    p.shutter_hold    = 3.0f;
    p.edge_threshold  = 3.0f;
    p.base_dist       = 70.0f;
    p.calib           = CalibTable{};
    p.estop_nc        = false;
    p.wifi_ssid[0]    = '\0';
    p.wifi_pass[0]    = '\0';
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool namesEqual(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) { if (*a++ != *b++) return false; }
    return *a == '\0' && *b == '\0';
}

/// Printable ASCII 0x20..0x7E, no '"'/\'.
static bool validStringChars(const char* s) {
    while (*s) {
        unsigned char c = static_cast<unsigned char>(*s);
        if (c < 0x20 || c > 0x7E || *s == '"' || *s == '\\') return false;
        ++s;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 2. paramSet – table-driven for numeric keys, explicit branches for bool/string
// ---------------------------------------------------------------------------

struct NumEntry { const char* key; float lo, hi;
    void apply(Params& p, float v) const;
};
void NumEntry::apply(Params& p, float v) const {
    if (strcmp(key, "measure_pos") == 0)     p.measure_pos      = v;
    else if (strcmp(key, "maint_pos") == 0)       p.maint_pos        = v;
    else if (strcmp(key, "traverse_start") == 0)  p.traverse_start   = v;
    else if (strcmp(key, "traverse_end") == 0)    p.traverse_end     = v;
    else if (strcmp(key, "traverse_speed") == 0)  p.traverse_speed   = v;
    else if (strcmp(key, "shutter_hold") == 0)    p.shutter_hold     = v;
    else if (strcmp(key, "edge_threshold") == 0)  p.edge_threshold   = v;
    else if (strcmp(key, "base_dist") == 0)       p.base_dist        = v;
    else if (strcmp(key, "calib_ref_1") == 0)     p.calib.ref[0]     = v;
    else if (strcmp(key, "calib_ref_2") == 0)     p.calib.ref[1]     = v;
    else if (strcmp(key, "calib_ref_3") == 0)     p.calib.ref[2]     = v;
    else if (strcmp(key, "calib_ref_4") == 0)     p.calib.ref[3]     = v;
    else if (strcmp(key, "calib_ref_5") == 0)     p.calib.ref[4]     = v;
    else if (strcmp(key, "calib_ref_6") == 0)     p.calib.ref[5]     = v;
}

ApiError paramSet(Params& p, SafetyFlags& f, const char* key,
                         ValueType vtype, double num, bool b, const char* str,
                         bool* persist) {
    if (!key) return ApiError::BAD_PARAM;

    // --- bool keys --------------------------------------------------------
    if (namesEqual(key, "interlock")) {
        if (vtype == ValueType::BOOL)          { f.interlock = b; *persist = false; return ApiError::OK; }
        if (vtype == ValueType::NUMBER) {
            if (!std::isfinite(num)) return ApiError::BAD_PARAM;
            if (num != 0.0 && num != 1.0) return ApiError::BAD_PARAM;
            f.interlock = (num == 1.0); *persist = false; return ApiError::OK;
        }
        return ApiError::BAD_PARAM;
    }
    if (namesEqual(key, "cooling_flow")) {
        if (vtype == ValueType::BOOL)          { f.cooling_flow = b; *persist = false; return ApiError::OK; }
        if (vtype == ValueType::NUMBER) {
            if (!std::isfinite(num)) return ApiError::BAD_PARAM;
            if (num != 0.0 && num != 1.0) return ApiError::BAD_PARAM;
            f.cooling_flow = (num == 1.0); *persist = false; return ApiError::OK;
        }
        return ApiError::BAD_PARAM;
    }
    if (namesEqual(key, "temp")) {
        if (vtype == ValueType::NUMBER) {
            if (!std::isfinite(num)) return ApiError::BAD_PARAM;
            if (num < -20.0 || num > 99.9) return ApiError::BAD_PARAM;
            f.temp_c = static_cast<float>(num); *persist = false; return ApiError::OK;
        }
        return ApiError::BAD_PARAM;
    }
    if (namesEqual(key, "estop_nc")) {
        if (vtype == ValueType::BOOL)          { p.estop_nc = b; *persist = true;  return ApiError::OK; }
        if (vtype == ValueType::NUMBER) {
            if (!std::isfinite(num)) return ApiError::BAD_PARAM;
            if (num != 0.0 && num != 1.0) return ApiError::BAD_PARAM;
            p.estop_nc = (num == 1.0); *persist = true;  return ApiError::OK;
        }
        return ApiError::BAD_PARAM;
    }

    // --- numeric keys (table-driven) --------------------------------------
    static const NumEntry numTable[] = {
        {"measure_pos",      61.0f, 242.0f},
        {"maint_pos",        21.0f,  60.0f},
        {"traverse_start",   61.0f, 242.0f},
        {"traverse_end",     61.0f, 242.0f},
        {"traverse_speed",    1.0f,  20.0f},
        {"shutter_hold",      0.0f,  10.0f},
        {"edge_threshold",    0.5f, 100.0f},
        {"base_dist",        40.0f, 300.0f},
        {"calib_ref_1",      40.0f, 300.0f},
        {"calib_ref_2",      40.0f, 300.0f},
        {"calib_ref_3",      40.0f, 300.0f},
        {"calib_ref_4",      40.0f, 300.0f},
        {"calib_ref_5",      40.0f, 300.0f},
        {"calib_ref_6",      40.0f, 300.0f},
    };

    for (const auto& e : numTable) {
        if (namesEqual(key, e.key)) {
            if (vtype != ValueType::NUMBER) return ApiError::BAD_PARAM;
            if (!std::isfinite(num))             return ApiError::BAD_PARAM;
            if (num < e.lo || num > e.hi)     return ApiError::BAD_PARAM;
            float fv = static_cast<float>(num);
            e.apply(p, fv);
            *persist = true;
            return ApiError::OK;
        }
    }

    // --- string keys ------------------------------------------------------
    if (namesEqual(key, "wifi_ssid")) {
        if (vtype != ValueType::STRING) return ApiError::BAD_PARAM;
        size_t len = strlen(str);
        if (len > 32 || !validStringChars(str)) return ApiError::BAD_PARAM;
        strcpy(p.wifi_ssid, str);
        *persist = true;
        return ApiError::OK;
    }
    if (namesEqual(key, "wifi_pass")) {
        if (vtype != ValueType::STRING) return ApiError::BAD_PARAM;
        size_t len = strlen(str);
        if ((len > 0 && len < 8) || len > 63 || !validStringChars(str))
            return ApiError::BAD_PARAM;
        strcpy(p.wifi_pass, str);
        *persist = true;
        return ApiError::OK;
    }

    // unknown key
    return ApiError::BAD_PARAM;
}

// ---------------------------------------------------------------------------
// 3. paramsToJsonMember – single snprintf pass
// ---------------------------------------------------------------------------
int paramsToJsonMember(const Params& p, const SafetyFlags& f, char* buf, size_t cap) {
    int len = snprintf(buf, cap,
        "\"config\":{\"measure_pos\":%.1f,\"maint_pos\":%.1f,\"traverse_start\":%.1f,"
        "\"traverse_end\":%.1f,\"traverse_speed\":%.1f,\"shutter_hold\":%.1f,\"edge_threshold\":%.1f,\"base_dist\":%.1f,"
        "\"calib_raw_1\":%.1f,\"calib_ref_1\":%.1f,\"calib_raw_2\":%.1f,\"calib_ref_2\":%.1f,"
        "\"calib_raw_3\":%.1f,\"calib_ref_3\":%.1f,\"calib_raw_4\":%.1f,\"calib_ref_4\":%.1f,"
        "\"calib_raw_5\":%.1f,\"calib_ref_5\":%.1f,\"calib_raw_6\":%.1f,\"calib_ref_6\":%.1f,"
        "\"calib_valid\":%s,\"interlock\":%s,"
        "\"cooling_flow\":%s,\"temp\":%.1f,\"estop_nc\":%s,\"wifi_ssid\":\"%s\",\"wifi_pass_set\":%s}",
        static_cast<double>(p.measure_pos),
        static_cast<double>(p.maint_pos),
        static_cast<double>(p.traverse_start),
        static_cast<double>(p.traverse_end),
        static_cast<double>(p.traverse_speed),
        static_cast<double>(p.shutter_hold),
        static_cast<double>(p.edge_threshold),
        static_cast<double>(p.base_dist),
        static_cast<double>(p.calib.raw[0]),
        static_cast<double>(p.calib.ref[0]),
        static_cast<double>(p.calib.raw[1]),
        static_cast<double>(p.calib.ref[1]),
        static_cast<double>(p.calib.raw[2]),
        static_cast<double>(p.calib.ref[2]),
        static_cast<double>(p.calib.raw[3]),
        static_cast<double>(p.calib.ref[3]),
        static_cast<double>(p.calib.raw[4]),
        static_cast<double>(p.calib.ref[4]),
        static_cast<double>(p.calib.raw[5]),
        static_cast<double>(p.calib.ref[5]),
        calibValid(p.calib) ? "true" : "false",
        f.interlock   ? "true"  : "false",
        f.cooling_flow ? "true"  : "false",
        static_cast<double>(f.temp_c),
        p.estop_nc    ? "true"  : "false",
        p.wifi_ssid,
        p.wifi_pass[0] ? "true" : "false");

    if (len < 0) return -1;
    // cap must accommodate the NUL terminator: need len+1 bytes
    if (static_cast<size_t>(len + 1) > cap) return -1;
    return len;
}
