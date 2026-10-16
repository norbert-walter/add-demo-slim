// FROZEN TESTS (Contract V1.5.20) – Sprint I1 must make these pass. Do not modify.
// Host test of the REAL src/app/net_task.cpp and src/main.cpp. Every function they call is a
// recording stub defined below, so the tests see the exact order of the calls.
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include "logic/params.h"

static std::vector<std::string> g_calls;
static void rec(const std::string& s) { g_calls.push_back(s); }
static int count(const std::string& s) { int n = 0; for (auto& c : g_calls) if (c == s) ++n; return n; }
static int indexOf(const std::string& s) { for (size_t i = 0; i < g_calls.size(); ++i) if (g_calls[i] == s) return (int)i; return -1; }

// ---- stubs (results controlled by the tests)
static bool g_persist = false, g_reboot = false, g_save_ok = true, g_ctrl_ok = true, g_disp_ok = true;
static Params g_loaded;
static Params g_config;
void systemInit() { rec("systemInit"); }
void systemRestart() { rec("systemRestart"); }
void nvsLoadParams(Params& p) { rec("nvsLoadParams"); p = g_loaded; }
bool nvsSaveParams(const Params& p) { rec(std::string("nvsSaveParams ") + p.wifi_ssid); return g_save_ok; }
void nvsErase() { rec("nvsErase"); }
void cmdQueueInit() { rec("cmdQueueInit"); }
bool controlTaskStart(const Params& p) { rec(std::string("controlTaskStart ") + p.wifi_ssid); return g_ctrl_ok; }
bool displayStart() { rec("displayStart"); return g_disp_ok; }
void wifiInit(const char* ssid, const char* pass) { rec(std::string("wifiInit ") + ssid + " " + pass); }
void wifiLoop(uint32_t now) { rec("wifiLoop " + std::to_string(now)); }
void httpServerInit() { rec("httpServerInit"); }
void httpServerLoop() { rec("httpServerLoop"); }
bool snapshotTakePersistRequest() { bool r = g_persist; g_persist = false; rec("takePersist"); return r; }
bool snapshotTakeRebootRequest() { bool r = g_reboot; g_reboot = false; rec("takeReboot"); return r; }
void snapshotGetConfig(Params& p, SafetyFlags& f) { rec("snapshotGetConfig"); p = g_config; f = SafetyFlags(); }
void logLine(const char* fmt, ...) {
    char b[160]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    rec(std::string("log ") + b);
}

#include "app/net_task.cpp"   // implementation under test
#include "main.cpp"           // implementation under test

void setUp() { g_calls.clear(); }
void tearDown() {}

static void step(uint32_t t) { netTaskStep(t); }

void test_setup_order() {
    paramsDefaults(g_loaded);
    strcpy(g_loaded.wifi_ssid, "Werkstatt"); strcpy(g_loaded.wifi_pass, "geheim123");
    SPIFFS.mount_ok = true; shimTasks() = ShimTasks();
    setup();
    const std::vector<std::string> exp = {"systemInit", "nvsLoadParams", "cmdQueueInit", "controlTaskStart Werkstatt",
        "displayStart", "wifiInit Werkstatt geheim123", "httpServerInit", "log boot: ready, FW " FW_VERSION};
    TEST_ASSERT_EQUAL_UINT32(exp.size(), g_calls.size());
    for (size_t i = 0; i < exp.size(); ++i) TEST_ASSERT_EQUAL_STRING(exp[i].c_str(), g_calls[i].c_str());
    TEST_ASSERT_EQUAL_INT(1, SPIFFS.begins);
    TEST_ASSERT_EQUAL_INT(1, SPIFFS.format_on_fail);                 // begin(true)
    TEST_ASSERT_EQUAL_UINT32(1, shimTasks().created.size());          // the net task, after httpServerInit
    const ShimTask& t = shimTasks().created[0];
    TEST_ASSERT_EQUAL_STRING("net", t.name.c_str());
    TEST_ASSERT_EQUAL_UINT32(NET_TASK_STACK, t.stack);
    TEST_ASSERT_EQUAL_UINT32(NET_TASK_PRIO, t.prio);
    TEST_ASSERT_EQUAL_INT(NET_TASK_CORE, t.core);
    TEST_ASSERT_NOT_NULL(t.fn);
}
void test_setup_continues_after_failures() {
    SPIFFS.mount_ok = false; g_ctrl_ok = false; g_disp_ok = false; shimTasks() = ShimTasks(); shimTasks().result = 0;
    setup();
    TEST_ASSERT_TRUE(indexOf("log spiffs: mount failed") > indexOf("systemInit"));
    TEST_ASSERT_TRUE(indexOf("log spiffs: mount failed") < indexOf("nvsLoadParams"));
    TEST_ASSERT_EQUAL_INT(indexOf("controlTaskStart Werkstatt") + 1, indexOf("log FATAL: control task start failed"));
    TEST_ASSERT_EQUAL_INT(indexOf("displayStart") + 1, indexOf("log display: start failed"));
    TEST_ASSERT_EQUAL_INT(1, count("httpServerInit"));                 // everything else still starts
    TEST_ASSERT_TRUE(indexOf("log FATAL: net task start failed") > indexOf("httpServerInit"));
    TEST_ASSERT_EQUAL_STRING(("log boot: ready, FW " FW_VERSION), g_calls.back().c_str());
    SPIFFS.mount_ok = true; g_ctrl_ok = true; g_disp_ok = true; shimTasks() = ShimTasks();
}
void test_loop_only_sleeps() {
    long d = shimTasks().delays;
    loop();
    TEST_ASSERT_EQUAL_UINT32(0, g_calls.size());
    TEST_ASSERT_EQUAL_INT(d + 1, shimTasks().delays);
}
void test_step_order_without_requests() {
    step(1000);
    const std::vector<std::string> exp = {"wifiLoop 1000", "httpServerLoop", "takePersist", "takeReboot"};
    TEST_ASSERT_EQUAL_UINT32(exp.size(), g_calls.size());
    for (size_t i = 0; i < exp.size(); ++i) TEST_ASSERT_EQUAL_STRING(exp[i].c_str(), g_calls[i].c_str());
}
void test_persist_saves_the_snapshot_config() {
    paramsDefaults(g_config); strcpy(g_config.wifi_ssid, "Neu");
    g_persist = true; step(1005);
    const std::vector<std::string> exp = {"wifiLoop 1005", "httpServerLoop", "takePersist", "snapshotGetConfig",
                                          "nvsSaveParams Neu", "log nvs: saved", "takeReboot"};
    TEST_ASSERT_EQUAL_UINT32(exp.size(), g_calls.size());
    for (size_t i = 0; i < exp.size(); ++i) TEST_ASSERT_EQUAL_STRING(exp[i].c_str(), g_calls[i].c_str());
    g_calls.clear(); g_save_ok = false; g_persist = true; step(1010);
    TEST_ASSERT_EQUAL_INT(1, count("log nvs: save FAILED"));
    g_save_ok = true;
    g_calls.clear(); step(1015);
    TEST_ASSERT_EQUAL_INT(0, count("nvsSaveParams Neu"));            // only on request
}
void test_reboot_after_delay() {
    g_reboot = true; step(2000);
    TEST_ASSERT_EQUAL_INT(1, count("log reboot: in 700 ms"));
    TEST_ASSERT_EQUAL_INT(0, count("systemRestart"));
    g_calls.clear();
    step(2000 + REBOOT_DELAY_MS - 1);
    TEST_ASSERT_EQUAL_INT(0, count("systemRestart"));
    g_reboot = true; step(2000 + REBOOT_DELAY_MS - 1);               // a second request does not postpone
    TEST_ASSERT_EQUAL_INT(0, count("log reboot: in 700 ms"));
    step(2000 + REBOOT_DELAY_MS);
    TEST_ASSERT_EQUAL_INT(1, count("systemRestart"));
    TEST_ASSERT_EQUAL_STRING("systemRestart", g_calls.back().c_str());   // last action of the step
    g_calls.clear(); step(2000 + REBOOT_DELAY_MS + 5); step(5000);
    TEST_ASSERT_EQUAL_INT(0, count("systemRestart"));                // only once
}
void test_persist_before_reboot_in_the_same_step() {
    paramsDefaults(g_config); strcpy(g_config.wifi_ssid, "Router");
    g_reboot = true; step(9000);
    g_calls.clear();
    g_persist = true; step(9000 + REBOOT_DELAY_MS);
    TEST_ASSERT_TRUE(indexOf("nvsSaveParams Router") >= 0);
    TEST_ASSERT_TRUE(indexOf("nvsSaveParams Router") < indexOf("systemRestart"));
}
void test_reboot_due_time_wraps_around() {
    const uint32_t t = 0xFFFFFFFFu - 300;                            // due = t + 700 wraps to 399
    g_reboot = true; step(t);
    g_calls.clear();
    step(t + 200); step(0xFFFFFFFFu); step(100);                     // 400, 300 and ... before due
    TEST_ASSERT_EQUAL_INT(0, count("systemRestart"));
    step(t + REBOOT_DELAY_MS);
    TEST_ASSERT_EQUAL_INT(1, count("systemRestart"));
}
void test_reboot_due_exactly_at_zero() {
    const uint32_t t = 0u - REBOOT_DELAY_MS;                         // due time is exactly 0
    g_reboot = true; step(t);
    g_calls.clear();
    step(t + REBOOT_DELAY_MS - 1);
    TEST_ASSERT_EQUAL_INT(0, count("systemRestart"));
    step(0);
    TEST_ASSERT_EQUAL_INT(1, count("systemRestart"));
}
void test_net_task_start_parameters() {
    shimTasks() = ShimTasks();
    TEST_ASSERT_TRUE(netTaskStart());
    TEST_ASSERT_EQUAL_STRING("net", shimTasks().created.back().name.c_str());
    TEST_ASSERT_EQUAL_INT(NET_TASK_CORE, shimTasks().created.back().core);
    shimTasks().result = 0;
    TEST_ASSERT_FALSE(netTaskStart());
    shimTasks() = ShimTasks();
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_setup_order);
    RUN_TEST(test_setup_continues_after_failures);
    RUN_TEST(test_loop_only_sleeps);
    RUN_TEST(test_step_order_without_requests);
    RUN_TEST(test_persist_saves_the_snapshot_config);
    RUN_TEST(test_reboot_after_delay);
    RUN_TEST(test_persist_before_reboot_in_the_same_step);
    RUN_TEST(test_reboot_due_time_wraps_around);
    RUN_TEST(test_reboot_due_exactly_at_zero);
    RUN_TEST(test_net_task_start_parameters);
    return UNITY_END();
}
