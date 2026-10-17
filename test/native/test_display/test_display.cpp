// FROZEN TESTS (Contract V1.5.20) – Sprint I1 must make these pass. Do not modify.
// Host test of the REAL src/display/display.cpp with the LovyanGFX shim (screen model + touch),
// the real snapshot.cpp and command_queue.cpp. wifi_manager getters are stubs set by the tests.
// The tests run in this order (display.cpp keeps its state in file-static variables).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <string>
#include "logic/wifi_logic.h"

static StaStatus g_sta;
static const char* g_ap = "192.168.4.1";
static const char* g_staip = "192.168.1.57";
StaStatus wifiStaStatus() { return g_sta; }
void wifiGetApIp(char* b, size_t cap) { if (b && cap) snprintf(b, cap, "%s", g_ap); }
void wifiGetStaIp(char* b, size_t cap) { if (b && cap) snprintf(b, cap, "%s", g_sta.state == StaState::CONNECTED ? g_staip : ""); }

#include "app/snapshot.cpp"
#include "app/command_queue.cpp"
#include "display/display.cpp"   // implementation under test

static ShimLcd& L() { return shimLcd(); }
static const ShimText* exact(const char* t) { for (auto& x : L().texts) if (x.text == t) return &x; return nullptr; }
static uint32_t fgOf(const char* t) {                                 // colour of the visible text t (exact)
    const ShimText* p = exact(t);
    if (!p) { std::string m = std::string("text not visible: ") + t; TEST_FAIL_MESSAGE(m.c_str()); }
    return p->fg;
}
static uint32_t T = 10000;
static ProcData D;
static void publish() { snapshotPublish(D); }
static void cfg(bool estop_nc) { Params p; paramsDefaults(p); p.estop_nc = estop_nc; snapshotPublishConfig(p, SafetyFlags()); }
static void tick(uint32_t dt = DISPLAY_TOUCH_POLL_MS) { T += dt; displayTick(T); }
static void settle() { tick(DISPLAY_PERIOD_MS); }                    // guarantees a periodic drawing
static void touch(int x, int y) { L().touch_down = true; L().touch_x = x; L().touch_y = y; }
static void release() { L().touch_down = false; }
static int drain(Command* got, int max) {                            // all queued commands, priority first
    Command pr[CMD_PRIO_MAX_PER_CYCLE]; int n = cmdQueueReceivePriority(pr, CMD_PRIO_MAX_PER_CYCLE), k = 0;
    for (int i = 0; i < n && k < max; ++i) got[k++] = pr[i];
    Command c; while (k < max && cmdQueueReceive(c, true)) got[k++] = c;
    return k;
}
static Command tap(int x, int y) {                                   // touch-down, one tick, release
    Command none; touch(x, y); tick(); release(); tick();
    Command got[8]; int n = drain(got, 8);
    TEST_ASSERT_TRUE_MESSAGE(n <= 1, "at most one command per tap");
    return n ? got[0] : none;
}

void setUp() {}
void tearDown() { release(); }

void test_start_initialises_panel_and_task() {
    cmdQueueInit(); cfg(true);
    TEST_ASSERT_TRUE(displayStart());
    TEST_ASSERT_EQUAL_INT(1, L().inits);
    TEST_ASSERT_EQUAL_INT(1, L().rotation);
    TEST_ASSERT_EQUAL_INT(255, L().brightness);
    TEST_ASSERT_EQUAL_HEX32(COL_BG, L().pixel(240, 160));
    TEST_ASSERT_EQUAL_UINT32(1, shimTasks().created.size());
    const ShimTask& t = shimTasks().created[0];
    TEST_ASSERT_EQUAL_STRING("disp", t.name.c_str());
    TEST_ASSERT_EQUAL_UINT32(DISPLAY_TASK_STACK, t.stack);
    TEST_ASSERT_EQUAL_UINT32(DISPLAY_TASK_PRIO, t.prio);
    TEST_ASSERT_EQUAL_INT(DISPLAY_TASK_CORE, t.core);
    L().init_result = false;
    TEST_ASSERT_FALSE(displayStart());                               // panel init failed: no task
    TEST_ASSERT_EQUAL_UINT32(1, shimTasks().created.size());
    L().init_result = true;
}
void test_normal_screen_content() {
    D = ProcData(); D.state = SafetyState::READY; D.motion.pos = 155.0f; D.motion.vel = 10.0f;
    D.measure.valid = true; D.measure.dist = 55.0f; D.measure.thick = 15.25f; D.measure.avg = 12.3f; D.measure.n = 143;
    D.motion.mode = OpMode::TRAVERSE; D.safety.temp = 23.0f; D.safety.interlock = true; D.safety.cooling_flow = false;
    publish(); g_sta = StaStatus(); g_sta.state = StaState::CONNECTED;
    tick();                                                           // first tick draws at once
    TEST_ASSERT_TRUE(L().visible("ADD DEMO"));
    auto st = L().find("[ READY ]"); TEST_ASSERT_EQUAL_UINT32(1, st.size());
    TEST_ASSERT_EQUAL_HEX32(COL_OK, st[0].fg);
    TEST_ASSERT_FALSE(L().visible("LOCAL"));
    const char* labels[] = {"POS", "SPEED", "DIST", "THICK"};
    const char* values[] = {"155.0", "10.0", "55.0", "15.2"};
    for (int i = 0; i < 4; ++i) {
        std::string s = L().textIn(i * 120, 34, i * 120 + 120, 108);
        TEST_ASSERT_TRUE_MESSAGE(s.find(labels[i]) != std::string::npos, labels[i]);
        TEST_ASSERT_TRUE_MESSAGE(s.find(values[i]) != std::string::npos, values[i]);
    }
    TEST_ASSERT_NOT_NULL(exact("55.0")); TEST_ASSERT_EQUAL_HEX32(COL_DIST, exact("55.0")->fg);
    TEST_ASSERT_NOT_NULL(exact("15.2")); TEST_ASSERT_EQUAL_HEX32(COL_THICK, exact("15.2")->fg);
    TEST_ASSERT_NOT_NULL(exact("155.0")); TEST_ASSERT_EQUAL_INT(3, exact("155.0")->size);
    TEST_ASSERT_TRUE(L().visible("ESTOP"));
    TEST_ASSERT_FALSE(L().visible("ESTOP(NO)"));                      // estop_nc = true
    TEST_ASSERT_EQUAL_HEX32(COL_OK, fgOf("ESTOP"));
    TEST_ASSERT_TRUE(L().visible("TEMP(SIM) 23.0C"));
    TEST_ASSERT_EQUAL_HEX32(COL_OK, fgOf("INTERLOCK(SIM)"));
    TEST_ASSERT_EQUAL_HEX32(COL_FAULT, fgOf("COOLING(SIM)"));
    TEST_ASSERT_TRUE(L().visible("MODE traverse  AVG 12.3 (143)"));
    TEST_ASSERT_TRUE(L().visible("AP 192.168.4.1  STA 192.168.1.57"));
    const char* btn[] = {"INIT", "HV", "MEAS", "TRAV", "PARK", "SERV"};
    for (int i = 0; i < 6; ++i) {
        std::string s = L().textIn(i * 64, 260, i * 64 + 64, 320);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(btn[i], s.c_str(), btn[i]);
        TEST_ASSERT_EQUAL_HEX32(COL_BTN, L().pixel(i * 64 + 5, 265));
    }
    TEST_ASSERT_EQUAL_STRING("STOP", L().textIn(384, 260, 480, 320).c_str());
    TEST_ASSERT_EQUAL_HEX32(COL_FAULT, L().pixel(440, 265));
    TEST_ASSERT_NOT_NULL(exact("STOP")); TEST_ASSERT_EQUAL_INT(3, exact("STOP")->size);
}
void test_position_bar_marker() {
    auto bx = [](float mm) { return 472 - (int)(mm / 242.0f * 468.0f); };   // V1.5.21: mirrored
    TEST_ASSERT_EQUAL_HEX32(COL_TEXT, L().pixel(bx(155.0f) + 1, 120));
    TEST_ASSERT_EQUAL_HEX32(COL_PARK, L().pixel(bx(10.0f), 120));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, L().pixel(bx(40.0f), 120));
    D.motion.pos = 30.0f; publish(); settle();
    TEST_ASSERT_EQUAL_HEX32(COL_TEXT, L().pixel(bx(30.0f) + 1, 120));
    TEST_ASSERT_NOT_EQUAL(COL_TEXT, L().pixel(bx(155.0f) + 1, 120)); // old marker gone
    D.motion.pos = 300.0f; publish(); settle();                       // clamped to 242
    TEST_ASSERT_EQUAL_HEX32(COL_TEXT, L().pixel(bx(242.0f) + 1, 120));
    D.motion.pos = 155.0f; publish(); settle();
}
void test_no_draw_calls_without_change() {
    settle();
    long n = L().draw_calls;
    settle(); settle();
    TEST_ASSERT_EQUAL_INT_MESSAGE(n, L().draw_calls, "unchanged snapshot -> no draw call (no flicker)");
    D.motion.vel = 12.5f; publish();
    tick(DISPLAY_PERIOD_MS - 1); tick(0);                            // < 200 ms: not drawn yet
    TEST_ASSERT_FALSE(L().visible("12.5"));
    tick(1);
    TEST_ASSERT_TRUE(L().visible("12.5"));
    TEST_ASSERT_TRUE(L().draw_calls - n < 12);                        // only the SPEED zone
    TEST_ASSERT_TRUE(L().visible("155.0"));                           // others untouched
}
void test_values_without_valid_measurement() {
    D.measure.valid = false; publish(); settle();
    TEST_ASSERT_EQUAL_INT(2, (int)L().find("---").size());            // DIST and THICK
    TEST_ASSERT_NULL(exact("55.0"));
    D.measure.valid = true; publish(); settle();
}
void test_header_flags_and_estop_wiring() {
    D.local = true; D.safety.hv_on = true; D.safety.shutter_open = true; D.state = SafetyState::RUNNING; publish(); settle();
    TEST_ASSERT_TRUE(L().visible("LOCAL"));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("LOCAL"));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("SHUTTER"));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, L().pixel(64 + 5, 265));        // HV button lit while on
    TEST_ASSERT_EQUAL_HEX32(COL_OK, fgOf("[ RUNNING ]"));
    const SafetyState normal[] = {SafetyState::SAFE_START, SafetyState::READY, SafetyState::RECOVER, SafetyState::RUNNING};
    for (SafetyState s : normal) {                                    // V1.5.21: state text never under "LOCAL"
        D.state = s; publish(); settle();
        char b[24]; snprintf(b, sizeof b, "[ %s ]", safetyStateName(s));
        const ShimText* st = exact(b); const ShimText* lo = exact("LOCAL");
        TEST_ASSERT_NOT_NULL_MESSAGE(st, b); TEST_ASSERT_NOT_NULL_MESSAGE(lo, b);
        TEST_ASSERT_TRUE_MESSAGE(st->x1 <= lo->x0, b);
    }
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("HV"));
    D.safety.hv_on = false; D.safety.shutter_open = false; publish(); settle();
    TEST_ASSERT_EQUAL_HEX32(COL_LABEL, fgOf("HV"));
    D.safety.shutter_open = true; publish(); settle();                // shutter opens with HV off
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("SHUTTER"));
    TEST_ASSERT_EQUAL_HEX32(COL_LABEL, fgOf("HV"));
    D.local = false; D.safety.hv_on = false; D.safety.shutter_open = false; publish(); settle();
    TEST_ASSERT_FALSE(L().visible("LOCAL"));
    TEST_ASSERT_EQUAL_HEX32(COL_LABEL, fgOf("SHUTTER"));
    TEST_ASSERT_EQUAL_HEX32(COL_BTN, L().pixel(64 + 5, 265));
    cfg(false); D.state = SafetyState::READY; D.safety.temp_ok = false; D.safety.temp = 30.5f; publish(); settle();
    TEST_ASSERT_TRUE(L().visible("ESTOP(NO)"));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("ESTOP(NO)"));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("TEMP(SIM) 30.5C"));
    D.safety.estop = true; publish(); settle();
    TEST_ASSERT_EQUAL_HEX32(COL_FAULT, fgOf("ESTOP(NO)"));
    D.safety.estop = false; D.safety.temp_ok = true; D.safety.temp = 23.0f; publish(); cfg(true); settle();
    TEST_ASSERT_FALSE(L().visible("ESTOP(NO)"));
    D.state = SafetyState::SAFE_START; publish(); settle();
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, fgOf("[ SAFE_START ]"));
    D.state = SafetyState::READY; publish(); settle();
}
void test_ip_line_states() {
    g_sta = StaStatus(); g_sta.state = StaState::CONNECTING; g_sta.attempt = 2; settle();
    TEST_ASSERT_TRUE(L().visible("AP 192.168.4.1  STA connecting 2/3"));
    g_sta.state = StaState::BACKOFF; g_sta.attempt = 0; g_sta.t0_ms = T - 33000 + DISPLAY_PERIOD_MS; settle();
    TEST_ASSERT_TRUE(L().visible("AP 192.168.4.1  STA retry 87 s"));   // 120 - 33 = 87 s
    g_sta.t0_ms = T - 33500 + DISPLAY_PERIOD_MS; settle();               // 86.5 s left -> 87 (rounded up)
    TEST_ASSERT_TRUE(L().visible("AP 192.168.4.1  STA retry 87 s"));
    g_sta = StaStatus(); settle();
    TEST_ASSERT_TRUE(L().visible("AP 192.168.4.1  STA off"));
}
void test_buttons_send_local_commands() {
    D.safety.hv_on = false; publish(); settle();
    struct { int x; CmdId id; } b[] = {{32, CmdId::INIT}, {96, CmdId::HV_ON}, {160, CmdId::START_MEASURE},
        {224, CmdId::START_TRAVERSE}, {288, CmdId::PARK}, {352, CmdId::SERVICE}, {430, CmdId::STOP}, {479, CmdId::STOP},
        {0, CmdId::INIT}, {383, CmdId::SERVICE}, {384, CmdId::STOP}, {400, CmdId::STOP}};
    for (auto& t : b) {
        Command c = tap(t.x, 290);
        char m[32]; snprintf(m, sizeof m, "x=%d", t.x);
        TEST_ASSERT_EQUAL_INT_MESSAGE((int)t.id, (int)c.id, m);
        TEST_ASSERT_EQUAL_INT_MESSAGE((int)CmdSource::LOCAL, (int)c.source, m);
        TEST_ASSERT_FALSE(c.want_reply);
        TEST_ASSERT_FALSE(c.has_pos || c.has_speed || c.has_start || c.has_end);
    }
    D.safety.hv_on = true; publish(); settle();
    TEST_ASSERT_EQUAL_INT((int)CmdId::HV_OFF, (int)tap(96, 290).id);
    D.safety.hv_on = false; publish(); settle();
}
void test_touch_outside_row_or_held_sends_nothing() {
    TEST_ASSERT_EQUAL_INT((int)CmdId::NONE, (int)tap(240, 200).id);   // status area
    TEST_ASSERT_EQUAL_INT((int)CmdId::NONE, (int)tap(240, 259).id);
    touch(32, 290); tick();                                           // INIT once ...
    for (int i = 0; i < 100; ++i) tick();                             // ... held for 2 s: no repeats
    release(); tick();
    Command got[8]; TEST_ASSERT_EQUAL_INT(1, drain(got, 8));
    touch(240, 100); tick(); touch(32, 290); tick(); release(); tick(); // slide from status into a button
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));
}
void test_fault_screen_hides_buttons() {
    touch(430, 290); tick();                                          // operator holds STOP ...
    Command got[8]; drain(got, 8);
    D.state = SafetyState::FAULT; D.err = ErrorCode::MOTOR_FAULT; publish();
    tick();                                                           // mode change: drawn at once
    TEST_ASSERT_TRUE(L().visible("FAULT: MOTOR_FAULT"));
    TEST_ASSERT_TRUE(L().visible("Check axis and end switches"));
    TEST_ASSERT_EQUAL_HEX32(COL_FAULT, fgOf("FAULT: MOTOR_FAULT"));
    TEST_ASSERT_TRUE(L().visible("[ FAULT ]"));
    TEST_ASSERT_EQUAL_STRING("", L().textIn(0, 260, 480, 320).c_str());
    TEST_ASSERT_EQUAL_HEX32(COL_BG, L().pixel(32, 290));
    TEST_ASSERT_FALSE(L().visible("INIT"));
    TEST_ASSERT_FALSE(L().visible("POS 155.0") && L().visible("SPEED"));
    release(); tick();
    for (int x : {32, 96, 430}) tap(x, 290);
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));                          // hidden buttons never send
}
void test_ack_screen_cause_active() {
    Command got[8];
    touch(430, 290); tick();                                          // finger still on (old STOP place)
    D.state = SafetyState::WAIT_ACK; D.err = ErrorCode::ESTOP; D.safety.estop = true; publish();
    tick();
    TEST_ASSERT_TRUE(L().visible("FAULT: ESTOP"));
    TEST_ASSERT_TRUE(L().visible("Release E-stop, then acknowledge"));
    TEST_ASSERT_TRUE(L().visible("ESTOP ACTIVE  TEMP(SIM) 23.0C"));
    TEST_ASSERT_TRUE(L().visible("POS 155.0 mm"));
    TEST_ASSERT_TRUE(L().visible("CAUSE ACTIVE: ESTOP - RELEASE FIRST"));
    TEST_ASSERT_EQUAL_HEX32(COL_GREY, L().pixel(5, 265));
    TEST_ASSERT_EQUAL_HEX32(COL_GREY, L().pixel(475, 316));
    for (int i = 0; i < 100; ++i) tick();
    release(); tick();
    touch(240, 290); for (int i = 0; i < 100; ++i) tick(); release(); tick();
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));                          // cause active: never ACK
}
void test_ack_hold_one_second_once() {
    Command got[8];
    D.safety.estop = false; publish(); settle();
    TEST_ASSERT_TRUE(L().visible("ACK - HOLD 1 s"));
    TEST_ASSERT_EQUAL_HEX32(COL_BLACK, fgOf("ACK - HOLD 1 s"));
    touch(240, 150); for (int i = 0; i < 60; ++i) tick(); release(); tick();       // outside the area
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, L().pixel(5, 265));
    TEST_ASSERT_TRUE(L().visible("ESTOP OK  TEMP(SIM) 23.0C"));
    touch(100, 300); tick();                                          // press starts (edge)
    for (int t = 20; t < ACK_HOLD_MS - 20; t += 20) tick();
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));                          // < 1 s
    settle();                                                         // the 1 s is reached in this tick
    TEST_ASSERT_EQUAL_INT(1, drain(got, 8));
    TEST_ASSERT_EQUAL_INT((int)CmdId::ACK, (int)got[0].id);
    TEST_ASSERT_EQUAL_INT((int)CmdSource::LOCAL, (int)got[0].source);
    for (int i = 0; i < 100; ++i) tick();
    release(); tick(); touch(100, 300); for (int i = 0; i < 100; ++i) tick(); release(); tick();
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));                          // sent once in this episode
}
void test_ack_release_cancels_and_progress_bar() {
    Command got[8];
    D.state = SafetyState::READY; D.err = ErrorCode::NONE; publish(); tick();     // leave WAIT_ACK ...
    TEST_ASSERT_TRUE(L().visible("INIT"));                            // ... normal screen at once
    D.state = SafetyState::WAIT_ACK; D.err = ErrorCode::TEMP_HIGH; publish(); tick();
    TEST_ASSERT_TRUE(L().visible("Lower temp <= 32 C"));
    touch(400, 280); for (int i = 0; i < 30; ++i) tick();             // 600 ms
    settle();                                                         // drawn with the progress of the previous tick (~0.6)
    int w = 0; while (w < 470 && L().pixel(10 + w, 310) == COL_TEXT) ++w;
    TEST_ASSERT_TRUE_MESSAGE(w > 200 && w < 460, "progress bar while holding (progress of the previous tick)");
    release(); tick(); settle();
    TEST_ASSERT_EQUAL_HEX32(COL_WARN, L().pixel(20, 310));            // bar gone after release
    touch(400, 280); for (int i = 0; i < 40; ++i) tick(); release(); tick();      // 800 ms: too short
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));
    touch(400, 280); for (int i = 0; i < 52; ++i) tick(); release(); tick();
    TEST_ASSERT_EQUAL_INT(1, drain(got, 8));
}
void test_temp_cause_and_remedies() {
    Command got[8];
    D.state = SafetyState::READY; publish(); tick();
    D.state = SafetyState::WAIT_ACK; D.err = ErrorCode::TEMP_HIGH; D.safety.temp = 35.0f; publish(); tick();
    TEST_ASSERT_TRUE(L().visible("CAUSE ACTIVE: TEMP_HIGH - RELEASE FIRST"));
    touch(240, 290); for (int i = 0; i < 60; ++i) tick(); release(); tick();
    TEST_ASSERT_EQUAL_INT(0, drain(got, 8));
    struct { ErrorCode e; const char* r; } rem[] = {{ErrorCode::COMM_TIMEOUT, "Remote client lost"},
        {ErrorCode::HOMING_FAILED, "Check home switch, then INIT"}, {ErrorCode::MOTOR_FAULT, "Check axis and end switches"}};
    D.safety.temp = 23.0f;
    for (auto& x : rem) {
        D.err = x.e; publish(); settle();
        TEST_ASSERT_TRUE_MESSAGE(L().visible(x.r), x.r);
        TEST_ASSERT_TRUE(L().visible((std::string("FAULT: ") + errorCodeName(x.e)).c_str()));
        TEST_ASSERT_TRUE(L().visible("ACK - HOLD 1 s"));
    }
    D.state = SafetyState::READY; D.err = ErrorCode::NONE; publish(); tick();
    TEST_ASSERT_FALSE(L().visible("FAULT:"));
    TEST_ASSERT_TRUE(L().visible("155.0"));                           // full normal screen again
    TEST_ASSERT_TRUE(L().visible("AP 192.168.4.1"));
}
void test_one_touch_poll_per_tick_and_balanced_writes() {
    int p = L().touch_polls;
    for (int i = 0; i < 25; ++i) tick();
    TEST_ASSERT_EQUAL_INT(p + 25, L().touch_polls);
    TEST_ASSERT_EQUAL_INT(0, L().writes_open);
    TEST_ASSERT_EQUAL_STRING("", Serial.out.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_start_initialises_panel_and_task);
    RUN_TEST(test_normal_screen_content);
    RUN_TEST(test_position_bar_marker);
    RUN_TEST(test_no_draw_calls_without_change);
    RUN_TEST(test_values_without_valid_measurement);
    RUN_TEST(test_header_flags_and_estop_wiring);
    RUN_TEST(test_ip_line_states);
    RUN_TEST(test_buttons_send_local_commands);
    RUN_TEST(test_touch_outside_row_or_held_sends_nothing);
    RUN_TEST(test_fault_screen_hides_buttons);
    RUN_TEST(test_ack_screen_cause_active);
    RUN_TEST(test_ack_hold_one_second_once);
    RUN_TEST(test_ack_release_cancels_and_progress_bar);
    RUN_TEST(test_temp_cause_and_remedies);
    RUN_TEST(test_one_touch_poll_per_tick_and_balanced_writes);
    return UNITY_END();
}
