// FROZEN TESTS (Contract V1.3) – Sprint S2.1 must make these pass. Do not modify.
// Covers: src/logic/types.cpp, src/logic/crc32.cpp  (CONTRACT §3, §4)
#include <unity.h>
#include <string.h>
#include <stdio.h>
#include "logic/types.h"
#include "logic/crc32.h"

void setUp() {}
void tearDown() {}

static uint32_t crcOf(const char* s) { return crc32Ieee((const uint8_t*)s, strlen(s)); }

// ---- names -----------------------------------------------------------------
void test_safety_state_names() {
    TEST_ASSERT_EQUAL_STRING("SAFE_START", safetyStateName(SafetyState::SAFE_START));
    TEST_ASSERT_EQUAL_STRING("READY", safetyStateName(SafetyState::READY));
    TEST_ASSERT_EQUAL_STRING("RUNNING", safetyStateName(SafetyState::RUNNING));
    TEST_ASSERT_EQUAL_STRING("FAULT", safetyStateName(SafetyState::FAULT));
    TEST_ASSERT_EQUAL_STRING("WAIT_ACK", safetyStateName(SafetyState::WAIT_ACK));
    TEST_ASSERT_EQUAL_STRING("RECOVER", safetyStateName(SafetyState::RECOVER));
}
void test_error_code_names_and_values() {
    TEST_ASSERT_EQUAL_STRING("NONE", errorCodeName(ErrorCode::NONE));
    TEST_ASSERT_EQUAL_STRING("ESTOP", errorCodeName(ErrorCode::ESTOP));
    TEST_ASSERT_EQUAL_STRING("TEMP_HIGH", errorCodeName(ErrorCode::TEMP_HIGH));
    TEST_ASSERT_EQUAL_STRING("COMM_TIMEOUT", errorCodeName(ErrorCode::COMM_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("MOTOR_FAULT", errorCodeName(ErrorCode::MOTOR_FAULT));
    TEST_ASSERT_EQUAL_STRING("HOMING_FAILED", errorCodeName(ErrorCode::HOMING_FAILED));
    TEST_ASSERT_EQUAL_INT(5, (int)ErrorCode::HOMING_FAILED);
    TEST_ASSERT_EQUAL_INT(-9, (int)ApiError::CALIB_INVALID);
    TEST_ASSERT_EQUAL_INT(-10, (int)ApiError::LOCAL_CONTROL);   // V1.1
}
void test_motion_and_mode_names() {
    TEST_ASSERT_EQUAL_STRING("idle", motionStateName(MotionState::IDLE));
    TEST_ASSERT_EQUAL_STRING("moving", motionStateName(MotionState::MOVING));
    TEST_ASSERT_EQUAL_STRING("homing", motionStateName(MotionState::HOMING));
    TEST_ASSERT_EQUAL_STRING("error", motionStateName(MotionState::ERROR));
    TEST_ASSERT_EQUAL_STRING("none", opModeName(OpMode::NONE));
    TEST_ASSERT_EQUAL_STRING("measure", opModeName(OpMode::MEASURE));
    TEST_ASSERT_EQUAL_STRING("traverse", opModeName(OpMode::TRAVERSE));
}
void test_cmd_name_roundtrip_all() {
    const CmdId all[] = {CmdId::INIT, CmdId::START_MEASURE, CmdId::START_TRAVERSE, CmdId::STOP,
                         CmdId::MOVE, CmdId::GET_STATE, CmdId::GET_CONFIG, CmdId::SET, CmdId::CALIB,
                         CmdId::ACK, CmdId::HV_ON, CmdId::HV_OFF, CmdId::SHUTTER_OPEN,
                         CmdId::SHUTTER_CLOSE, CmdId::REBOOT, CmdId::PARK, CmdId::SERVICE};
    for (CmdId c : all) {
        TEST_ASSERT_TRUE(cmdFromName(cmdName(c)) == c);
    }
    TEST_ASSERT_EQUAL_STRING("START_TRAVERSE", cmdName(CmdId::START_TRAVERSE));
    TEST_ASSERT_EQUAL_STRING("NONE", cmdName(CmdId::NONE));
}
void test_cmd_from_name_rejects_unknown_and_case() {
    TEST_ASSERT_TRUE(cmdFromName("hv_on") == CmdId::NONE);
    TEST_ASSERT_TRUE(cmdFromName("FOO") == CmdId::NONE);
    TEST_ASSERT_EQUAL_STRING("PARK", cmdName(CmdId::PARK));          // V1.2
    TEST_ASSERT_EQUAL_STRING("SERVICE", cmdName(CmdId::SERVICE));
    TEST_ASSERT_TRUE(cmdFromName("") == CmdId::NONE);
    TEST_ASSERT_TRUE(cmdFromName(nullptr) == CmdId::NONE);
    TEST_ASSERT_TRUE(cmdFromName("NONE") == CmdId::NONE);
}
void test_calib_action_names() {
    TEST_ASSERT_TRUE(calibActionFromName("near") == CalibAction::NEAR);
    TEST_ASSERT_TRUE(calibActionFromName("far") == CalibAction::FAR);
    TEST_ASSERT_TRUE(calibActionFromName("base") == CalibAction::BASE);
    TEST_ASSERT_TRUE(calibActionFromName("save") == CalibAction::SAVE);
    TEST_ASSERT_TRUE(calibActionFromName("reset") == CalibAction::RESET);
    TEST_ASSERT_TRUE(calibActionFromName("NEAR") == CalibAction::NONE);
    TEST_ASSERT_TRUE(calibActionFromName(nullptr) == CalibAction::NONE);
}
void test_api_error_messages_non_null() {
    TEST_ASSERT_EQUAL_STRING("ok", apiErrorMessage(ApiError::OK));
    TEST_ASSERT_EQUAL_STRING("local control active", apiErrorMessage(ApiError::LOCAL_CONTROL));
    for (int e = -10; e <= -1; ++e) {
        const char* m = apiErrorMessage((ApiError)e);
        TEST_ASSERT_NOT_NULL(m);
        TEST_ASSERT_TRUE(strlen(m) > 0);
        TEST_ASSERT_NULL(strchr(m, '"'));  // must be embeddable in JSON
    }
}

// ---- CRC32 -------------------------------------------------------------------
void test_crc_reference_vector() { TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crcOf("123456789")); }
void test_crc_empty() { TEST_ASSERT_EQUAL_HEX32(0u, crcOf("")); }
void test_crc_matches_zlib_vectors() {
    TEST_ASSERT_EQUAL_HEX32(0x88F7C802u, crcOf("{\"cmd\":\"HV_ON\",\"params\":{}}"));
    TEST_ASSERT_EQUAL_HEX32(0xC93F2D33u, crcOf("{\"cmd\":\"SET\",\"params\":{\"key\":\"temp\",\"value\":35.0}}"));
}
void test_crc_chaining() {
    uint32_t a = crc32Ieee((const uint8_t*)"12345", 5);
    uint32_t ab = crc32Ieee((const uint8_t*)"6789", 4, a);
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, ab);
}
void test_crc_hex_uppercase_padded() {
    char h[9];
    crc32Hex(0x0000ABCDu, h);
    TEST_ASSERT_EQUAL_STRING("0000ABCD", h);
    crc32Hex(0xCBF43926u, h);
    TEST_ASSERT_EQUAL_STRING("CBF43926", h);
}

// ---- framing -----------------------------------------------------------------
void test_frame_append_exact() {
    char buf[64] = "{\"cmd\":\"HV_ON\",\"params\":{}}";
    int n = crcFrameAppend(buf, strlen(buf), sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88F7C802\"}", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
}
void test_frame_append_rejects() {
    char buf[64] = "{\"a\":1}";
    TEST_ASSERT_EQUAL_INT(-1, crcFrameAppend(buf, strlen(buf), strlen(buf) + 17));  // no room for NUL
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
    char e[32] = "{}";
    TEST_ASSERT_EQUAL_INT(-1, crcFrameAppend(e, 2, sizeof(e)));
    char nb[32] = "{\"a\":1} ";
    TEST_ASSERT_EQUAL_INT(-1, crcFrameAppend(nb, strlen(nb), sizeof(nb)));
    char ok[32] = "{\"a\":1}";
    TEST_ASSERT_TRUE(crcFrameAppend(ok, strlen(ok), strlen(ok) + 18) > 0);
}
void test_frame_verify_roundtrip() {
    char buf[128] = "{\"cmd\":\"SET\",\"params\":{\"key\":\"temp\",\"value\":35.0}}";
    int n = crcFrameAppend(buf, strlen(buf), sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(crcFrameVerify(buf, (size_t)n));
}
void test_frame_verify_lowercase_hex_and_trailing_ws() {
    const char* s = "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88f7c802\"}\r\n ";
    TEST_ASSERT_TRUE(crcFrameVerify(s, strlen(s)));
}
void test_frame_verify_detects_tampering() {
    const char* s = "{\"cmd\":\"HV_OF\",\"params\":{},\"crc\":\"88F7C802\"}";
    TEST_ASSERT_FALSE(crcFrameVerify(s, strlen(s)));
}
void test_frame_verify_malformed() {
    const char* a[] = {
        "{\"cmd\":\"HV_ON\",\"params\":{}}",                              // no crc
        "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88F7C80\"}",          // 7 digits
        "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88F7C8021\"}",        // 9 digits
        "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88F7C8G2\"}",         // non-hex
        "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":\"88F7C802\",\"x\":1}", // crc not last
        "{\"cmd\":\"HV_ON\",\"params\":{},\"crc\":88F7C802}",             // not a string
        "",
    };
    for (const char* s : a) TEST_ASSERT_FALSE_MESSAGE(crcFrameVerify(s, strlen(s)), s);
    TEST_ASSERT_FALSE(crcFrameVerify(nullptr, 0));
}
void test_frame_verify_uses_last_occurrence() {
    // A string value that itself contains the tag text must not confuse the receiver.
    char buf[160] = "{\"cmd\":\"SET\",\"params\":{\"key\":\"wifi_ssid\",\"value\":\"a,\\\"crc\\\":\\\"x\"}}";
    int n = crcFrameAppend(buf, strlen(buf), sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(crcFrameVerify(buf, (size_t)n));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_safety_state_names);
    RUN_TEST(test_error_code_names_and_values);
    RUN_TEST(test_motion_and_mode_names);
    RUN_TEST(test_cmd_name_roundtrip_all);
    RUN_TEST(test_cmd_from_name_rejects_unknown_and_case);
    RUN_TEST(test_calib_action_names);
    RUN_TEST(test_api_error_messages_non_null);
    RUN_TEST(test_crc_reference_vector);
    RUN_TEST(test_crc_empty);
    RUN_TEST(test_crc_matches_zlib_vectors);
    RUN_TEST(test_crc_chaining);
    RUN_TEST(test_crc_hex_uppercase_padded);
    RUN_TEST(test_frame_append_exact);
    RUN_TEST(test_frame_append_rejects);
    RUN_TEST(test_frame_verify_roundtrip);
    RUN_TEST(test_frame_verify_lowercase_hex_and_trailing_ws);
    RUN_TEST(test_frame_verify_detects_tampering);
    RUN_TEST(test_frame_verify_malformed);
    RUN_TEST(test_frame_verify_uses_last_occurrence);
    return UNITY_END();
}
