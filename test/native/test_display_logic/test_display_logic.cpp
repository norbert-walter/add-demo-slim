// FROZEN TESTS (Contract V1.6.8) – Sprints L5 and B2 must make these pass. Do not modify.
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

// ---- V1.5.12: review of L5.1 ----
void test_v1512_progress_stays_1_after_sent() {
    float pr = -1.0f;
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(1, pollFor(1100, ScreenMode::ACK, true));        // fired once
    poll(ScreenMode::ACK, true, false, &pr);  TEST_ASSERT_EQUAL_FLOAT(1.0f, pr);
    poll(ScreenMode::ACK, false, false, &pr); TEST_ASSERT_EQUAL_FLOAT(1.0f, pr);   // released, still WAIT_ACK
    poll(ScreenMode::NORMAL, false, false, &pr); TEST_ASSERT_EQUAL_FLOAT(0.0f, pr);
}

// ---- V1.5.13: review of L5.4 ----
void test_v1513_progress_1_after_sent_across_wraparound() {
    float pr = -1.0f;
    T = 0xFFFFFFFFu - 1500;
    poll(ScreenMode::ACK, false);
    TEST_ASSERT_EQUAL_INT(1, pollFor(1100, ScreenMode::ACK, true));    // fires before the wrap
    for (int i = 0; i < 40; ++i) {                                     // released, time wraps past 0
        poll(ScreenMode::ACK, false, false, &pr);
        TEST_ASSERT_EQUAL_FLOAT(1.0f, pr);                             // sent -> always 1
    }
}

// ---- V1.6.8 (Sprint B2): touch debounce and button lock ----
void test_v168_touch_down_at_once_with_coordinates() {
    TouchFilter f;
    TEST_ASSERT_FALSE(touchFilterUpdate(f, false, 0, 0, 1000, 100));
    TEST_ASSERT_FALSE(f.down);
    TEST_ASSERT_TRUE(touchFilterUpdate(f, true, 96, 290, 1020, 100));   // first contact: down at once
    TEST_ASSERT_EQUAL_INT(96, f.x); TEST_ASSERT_EQUAL_INT(290, f.y);
    TEST_ASSERT_EQUAL_UINT32(1020, f.last_ms);
    TEST_ASSERT_TRUE(touchFilterUpdate(f, true, 100, 300, 1040, 100));  // finger moves: coordinates follow
    TEST_ASSERT_EQUAL_INT(100, f.x); TEST_ASSERT_EQUAL_INT(300, f.y);
}
void test_v168_short_gap_is_no_release() {
    TouchFilter f;
    touchFilterUpdate(f, true, 96, 290, 1000, 100);
    for (uint32_t t = 1020; t < 1100; t += 20)                           // 80 ms without contact
        TEST_ASSERT_TRUE_MESSAGE(touchFilterUpdate(f, false, 0, 0, t, 100), "gap < 100 ms: still down");
    TEST_ASSERT_EQUAL_INT(96, f.x); TEST_ASSERT_EQUAL_INT(290, f.y);     // kept, not overwritten by 0/0
    TEST_ASSERT_TRUE(touchFilterUpdate(f, true, 97, 291, 1100, 100));   // contact again: same press
    TEST_ASSERT_EQUAL_UINT32(1100, f.last_ms);
}
void test_v168_release_after_release_ms() {
    TouchFilter f;
    touchFilterUpdate(f, true, 10, 270, 1000, 100);
    TEST_ASSERT_TRUE(touchFilterUpdate(f, false, 0, 0, 1099, 100));     // 99 ms
    TEST_ASSERT_FALSE(touchFilterUpdate(f, false, 0, 0, 1100, 100));    // 100 ms: released
    TEST_ASSERT_EQUAL_INT(10, f.x); TEST_ASSERT_EQUAL_INT(270, f.y);     // coordinates kept
    TEST_ASSERT_FALSE(touchFilterUpdate(f, false, 0, 0, 5000, 100));
    TEST_ASSERT_TRUE(touchFilterUpdate(f, true, 20, 280, 5020, 100));   // new press
}
void test_v168_release_across_wraparound() {
    TouchFilter f;
    touchFilterUpdate(f, true, 1, 2, 0xFFFFFFFFu - 40, 100);
    TEST_ASSERT_TRUE(touchFilterUpdate(f, false, 0, 0, 20, 100));       // 61 ms after the wrap
    TEST_ASSERT_FALSE(touchFilterUpdate(f, false, 0, 0, 60, 100));      // 101 ms
}
void test_v168_button_lock() {
    ButtonLock b;
    TEST_ASSERT_TRUE(buttonLockAllow(b, false, 1000, 400));             // first command
    TEST_ASSERT_TRUE(b.used); TEST_ASSERT_EQUAL_UINT32(1000, b.last_ms);
    TEST_ASSERT_FALSE(buttonLockAllow(b, false, 1399, 400));            // 399 ms: locked ...
    TEST_ASSERT_EQUAL_UINT32(1000, b.last_ms);                           // ... and nothing changes
    TEST_ASSERT_TRUE(buttonLockAllow(b, true, 1200, 400));              // STOP never locked ...
    TEST_ASSERT_EQUAL_UINT32(1200, b.last_ms);                           // ... and restarts the lock
    TEST_ASSERT_FALSE(buttonLockAllow(b, false, 1599, 400));
    TEST_ASSERT_TRUE(buttonLockAllow(b, false, 1600, 400));             // 400 ms after STOP
    TEST_ASSERT_TRUE(buttonLockAllow(b, true, 1601, 400));              // STOP right after a button
}
void test_v168_button_lock_first_use_and_wraparound() {
    ButtonLock b;                                                        // unused: no lock even at t = 0
    TEST_ASSERT_TRUE(buttonLockAllow(b, false, 0, 400));
    b = ButtonLock{};
    TEST_ASSERT_TRUE(buttonLockAllow(b, false, 0xFFFFFFFFu - 100, 400));
    TEST_ASSERT_FALSE(buttonLockAllow(b, false, 200, 400));             // 301 ms after the wrap
    TEST_ASSERT_TRUE(buttonLockAllow(b, false, 299, 400));              // 400 ms
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
    RUN_TEST(test_v1512_progress_stays_1_after_sent);
    RUN_TEST(test_v1513_progress_1_after_sent_across_wraparound);
    RUN_TEST(test_v168_touch_down_at_once_with_coordinates);
    RUN_TEST(test_v168_short_gap_is_no_release);
    RUN_TEST(test_v168_release_after_release_ms);
    RUN_TEST(test_v168_release_across_wraparound);
    RUN_TEST(test_v168_button_lock);
    RUN_TEST(test_v168_button_lock_first_use_and_wraparound);
    return UNITY_END();
}
