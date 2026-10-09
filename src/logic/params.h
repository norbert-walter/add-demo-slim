/**
 * @file params.h
 * @brief Parameters, simulated safety flags and the SET key table (FROZEN CONTRACT V1.2 §9).
 *
 * Implementation: src/logic/params.cpp (Sprint S2.2).
 *
 * SET keys (key | type | default | valid range | persisted in NVS):
 *   interlock       bool    false   -                    no  (safety flag)
 *   cooling_flow    bool    false   -                    no  (safety flag)
 *   temp            number  18.0    -20.0 .. 99.9 °C     no  (safety flag)
 *   measure_pos     number  145.0   21.0 .. 248.0 mm     yes  (outside the park zone)
 *   maint_pos       number  40.0    21.0 .. 60.0 mm      yes  (V1.2: service position, service zone)
 *   traverse_start  number  60.0    21.0 .. 248.0 mm     yes
 *   traverse_end    number  240.0   21.0 .. 248.0 mm     yes
 *   traverse_speed  number  200.0   10 .. 3000 mm/min    yes
 *   edge_threshold  number  3.0     0.5 .. 100.0 mm      yes  (thickness threshold!)
 *   base_dist       number  70.0    40.0 .. 300.0 mm     yes  (V1.2: virtual reference plane,
 *                                                              thick = base_dist - dist; 70 mm = material edge)
 *   calib_near_ref  number  40.0    40.0 .. 300.0 mm     yes
 *   calib_far_ref   number  65.0    40.0 .. 300.0 mm     yes
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
    float       measure_pos    = 145.0f;
    float       maint_pos      = 40.0f;
    float       traverse_start = 60.0f;
    float       traverse_end   = 240.0f;
    float       traverse_speed = 200.0f;
    float       edge_threshold = 3.0f;
    float       base_dist      = 70.0f;
    CalibPoints calib;                  ///< near_ref/far_ref settable via SET, *_raw via CALIB
    char        wifi_ssid[33]  = {0};
    char        wifi_pass[64]  = {0};
};

struct SafetyFlags {
    bool  interlock    = false;
    bool  cooling_flow = false;
    float temp_c       = 18.0f;
};

/// Resets p to the defaults listed above (calib = identity 40/40, 65/65).
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
 *   "config":{"measure_pos":145.0,"maint_pos":40.0,"traverse_start":60.0,
 *   "traverse_end":240.0,"traverse_speed":200.0,"edge_threshold":3.0,"base_dist":70.0,
 *   "calib_near_raw":40.0,"calib_near_ref":40.0,"calib_far_raw":65.0,
 *   "calib_far_ref":65.0,"calib_valid":true,"interlock":false,
 *   "cooling_flow":false,"temp":18.0,"wifi_ssid":"","wifi_pass_set":false}
 * (one line, no spaces).
 * Floats with one decimal. wifi_pass is never written; instead
 * "wifi_pass_set":true|false. Also contains the calibration raw values and
 * "calib_valid".
 * @return length written (without NUL) or -1 if cap is too small.
 */
int paramsToJsonMember(const Params& p, const SafetyFlags& f, char* buf, size_t cap);
