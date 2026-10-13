// FROZEN TESTS (Contract V1.5.7) – Sprint L3 must make these pass. Do not modify.
// Covers: src/logic/sound_logic.cpp (CONTRACT §11.1) – patterns of the active buzzer.
#include <unity.h>
#include "logic/sound_logic.h"

static SoundPlayer P;
void setUp() { P = SoundPlayer{}; }
void tearDown() {}

// Returns the buzzer level sampled every ms from t0 for n ms as a run-length list.
static int runs(Sound s, uint32_t t0, int* out, int max) {
    soundStart(P, s, t0);
    int n = 0; bool lvl = true; int len = 0;
    for (uint32_t t = t0; t < t0 + 6000; ++t) {
        bool b = soundOutput(P, t);
        if (P.cur == Sound::NONE && !b) break;
        if (b == lvl) len++;
        else { if (n < max) out[n++] = len; lvl = b; len = 1; }
    }
    if (n < max && len) out[n++] = len;
    return n;
}

void test_durations() {
    TEST_ASSERT_EQUAL_UINT32(0, soundDurationMs(Sound::NONE));
    TEST_ASSERT_EQUAL_UINT32(50, soundDurationMs(Sound::CLICK));
    TEST_ASSERT_EQUAL_UINT32(150, soundDurationMs(Sound::HOMED));
    TEST_ASSERT_EQUAL_UINT32(500, soundDurationMs(Sound::BOOT));
    TEST_ASSERT_EQUAL_UINT32(500, soundDurationMs(Sound::REBOOT));
    TEST_ASSERT_EQUAL_UINT32(750, soundDurationMs(Sound::FAULT));
    TEST_ASSERT_EQUAL_UINT32(4000, soundDurationMs(Sound::ESTOP));
}
void test_boot_short_long_and_reboot_long_short() {
    int r[8];
    int n = runs(Sound::BOOT, 1000, r, 8);
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_INT(100, r[0]); TEST_ASSERT_EQUAL_INT(100, r[1]); TEST_ASSERT_EQUAL_INT(300, r[2]);
    P = SoundPlayer{};
    n = runs(Sound::REBOOT, 1000, r, 8);
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_INT(300, r[0]); TEST_ASSERT_EQUAL_INT(100, r[1]); TEST_ASSERT_EQUAL_INT(100, r[2]);
}
void test_fault_three_beeps_estop_one_long_tone() {
    int r[8];
    TEST_ASSERT_EQUAL_INT(5, runs(Sound::FAULT, 0, r, 8));
    for (int i = 0; i < 5; ++i) TEST_ASSERT_EQUAL_INT(150, r[i]);
    P = SoundPlayer{};
    TEST_ASSERT_EQUAL_INT(1, runs(Sound::ESTOP, 0, r, 8));
    TEST_ASSERT_EQUAL_INT(4000, r[0]);
}
void test_ends_and_returns_to_none() {
    soundStart(P, Sound::CLICK, 500);
    TEST_ASSERT_TRUE(soundOutput(P, 500));
    TEST_ASSERT_TRUE(soundOutput(P, 549));
    TEST_ASSERT_FALSE(soundOutput(P, 550));
    TEST_ASSERT_TRUE(P.cur == Sound::NONE);
    TEST_ASSERT_FALSE(soundOutput(P, 600));
}
void test_priority() {
    TEST_ASSERT_TRUE(soundStart(P, Sound::ESTOP, 0));
    TEST_ASSERT_FALSE(soundStart(P, Sound::CLICK, 10));      // never cut the E-stop tone
    TEST_ASSERT_FALSE(soundStart(P, Sound::FAULT, 10));
    TEST_ASSERT_TRUE(P.cur == Sound::ESTOP);
    TEST_ASSERT_TRUE(soundStart(P, Sound::ESTOP, 20));       // same priority restarts
    TEST_ASSERT_EQUAL_UINT32(20, P.t0_ms);
    P = SoundPlayer{};
    TEST_ASSERT_TRUE(soundStart(P, Sound::CLICK, 0));
    TEST_ASSERT_TRUE(soundStart(P, Sound::HOMED, 5));        // higher replaces lower
    TEST_ASSERT_TRUE(soundStart(P, Sound::FAULT, 6));
    TEST_ASSERT_TRUE(soundStart(P, Sound::ESTOP, 7));
    TEST_ASSERT_FALSE(soundStart(P, Sound::NONE, 8));
    soundOutput(P, 5000);                                    // ESTOP ended
    TEST_ASSERT_TRUE(soundStart(P, Sound::CLICK, 5001));     // after the end anything may start
}
void test_wraparound() {
    soundStart(P, Sound::HOMED, 0xFFFFFFFFu - 50);
    TEST_ASSERT_TRUE(soundOutput(P, 0xFFFFFFFFu));
    TEST_ASSERT_TRUE(soundOutput(P, 90));                    // 141 ms after start
    TEST_ASSERT_FALSE(soundOutput(P, 100));                  // 151 ms
}
void test_names() {
    TEST_ASSERT_EQUAL_STRING("estop", soundName(Sound::ESTOP));
    TEST_ASSERT_EQUAL_STRING("boot", soundName(Sound::BOOT));
    TEST_ASSERT_EQUAL_STRING("none", soundName(Sound::NONE));
}

// ---- V1.5.7 -------------------------------------------------------------------------
void test_all_names_exact() {
    TEST_ASSERT_EQUAL_STRING("none", soundName(Sound::NONE));
    TEST_ASSERT_EQUAL_STRING("click", soundName(Sound::CLICK));
    TEST_ASSERT_EQUAL_STRING("homed", soundName(Sound::HOMED));
    TEST_ASSERT_EQUAL_STRING("boot", soundName(Sound::BOOT));
    TEST_ASSERT_EQUAL_STRING("reboot", soundName(Sound::REBOOT));
    TEST_ASSERT_EQUAL_STRING("fault", soundName(Sound::FAULT));
    TEST_ASSERT_EQUAL_STRING("estop", soundName(Sound::ESTOP));
}
void test_start_none_and_full_priority_order() {
    TEST_ASSERT_FALSE(soundStart(P, Sound::NONE, 0));
    TEST_ASSERT_TRUE(soundStart(P, Sound::CLICK, 0));
    TEST_ASSERT_FALSE(soundStart(P, Sound::NONE, 10));                 // NONE never replaces
    TEST_ASSERT_EQUAL_INT((int)Sound::CLICK, (int)P.cur);
    const Sound order[] = {Sound::CLICK, Sound::HOMED, Sound::BOOT, Sound::REBOOT, Sound::FAULT, Sound::ESTOP};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j) {
            P = SoundPlayer{};
            soundStart(P, order[i], 0);
            TEST_ASSERT_EQUAL_MESSAGE(j >= i, soundStart(P, order[j], 10), soundName(order[j]));
        }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_durations);
    RUN_TEST(test_boot_short_long_and_reboot_long_short);
    RUN_TEST(test_fault_three_beeps_estop_one_long_tone);
    RUN_TEST(test_ends_and_returns_to_none);
    RUN_TEST(test_priority);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_names);
    RUN_TEST(test_all_names_exact);
    RUN_TEST(test_start_none_and_full_priority_order);
    return UNITY_END();
}
