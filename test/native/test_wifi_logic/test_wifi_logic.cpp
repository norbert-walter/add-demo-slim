// FROZEN TESTS (Contract V1.3) – Sprint L3 must make these pass. Do not modify.
// Covers: src/logic/wifi_logic.cpp (CONTRACT §12.1) – router connection with retries and pause.
#include <unity.h>
#include "logic/wifi_logic.h"

static StaConfig C;
static StaStatus S;
static uint32_t now;
void setUp() { C = StaConfig{}; S = StaStatus{}; now = 5000; }
void tearDown() {}

#define ACT(e, x) TEST_ASSERT_EQUAL_INT((int)(e), (int)(x))
#define ST(e)     TEST_ASSERT_EQUAL_STRING(staStateName(e), staStateName(S.state))

// advances time in 100 ms steps (like wifiLoop) and counts BEGIN actions
static int advance(uint32_t ms, bool configured, bool link) {
    int begins = 0;
    for (uint32_t t = 0; t < ms; t += 100) {
        now += 100;
        if (staStep(S, configured, link, now, C) == StaAction::BEGIN) begins++;
    }
    return begins;
}

void test_defaults() {
    TEST_ASSERT_EQUAL_UINT8(3, C.attempts);
    TEST_ASSERT_EQUAL_UINT32(10000, C.attempt_timeout_ms);
    TEST_ASSERT_EQUAL_UINT32(120000, C.backoff_ms);
}
void test_not_configured_stays_off() {
    ACT(StaAction::NONE, staStep(S, false, false, now, C));
    ST(StaState::OFF);
    TEST_ASSERT_EQUAL_INT(0, advance(60000, false, false));
}
void test_boot_logs_in_automatically() {
    ACT(StaAction::BEGIN, staStep(S, true, false, now, C));
    ST(StaState::CONNECTING);
    TEST_ASSERT_EQUAL_UINT8(1, S.attempt);
    ACT(StaAction::NONE, staStep(S, true, true, now + 3000, C));
    ST(StaState::CONNECTED);
    TEST_ASSERT_EQUAL_UINT8(0, S.attempt);
}
void test_three_attempts_then_two_minute_pause_then_again() {
    staStep(S, true, false, now, C);                          // attempt 1
    TEST_ASSERT_EQUAL_INT(2, advance(30000, true, false));    // attempts 2 and 3
    ST(StaState::BACKOFF);
    TEST_ASSERT_EQUAL_INT(0, advance(119000, true, false));   // quiet for 2 minutes
    ST(StaState::BACKOFF);
    TEST_ASSERT_TRUE(staBackoffRemainingMs(S, now, C) > 0);
    TEST_ASSERT_EQUAL_INT(1, advance(1100, true, false));     // next cycle starts
    ST(StaState::CONNECTING);
    TEST_ASSERT_EQUAL_UINT8(1, S.attempt);
}
void test_attempt_timing_and_disconnect_before_pause() {
    staStep(S, true, false, now, C);
    ACT(StaAction::NONE, staStep(S, true, false, now + 9999, C));
    ACT(StaAction::BEGIN, staStep(S, true, false, now + 10000, C));
    TEST_ASSERT_EQUAL_UINT8(2, S.attempt);
    ACT(StaAction::BEGIN, staStep(S, true, false, now + 20000, C));
    TEST_ASSERT_EQUAL_UINT8(3, S.attempt);
    ACT(StaAction::DISCONNECT, staStep(S, true, false, now + 30000, C));
    ST(StaState::BACKOFF);
    TEST_ASSERT_EQUAL_UINT32(120000, staBackoffRemainingMs(S, now + 30000, C));
}
void test_connection_lost_reconnects_with_three_attempts() {
    staStep(S, true, false, now, C);
    staStep(S, true, true, now + 1000, C);
    ST(StaState::CONNECTED);
    TEST_ASSERT_EQUAL_INT(0, advance(600000, true, true));   // stays connected
    ACT(StaAction::BEGIN, staStep(S, true, false, now + 100, C));
    ST(StaState::CONNECTING);
    TEST_ASSERT_EQUAL_UINT8(1, S.attempt);
    now += 100;
    TEST_ASSERT_EQUAL_INT(2, advance(30000, true, false));
    ST(StaState::BACKOFF);
}
void test_success_in_third_attempt() {
    staStep(S, true, false, now, C);
    advance(20000, true, false);
    TEST_ASSERT_EQUAL_UINT8(3, S.attempt);
    staStep(S, true, true, now + 5000, C);
    ST(StaState::CONNECTED);
}
void test_ssid_removed_disconnects() {
    staStep(S, true, false, now, C);
    staStep(S, true, true, now + 1000, C);
    ACT(StaAction::DISCONNECT, staStep(S, false, true, now + 2000, C));
    ST(StaState::OFF);
}
void test_wraparound() {
    now = 0xFFFFFFFFu - 4000;
    staStep(S, true, false, now, C);
    ACT(StaAction::NONE, staStep(S, true, false, now + 9000, C));   // wraps
    ACT(StaAction::BEGIN, staStep(S, true, false, now + 10000, C));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_not_configured_stays_off);
    RUN_TEST(test_boot_logs_in_automatically);
    RUN_TEST(test_three_attempts_then_two_minute_pause_then_again);
    RUN_TEST(test_attempt_timing_and_disconnect_before_pause);
    RUN_TEST(test_connection_lost_reconnects_with_three_attempts);
    RUN_TEST(test_success_in_third_attempt);
    RUN_TEST(test_ssid_removed_disconnects);
    RUN_TEST(test_wraparound);
    return UNITY_END();
}
