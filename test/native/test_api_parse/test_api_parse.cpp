// FROZEN TESTS (Contract V1.3) – Sprint S3 must make these pass. Do not modify.
// Covers: src/logic/api_parse.cpp (CONTRACT §4, §5.2)
#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "logic/api_parse.h"
#include "logic/crc32.h"

void setUp() {}
void tearDown() {}

static char B[512];
static Command Cm;

// Frames `json` with a correct CRC and parses it.
static ApiError parse(const char* json) {
    strcpy(B, json);
    int n = crcFrameAppend(B, strlen(B), sizeof(B));
    TEST_ASSERT_TRUE_MESSAGE(n > 0, json);
    return apiParseRequest(B, (size_t)n, Cm);
}
#define API(e, x) TEST_ASSERT_EQUAL_INT((int)(e), (int)(x))

// ---- framing / syntax ---------------------------------------------------------------
void test_bad_crc() {
    const char* s = "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"00000000\"}";
    API(ApiError::BAD_CRC, apiParseRequest(s, strlen(s), Cm));
    const char* t = "{\"cmd\":\"HV_ON\",\"params\":{}}";
    API(ApiError::BAD_CRC, apiParseRequest(t, strlen(t), Cm));
}
void test_known_good_frame_from_contract() {
    const char* s = "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88F7C802\"}";
    API(ApiError::OK, apiParseRequest(s, strlen(s), Cm));
    TEST_ASSERT_TRUE(Cm.id == CmdId::HV_ON);
    TEST_ASSERT_TRUE(Cm.source == CmdSource::API);
    TEST_ASSERT_TRUE(Cm.want_reply);
}
void test_bad_json() {
    API(ApiError::BAD_JSON, parse("{\"cmd\":\"HV_ON\",\"params\":{}"  "x}"));
    API(ApiError::BAD_JSON, parse("{\"cmd\" \"HV_ON\"}"));
}
void test_unknown_or_missing_cmd() {
    API(ApiError::UNKNOWN_CMD, parse("{\"cmd\":\"FLY\",\"params\":{}}"));
    API(ApiError::UNKNOWN_CMD, parse("{\"cmd\":\"hv_on\"}"));
    API(ApiError::UNKNOWN_CMD, parse("{\"params\":{}}"));
    API(ApiError::UNKNOWN_CMD, parse("{\"cmd\":42}"));
}
void test_params_must_be_object() {
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"STOP\",\"params\":[1,2]}"));
    TEST_ASSERT_TRUE(Cm.id == CmdId::STOP);     // id known for the error response
    API(ApiError::OK, parse("{\"cmd\":\"STOP\"}"));   // params optional
}
void test_extra_params_ignored_for_simple_commands() {
    API(ApiError::OK, parse("{\"cmd\":\"ACK\",\"params\":{\"foo\":1}}"));
    TEST_ASSERT_TRUE(Cm.id == CmdId::ACK);
}

// ---- motion commands --------------------------------------------------------------------
void test_park_and_service_without_params() {   // V1.2
    API(ApiError::OK, parse("{\"cmd\":\"PARK\"}"));
    TEST_ASSERT_TRUE(Cm.id == CmdId::PARK);
    API(ApiError::OK, parse("{\"cmd\":\"SERVICE\",\"params\":{}}"));
    TEST_ASSERT_TRUE(Cm.id == CmdId::SERVICE);
    API(ApiError::UNKNOWN_CMD, parse("{\"cmd\":\"SERV\"}"));   // display label is not the API name
}
void test_move() {
    API(ApiError::OK, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":120,\"speed\":300.5}}"));
    TEST_ASSERT_TRUE(Cm.id == CmdId::MOVE);
    TEST_ASSERT_TRUE(Cm.has_pos); TEST_ASSERT_TRUE(Cm.has_speed);
    TEST_ASSERT_EQUAL_FLOAT(120.0f, Cm.pos); TEST_ASSERT_EQUAL_FLOAT(300.5f, Cm.speed);
}
void test_move_pos_required_and_typed() {
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"MOVE\",\"params\":{\"speed\":300}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":\"120\"}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":true}}"));
    API(ApiError::OK, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":10}}"));
    TEST_ASSERT_FALSE(Cm.has_speed);
}
void test_start_traverse_optional_params() {
    API(ApiError::OK, parse("{\"cmd\":\"START_TRAVERSE\",\"params\":{}}"));
    TEST_ASSERT_FALSE(Cm.has_start); TEST_ASSERT_FALSE(Cm.has_end); TEST_ASSERT_FALSE(Cm.has_speed);
    API(ApiError::OK, parse("{\"cmd\":\"START_TRAVERSE\",\"params\":{\"start\":10,\"end\":230,\"speed\":200}}"));
    TEST_ASSERT_TRUE(Cm.has_start && Cm.has_end && Cm.has_speed);
    TEST_ASSERT_EQUAL_FLOAT(230.0f, Cm.end);
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"START_TRAVERSE\",\"params\":{\"end\":\"x\"}}"));
}
void test_start_measure() {
    API(ApiError::OK, parse("{\"cmd\":\"START_MEASURE\",\"params\":{\"pos\":99.5}}"));
    TEST_ASSERT_TRUE(Cm.has_pos); TEST_ASSERT_EQUAL_FLOAT(99.5f, Cm.pos);
    API(ApiError::OK, parse("{\"cmd\":\"START_MEASURE\"}"));
    TEST_ASSERT_FALSE(Cm.has_pos);
}
// V1.4.1: a number must be a finite float. 1e39 is valid JSON but +inf as float.
// NaN / Infinity are not JSON at all.
void test_numbers_must_be_finite() {
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":1e39}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":-1e39}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":10,\"speed\":1e39}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"START_TRAVERSE\",\"params\":{\"start\":1e39}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"START_TRAVERSE\",\"params\":{\"end\":-1e39}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"START_TRAVERSE\",\"params\":{\"speed\":1e999}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"START_MEASURE\",\"params\":{\"pos\":1e39}}"));
    API(ApiError::BAD_JSON, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":NaN}}"));
    API(ApiError::BAD_JSON, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":Infinity}}"));
    API(ApiError::OK, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":1.2e2}}"));   // exponent notation is fine
    TEST_ASSERT_EQUAL_FLOAT(120.0f, Cm.pos);
}

// ---- SET ---------------------------------------------------------------------------------------
void test_set_number() {
    API(ApiError::OK, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"temp\",\"value\":35.0}}"));
    TEST_ASSERT_EQUAL_STRING("temp", Cm.key);
    TEST_ASSERT_TRUE(Cm.vtype == ValueType::NUMBER);
    TEST_ASSERT_EQUAL_DOUBLE(35.0, Cm.num);
}
void test_set_integer_is_number() {
    API(ApiError::OK, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"interlock\",\"value\":1}}"));
    TEST_ASSERT_TRUE(Cm.vtype == ValueType::NUMBER);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, Cm.num);
}
void test_set_bool() {
    API(ApiError::OK, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"cooling_flow\",\"value\":true}}"));
    TEST_ASSERT_TRUE(Cm.vtype == ValueType::BOOL);
    TEST_ASSERT_TRUE(Cm.b);
    API(ApiError::OK, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"cooling_flow\",\"value\":false}}"));
    TEST_ASSERT_FALSE(Cm.b);
}
void test_set_string() {
    API(ApiError::OK, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"wifi_ssid\",\"value\":\"Lab Net\"}}"));
    TEST_ASSERT_TRUE(Cm.vtype == ValueType::STRING);
    TEST_ASSERT_EQUAL_STRING("Lab Net", Cm.str);
}
void test_set_errors() {
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"SET\",\"params\":{\"value\":1}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"temp\"}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"SET\",\"params\":{\"key\":5,\"value\":1}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"temp\",\"value\":null}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"temp\",\"value\":[1]}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"abcdefghijklmnopqrstuvwx\",\"value\":1}}")); // 24 chars
    API(ApiError::OK,        parse("{\"cmd\":\"SET\",\"params\":{\"key\":\"abcdefghijklmnopqrstuvw\",\"value\":1}}"));  // 23 chars
}
void test_set_string_too_long() {
    char json[300];
    char v[80]; memset(v, 'a', 64); v[64] = 0;          // 64 chars > 63
    snprintf(json, sizeof(json), "{\"cmd\":\"SET\",\"params\":{\"key\":\"wifi_pass\",\"value\":\"%s\"}}", v);
    API(ApiError::BAD_PARAM, parse(json));
    v[63] = 0;
    snprintf(json, sizeof(json), "{\"cmd\":\"SET\",\"params\":{\"key\":\"wifi_pass\",\"value\":\"%s\"}}", v);
    API(ApiError::OK, parse(json));
}

// ---- CALIB ---------------------------------------------------------------------------------------
void test_calib_actions() {
    API(ApiError::OK, parse("{\"cmd\":\"CALIB\",\"params\":{\"action\":\"near\"}}"));
    TEST_ASSERT_TRUE(Cm.calib == CalibAction::NEAR);
    API(ApiError::OK, parse("{\"cmd\":\"CALIB\",\"params\":{\"action\":\"save\"}}"));
    TEST_ASSERT_TRUE(Cm.calib == CalibAction::SAVE);
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"CALIB\",\"params\":{\"action\":\"middle\"}}"));
    API(ApiError::BAD_PARAM, parse("{\"cmd\":\"CALIB\",\"params\":{}}"));
}

// ---- reset behaviour -------------------------------------------------------------------------------
void test_command_is_reset_between_parses() {
    API(ApiError::OK, parse("{\"cmd\":\"MOVE\",\"params\":{\"pos\":120,\"speed\":300}}"));
    API(ApiError::OK, parse("{\"cmd\":\"STOP\"}"));
    TEST_ASSERT_FALSE(Cm.has_pos); TEST_ASSERT_FALSE(Cm.has_speed);
    TEST_ASSERT_EQUAL_STRING("", Cm.key);
    TEST_ASSERT_TRUE(Cm.vtype == ValueType::NONE);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_bad_crc);
    RUN_TEST(test_known_good_frame_from_contract);
    RUN_TEST(test_bad_json);
    RUN_TEST(test_unknown_or_missing_cmd);
    RUN_TEST(test_params_must_be_object);
    RUN_TEST(test_extra_params_ignored_for_simple_commands);
    RUN_TEST(test_park_and_service_without_params);
    RUN_TEST(test_move);
    RUN_TEST(test_move_pos_required_and_typed);
    RUN_TEST(test_start_traverse_optional_params);
    RUN_TEST(test_start_measure);
    RUN_TEST(test_numbers_must_be_finite);
    RUN_TEST(test_set_number);
    RUN_TEST(test_set_integer_is_number);
    RUN_TEST(test_set_bool);
    RUN_TEST(test_set_string);
    RUN_TEST(test_set_errors);
    RUN_TEST(test_set_string_too_long);
    RUN_TEST(test_calib_actions);
    RUN_TEST(test_command_is_reset_between_parses);
    return UNITY_END();
}
