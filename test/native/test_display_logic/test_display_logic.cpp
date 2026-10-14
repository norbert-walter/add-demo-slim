// FROZEN TESTS (Contract V1.5.11) – Sprint L5 must make these pass. Do not modify.
// Covers: src/logic/display_logic.cpp (CONTRACT §13)
#include <unity.h>
#include "logic/display_logic.h"

static SafetyConfig CFG;
static AckPress A;
static uint32_t T;

void setUp() { CFG = SafetyConfig{}; A = AckPress{}; T = 1000; }
void tearDown() {}

// one 20 ms poll
static bool poll(ScreenMode m, bool touch, bool cause = false, float* pr = nullptr) {
    T += 20;
    return ackPressUpdate(A, m, cause, touch, T, 1000, pr);
}
static int pollFor(uint32_t ms, ScreenMode m, bool touch, bool cause = false) {
    int fired = 0;
    for (uint32_t t = 0; t < ms; t += 20) fired += poll(m, touch, cause) ? 1 : 0;
    return fired;
}

void test_screen_mode() {
    TEST_ASSERT_TRUE(screenModeOf(SafetyState::SAFE_START) == ScreenMode::NORMAL);
    TEST_ASSERT_TRUE(screenModeOf(SafetyState::READY) == ScreenMode::NORMAL);
    TEST_ASSERT_TRUE(screenModeOf(SafetyState::RUNNING) == ScreenMode::NORMAL);
    TEST_ASSERT_TRUE(screenModeOf(SafetyState::RECOVER) == ScreenMode::NORMAL);
    TEST_ASSERT_TRUE(screenModeOf(SafetyState::FAULT) == ScreenMode::FAULT);
    TEST_ASSERT_TRUE(screenModeOf(SafetyState::WAIT_ACK) == ScreenMode::ACK);
}
void test_cause_active() {
    ProcData p; p.state = SafetyState::WAIT_ACK;
    p.err = ErrorCode::ESTOP; p.safety.estop = true;   TEST_ASSERT_TRUE(ackCauseActive(p, CFG));
    p.safety.estop = false;                            TEST_ASSERT_FALSE(ackCauseActive(p, CFG));
    p.err = ErrorCode::TEMP_HIGH; p.safety.temp = 32.5f; TEST_ASSERT_TRUE(ackCauseActive(p, CFG));
    p.safety.temp = 32.0f;                             TEST_ASSERT_FALSE(ackCauseActive(p, CFG));   // fault is > 32
    p.err = ErrorCode::MOTOR_FAULT; p.safety.estop = true; TEST_ASSERT_FALSE(ackCauseActive(p, CFG));
    p.err = ErrorCode::COMM_TIMEOUT;                   TEST_ASSERT_FALSE(ackCauseActive(p, CFG));
    p.err = ErrorCode::HOMING_FAILED;                  TEST_ASSERT_FALSE(ackCauseActive(p, CFG));
}
void test_ack_after_1s_hold_exactly_once() {
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(0, pollFor(1000, ScreenMode::ACK, true));   // t0 .. t0+980
    TEST_ASSERT_TRUE(poll(ScreenMode::ACK, true));                 // 1000 ms reached
    TEST_ASSERT_EQUAL_INT(0, pollFor(3000, ScreenMode::ACK, true)); // held further: no repeat
    TEST_ASSERT_EQUAL_INT(0, pollFor(100, ScreenMode::ACK, false));
    TEST_ASSERT_EQUAL_INT(0, pollFor(2000, ScreenMode::ACK, true)); // still WAIT_ACK: locked
}
void test_short_press_cancels() {
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(0, pollFor(800, ScreenMode::ACK, true));
    TEST_ASSERT_EQUAL_INT(0, pollFor(40, ScreenMode::ACK, false));
    TEST_ASSERT_EQUAL_INT(0, pollFor(800, ScreenMode::ACK, true));  // timer restarted
    TEST_ASSERT_EQUAL_INT(1, pollFor(400, ScreenMode::ACK, true));
}
void test_finger_already_down_never_acks() {
    // operator holds STOP while the fault happens; ACK area appears under the finger
    pollFor(200, ScreenMode::NORMAL, true);
    pollFor(20, ScreenMode::FAULT, true);
    TEST_ASSERT_EQUAL_INT(0, pollFor(5000, ScreenMode::ACK, true));
    TEST_ASSERT_EQUAL_INT(0, pollFor(40, ScreenMode::ACK, false)); // lift ...
    TEST_ASSERT_EQUAL_INT(1, pollFor(1100, ScreenMode::ACK, true)); // ... and press deliberately
}
void test_cause_active_blocks_and_cancels() {
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(0, pollFor(3000, ScreenMode::ACK, true, true));  // grey area
    TEST_ASSERT_EQUAL_INT(0, pollFor(2000, ScreenMode::ACK, true, false)); // cause gone, finger still down
    TEST_ASSERT_EQUAL_INT(0, pollFor(40, ScreenMode::ACK, false));
    TEST_ASSERT_EQUAL_INT(0, pollFor(600, ScreenMode::ACK, true));
    TEST_ASSERT_EQUAL_INT(0, pollFor(100, ScreenMode::ACK, true, true));   // cause returns mid-press
    TEST_ASSERT_EQUAL_INT(0, pollFor(1000, ScreenMode::ACK, true, false));
}
void test_new_episode_after_leaving_wait_ack() {
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(1, pollFor(1100, ScreenMode::ACK, true));
    pollFor(100, ScreenMode::NORMAL, false);
    TEST_ASSERT_EQUAL_INT(1, pollFor(1100, ScreenMode::ACK, true));   // second fault, second ACK
}
void test_progress() {
    float pr = -1;
    poll(ScreenMode::ACK, false, false, &pr);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pr);
    poll(ScreenMode::ACK, true, false, &pr);                 // press starts (t0)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pr);
    for (int i = 0; i < 25; ++i) poll(ScreenMode::ACK, true, false, &pr);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, pr);
    for (int i = 0; i < 25; ++i) poll(ScreenMode::ACK, true, false, &pr);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, pr);
    poll(ScreenMode::NORMAL, false, false, &pr);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pr);
}
void test_wraparound() {
    T = 0xFFFFFFFFu - 500;
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(0, pollFor(1000, ScreenMode::ACK, true));   // t0 .. t0+980
    TEST_ASSERT_TRUE(poll(ScreenMode::ACK, true));
}

// ---- V1.5.11: mutation test before Sprint L5 ----
void test_v1511_leaving_ack_screen_cancels_running_press() {
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(0, pollFor(400, ScreenMode::ACK, true));       // press started
    TEST_ASSERT_EQUAL_INT(0, pollFor(100, ScreenMode::NORMAL, true));    // screen changes, finger stays
    TEST_ASSERT_EQUAL_INT(0, pollFor(2000, ScreenMode::ACK, true));      // back: no new touch-down edge
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_screen_mode);
    RUN_TEST(test_cause_active);
    RUN_TEST(test_ack_after_1s_hold_exactly_once);
    RUN_TEST(test_short_press_cancels);
    RUN_TEST(test_finger_already_down_never_acks);
    RUN_TEST(test_cause_active_blocks_and_cancels);
    RUN_TEST(test_new_episode_after_leaving_wait_ack);
    RUN_TEST(test_progress);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_v1511_leaving_ack_screen_cancels_running_press);
    return UNITY_END();
}
