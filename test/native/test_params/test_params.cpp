// FROZEN TESTS (Contract V1.3) – Sprint S2.2 must make these pass. Do not modify.
// Covers: src/logic/params.cpp (CONTRACT §9)
#include <unity.h>
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
    TEST_ASSERT_EQUAL_FLOAT(60.0f, p.traverse_start);
    TEST_ASSERT_EQUAL_FLOAT(240.0f, p.traverse_end);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, p.traverse_speed);        // mm/s (V1.3)
    TEST_ASSERT_FALSE(p.estop_nc);                           // simple switch, active LOW
    TEST_ASSERT_EQUAL_FLOAT(3.0f, p.edge_threshold);
    TEST_ASSERT_EQUAL_FLOAT(70.0f, p.base_dist);
    TEST_ASSERT_EQUAL_FLOAT(40.0f, p.calib.near_ref);
    TEST_ASSERT_EQUAL_FLOAT(65.0f, p.calib.far_raw);
    TEST_ASSERT_EQUAL_STRING("", p.wifi_ssid);
    SafetyFlags f;
    TEST_ASSERT_FALSE(f.interlock); TEST_ASSERT_FALSE(f.cooling_flow);
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
        {"measure_pos", 21, 242}, {"maint_pos", 21, 60},
        {"traverse_start", 21, 242}, {"traverse_end", 21, 242},
        {"traverse_speed", 1, 20}, {"edge_threshold", 0.5, 100}, {"base_dist", 40, 300},
        {"calib_near_ref", 40, 300}, {"calib_far_ref", 40, 300},
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
    OK(setNum("traverse_start", 22)); TEST_ASSERT_EQUAL_FLOAT(22.0f, P.traverse_start);
    OK(setNum("traverse_end", 222)); TEST_ASSERT_EQUAL_FLOAT(222.0f, P.traverse_end);
    OK(setNum("traverse_speed", 13.5)); TEST_ASSERT_EQUAL_FLOAT(13.5f, P.traverse_speed);
    OK(setNum("edge_threshold", 4.5)); TEST_ASSERT_EQUAL_FLOAT(4.5f, P.edge_threshold);
    OK(setNum("base_dist", 199)); TEST_ASSERT_EQUAL_FLOAT(199.0f, P.base_dist);
    OK(setNum("calib_near_ref", 60)); TEST_ASSERT_EQUAL_FLOAT(60.0f, P.calib.near_ref);
    OK(setNum("calib_far_ref", 190)); TEST_ASSERT_EQUAL_FLOAT(190.0f, P.calib.far_ref);
}
void test_error_changes_nothing() {
    Params before = P; SafetyFlags fb = F;
    persist = true;
    BAD(paramSet(P, F, "measure_pos", ValueType::NUMBER, 500, false, "", &persist));
    TEST_ASSERT_TRUE(persist);   // untouched on error
    TEST_ASSERT_EQUAL_FLOAT(before.measure_pos, P.measure_pos);
    TEST_ASSERT_EQUAL_FLOAT(before.traverse_speed, P.traverse_speed);
    TEST_ASSERT_EQUAL_FLOAT(before.base_dist, P.base_dist);
    TEST_ASSERT_EQUAL_FLOAT(before.calib.near_ref, P.calib.near_ref);
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
}
void test_nan_rejected() {
    BAD(setNum("measure_pos", 0.0 / 0.0));
    // V1.4.1: +-Inf as well, for every numeric key (SET value 1e999 arrives as +inf)
    const char* keys[] = {"temp", "measure_pos", "maint_pos", "traverse_start", "traverse_end",
                          "traverse_speed", "edge_threshold", "base_dist", "calib_near_ref", "calib_far_ref"};
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
        "\"config\":{\"measure_pos\":155.0,\"maint_pos\":40.0,\"traverse_start\":60.0,"
        "\"traverse_end\":240.0,\"traverse_speed\":10.0,\"edge_threshold\":3.0,\"base_dist\":70.0,"
        "\"calib_near_raw\":40.0,\"calib_near_ref\":40.0,\"calib_far_raw\":65.0,"
        "\"calib_far_ref\":65.0,\"calib_valid\":true,\"interlock\":false,"
        "\"cooling_flow\":false,\"temp\":18.0,\"estop_nc\":false,\"wifi_ssid\":\"\",\"wifi_pass_set\":false}", buf);
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
    return UNITY_END();
}
