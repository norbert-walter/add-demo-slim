// FROZEN TESTS (Contract V1.5.23) – Sprint I1 (+ V1.5.23 serial console). Do not modify.
// Host test of the REAL src/app/net_task.cpp and src/main.cpp. Every function they call is a
// recording stub defined below, so the tests see the exact order of the calls.
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "logic/params.h"
#include "logic/types.h"
#include "logic/wifi_logic.h"
#include "config.h"

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
void controlResetCycleStats() { rec("controlResetCycleStats"); }   // V1.5.25
void wifiInit(const char* ssid, const char* pass) { rec(std::string("wifiInit ") + ssid + " " + pass); }
void wifiLoop(uint32_t now) { rec("wifiLoop " + std::to_string(now)); }
void httpServerInit() { rec("httpServerInit"); }
static int g_http_sleep_ms = 0;                                   // V1.5.25: slow handleClient
void httpServerLoop() { rec("httpServerLoop"); if (g_http_sleep_ms) std::this_thread::sleep_for(std::chrono::milliseconds(g_http_sleep_ms)); }
bool snapshotTakePersistRequest() { bool r = g_persist; g_persist = false; rec("takePersist"); return r; }
bool snapshotTakeRebootRequest() { bool r = g_reboot; g_reboot = false; rec("takeReboot"); return r; }
void snapshotGetConfig(Params& p, SafetyFlags& f) { rec("snapshotGetConfig"); p = g_config; f = SafetyFlags(); }
void logLine(const char* fmt, ...) {
    char b[160]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    rec(std::string("log ") + b);
}

// V1.5.23: serial console stubs
static std::vector<ApiError> g_results;              // results of cmdQueueSendAndWait in order; empty -> OK
static ProcData g_proc;
static StaStatus g_sta;
ApiError cmdQueueSendAndWait(Command& c, uint32_t timeout_ms) {
    std::string s = std::string("cmd ") + cmdName(c.id);
    if (c.id == CmdId::SET) s += std::string(" ") + c.key + "=" + c.str + (c.vtype == ValueType::STRING ? "" : " (not STRING)");
    if (c.source != CmdSource::API) s += " (not API)";
    if (!c.want_reply) s += " (no reply)";
    if (timeout_ms != API_REPLY_TIMEOUT_MS) s += " (timeout " + std::to_string(timeout_ms) + ")";
    rec(s);
    ApiError r = ApiError::OK;
    if (!g_results.empty()) { r = g_results.front(); g_results.erase(g_results.begin()); }
    return r;
}
void snapshotGet(ProcData& d) { rec("snapshotGet"); d = g_proc; }
static uint32_t g_last_rx = 0;                                    // V1.5.24
uint32_t snapshotLastApiRx() { return g_last_rx; }
StaStatus wifiStaStatus() { return g_sta; }
void wifiGetApIp(char* b, size_t cap) { snprintf(b, cap, "192.168.4.1"); }
void wifiGetStaIp(char* b, size_t cap) { snprintf(b, cap, "%s", g_sta.state == StaState::CONNECTED ? "192.168.1.57" : ""); }

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
        "displayStart", "wifiInit Werkstatt geheim123", "httpServerInit", "log boot: ready, FW " FW_VERSION, "controlResetCycleStats"};
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
    TEST_ASSERT_EQUAL_STRING(("log boot: ready, FW " FW_VERSION), g_calls[g_calls.size() - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("controlResetCycleStats", g_calls.back().c_str());   // V1.5.25
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
    const std::vector<std::string> exp = {"wifiLoop 1000", "httpServerLoop", "snapshotGet", "takePersist", "takeReboot"};
    TEST_ASSERT_EQUAL_UINT32(exp.size(), g_calls.size());
    for (size_t i = 0; i < exp.size(); ++i) TEST_ASSERT_EQUAL_STRING(exp[i].c_str(), g_calls[i].c_str());
}
void test_persist_saves_the_snapshot_config() {
    paramsDefaults(g_config); strcpy(g_config.wifi_ssid, "Neu");
    g_persist = true; step(1005);
    const std::vector<std::string> exp = {"wifiLoop 1005", "httpServerLoop", "snapshotGet", "takePersist", "snapshotGetConfig",
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

// ---- V1.5.23: serial console --------------------------------------------------------------------
static std::vector<std::string> outLines() {                    // complete answer lines
    std::vector<std::string> r; std::string cur;
    for (char c : Serial.out) { if (c == '\n') { r.push_back(cur); cur.clear(); } else cur += c; }
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", cur.c_str(), "every answer ends with \\n");
    return r;
}
static std::string line0() { std::vector<std::string> l = outLines(); return l.empty() ? std::string("<no answer>") : l[0]; }
static std::vector<std::string> cmds() { std::vector<std::string> r; for (auto& c : g_calls) if (c.rfind("cmd ", 0) == 0) r.push_back(c); return r; }
static void feed(const char* s) { Serial.in += s; }
static void console(const char* line, uint32_t t = 5000) { Serial.clear(); g_calls.clear(); feed(line); step(t); }

void test_v1523_console_help_ver_unknown_empty() {
    console("ver\r\n");
    TEST_ASSERT_EQUAL_UINT32(1, outLines().size());
    TEST_ASSERT_EQUAL_STRING("{\"fw\":\"" FW_VERSION "\"}", line0().c_str());
    console("help\n");
    TEST_ASSERT_EQUAL_STRING("{\"help\":\"help | ver | status | reboot | wifi <ssid> <password> | wifi \\\"<ssid>\\\" <password> | wifi off\"}",
                             line0().c_str());
    console("  bogus\n");
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"unknown command, try help\"}", line0().c_str());
    console("Ver\n");                                                       // case-sensitive
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"unknown command, try help\"}", line0().c_str());
    console("\n   \r\n");
    TEST_ASSERT_EQUAL_STRING("", Serial.out.c_str());                      // empty lines: no answer
    TEST_ASSERT_TRUE(cmds().empty());
}
void test_v1523_console_status() {
    g_proc = ProcData(); g_proc.state = SafetyState::READY; g_proc.err = ErrorCode::NONE; g_proc.local = true;
    paramsDefaults(g_config); strcpy(g_config.wifi_ssid, "My \"Lab\"\\"); strcpy(g_config.wifi_pass, "secret123");
    g_sta = StaStatus(); g_sta.state = StaState::CONNECTED;
    console("status\n");
    TEST_ASSERT_EQUAL_STRING("{\"state\":\"READY\",\"err\":0,\"local\":true,\"ap\":\"192.168.4.1\",\"sta\":\"connected\","
                             "\"sta_ip\":\"192.168.1.57\",\"ssid\":\"My \\\"Lab\\\"\\\\\",\"pass_set\":true}", line0().c_str());
    TEST_ASSERT_TRUE(Serial.out.find("secret123") == std::string::npos);
    g_proc.state = SafetyState::WAIT_ACK; g_proc.err = ErrorCode::ESTOP; g_proc.local = false;
    g_config.wifi_ssid[0] = 0; g_config.wifi_pass[0] = 0; g_sta.state = StaState::BACKOFF;
    console("status\n");
    TEST_ASSERT_EQUAL_STRING("{\"state\":\"WAIT_ACK\",\"err\":1,\"local\":false,\"ap\":\"192.168.4.1\",\"sta\":\"backoff\","
                             "\"sta_ip\":\"\",\"ssid\":\"\",\"pass_set\":false}", line0().c_str());
    TEST_ASSERT_TRUE(cmds().empty());
}
void test_v1523_console_wifi_sets_and_reboots_in_one_step() {
    g_persist = true; g_reboot = true;                                    // what the control task would request
    console("wifi Lab secret 123\n", 1000);
    const std::vector<std::string> exp = {"cmd SET wifi_ssid=Lab", "cmd SET wifi_pass=secret 123", "cmd REBOOT"};
    TEST_ASSERT_TRUE(cmds() == exp);
    TEST_ASSERT_EQUAL_STRING("{\"wifi\":\"saved\",\"ssid\":\"Lab\",\"reboot\":true}", line0().c_str());
    TEST_ASSERT_TRUE(Serial.out.find("secret") == std::string::npos);    // password never echoed
    TEST_ASSERT_TRUE(indexOf("cmd REBOOT") < indexOf("wifiLoop 1000"));  // step 0 before step 1
    TEST_ASSERT_TRUE(indexOf("wifiLoop 1000") < indexOf("takePersist"));
    TEST_ASSERT_EQUAL_INT(1, count("log reboot: in 700 ms"));             // same step schedules the reboot
    step(1000 + REBOOT_DELAY_MS);                                         // and it happens
    TEST_ASSERT_EQUAL_INT(1, count("systemRestart"));
}
void test_v1523_console_wifi_quoted_ssid_and_usage() {
    console("wifi \"My Net\" pw with spaces\n");
    const std::vector<std::string> exp = {"cmd SET wifi_ssid=My Net", "cmd SET wifi_pass=pw with spaces", "cmd REBOOT"};
    TEST_ASSERT_TRUE(cmds() == exp);
    TEST_ASSERT_EQUAL_STRING("{\"wifi\":\"saved\",\"ssid\":\"My Net\",\"reboot\":true}", line0().c_str());
    const char* bad[] = {"wifi\n", "wifi Lab\n", "wifi Lab \n", "wifi \"My Net\"\n", "wifi \"My Net pw\n"};
    for (const char* b : bad) {
        console(b);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("{\"error\":\"usage: wifi <ssid> <password> | wifi off\"}", line0().c_str(), b);
        TEST_ASSERT_TRUE_MESSAGE(cmds().empty(), b);
    }
}
void test_v1523_console_wifi_errors_stop_the_sequence() {
    g_results = {ApiError::OK, ApiError::BAD_PARAM};
    console("wifi Lab short\n");
    const std::vector<std::string> exp = {"cmd SET wifi_ssid=Lab", "cmd SET wifi_pass=short"};
    TEST_ASSERT_TRUE(cmds() == exp);                                       // no REBOOT
    char e[160]; snprintf(e, sizeof e, "{\"error\":\"%s\",\"err\":-4,\"key\":\"wifi_pass\"}", apiErrorMessage(ApiError::BAD_PARAM));
    TEST_ASSERT_EQUAL_STRING(e, line0().c_str());
    g_results = {ApiError::BUSY};
    console("wifi Lab secret123\n");
    TEST_ASSERT_EQUAL_UINT32(1, cmds().size());
    snprintf(e, sizeof e, "{\"error\":\"%s\",\"err\":-7,\"key\":\"wifi_ssid\"}", apiErrorMessage(ApiError::BUSY));
    TEST_ASSERT_EQUAL_STRING(e, line0().c_str());
    g_results = {ApiError::OK, ApiError::OK, ApiError::STATE};            // REBOOT not allowed (RUNNING)
    console("wifi Lab secret123\n");
    TEST_ASSERT_EQUAL_UINT32(3, cmds().size());
    TEST_ASSERT_EQUAL_STRING("{\"wifi\":\"saved\",\"ssid\":\"Lab\",\"reboot\":false,\"err\":-5}", line0().c_str());
    g_results.clear();
}
void test_v1523_console_wifi_off_and_reboot() {
    console("wifi off\n");
    const std::vector<std::string> exp = {"cmd SET wifi_ssid=", "cmd SET wifi_pass=", "cmd REBOOT"};
    TEST_ASSERT_TRUE(cmds() == exp);
    TEST_ASSERT_EQUAL_STRING("{\"wifi\":\"off\",\"reboot\":true}", line0().c_str());
    console("reboot\n");
    TEST_ASSERT_TRUE(cmds() == std::vector<std::string>{"cmd REBOOT"});
    TEST_ASSERT_EQUAL_STRING("{\"reboot\":true}", line0().c_str());
    g_results = {ApiError::LOCAL_CONTROL};
    console("reboot\n");
    char e[160]; snprintf(e, sizeof e, "{\"error\":\"%s\",\"err\":-10}", apiErrorMessage(ApiError::LOCAL_CONTROL));
    TEST_ASSERT_EQUAL_STRING(e, line0().c_str());
    g_results.clear();
    g_reboot = false; g_persist = false;
}
void test_v1523_console_partial_lines_limits_and_one_command_per_step() {
    Serial.clear(); g_calls.clear();
    feed("ve"); step(10);
    TEST_ASSERT_EQUAL_STRING("", Serial.out.c_str());                       // incomplete line: wait
    feed("r\nver\n"); step(11);
    TEST_ASSERT_EQUAL_UINT32(1, outLines().size());                         // one command per step
    TEST_ASSERT_EQUAL_STRING("ver\n", Serial.in.c_str());                   // the rest stays
    step(12);
    TEST_ASSERT_EQUAL_UINT32(2, outLines().size());
    Serial.clear();
    std::string longl(CONSOLE_LINE_MAX + 5, 'x'); longl += "\nver\n";
    feed(longl.c_str());
    for (int i = 0; i < 4; ++i) step(20 + i);
    std::vector<std::string> l = outLines();
    TEST_ASSERT_EQUAL_UINT32(2, l.size());
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"line too long\"}", l[0].c_str());
    TEST_ASSERT_EQUAL_STRING("{\"fw\":\"" FW_VERSION "\"}", l[1].c_str());       // the next line works again
    Serial.clear();
    std::string exact(CONSOLE_LINE_MAX, ' '); exact.replace(exact.size() - 3, 3, "ver"); exact += "\n";
    feed(exact.c_str()); for (int i = 0; i < 3; ++i) step(30 + i);
    TEST_ASSERT_EQUAL_STRING("{\"fw\":\"" FW_VERSION "\"}", line0().c_str());  // exactly the maximum is accepted
    Serial.clear(); Serial.reads = 0;
    std::string many(300, ' '); feed(many.c_str());
    step(40);
    TEST_ASSERT_TRUE(Serial.reads <= CONSOLE_READ_MAX);                     // bounded work per step
    TEST_ASSERT_TRUE(Serial.in.size() >= 300 - CONSOLE_READ_MAX);
    Serial.clear();
}

// ---- V1.5.24: comm diagnostics -------------------------------------------------------------------
static int logs(const char* prefix) { int n = 0; for (auto& c : g_calls) if (c.rfind(std::string("log ") + prefix, 0) == 0) ++n; return n; }
void test_v1525_slow_parts_and_gaps_logged() {
    step(9000);                                                     // reference end of step
    g_calls.clear();
    step(9005);
    TEST_ASSERT_EQUAL_INT(0, logs("net: "));                        // fast step: nothing
    g_http_sleep_ms = NET_SLOW_LOG_MS + 60;
    g_calls.clear();
    step(9010);
    g_http_sleep_ms = 0;
    TEST_ASSERT_EQUAL_INT(1, logs("net: http "));
    TEST_ASSERT_EQUAL_INT(1, logs("net: "));
    std::this_thread::sleep_for(std::chrono::milliseconds(NET_SLOW_LOG_MS + 60));
    g_calls.clear();
    step(9300);
    TEST_ASSERT_EQUAL_INT(1, logs("net: gap "));
    TEST_ASSERT_EQUAL_INT(1, logs("net: "));
    TEST_ASSERT_TRUE(indexOf("wifiLoop 9300") > 0);                 // the gap line comes first
}
void test_v1524_comm_gap_and_timeout_logged() {
    g_proc = ProcData(); g_proc.safety.comm_armed = false; g_last_rx = 500;
    g_calls.clear(); step(500);
    g_last_rx = 9000; step(9000);                                         // long gap, but not armed
    TEST_ASSERT_EQUAL_INT(0, logs("comm:"));
    g_proc.safety.comm_armed = true; g_last_rx = 10000; step(10000);      // armed now, but not at the previous packet
    TEST_ASSERT_EQUAL_INT(0, logs("comm:"));
    g_last_rx = 11500; step(11500);                                       // 1500 ms: no log
    g_last_rx = 13500; step(13500);                                       // exactly 2000: no log
    TEST_ASSERT_EQUAL_INT(0, logs("comm:"));
    g_last_rx = 16000; step(16000);                                       // 2500 ms
    TEST_ASSERT_EQUAL_INT(1, count("log comm: /api gap 2500 ms"));
    step(17000); step(18000);                                             // no new packet: nothing
    TEST_ASSERT_EQUAL_INT(1, logs("comm:"));
    g_proc.err = ErrorCode::COMM_TIMEOUT; g_proc.safety.comm_armed = false; step(21100);
    TEST_ASSERT_EQUAL_INT(1, count("log comm: timeout, last /api packet 5100 ms ago, longest gap while armed 2500 ms"));
    step(21200); step(21300);                                             // only once (edge)
    TEST_ASSERT_EQUAL_INT(2, logs("comm:"));
    g_proc.err = ErrorCode::NONE; step(30000);                            // acknowledged, disarmed: reset
    g_proc.safety.comm_armed = true; g_last_rx = 31000; step(31000);
    g_last_rx = 32000; step(32000);
    g_proc.err = ErrorCode::COMM_TIMEOUT; step(37500);
    TEST_ASSERT_EQUAL_INT(1, count("log comm: timeout, last /api packet 5500 ms ago, longest gap while armed 1000 ms"));
    g_proc = ProcData();
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
    RUN_TEST(test_v1523_console_help_ver_unknown_empty);
    RUN_TEST(test_v1523_console_status);
    RUN_TEST(test_v1523_console_wifi_sets_and_reboots_in_one_step);
    RUN_TEST(test_v1523_console_wifi_quoted_ssid_and_usage);
    RUN_TEST(test_v1523_console_wifi_errors_stop_the_sequence);
    RUN_TEST(test_v1523_console_wifi_off_and_reboot);
    RUN_TEST(test_v1523_console_partial_lines_limits_and_one_command_per_step);
    RUN_TEST(test_v1524_comm_gap_and_timeout_logged);
    RUN_TEST(test_v1525_slow_parts_and_gaps_logged);
    return UNITY_END();
}
