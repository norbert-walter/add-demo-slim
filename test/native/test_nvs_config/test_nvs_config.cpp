// FROZEN TESTS (Contract V1.5.14) – Sprint P1 must make these pass. Do not modify.
// Host test of the REAL src/system/nvs_config.cpp with the Preferences shim in
// test/native/freertos_shim/Preferences.h (in-memory NVS, same return values as Arduino-ESP32).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <math.h>
#include <string.h>
#include "system/nvs_config.cpp"   // implementation under test

static const char* NS = NVS_NAMESPACE;

void setUp() { shimNvsReset(); }
void tearDown() {}

static Params custom() {
    Params p; paramsDefaults(p);
    p.measure_pos = 200.5f; p.maint_pos = 33.0f; p.traverse_start = 70.0f; p.traverse_end = 230.0f;
    p.traverse_speed = 12.5f; p.shutter_hold = 0.0f; p.edge_threshold = 4.5f; p.base_dist = 80.0f;
    p.calib.near_raw = 41.5f; p.calib.near_ref = 45.0f; p.calib.far_raw = 66.0f; p.calib.far_ref = 70.0f;
    p.estop_nc = true; strcpy(p.wifi_ssid, "Werkstatt"); strcpy(p.wifi_pass, "geheim123");
    return p;
}
static void assertEqualParams(const Params& a, const Params& b) {
    TEST_ASSERT_EQUAL_FLOAT(a.measure_pos, b.measure_pos);
    TEST_ASSERT_EQUAL_FLOAT(a.maint_pos, b.maint_pos);
    TEST_ASSERT_EQUAL_FLOAT(a.traverse_start, b.traverse_start);
    TEST_ASSERT_EQUAL_FLOAT(a.traverse_end, b.traverse_end);
    TEST_ASSERT_EQUAL_FLOAT(a.traverse_speed, b.traverse_speed);
    TEST_ASSERT_EQUAL_FLOAT(a.shutter_hold, b.shutter_hold);
    TEST_ASSERT_EQUAL_FLOAT(a.edge_threshold, b.edge_threshold);
    TEST_ASSERT_EQUAL_FLOAT(a.base_dist, b.base_dist);
    TEST_ASSERT_EQUAL_FLOAT(a.calib.near_raw, b.calib.near_raw);
    TEST_ASSERT_EQUAL_FLOAT(a.calib.near_ref, b.calib.near_ref);
    TEST_ASSERT_EQUAL_FLOAT(a.calib.far_raw, b.calib.far_raw);
    TEST_ASSERT_EQUAL_FLOAT(a.calib.far_ref, b.calib.far_ref);
    TEST_ASSERT_EQUAL(a.estop_nc, b.estop_nc);
    TEST_ASSERT_EQUAL_STRING(a.wifi_ssid, b.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(a.wifi_pass, b.wifi_pass);
}
static Params defaults() { Params d; paramsDefaults(d); return d; }
static void putF(const char* k, float v) { shimNvsPutRaw(NS, k, 'f', &v, sizeof v); }
static void putS(const char* k, const char* v) { shimNvsPutRaw(NS, k, 's', v, strlen(v) + 1); }
static void putVer(uint8_t v) { shimNvsPutRaw(NS, "ver", 'u', &v, 1); }

void test_empty_nvs_gives_defaults() {
    Params p = custom();
    nvsLoadParams(p);
    assertEqualParams(defaults(), p);
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);           // nothing left open
    TEST_ASSERT_EQUAL_INT(0, shimNvs().misuse);
}
void test_roundtrip_all_fields() {
    Params a = custom();
    TEST_ASSERT_TRUE(nvsSaveParams(a));
    TEST_ASSERT_FALSE(shimNvs().last_read_only);
    Params b; nvsLoadParams(b);
    TEST_ASSERT_TRUE(shimNvs().last_read_only);          // load opens read-only
    assertEqualParams(a, b);
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);
    TEST_ASSERT_EQUAL_INT(0, shimNvs().misuse);
}
void test_exact_key_set_and_types() {
    TEST_ASSERT_TRUE(nvsSaveParams(custom()));
    auto& t = shimNvs().ns[NS];
    static const char* fkeys[] = {"mpos","maint","tstart","tend","tspeed","shold","edge","base",
                                  "cnraw","cnref","cfraw","cfref"};
    for (const char* k : fkeys) { TEST_ASSERT_TRUE_MESSAGE(t.count(k), k); TEST_ASSERT_EQUAL_CHAR('f', t[k].type); }
    TEST_ASSERT_EQUAL_CHAR('u', t["estnc"].type);
    TEST_ASSERT_EQUAL_CHAR('s', t["ssid"].type);
    TEST_ASSERT_EQUAL_CHAR('s', t["pass"].type);
    TEST_ASSERT_EQUAL_CHAR('u', t["ver"].type);
    TEST_ASSERT_EQUAL_INT(4, (int)(uint8_t)t["ver"].bytes[0]);
    TEST_ASSERT_EQUAL_UINT32(16, t.size());              // no safety flags, nothing else
    float mp; memcpy(&mp, t["mpos"].bytes.data(), 4);
    TEST_ASSERT_EQUAL_FLOAT(200.5f, mp);
}
void test_wrong_or_missing_version_gives_defaults() {
    TEST_ASSERT_TRUE(nvsSaveParams(custom()));
    putVer(3);
    Params p = custom(); nvsLoadParams(p);
    assertEqualParams(defaults(), p);
    putVer(5);
    nvsLoadParams(p);
    assertEqualParams(defaults(), p);
    shimNvs().ns[NS].erase("ver");
    nvsLoadParams(p);
    assertEqualParams(defaults(), p);
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);
}
void test_out_of_range_value_resets_only_that_field() {
    Params a = custom();
    TEST_ASSERT_TRUE(nvsSaveParams(a));
    putF("mpos", 50.0f);                                  // service zone, not allowed
    putF("tspeed", NAN);
    putF("shold", 10.5f);
    Params p; nvsLoadParams(p);
    Params exp = a; exp.measure_pos = 155.0f; exp.traverse_speed = 10.0f; exp.shutter_hold = 3.0f;
    assertEqualParams(exp, p);
}
void test_missing_key_keeps_default() {
    Params a = custom();
    TEST_ASSERT_TRUE(nvsSaveParams(a));
    shimNvs().ns[NS].erase("edge");
    shimNvs().ns[NS].erase("ssid");
    Params p; nvsLoadParams(p);
    Params exp = a; exp.edge_threshold = 3.0f; exp.wifi_ssid[0] = 0;
    assertEqualParams(exp, p);
}
void test_invalid_strings_become_empty() {
    Params a = custom();
    TEST_ASSERT_TRUE(nvsSaveParams(a));
    putS("ssid", "0123456789012345678901234567890123456789");   // 40 > 32 chars
    putS("pass", "kurz");                                       // 4: neither 0 nor 8..63
    Params p; nvsLoadParams(p);
    TEST_ASSERT_EQUAL_STRING("", p.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("", p.wifi_pass);
    putS("ssid", "bad\"quote");
    putS("pass", "back\\slash1");
    nvsLoadParams(p);
    TEST_ASSERT_EQUAL_STRING("", p.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("", p.wifi_pass);
    putS("ssid", "");                                            // empty is valid
    putS("pass", "12345678");
    nvsLoadParams(p);
    TEST_ASSERT_EQUAL_STRING("", p.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("12345678", p.wifi_pass);
}
void test_invalid_calibration_resets_whole_calib() {
    Params a = custom();
    TEST_ASSERT_TRUE(nvsSaveParams(a));
    putF("cfraw", 43.0f);                                 // |far_raw - near_raw| < 5 -> invalid
    Params p; nvsLoadParams(p);
    CalibPoints d;
    TEST_ASSERT_EQUAL_FLOAT(d.near_raw, p.calib.near_raw);
    TEST_ASSERT_EQUAL_FLOAT(d.near_ref, p.calib.near_ref);
    TEST_ASSERT_EQUAL_FLOAT(d.far_raw, p.calib.far_raw);
    TEST_ASSERT_EQUAL_FLOAT(d.far_ref, p.calib.far_ref);
    TEST_ASSERT_EQUAL_FLOAT(a.base_dist, p.base_dist);    // other fields untouched
    putF("cfraw", INFINITY);                              // non-finite raw -> default raw 65
    nvsLoadParams(p);
    TEST_ASSERT_EQUAL_FLOAT(65.0f, p.calib.far_raw);
    TEST_ASSERT_EQUAL_FLOAT(41.5f, p.calib.near_raw);     // 41.5 / 45 and 65 / 70 is valid
    putF("cnref", 30.0f);                                 // ref below 40 -> rejected by paramSet
    nvsLoadParams(p);
    TEST_ASSERT_EQUAL_FLOAT(40.0f, p.calib.near_ref);
}
void test_type_mismatch_keeps_default() {
    TEST_ASSERT_TRUE(nvsSaveParams(custom()));
    putS("mpos", "200");                                  // string instead of float
    Params p; nvsLoadParams(p);
    TEST_ASSERT_EQUAL_FLOAT(155.0f, p.measure_pos);
    TEST_ASSERT_EQUAL_FLOAT(33.0f, p.maint_pos);
}
void test_save_errors() {
    shimNvs().fail_begin = true;
    TEST_ASSERT_FALSE(nvsSaveParams(custom()));
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);
    shimNvsReset();
    shimNvs().fail_put_key = "cfref";
    TEST_ASSERT_FALSE(nvsSaveParams(custom()));
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);             // end() also after an error
    shimNvsReset();
    shimNvs().fail_put_key = "ver";
    TEST_ASSERT_FALSE(nvsSaveParams(custom()));
    shimNvsReset();
    shimNvs().fail_put_key = "pass";
    TEST_ASSERT_FALSE(nvsSaveParams(custom()));           // non-empty pass not written
}
void test_save_with_empty_strings_succeeds() {
    Params d = defaults();                                // ssid and pass empty
    TEST_ASSERT_TRUE(nvsSaveParams(d));                   // putString("") returns 0 by design
    Params p = custom(); nvsLoadParams(p);
    assertEqualParams(d, p);
}
void test_load_with_begin_failure_gives_defaults() {
    TEST_ASSERT_TRUE(nvsSaveParams(custom()));
    shimNvs().fail_begin = true;
    Params p = custom(); nvsLoadParams(p);
    assertEqualParams(defaults(), p);
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);
}
void test_erase() {
    TEST_ASSERT_TRUE(nvsSaveParams(custom()));
    nvsErase();
    TEST_ASSERT_EQUAL_UINT32(0, shimNvs().ns[NS].size());
    TEST_ASSERT_EQUAL_INT(0, shimNvs().open);
    TEST_ASSERT_FALSE(shimNvs().last_read_only);
    Params p = custom(); nvsLoadParams(p);
    assertEqualParams(defaults(), p);
}
void test_other_namespace_untouched() {
    float x = 1.0f; shimNvsPutRaw("other", "mpos", 'f', &x, 4);
    TEST_ASSERT_TRUE(nvsSaveParams(custom()));
    nvsErase();
    TEST_ASSERT_EQUAL_UINT32(1, shimNvs().ns["other"].size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_nvs_gives_defaults);
    RUN_TEST(test_roundtrip_all_fields);
    RUN_TEST(test_exact_key_set_and_types);
    RUN_TEST(test_wrong_or_missing_version_gives_defaults);
    RUN_TEST(test_out_of_range_value_resets_only_that_field);
    RUN_TEST(test_missing_key_keeps_default);
    RUN_TEST(test_invalid_strings_become_empty);
    RUN_TEST(test_invalid_calibration_resets_whole_calib);
    RUN_TEST(test_type_mismatch_keeps_default);
    RUN_TEST(test_save_errors);
    RUN_TEST(test_save_with_empty_strings_succeeds);
    RUN_TEST(test_load_with_begin_failure_gives_defaults);
    RUN_TEST(test_erase);
    RUN_TEST(test_other_namespace_untouched);
    return UNITY_END();
}
