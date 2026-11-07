// FROZEN TESTS (Contract V1.5.6, calibration table V1.6.0 / Sprint K1) – do not modify.
// Covers: src/logic/params.cpp (CONTRACT §9)
#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "logic/params.h"

static Params P;
static SafetyFlags F;
static bool persist;

void setUp() { paramsDefaults(P); F = SafetyFlags{}; persist = false; }
void tearDown() {}

static ApiError setNum(const char* k, double v) { persist = false; return paramSet(P, F, k, ValueType::NUMBER, v, false, "", &persist); }
static ApiError setBool(const char* k, bool v) { persist = false; return paramSet(P, F, k, ValueType::BOOL, 0, v, "", &persist); }
static ApiError setStr(const char* k, const char* v) { persist = false; return paramSet(P, F, k, ValueType::STRING, 0, false, v, &persist); }
#define OK(x)  TEST_ASSERT_EQUAL_INT((int)ApiError::OK, (int)(x))
#define BAD(x) TEST_ASSERT_EQUAL_INT((int)ApiError::BAD_PARAM, (int)(x))

void test_defaults() {
    Params p; p.measure_pos = 1; paramsDefaults(p);
    TEST_ASSERT_EQUAL_FLOAT(155.0f, p.measure_pos);
    TEST_ASSERT_EQUAL_FLOAT(40.0f, p.maint_pos);
    TEST_ASSERT_EQUAL_FLOAT(80.0f, p.traverse_start);        // V1.5: in the measuring zone
    TEST_ASSERT_EQUAL_FLOAT(240.0f, p.traverse_end);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, p.traverse_speed);        // mm/s (V1.3)
    TEST_ASSERT_EQUAL_FLOAT(3.0f, p.shutter_hold);           // s (V1.5)
    TEST_ASSERT_FALSE(p.estop_nc);                           // simple switch, active LOW
    TEST_ASSERT_EQUAL_FLOAT(3.0f, p.edge_threshold);
    TEST_ASSERT_EQUAL_FLOAT(70.0f, p.base_dist);
    for (int i = 0; i < CALIB_POINTS; ++i) {                              // V1.6.0: identity table 65..40
        TEST_ASSERT_EQUAL_FLOAT(65.0f - 5.0f * i, p.calib.raw[i]);
        TEST_ASSERT_EQUAL_FLOAT(65.0f - 5.0f * i, p.calib.ref[i]);
    }
    TEST_ASSERT_EQUAL_STRING("", p.wifi_ssid);
    SafetyFlags f;
    TEST_ASSERT_TRUE(f.interlock); TEST_ASSERT_TRUE(f.cooling_flow);     // V1.5.23: true after boot
    TEST_ASSERT_EQUAL_FLOAT(18.0f, f.temp_c);
}
void test_flags_bool_and_not_persisted() {
    persist = true;
    OK(paramSet(P, F, "interlock", ValueType::BOOL, 0, true, "", &persist));
    TEST_ASSERT_TRUE(F.interlock); TEST_ASSERT_FALSE(persist);
    OK(setBool("cooling_flow", true)); TEST_ASSERT_TRUE(F.cooling_flow); TEST_ASSERT_FALSE(persist);
    OK(setBool("interlock", false)); TEST_ASSERT_FALSE(F.interlock);
}
// V1.3: E-stop wiring parameter – bool like the flags, but PERSISTED
void test_estop_nc_bool_persisted() {
    OK(setBool("estop_nc", true)); TEST_ASSERT_TRUE(P.estop_nc); TEST_ASSERT_TRUE(persist);
    OK(setNum("estop_nc", 0)); TEST_ASSERT_FALSE(P.estop_nc);
    BAD(setNum("estop_nc", 2)); BAD(setStr("estop_nc", "nc"));
    TEST_ASSERT_FALSE(P.estop_nc);
    OK(setBool("estop_nc", true));
    char buf[600];
    paramsToJsonMember(P, F, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"estop_nc\":true,"));
}
void test_flags_accept_0_1_numbers_only() {
    OK(setNum("interlock", 1)); TEST_ASSERT_TRUE(F.interlock);
    OK(setNum("interlock", 0)); TEST_ASSERT_FALSE(F.interlock);
    BAD(setNum("interlock", 2));
    BAD(setNum("cooling_flow", 0.5));
    BAD(setStr("interlock", "true"));
}
void test_temp() {
    OK(setNum("temp", 35.0)); TEST_ASSERT_EQUAL_FLOAT(35.0f, F.temp_c); TEST_ASSERT_FALSE(persist);
    OK(setNum("temp", -20.0)); OK(setNum("temp", 99.9));
    BAD(setNum("temp", -20.1)); BAD(setNum("temp", 100.0));
    BAD(setBool("temp", true));
    TEST_ASSERT_EQUAL_FLOAT(99.9f, F.temp_c);   // unchanged after errors
}
void test_numeric_params_ranges_and_persist() {
    struct R { const char* k; double lo, hi; } rs[] = {
        {"measure_pos", 61, 242}, {"maint_pos", 21, 60},          // V1.5: measuring zone > 60 mm
        {"traverse_start", 61, 242}, {"traverse_end", 61, 242},
        {"traverse_speed", 1, 20}, {"shutter_hold", 0, 10},
        {"edge_threshold", 0.5, 100}, {"base_dist", 40, 300},
        {"calib_ref_1", 40, 300}, {"calib_ref_2", 40, 300}, {"calib_ref_3", 40, 300},
        {"calib_ref_4", 40, 300}, {"calib_ref_5", 40, 300}, {"calib_ref_6", 40, 300},
    };
    for (auto& r : rs) {
        OK(setNum(r.k, r.lo)); TEST_ASSERT_TRUE_MESSAGE(persist, r.k);
        OK(setNum(r.k, r.hi));
        BAD(setNum(r.k, r.lo - 0.01));
        BAD(setNum(r.k, r.hi + 0.01));
        BAD(setBool(r.k, true));
        BAD(setStr(r.k, "1"));
    }
}
void test_values_land_in_right_fields() {
    OK(setNum("measure_pos", 111)); TEST_ASSERT_EQUAL_FLOAT(111.0f, P.measure_pos);
    OK(setNum("maint_pos", 33)); TEST_ASSERT_EQUAL_FLOAT(33.0f, P.maint_pos);
    OK(setNum("traverse_start", 72)); TEST_ASSERT_EQUAL_FLOAT(72.0f, P.traverse_start);
    OK(setNum("traverse_end", 222)); TEST_ASSERT_EQUAL_FLOAT(222.0f, P.traverse_end);
    OK(setNum("traverse_speed", 13.5)); TEST_ASSERT_EQUAL_FLOAT(13.5f, P.traverse_speed);
    OK(setNum("shutter_hold", 2.5)); TEST_ASSERT_EQUAL_FLOAT(2.5f, P.shutter_hold);
    OK(setNum("edge_threshold", 4.5)); TEST_ASSERT_EQUAL_FLOAT(4.5f, P.edge_threshold);
    OK(setNum("base_dist", 199)); TEST_ASSERT_EQUAL_FLOAT(199.0f, P.base_dist);
    for (int i = 1; i <= 6; ++i) {                                         // each key writes its own point
        char k[16]; snprintf(k, sizeof k, "calib_ref_%d", i);
        Params before = P;
        OK(setNum(k, 100 + i)); TEST_ASSERT_EQUAL_FLOAT(100.0f + i, P.calib.ref[i - 1]);
        for (int j = 0; j < 6; ++j) {
            TEST_ASSERT_EQUAL_FLOAT(before.calib.raw[j], P.calib.raw[j]);  // raw never via SET
            if (j != i - 1) TEST_ASSERT_EQUAL_FLOAT(before.calib.ref[j], P.calib.ref[j]);
        }
    }
}
void test_error_changes_nothing() {
    Params before = P; SafetyFlags fb = F;
    persist = true;
    BAD(paramSet(P, F, "measure_pos", ValueType::NUMBER, 500, false, "", &persist));
    TEST_ASSERT_TRUE(persist);   // untouched on error
    TEST_ASSERT_EQUAL_FLOAT(before.measure_pos, P.measure_pos);
    TEST_ASSERT_EQUAL_FLOAT(before.traverse_speed, P.traverse_speed);
    TEST_ASSERT_EQUAL_FLOAT(before.base_dist, P.base_dist);
    for (int i = 0; i < 6; ++i) TEST_ASSERT_EQUAL_FLOAT(before.calib.ref[i], P.calib.ref[i]);
    TEST_ASSERT_EQUAL_STRING(before.wifi_ssid, P.wifi_ssid);
    TEST_ASSERT_EQUAL(fb.interlock, F.interlock);
    TEST_ASSERT_EQUAL(fb.cooling_flow, F.cooling_flow);
    TEST_ASSERT_EQUAL_FLOAT(fb.temp_c, F.temp_c);
    persist = true;
    BAD(paramSet(P, F, "temp", ValueType::NUMBER, 150, false, "", &persist));
    TEST_ASSERT_EQUAL_FLOAT(fb.temp_c, F.temp_c);
    BAD(paramSet(P, F, "wifi_ssid", ValueType::STRING, 0, false, "bad\"q", &persist));
    TEST_ASSERT_EQUAL_STRING(before.wifi_ssid, P.wifi_ssid);
}
void test_unknown_key_and_null() {
    BAD(setNum("measure_position", 100));
    BAD(setNum("", 100));
    BAD(paramSet(P, F, nullptr, ValueType::NUMBER, 1, false, "", &persist));
    BAD(setNum("calib_near_raw", 100));   // raw values only via CALIB
    BAD(setNum("calib_raw_1", 50));
    // V1.6.0: the 2-point keys are gone, index outside 1..6 and odd spellings are unknown keys
    const char* gone[] = {"calib_near_ref", "calib_far_ref", "calib_ref_0", "calib_ref_7", "calib_ref_",
                          "calib_ref_1x", "calib_ref_10", "calib_ref_a", "calib_ref"};
    for (const char* k : gone) {
        persist = true; BAD(paramSet(P, F, k, ValueType::NUMBER, 50, false, "", &persist));
        TEST_ASSERT_TRUE_MESSAGE(persist, k);                       // untouched on error
    }
}
void test_nan_rejected() {
    BAD(setNum("measure_pos", 0.0 / 0.0));
    // V1.4.1: +-Inf as well, for every numeric key (SET value 1e999 arrives as +inf)
    const char* keys[] = {"temp", "measure_pos", "maint_pos", "traverse_start", "traverse_end",
                          "traverse_speed", "shutter_hold", "edge_threshold", "base_dist", "calib_ref_1",
                          "calib_ref_2", "calib_ref_3", "calib_ref_4", "calib_ref_5", "calib_ref_6"};
    const double bad[] = {0.0 / 0.0, 1.0 / 0.0, -1.0 / 0.0};
    for (const char* k : keys)
        for (double v : bad) BAD(setNum(k, v));
    TEST_ASSERT_EQUAL_FLOAT(155.0f, P.measure_pos);   // defaults untouched
}
void test_wifi_ssid() {
    OK(setStr("wifi_ssid", "MyNet")); TEST_ASSERT_EQUAL_STRING("MyNet", P.wifi_ssid); TEST_ASSERT_TRUE(persist);
    OK(setStr("wifi_ssid", ""));      TEST_ASSERT_EQUAL_STRING("", P.wifi_ssid);
    OK(setStr("wifi_ssid", "12345678901234567890123456789012"));   // 32
    BAD(setStr("wifi_ssid", "123456789012345678901234567890123")); // 33
    BAD(setStr("wifi_ssid", "a\"b"));
    BAD(setStr("wifi_ssid", "a\\b"));
    BAD(setStr("wifi_ssid", "tab\there"));
    BAD(setNum("wifi_ssid", 1));
}
void test_wifi_pass() {
    OK(setStr("wifi_pass", "secret12")); TEST_ASSERT_EQUAL_STRING("secret12", P.wifi_pass);
    OK(setStr("wifi_pass", ""));
    BAD(setStr("wifi_pass", "short7!"));   // 7 chars
    char longp[65]; memset(longp, 'x', 64); longp[64] = 0;
    BAD(setStr("wifi_pass", longp));
}
void test_config_json_exact_defaults() {
    char buf[600];
    int n = paramsToJsonMember(P, F, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(
        "\"config\":{\"measure_pos\":155.0,\"maint_pos\":40.0,\"traverse_start\":80.0,"
        "\"traverse_end\":240.0,\"traverse_speed\":10.0,\"shutter_hold\":3.0,\"edge_threshold\":3.0,\"base_dist\":70.0,"
        "\"calib_raw_1\":65.0,\"calib_ref_1\":65.0,\"calib_raw_2\":60.0,\"calib_ref_2\":60.0,"
        "\"calib_raw_3\":55.0,\"calib_ref_3\":55.0,\"calib_raw_4\":50.0,\"calib_ref_4\":50.0,"
        "\"calib_raw_5\":45.0,\"calib_ref_5\":45.0,\"calib_raw_6\":40.0,\"calib_ref_6\":40.0,"
        "\"calib_valid\":true,\"interlock\":true,"
        "\"cooling_flow\":true,\"temp\":18.0,\"estop_nc\":false,\"wifi_ssid\":\"\",\"wifi_pass_set\":false}", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
}
void test_config_json_never_contains_password() {
    OK(setStr("wifi_ssid", "Lab")); OK(setStr("wifi_pass", "topsecret9"));
    OK(setBool("interlock", true));
    char buf[600];
    TEST_ASSERT_TRUE(paramsToJsonMember(P, F, buf, sizeof(buf)) > 0);
    TEST_ASSERT_NULL(strstr(buf, "topsecret9"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"wifi_pass_set\":true"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"wifi_ssid\":\"Lab\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"interlock\":true"));
}
void test_config_json_cap_too_small() {
    char buf[50];
    TEST_ASSERT_EQUAL_INT(-1, paramsToJsonMember(P, F, buf, sizeof(buf)));
}

// ---- V1.5.5 -------------------------------------------------------------------------
// every field in the right place: all values distinct from each other and from the defaults
void test_config_json_exact_all_fields() {
    OK(setNum("measure_pos", 150.5)); OK(setNum("maint_pos", 33.3)); OK(setNum("traverse_start", 70.1));
    OK(setNum("traverse_end", 230.2)); OK(setNum("traverse_speed", 12.5)); OK(setNum("shutter_hold", 7.7));
    OK(setNum("edge_threshold", 4.4)); OK(setNum("base_dist", 88.8));
    const double refs[6] = {66.1, 61.2, 56.3, 51.4, 46.5, 41.6};
    const float  raws[6] = {67.7f, 62.8f, 57.9f, 52.1f, 47.2f, 42.3f};   // raw values come from CALIB
    for (int i = 0; i < 6; ++i) {
        char k[16]; snprintf(k, sizeof k, "calib_ref_%d", i + 1);
        OK(setNum(k, refs[i])); P.calib.raw[i] = raws[i];
    }
    OK(setBool("interlock", false)); OK(setBool("cooling_flow", true)); OK(setNum("temp", -12.3)); OK(setBool("estop_nc", true));
    OK(setStr("wifi_ssid", "Net ~1"));
    char buf[600];
    int n = paramsToJsonMember(P, F, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(
        "\"config\":{\"measure_pos\":150.5,\"maint_pos\":33.3,\"traverse_start\":70.1,"
        "\"traverse_end\":230.2,\"traverse_speed\":12.5,\"shutter_hold\":7.7,\"edge_threshold\":4.4,\"base_dist\":88.8,"
        "\"calib_raw_1\":67.7,\"calib_ref_1\":66.1,\"calib_raw_2\":62.8,\"calib_ref_2\":61.2,"
        "\"calib_raw_3\":57.9,\"calib_ref_3\":56.3,\"calib_raw_4\":52.1,\"calib_ref_4\":51.4,"
        "\"calib_raw_5\":47.2,\"calib_ref_5\":46.5,\"calib_raw_6\":42.3,\"calib_ref_6\":41.6,"
        "\"calib_valid\":true,\"interlock\":false,"
        "\"cooling_flow\":true,\"temp\":-12.3,\"estop_nc\":true,\"wifi_ssid\":\"Net ~1\",\"wifi_pass_set\":false}", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
    P.calib.raw[0] = 43.0f;                                       // raw no longer rising with ref -> invalid
    paramsToJsonMember(P, F, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"calib_valid\":false,"));
}
void test_config_json_cap_exact() {
    char buf[600];
    int n = paramsToJsonMember(P, F, buf, sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    char exact[600];
    TEST_ASSERT_EQUAL_INT(n, paramsToJsonMember(P, F, exact, (size_t)n + 1));   // room for NUL
    TEST_ASSERT_EQUAL_STRING(buf, exact);
    TEST_ASSERT_EQUAL_INT(-1, paramsToJsonMember(P, F, exact, (size_t)n));      // no room for NUL
}
void test_string_rule_boundaries() {
    OK(setStr("wifi_ssid", " ~ "));                                // 0x20 and 0x7E are allowed
    char s1[2] = {0x7F, 0}; BAD(setStr("wifi_ssid", s1));
    char s2[2] = {0x1F, 0}; BAD(setStr("wifi_ssid", s2));
    char p63[64]; memset(p63, 'p', 63); p63[63] = 0;
    OK(setStr("wifi_pass", p63)); TEST_ASSERT_TRUE(persist);       // 63 chars, persisted
    BAD(setStr("wifi_pass", "secret\"12"));
    BAD(setStr("wifi_pass", "secret\\12"));
    TEST_ASSERT_EQUAL_STRING(p63, P.wifi_pass);
}
void test_persist_untouched_on_every_error() {
    const char* typed[] = {"interlock", "cooling_flow", "estop_nc", "temp", "measure_pos", "calib_ref_6"};
    for (const char* k : typed) {                       // wrong type: string for a bool/number key
        bool pz = true;
        BAD(paramSet(P, F, k, ValueType::STRING, 0, false, "x", &pz));
        TEST_ASSERT_TRUE_MESSAGE(pz, k);
        pz = false;
        BAD(paramSet(P, F, k, ValueType::STRING, 0, false, "x", &pz));
        TEST_ASSERT_FALSE_MESSAGE(pz, k);
    }
    const char* strs[] = {"wifi_ssid", "wifi_pass"};
    for (const char* k : strs) {                        // wrong type: number for a string key
        bool pz = true;
        BAD(paramSet(P, F, k, ValueType::NUMBER, 1, false, "", &pz));
        TEST_ASSERT_TRUE_MESSAGE(pz, k);
    }
    bool pz = true;
    BAD(paramSet(P, F, "interlock", ValueType::NUMBER, 2, false, "", &pz));   // bad number for a flag
    TEST_ASSERT_TRUE(pz);
}

// V1.5.6: the range check uses the double value; values just outside a limit are rejected
// even if they round to the limit as float
void test_range_check_in_double() {
    struct R { const char* k; double lo, hi; } rs[] = {
        {"measure_pos", 61, 242}, {"maint_pos", 21, 60}, {"traverse_start", 61, 242},
        {"traverse_end", 61, 242}, {"traverse_speed", 1, 20}, {"shutter_hold", 0, 10},
        {"edge_threshold", 0.5, 100}, {"base_dist", 40, 300},
        {"calib_ref_1", 40, 300}, {"calib_ref_6", 40, 300}, {"temp", -20, 99.9},
    };
    for (auto& r : rs) {
        BAD(setNum(r.k, r.lo - 1e-9));
        BAD(setNum(r.k, r.hi + 1e-9));
        OK(setNum(r.k, r.lo));
        OK(setNum(r.k, r.hi));
    }
}

// ---- V1.6.0: the longest possible member still fits the 600-byte buffer of GET_CONFIG ----
void test_config_json_worst_case_fits_600() {
    OK(setNum("measure_pos", 241.9)); OK(setNum("maint_pos", 59.9)); OK(setNum("traverse_start", 241.9));
    OK(setNum("traverse_end", 241.9)); OK(setNum("traverse_speed", 19.9)); OK(setNum("shutter_hold", 9.9));
    OK(setNum("edge_threshold", 99.9)); OK(setNum("base_dist", 299.9));
    for (int i = 1; i <= 6; ++i) { char k[16]; snprintf(k, sizeof k, "calib_ref_%d", i); OK(setNum(k, 299.9)); }
    for (int i = 0; i < 6; ++i) P.calib.raw[i] = -99999.9f;      // raw from the sensor can be anything finite
    OK(setBool("interlock", false)); OK(setBool("cooling_flow", false)); OK(setNum("temp", -19.9));
    OK(setBool("estop_nc", true));
    char ssid[33]; memset(ssid, 'W', 32); ssid[32] = 0; OK(setStr("wifi_ssid", ssid));
    OK(setStr("wifi_pass", "longpassword"));
    char buf[600];
    int n = paramsToJsonMember(P, F, buf, sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(n <= 599);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_flags_bool_and_not_persisted);
    RUN_TEST(test_estop_nc_bool_persisted);
    RUN_TEST(test_flags_accept_0_1_numbers_only);
    RUN_TEST(test_temp);
    RUN_TEST(test_numeric_params_ranges_and_persist);
    RUN_TEST(test_values_land_in_right_fields);
    RUN_TEST(test_error_changes_nothing);
    RUN_TEST(test_unknown_key_and_null);
    RUN_TEST(test_nan_rejected);
    RUN_TEST(test_wifi_ssid);
    RUN_TEST(test_wifi_pass);
    RUN_TEST(test_config_json_exact_defaults);
    RUN_TEST(test_config_json_never_contains_password);
    RUN_TEST(test_config_json_cap_too_small);
    RUN_TEST(test_config_json_exact_all_fields);
    RUN_TEST(test_config_json_cap_exact);
    RUN_TEST(test_string_rule_boundaries);
    RUN_TEST(test_persist_untouched_on_every_error);
    RUN_TEST(test_range_check_in_double);
    RUN_TEST(test_config_json_worst_case_fits_600);
    return UNITY_END();
}
