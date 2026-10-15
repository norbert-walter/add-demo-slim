// FROZEN TESTS (Contract V1.5.17) – Sprint P2 must make these pass. Do not modify.
// Host test of the REAL src/comm/wifi_manager.cpp with WiFi/ESPmDNS shims
// (test/native/freertos_shim). The tests form ONE scenario and must run in this order
// (wifi_manager keeps its state in file-static variables, like on the device).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <string.h>
#include <string>
#include <thread>
#include <atomic>
#include "comm/log_buffer.cpp"
#include "comm/wifi_manager.cpp"   // implementation under test

void setUp() {}
void tearDown() {}

static char L[LOG_LINES * LOG_LINE_LEN + 1];
static std::string lastLines(int n) {               // the last n log lines, '\n' separated
    logDump(L, sizeof L);
    std::string s(L);
    int cnt = 0; size_t pos = s.size();
    while (pos > 0 && cnt <= n) { pos = s.rfind('\n', pos - 1); if (pos == std::string::npos) { pos = 0; break; } ++cnt; }
    return cnt > n ? s.substr(pos + 1) : s;
}
static std::string lastLine() { std::string s = lastLines(1); if (!s.empty() && s.back() == '\n') s.pop_back(); return s; }
static uint32_t T = 40;                              // first wifiLoop 40 ms after boot
static void runFor(uint32_t ms, uint32_t step = 10) { for (uint32_t t = 0; t < ms; t += step) { T += step; wifiLoop(T); } }

static char g_ssid[33] = "Werkstatt";
static char g_pass[64] = "geheim123";

void test_init_starts_ap_and_mdns() {
    wifiInit(g_ssid, g_pass);
    strcpy(g_ssid, "XXX"); strcpy(g_pass, "YYY");          // caller buffers change: must be copied
    ShimWiFi& w = shimWiFi();
    TEST_ASSERT_EQUAL_INT(WIFI_AP_STA, w.mode);
    TEST_ASSERT_FALSE(w.auto_reconnect);
    TEST_ASSERT_EQUAL_STRING(WIFI_AP_SSID, w.ap_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(WIFI_AP_PASS, w.ap_pass.c_str());
    TEST_ASSERT_EQUAL_STRING("add-demo", shimMdns().host.c_str());
    TEST_ASSERT_EQUAL_STRING("http", shimMdns().service.c_str());
    TEST_ASSERT_EQUAL_STRING("tcp", shimMdns().proto.c_str());
    TEST_ASSERT_EQUAL_UINT16(HTTP_PORT, shimMdns().port);
    TEST_ASSERT_EQUAL_STRING("ap: ADD-Demo 192.168.4.1", lastLine().c_str());
    TEST_ASSERT_EQUAL_INT(0, w.begins);                     // the station starts in wifiLoop
    char ip[16]; wifiGetApIp(ip, sizeof ip);
    TEST_ASSERT_EQUAL_STRING("192.168.4.1", ip);
}
void test_first_loop_begins_immediately() {
    wifiLoop(T);
    ShimWiFi& w = shimWiFi();
    TEST_ASSERT_EQUAL_INT(1, w.begins);
    TEST_ASSERT_EQUAL_STRING("Werkstatt", w.sta_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("geheim123", w.sta_pass.c_str());
    StaStatus s = wifiStaStatus();
    TEST_ASSERT_EQUAL_INT((int)StaState::CONNECTING, (int)s.state);
    TEST_ASSERT_EQUAL_UINT8(1, s.attempt);
    TEST_ASSERT_EQUAL_STRING("sta: attempt 1/3", lastLine().c_str());
    char ip[16] = "x"; wifiGetStaIp(ip, sizeof ip);
    TEST_ASSERT_EQUAL_STRING("", ip);
}
void test_steps_only_every_100ms() {
    shimWiFi().status = WL_CONNECTED;
    wifiLoop(T + 50);
    wifiLoop(T + 99);
    TEST_ASSERT_EQUAL_INT((int)StaState::CONNECTING, (int)wifiStaStatus().state);   // not stepped yet
    T += 100;
    wifiLoop(T);
    TEST_ASSERT_EQUAL_INT((int)StaState::CONNECTED, (int)wifiStaStatus().state);
    TEST_ASSERT_EQUAL_STRING("sta: connected 192.168.1.57", lastLine().c_str());
    char ip[16]; wifiGetStaIp(ip, sizeof ip);
    TEST_ASSERT_EQUAL_STRING("192.168.1.57", ip);
    runFor(1000);                                           // no new log lines while connected
    TEST_ASSERT_EQUAL_STRING("sta: connected 192.168.1.57", lastLine().c_str());
    TEST_ASSERT_EQUAL_INT(1, shimWiFi().begins);
}
void test_connection_lost_three_attempts_then_backoff() {
    shimWiFi().status = WL_DISCONNECTED;
    T += 100; wifiLoop(T);
    TEST_ASSERT_EQUAL_STRING("sta: connection lost\nsta: attempt 1/3\n", lastLines(2).c_str());
    TEST_ASSERT_EQUAL_INT(2, shimWiFi().begins);
    runFor(10000);
    TEST_ASSERT_EQUAL_STRING("sta: attempt 2/3", lastLine().c_str());
    TEST_ASSERT_EQUAL_INT(3, shimWiFi().begins);
    runFor(10000);
    TEST_ASSERT_EQUAL_STRING("sta: attempt 3/3", lastLine().c_str());
    TEST_ASSERT_EQUAL_INT(0, shimWiFi().disconnects);
    runFor(10000);
    TEST_ASSERT_EQUAL_INT((int)StaState::BACKOFF, (int)wifiStaStatus().state);
    TEST_ASSERT_EQUAL_STRING("sta: backoff 120 s", lastLine().c_str());
    TEST_ASSERT_EQUAL_INT(1, shimWiFi().disconnects);
    TEST_ASSERT_EQUAL_INT(0, shimWiFi().disconnect_wifioff);   // the AP must stay on
    TEST_ASSERT_EQUAL_INT(4, shimWiFi().begins);
    runFor(119000);
    TEST_ASSERT_EQUAL_INT(4, shimWiFi().begins);
    runFor(1100);
    TEST_ASSERT_EQUAL_INT(5, shimWiFi().begins);              // after 120 s: attempts again
    TEST_ASSERT_EQUAL_STRING("sta: attempt 1/3", lastLine().c_str());
}
void test_step_gap_near_wraparound() {
    T = 0xFFFFFFFFu - 40;
    shimWiFi().status = WL_DISCONNECTED;
    wifiLoop(T);                                              // steps (long since the last step)
    shimWiFi().status = WL_CONNECTED;
    wifiLoop(T + 30);                                         // only 30 ms later, before the wrap: no step
    TEST_ASSERT_NOT_EQUAL((int)StaState::CONNECTED, (int)wifiStaStatus().state);
    wifiLoop(T + 99);                                         // wrapped, 99 ms: still no step
    TEST_ASSERT_NOT_EQUAL((int)StaState::CONNECTED, (int)wifiStaStatus().state);
    T += 100; wifiLoop(T);                                    // wrapped, 100 ms: steps
    TEST_ASSERT_EQUAL_INT((int)StaState::CONNECTED, (int)wifiStaStatus().state);
}
void test_wraparound() {
    T = 0xFFFFFFFFu - 5000;
    shimWiFi().status = WL_CONNECTED;
    wifiLoop(T);                                              // >= 100 ms since the last step (wraps)
    TEST_ASSERT_EQUAL_INT((int)StaState::CONNECTED, (int)wifiStaStatus().state);
    shimWiFi().status = WL_DISCONNECTED;
    int b = shimWiFi().begins;
    runFor(30500);                                            // crosses 0: three attempts ...
    TEST_ASSERT_EQUAL_INT((int)StaState::BACKOFF, (int)wifiStaStatus().state);
    TEST_ASSERT_EQUAL_INT(b + 3, shimWiFi().begins);
}
void test_status_read_from_other_task() {
    std::atomic<bool> run{true}; std::atomic<int> bad{0};
    std::thread reader([&] {
        while (run) {
            StaStatus s = wifiStaStatus();
            bool attemptOk = (s.state == StaState::CONNECTING) ? (s.attempt >= 1 && s.attempt <= 3) : (s.attempt == 0);
            if (!attemptOk) ++bad;
        }
    });
    for (int k = 0; k < 20; ++k) {
        shimWiFi().status = (k % 2) ? WL_CONNECTED : WL_DISCONNECTED;
        runFor(15000, 100);
    }
    run = false; reader.join();
    TEST_ASSERT_EQUAL_INT(0, bad);
}
void test_ip_buffers() {
    char b[5] = "zzzz";
    wifiGetApIp(b, sizeof b);
    TEST_ASSERT_EQUAL_STRING("192.", b);                      // truncated, terminated
    char c = 'q';
    wifiGetApIp(&c, 0);
    wifiGetStaIp(&c, 0);
    TEST_ASSERT_EQUAL_CHAR('q', c);                           // cap 0: nothing written
}
void test_empty_ssid_switches_off() {
    wifiInit("", "");
    int d = shimWiFi().disconnects, b = shimWiFi().begins;
    T += 100; wifiLoop(T);
    TEST_ASSERT_EQUAL_INT((int)StaState::OFF, (int)wifiStaStatus().state);
    if (d == shimWiFi().disconnects) TEST_FAIL_MESSAGE("DISCONNECT not executed");
    TEST_ASSERT_EQUAL_STRING("sta: off", lastLine().c_str());
    runFor(30000);
    TEST_ASSERT_EQUAL_INT(b, shimWiFi().begins);              // never begins without ssid
    char ip[16] = "x"; wifiGetStaIp(ip, sizeof ip);
    TEST_ASSERT_EQUAL_STRING("", ip);
}

void test_v1517_no_wifi_call_inside_critical_section() {
    // WiFi.status(), begin(), localIP() ... use FreeRTOS/lwIP internally and must never run while
    // interrupts are masked (taskENTER_CRITICAL). The shim counts every call made inside one.
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, shimWiFi().in_critical.load(), "WiFi called inside taskENTER_CRITICAL");
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_init_starts_ap_and_mdns);
    RUN_TEST(test_first_loop_begins_immediately);
    RUN_TEST(test_steps_only_every_100ms);
    RUN_TEST(test_connection_lost_three_attempts_then_backoff);
    RUN_TEST(test_step_gap_near_wraparound);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_status_read_from_other_task);
    RUN_TEST(test_ip_buffers);
    RUN_TEST(test_empty_ssid_switches_off);
    RUN_TEST(test_v1517_no_wifi_call_inside_critical_section);
    return UNITY_END();
}
