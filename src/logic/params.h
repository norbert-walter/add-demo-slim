/**
 * @file params.h
 * @brief Parameters, simulated safety flags and the SET key table (FROZEN CONTRACT §9).
 *
 * Implementation: src/logic/params.cpp (Sprint S2.2).
 *
 * SET keys (key | type | default | valid range | persisted in NVS):
 *   interlock       bool    false   -                    no  (safety flag)
 *   cooling_flow    bool    false   -                    no  (safety flag)
 *   temp            number  18.0    -20.0 .. 99.9 °C     no  (safety flag)
 *   measure_pos     number  120.0   6.0 .. 240.0 mm      yes
 *   traverse_start  number  10.0    6.0 .. 240.0 mm      yes
 *   traverse_end    number  230.0   6.0 .. 240.0 mm      yes
 *   traverse_speed  number  200.0   10 .. 3000 mm/min    yes
 *   edge_threshold  number  3.0     0.5 .. 100.0 mm      yes  (thickness threshold!)
 *   base_dist       number  200.0   40.0 .. 300.0 mm     yes  (sensor -> empty support)
 *   calib_near_ref  number  50.0    40.0 .. 300.0 mm     yes
 *   calib_far_ref   number  200.0   40.0 .. 300.0 mm     yes
 *   wifi_ssid       string  ""      0 .. 32 chars        yes  (empty = AP mode)
 *   wifi_pass       string  ""      0 or 8 .. 63 chars   yes  (never reported back)
 *
 * String rule: only printable ASCII 0x20..0x7E, without '"' and '\'.
 * Type rules: bool keys accept JSON true/false or the numbers 0 and 1 (exactly).
 * Number keys accept numbers only. String keys accept strings only.
 * Safety flags are NEVER persisted: after every boot interlock=false,
 * cooling_flow=false, temp=18.0 (fail-safe).
 */
#pragma once

#include "types.h"
#include "measure_logic.h"

struct Params {
    float       measure_pos    = 120.0f;
    float       traverse_start = 10.0f;
    float       traverse_end   = 230.0f;
    float       traverse_speed = 200.0f;
    float       edge_threshold = 3.0f;
    float       base_dist      = 200.0f;
    CalibPoints calib;                  ///< near_ref/far_ref settable via SET, *_raw via CALIB
    char        wifi_ssid[33]  = {0};
    char        wifi_pass[64]  = {0};
};

struct SafetyFlags {
    bool  interlock    = false;
    bool  cooling_flow = false;
    float temp_c       = 18.0f;
};

/// Resets p to the defaults listed above (calib = identity 50/50, 200/200).
void paramsDefaults(Params& p);

/**
 * Applies one SET. On any error nothing is changed.
 * @param vtype/num/b/str  value as delivered by api_parse (Command fields)
 * @param persist          set to true if the key is persisted (caller saves NVS),
 *                         false otherwise; untouched on error
 * @return OK, or BAD_PARAM (unknown key, wrong type, out of range, too long)
 */
ApiError paramSet(Params& p, SafetyFlags& f, const char* key,
                  ValueType vtype, double num, bool b, const char* str, bool* persist);

/**
 * Writes the configuration as ONE JSON member (no surrounding braces), exactly:
 *   "config":{"measure_pos":120.0,"traverse_start":10.0,"traverse_end":230.0,
 *   "traverse_speed":200.0,"edge_threshold":3.0,"base_dist":200.0,
 *   "calib_near_raw":50.0,"calib_near_ref":50.0,"calib_far_raw":200.0,
 *   "calib_far_ref":200.0,"calib_valid":true,"interlock":false,
 *   "cooling_flow":false,"temp":18.0,"wifi_ssid":"","wifi_pass_set":false}
 * (one line, no spaces).
 * Floats with one decimal. wifi_pass is never written; instead
 * "wifi_pass_set":true|false. Also contains the calibration raw values and
 * "calib_valid".
 * @return length written (without NUL) or -1 if cap is too small.
 */
int paramsToJsonMember(const Params& p, const SafetyFlags& f, char* buf, size_t cap);
