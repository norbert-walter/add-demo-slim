/**
 * @file params.h
 * @brief Parameters, simulated safety flags and the SET key table (FROZEN CONTRACT V1.5 §9).
 *
 * Implementation: src/logic/params.cpp (Sprint L2).
 *
 * SET keys (key | type | default | valid range | persisted in NVS):
 *   interlock       bool    false   -                    no  (safety flag)
 *   cooling_flow    bool    false   -                    no  (safety flag)
 *   temp            number  18.0    -20.0 .. 99.9 °C     no  (safety flag)
 *   measure_pos     number  155.0   61.0 .. 242.0 mm     yes  (V1.4.2: centre of step 3; V1.5: measuring zone)
 *   maint_pos       number  40.0    21.0 .. 60.0 mm      yes  (V1.2: service position, service zone)
 *   traverse_start  number  80.0    61.0 .. 242.0 mm     yes  (V1.5: measuring zone, default 80)
 *   traverse_end    number  240.0   61.0 .. 242.0 mm     yes  (V1.5: measuring zone)
 *   traverse_speed  number  10.0    1 .. 20 mm/s         yes  (V1.3: mm/s)
 *   shutter_hold    number  3.0     0.0 .. 10.0 s        yes  (V1.5: automatic shutter stays open this
 *                                                              long after a measuring mode ended)
 *   edge_threshold  number  3.0     0.5 .. 100.0 mm      yes  (thickness threshold!)
 *   base_dist       number  70.0    40.0 .. 300.0 mm     yes  (V1.2: virtual reference plane,
 *                                                              thick = base_dist - dist; 70 mm = material edge)
 *   calib_near_ref  number  40.0    40.0 .. 300.0 mm     yes
 *   calib_far_ref   number  65.0    40.0 .. 300.0 mm     yes
 *   estop_nc        bool    false   -                    yes  (V1.3: E-stop wiring, see below)
 *   wifi_ssid       string  ""      0 .. 32 chars        yes  (empty = no router connection)
 *   wifi_pass       string  ""      0 or 8 .. 63 chars   yes  (never reported back)
 *
 * String rule: only printable ASCII 0x20..0x7E, without '"' and '\'.
 * Type rules: bool keys accept JSON true/false or the numbers 0 and 1 (exactly).
 * Number keys accept numbers only. String keys accept strings only.
 * estop_nc (V1.3): false = normally-open switch to GND (pull-up), E-stop active
 *   when LOW, NO wire-break detection (default, simple test switch);
 *   true = normally-closed contact to GND, E-stop active when HIGH, a broken wire
 *   is detected as E-stop. A wrong setting always shows a permanent E-stop (safe).
 *   app_logic accepts SET estop_nc only while the axis is idle and HV is off.
 * Safety flags are NEVER persisted: after every boot interlock=false,
 * cooling_flow=false, temp=18.0 (fail-safe).
 */
#pragma once

#include "types.h"
#include "measure_logic.h"

struct Params {
    float       measure_pos    = 155.0f;    ///< V1.4.2: centre of step 3 (145 was a step edge)
    float       maint_pos      = 40.0f;
    float       traverse_start = 80.0f;     ///< V1.5 (was 60, now in the measuring zone)
    float       traverse_end   = 240.0f;
    float       traverse_speed = 10.0f;     ///< mm/s
    float       shutter_hold   = 3.0f;      ///< V1.5: s, automatic shutter hold time
    float       edge_threshold = 3.0f;
    float       base_dist      = 70.0f;
    CalibPoints calib;                  ///< near_ref/far_ref settable via SET, *_raw via CALIB
    bool        estop_nc       = false; ///< V1.3: E-stop wiring (see above)
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
 * @return OK, or BAD_PARAM (unknown key, wrong type, out of range incl. NaN and +-Inf (V1.4.1), too long)
 */
ApiError paramSet(Params& p, SafetyFlags& f, const char* key,
                  ValueType vtype, double num, bool b, const char* str, bool* persist);

/**
 * Writes the configuration as ONE JSON member (no surrounding braces), exactly:
 *   "config":{"measure_pos":155.0,"maint_pos":40.0,"traverse_start":80.0,
 *   "traverse_end":240.0,"traverse_speed":10.0,"shutter_hold":3.0,"edge_threshold":3.0,"base_dist":70.0,
 *   "calib_near_raw":40.0,"calib_near_ref":40.0,"calib_far_raw":65.0,
 *   "calib_far_ref":65.0,"calib_valid":true,"interlock":false,
 *   "cooling_flow":false,"temp":18.0,"estop_nc":false,"wifi_ssid":"","wifi_pass_set":false}
 * (one line, no spaces).
 * Floats with one decimal. wifi_pass is never written; instead
 * "wifi_pass_set":true|false. Also contains the calibration raw values and
 * "calib_valid".
 * @return length written (without NUL) or -1 if cap is too small.
 */
int paramsToJsonMember(const Params& p, const SafetyFlags& f, char* buf, size_t cap);
