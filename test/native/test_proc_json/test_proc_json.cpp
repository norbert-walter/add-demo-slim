// FROZEN TESTS (Contract V1.5.11) – Sprint L5 must make these pass. Do not modify.
// Covers: src/logic/proc_json.cpp (CONTRACT §5)
#include <unity.h>
#include <string.h>
#include <math.h>
#include "logic/proc_json.h"
#include "logic/crc32.h"

void setUp() {}
void tearDown() {}

static char B[PROC_JSON_MAX];

// Removes the crc frame so that the content can be compared exactly.
static void stripCrc(char* s) {
    char* p = strstr(s, ",\"crc\":\"");
    TEST_ASSERT_NOT_NULL(p);
    p[0] = '}'; p[1] = 0;
}

void test_proc_default_exact() {
    ProcData d;
    int n = procToJson(d, B, sizeof(B));
    TEST_ASSERT_EQUAL_INT((int)strlen(B), n);
    TEST_ASSERT_TRUE(crcFrameVerify(B, (size_t)n));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING(
        "{\"ts\":0,\"state\":\"SAFE_START\",\"err\":0,\"local\":false,"
        "\"motion\":{\"pos\":0.0,\"vel\":0.0,\"state\":\"idle\",\"mode\":\"none\","
        "\"homed\":false,\"limit_fwd\":false,\"limit_bwd\":false},"
        "\"measure\":{\"dist\":0.0,\"thick\":0.0,\"on_material\":false,\"valid\":false,"
        "\"in_range\":false,\"avg\":0.0,\"n\":0},"
        "\"safety\":{\"estop\":false,\"hv_on\":false,\"hv_ok\":false,\"shutter\":\"closed\","
        "\"interlock\":false,\"cooling_flow\":false,\"temp\":18.0,\"temp_ok\":true,"
        "\"comm_armed\":false}}", B);
}
void test_proc_typical_values() {
    ProcData d;
    d.ts_ms = 123456; d.state = SafetyState::RUNNING; d.err = ErrorCode::NONE;
    d.motion.pos = 120.04f; d.motion.vel = 200.0f; d.motion.state = MotionState::MOVING;
    d.motion.mode = OpMode::TRAVERSE; d.motion.homed = true;
    d.measure.dist = 187.66f; d.measure.thick = 12.34f; d.measure.on_material = true;
    d.measure.valid = true; d.measure.in_range = true; d.measure.avg = 12.26f; d.measure.n = 143;
    d.safety.hv_on = true; d.safety.hv_ok = true; d.safety.shutter_open = true;
    d.safety.interlock = true; d.safety.cooling_flow = true; d.safety.temp = 23.0f;
    d.safety.comm_armed = true; d.local = true;
    int n = procToJson(d, B, sizeof(B));
    TEST_ASSERT_TRUE(n > 0);
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING(
        "{\"ts\":123456,\"state\":\"RUNNING\",\"err\":0,\"local\":true,"
        "\"motion\":{\"pos\":120.0,\"vel\":200.0,\"state\":\"moving\",\"mode\":\"traverse\","
        "\"homed\":true,\"limit_fwd\":false,\"limit_bwd\":false},"
        "\"measure\":{\"dist\":187.7,\"thick\":12.3,\"on_material\":true,\"valid\":true,"
        "\"in_range\":true,\"avg\":12.3,\"n\":143},"
        "\"safety\":{\"estop\":false,\"hv_on\":true,\"hv_ok\":true,\"shutter\":\"open\","
        "\"interlock\":true,\"cooling_flow\":true,\"temp\":23.0,\"temp_ok\":true,"
        "\"comm_armed\":true}}", B);
}
void test_proc_error_and_fault_state() {
    ProcData d; d.state = SafetyState::WAIT_ACK; d.err = ErrorCode::TEMP_HIGH; d.motion.state = MotionState::ERROR;
    procToJson(d, B, sizeof(B));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"state\":\"WAIT_ACK\",\"err\":2,\"local\":false,"));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"state\":\"error\""));
}
void test_proc_no_negative_zero_and_nonfinite() {
    ProcData d; d.motion.pos = -0.04f; d.motion.vel = NAN; d.measure.dist = INFINITY; d.measure.avg = -INFINITY;
    procToJson(d, B, sizeof(B));
    TEST_ASSERT_NULL(strstr(B, "-0.0"));
    TEST_ASSERT_NULL(strstr(B, "nan"));
    TEST_ASSERT_NULL(strstr(B, "inf"));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"pos\":0.0,\"vel\":0.0,"));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"dist\":0.0,"));
}
void test_proc_clamps_huge_values() {
    ProcData d; d.measure.dist = 1e30f; d.motion.pos = -1e30f;
    TEST_ASSERT_TRUE(procToJson(d, B, sizeof(B)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(B, "\"dist\":99999.9,"));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"pos\":-99999.9,"));
}
void test_proc_worst_case_fits_512() {
    ProcData d;
    d.ts_ms = 4294967295u; d.state = SafetyState::SAFE_START; d.err = (ErrorCode)255;
    d.motion.pos = -1e9f; d.motion.vel = -1e9f; d.motion.state = MotionState::HOMING; d.motion.mode = OpMode::TRAVERSE;
    d.measure.dist = -1e9f; d.measure.thick = -1e9f; d.measure.avg = -1e9f; d.measure.n = 4294967295u;
    d.safety.temp = -1e9f; d.safety.temp_ok = false; d.local = true;
    d.motion.homed = d.motion.limit_fwd = d.motion.limit_bwd = false;   // "false" is longer than "true"
    int n = procToJson(d, B, sizeof(B));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(n < (int)PROC_JSON_MAX);
    TEST_ASSERT_TRUE(crcFrameVerify(B, (size_t)n));
}
void test_proc_cap_too_small() {
    ProcData d; char small[100];
    TEST_ASSERT_EQUAL_INT(-1, procToJson(d, small, sizeof(small)));
}
void test_response_ok() {
    int n = responseToJson(CmdId::HV_ON, ApiError::OK, nullptr, nullptr, B, sizeof(B));
    TEST_ASSERT_TRUE(crcFrameVerify(B, (size_t)n));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\",\"err\":0,\"cmd\":\"HV_ON\"}", B);
}
void test_response_error_default_msg() {
    responseToJson(CmdId::HV_ON, ApiError::PRECONDITION, nullptr, nullptr, B, sizeof(B));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING(
        "{\"status\":\"error\",\"err\":-6,\"cmd\":\"HV_ON\",\"msg\":\"precondition not met\"}", B);
}
void test_response_error_custom_msg_and_unknown_cmd() {
    responseToJson(CmdId::NONE, ApiError::UNKNOWN_CMD, "no such command", nullptr, B, sizeof(B));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING(
        "{\"status\":\"error\",\"err\":-3,\"cmd\":\"NONE\",\"msg\":\"no such command\"}", B);
}
void test_response_with_extra() {
    int n = responseToJson(CmdId::GET_STATE, ApiError::OK, nullptr, "\"state\":\"READY\",\"sys_err\":0", B, sizeof(B));
    TEST_ASSERT_TRUE(crcFrameVerify(B, (size_t)n));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\",\"err\":0,\"cmd\":\"GET_STATE\",\"state\":\"READY\",\"sys_err\":0}", B);
}
void test_response_cap_too_small() {
    char small[30];
    TEST_ASSERT_EQUAL_INT(-1, responseToJson(CmdId::HV_ON, ApiError::OK, nullptr, nullptr, small, sizeof(small)));
}

// V1.1: GET_STATE extra
void test_state_extra_exact() {
    char e[192];
    CycleStats cs; cs.cycle_max_us = 312; cs.period_max_us = 1010; cs.overruns = 0; cs.prio_lat_max_us = 1450;
    int n = stateExtraToJson(SafetyState::READY, ErrorCode::NONE, true, cs, e, sizeof(e));
    TEST_ASSERT_EQUAL_INT((int)strlen(e), n);
    TEST_ASSERT_EQUAL_STRING(
        "\"state\":\"READY\",\"sys_err\":0,\"local\":true,\"cycle_max_us\":312,"
        "\"period_max_us\":1010,\"overruns\":0,\"prio_lat_max_us\":1450", e);
}
void test_state_extra_worst_case_and_in_response() {
    char e[192];
    CycleStats cs; cs.cycle_max_us = cs.period_max_us = cs.overruns = cs.prio_lat_max_us = 4294967295u;
    int n = stateExtraToJson(SafetyState::SAFE_START, (ErrorCode)255, false, cs, e, sizeof(e));
    TEST_ASSERT_TRUE(n > 0 && n < 192);
    TEST_ASSERT_EQUAL_INT(-1, stateExtraToJson(SafetyState::READY, ErrorCode::NONE, false, CycleStats{}, e, 20));
    int m = responseToJson(CmdId::GET_STATE, ApiError::OK, nullptr, e, B, sizeof(B));
    TEST_ASSERT_TRUE(m > 0);
    TEST_ASSERT_TRUE(crcFrameVerify(B, (size_t)m));
}
void test_response_local_control() {
    responseToJson(CmdId::HV_ON, ApiError::LOCAL_CONTROL, nullptr, nullptr, B, sizeof(B));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING(
        "{\"status\":\"error\",\"err\":-10,\"cmd\":\"HV_ON\",\"msg\":\"local control active\"}", B);
}

// ---- V1.5.11: mutation test before Sprint L5 ----
void test_v1511_zero_band_and_rounding() {
    ProcData d; d.motion.pos = -0.045f; d.motion.vel = 0.07f; d.measure.dist = -0.06f;
    procToJson(d, B, sizeof(B));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"pos\":0.0,\"vel\":0.1,"));    // |v| < 0.05 -> 0.0, else %.1f
    TEST_ASSERT_NOT_NULL(strstr(B, "\"dist\":-0.1,"));
}
void test_v1511_limit_switch_order() {
    ProcData d; d.motion.limit_fwd = true;
    procToJson(d, B, sizeof(B));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"limit_fwd\":true,\"limit_bwd\":false}"));
}
void test_v1511_temp_sanitised() {
    ProcData d; d.safety.temp = NAN;
    procToJson(d, B, sizeof(B));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"temp\":0.0,"));
    d.safety.temp = 1e30f;
    procToJson(d, B, sizeof(B));
    TEST_ASSERT_NOT_NULL(strstr(B, "\"temp\":99999.9,"));
}
void test_v1511_response_status_only_ok_for_ok() {
    responseToJson(CmdId::MOVE, ApiError::BUSY, nullptr, nullptr, B, sizeof(B));
    TEST_ASSERT_EQUAL_INT(0, strncmp(B, "{\"status\":\"error\",\"err\":-7,\"cmd\":\"MOVE\",\"msg\":", 46));
}
void test_v1511_error_response_with_extra() {
    int n = responseToJson(CmdId::SET, ApiError::BAD_PARAM, "bad value", "\"key\":\"x\"", B, sizeof(B));
    TEST_ASSERT_TRUE(crcFrameVerify(B, (size_t)n));
    stripCrc(B);
    TEST_ASSERT_EQUAL_STRING(
        "{\"status\":\"error\",\"err\":-4,\"cmd\":\"SET\",\"msg\":\"bad value\",\"key\":\"x\"}", B);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_proc_default_exact);
    RUN_TEST(test_proc_typical_values);
    RUN_TEST(test_proc_error_and_fault_state);
    RUN_TEST(test_proc_no_negative_zero_and_nonfinite);
    RUN_TEST(test_proc_clamps_huge_values);
    RUN_TEST(test_proc_worst_case_fits_512);
    RUN_TEST(test_proc_cap_too_small);
    RUN_TEST(test_response_ok);
    RUN_TEST(test_response_error_default_msg);
    RUN_TEST(test_response_error_custom_msg_and_unknown_cmd);
    RUN_TEST(test_response_with_extra);
    RUN_TEST(test_response_cap_too_small);
    RUN_TEST(test_state_extra_exact);
    RUN_TEST(test_state_extra_worst_case_and_in_response);
    RUN_TEST(test_response_local_control);
    RUN_TEST(test_v1511_zero_band_and_rounding);
    RUN_TEST(test_v1511_limit_switch_order);
    RUN_TEST(test_v1511_temp_sanitised);
    RUN_TEST(test_v1511_response_status_only_ok_for_ok);
    RUN_TEST(test_v1511_error_response_with_extra);
    return UNITY_END();
}
