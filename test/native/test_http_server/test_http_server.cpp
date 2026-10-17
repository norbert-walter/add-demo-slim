// FROZEN TESTS (Contract V1.5.22) – Sprint P2 must make these pass. Do not modify.
// Host test of the REAL src/comm/http_server.cpp together with the real snapshot.cpp,
// command_queue.cpp and log_buffer.cpp, using the WebServer/SPIFFS shims in
// test/native/freertos_shim. A thread plays the control task (replies to queued commands).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <string.h>
#include <string>
#include <thread>
#include <atomic>
#include "logic/crc32.h"
#include "comm/log_buffer.cpp"
#include "app/snapshot.cpp"
#include "app/command_queue.cpp"
#include "comm/http_server.cpp"   // implementation under test

static std::atomic<bool> g_run{false};
static std::atomic<bool> g_reply{true};
static std::thread g_ctrl;
static std::atomic<int> g_cmds{0};
static ApiError resultFor(const Command& c) {          // deterministic result per command
    switch (c.id) {
        case CmdId::HV_ON: return ApiError::PRECONDITION;
        case CmdId::MOVE:  return c.pos > 100 ? ApiError::LOCAL_CONTROL : ApiError::OK;
        default:           return ApiError::OK;
    }
}
static void ctrlLoop() {
    while (g_run) {
        Command c;
        if (g_reply && cmdQueueReceive(c, true)) {
            ++g_cmds;
            if (c.id == CmdId::MOVE && c.pos == 77.0f)                       // slow reply, within API_REPLY_TIMEOUT_MS
                std::this_thread::sleep_for(std::chrono::milliseconds(API_REPLY_TIMEOUT_MS / 2));
            cmdQueueReply(c, resultFor(c));
        }
        std::this_thread::sleep_for(std::chrono::microseconds(300));
    }
}
static WebServer& srv() { return *shimServer(); }
static const ShimResponse& request(HTTPMethod m, const char* uri, const std::string* body = nullptr) {
    size_t before = srv().responses.size();
    srv().shimQueue(m, uri, body);
    httpServerLoop();
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(before + 1, srv().responses.size(), "exactly one response per request");
    return srv().responses.back();
}
static std::string framed(const char* json) {
    char b[600]; strcpy(b, json);
    int n = crcFrameAppend(b, strlen(b), sizeof b);
    return std::string(b, n);
}
static std::string unframe(const std::string& s) {     // remove ,"crc":"XXXXXXXX"
    size_t p = s.find(",\"crc\":\"");
    return p == std::string::npos ? s : s.substr(0, p) + "}";
}
static std::string errBody(ApiError e, const char* cmd) {   // message text comes from apiErrorMessage (types.h)
    return std::string("{\"status\":\"error\",\"err\":") + std::to_string((int)e) + ",\"cmd\":\"" + cmd +
           "\",\"msg\":\"" + apiErrorMessage(e) + "\"}";
}
static void assertCors(const ShimResponse& r) { TEST_ASSERT_EQUAL_STRING("*", r.header("Access-Control-Allow-Origin").c_str()); }

void setUp() {}
void tearDown() {}

void test_init_registers_routes_and_begins() {
    cmdQueueInit();
    g_run = true; g_ctrl = std::thread(ctrlLoop);
    httpServerInit();
    TEST_ASSERT_NOT_NULL(shimServer());
    TEST_ASSERT_EQUAL_INT(HTTP_PORT, srv().port());
    TEST_ASSERT_TRUE(srv().begun());
    int h = srv().handle_calls;
    httpServerLoop();
    TEST_ASSERT_EQUAL_INT(h + 1, srv().handle_calls);           // loop = handleClient
}
void test_static_files_and_404() {
    shimFiles()["/www/index.html"] = "<html>index</html>";
    shimFiles()["/www/settings.html"] = "<html>settings</html>";
    shimFiles()["/add.json"] = "{\"add\":1}";
    shimFiles()["/doc.md"] = "# Doc";
    const ShimResponse& a = request(HTTP_GET, "/");
    TEST_ASSERT_EQUAL_INT(200, a.code); TEST_ASSERT_EQUAL_STRING("text/html", a.type.c_str());
    TEST_ASSERT_EQUAL_STRING("<html>index</html>", a.body.c_str()); assertCors(a);
    const ShimResponse& s = request(HTTP_GET, "/settings");
    TEST_ASSERT_EQUAL_STRING("<html>settings</html>", s.body.c_str()); TEST_ASSERT_EQUAL_STRING("text/html", s.type.c_str());
    const ShimResponse& j = request(HTTP_GET, "/add");
    TEST_ASSERT_EQUAL_STRING("application/json", j.type.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"add\":1}", j.body.c_str());
    TEST_ASSERT_EQUAL_STRING("no-store", j.header("Cache-Control").c_str()); assertCors(j);
    const ShimResponse& d = request(HTTP_GET, "/doc");
    TEST_ASSERT_EQUAL_STRING("text/markdown", d.type.c_str()); TEST_ASSERT_EQUAL_STRING("# Doc", d.body.c_str());
    TEST_ASSERT_EQUAL_INT(0, shimOpenFiles());                  // every file closed again
    const ShimResponse& n = request(HTTP_GET, "/nothing");
    TEST_ASSERT_EQUAL_INT(404, n.code); TEST_ASSERT_EQUAL_STRING("application/json", n.type.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"not found\"}", n.body.c_str()); assertCors(n);
    shimFiles().erase("/doc.md");
    const ShimResponse& m = request(HTTP_GET, "/doc");          // missing file -> 404
    TEST_ASSERT_EQUAL_INT(404, m.code); TEST_ASSERT_EQUAL_STRING("{\"error\":\"not found\"}", m.body.c_str());
}
void test_v1522_web_assets() {
    shimFiles()["/www/app.js"] = "var ADDAPP={};";
    shimFiles()["/www/style.css"] = "body{}";
    const char* fonts[] = {"u400", "u700", "m400", "m700"};
    for (const char* f : fonts) shimFiles()[std::string("/www/font-") + f + ".woff2"] = std::string("wOF2") + f;
    const ShimResponse& j = request(HTTP_GET, "/app.js");
    TEST_ASSERT_EQUAL_INT(200, j.code); TEST_ASSERT_EQUAL_STRING("application/javascript", j.type.c_str());
    TEST_ASSERT_EQUAL_STRING("var ADDAPP={};", j.body.c_str()); assertCors(j);
    const ShimResponse& c = request(HTTP_GET, "/style.css");
    TEST_ASSERT_EQUAL_STRING("text/css", c.type.c_str()); TEST_ASSERT_EQUAL_STRING("body{}", c.body.c_str()); assertCors(c);
    for (const char* f : fonts) {
        std::string uri = std::string("/font-") + f + ".woff2";
        const ShimResponse& r = request(HTTP_GET, uri.c_str());
        TEST_ASSERT_EQUAL_INT_MESSAGE(200, r.code, uri.c_str());
        TEST_ASSERT_EQUAL_STRING("font/woff2", r.type.c_str());
        TEST_ASSERT_EQUAL_STRING((std::string("wOF2") + f).c_str(), r.body.c_str());
        TEST_ASSERT_EQUAL_STRING("max-age=86400", r.header("Cache-Control").c_str()); assertCors(r);
        TEST_ASSERT_TRUE_MESSAGE(strlen("/www/font-") + strlen(f) + strlen(".woff2") <= 31, "SPIFFS name length");
    }
    TEST_ASSERT_EQUAL_INT(0, shimOpenFiles());
    shimFiles().erase("/www/font-m400.woff2");
    TEST_ASSERT_EQUAL_INT(404, request(HTTP_GET, "/font-m400.woff2").code);
    TEST_ASSERT_EQUAL_INT(404, request(HTTP_GET, "/www/app.js").code);   // only the listed routes
}
void test_add_html_embeds_and_escapes() {
    std::string json = "{\"name\":\"a<b & c>d\",\"x\":\"</script>\"}";
    std::string big; for (int i = 0; i < 40; ++i) big += "{\"k\":\"</x><&>\"},";      // > 256 bytes: chunks
    json = "[" + big + json + "]";
    shimFiles()["/add.json"] = json;
    const ShimResponse& r = request(HTTP_GET, "/add.html");
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING("text/html; charset=utf-8", r.type.c_str());
    TEST_ASSERT_EQUAL_STRING("no-store, no-cache, must-revalidate", r.header("Cache-Control").c_str());
    TEST_ASSERT_EQUAL_STRING("no-cache", r.header("Pragma").c_str());
    assertCors(r);
    std::string script, pre;                                    // expected: "</" -> "<\/" in the script
    for (size_t i = 0; i < json.size(); ++i) {
        if (json[i] == '<' && i + 1 < json.size() && json[i + 1] == '/') { script += "<\\/"; ++i; } else script += json[i];
    }
    for (char c : json) { if (c == '&') pre += "&amp;"; else if (c == '<') pre += "&lt;"; else if (c == '>') pre += "&gt;"; else pre += c; }
    std::string exp = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>ADD</title>\n"
                      "<script type=\"application/ld+json\">\n" + script + "\n</script></head><body><pre>\n" +
                      pre + "\n</pre></body></html>\n";
    TEST_ASSERT_EQUAL_STRING(exp.c_str(), r.body.c_str());
    TEST_ASSERT_TRUE(r.chunked);
    TEST_ASSERT_TRUE(r.chunk_end);                              // sendContent("") at the end
    TEST_ASSERT_EQUAL_INT(0, shimOpenFiles());
    shimFiles().erase("/add.json");
    TEST_ASSERT_EQUAL_INT(404, request(HTTP_GET, "/add.html").code);
}
void test_llms_txt_exact() {
    const ShimResponse& r = request(HTTP_GET, "/llms.txt");
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING("text/plain", r.type.c_str());
    TEST_ASSERT_EQUAL_STRING(
        "# AI Device Description (ADD)\n"
        "- ADD (JSON): [Device Description](/add)\n"
        "- ADD (HTML): [Device Description for Cloud-AI](/add.html)\n"
        "- Documentation: [Device manual](/doc)\n", r.body.c_str());
    assertCors(r);
}
void test_log_endpoint() {
    logLine("http test %d", 1);
    logLine("http test %d", 2);
    static char exp[LOG_LINES * LOG_LINE_LEN + 1];
    logDump(exp, sizeof exp);
    const ShimResponse& r = request(HTTP_GET, "/log");
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING("text/plain", r.type.c_str());
    TEST_ASSERT_EQUAL_STRING(exp, r.body.c_str());
    assertCors(r);
}
void test_proc_endpoint() {
    ProcData d; d.ts_ms = 123456; d.state = SafetyState::READY; d.motion.pos = 87.25f; d.measure.n = 42;
    snapshotPublish(d);
    char exp[PROC_JSON_MAX]; procToJson(d, exp, sizeof exp);
    const ShimResponse& r = request(HTTP_GET, "/proc");
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING("application/json", r.type.c_str());
    TEST_ASSERT_EQUAL_STRING("no-store", r.header("Cache-Control").c_str());
    TEST_ASSERT_EQUAL_STRING(exp, r.body.c_str());
    assertCors(r);
}
void test_options_preflight() {
    const ShimResponse& r = request(HTTP_OPTIONS, "/api");
    TEST_ASSERT_EQUAL_INT(204, r.code);
    TEST_ASSERT_EQUAL_STRING("", r.body.c_str());
    assertCors(r);
    TEST_ASSERT_EQUAL_STRING("GET, POST, OPTIONS", r.header("Access-Control-Allow-Methods").c_str());
    TEST_ASSERT_EQUAL_STRING("Content-Type", r.header("Access-Control-Allow-Headers").c_str());
}
void test_api_syntax_errors_are_400() {
    snapshotSetLastApiRx(7);
    std::string bad = "{\"cmd\":\"STOP\",\"crc\":\"00000000\"}";
    const ShimResponse& r = request(HTTP_POST, "/api", &bad);
    TEST_ASSERT_EQUAL_INT(400, r.code);
    TEST_ASSERT_EQUAL_STRING("application/json", r.type.c_str());
    TEST_ASSERT_EQUAL_STRING(errBody(ApiError::BAD_CRC, "NONE").c_str(), unframe(r.body).c_str());
    TEST_ASSERT_TRUE(crcFrameVerify(r.body.c_str(), r.body.size()));
    assertCors(r);
    std::string notjson = framed("{\"cmd\" \"STOP\"}");
    TEST_ASSERT_EQUAL_INT(400, request(HTTP_POST, "/api", &notjson).code);
    const ShimResponse& nb = request(HTTP_POST, "/api");                    // no body at all
    TEST_ASSERT_EQUAL_INT(400, nb.code);
    TEST_ASSERT_NOT_NULL(strstr(nb.body.c_str(), "\"err\":-2"));
    std::string pad(API_BODY_MAX, ' ');
    std::string big = framed("{\"cmd\":\"STOP\"}") + pad;                    // > API_BODY_MAX
    const ShimResponse& tb = request(HTTP_POST, "/api", &big);
    TEST_ASSERT_EQUAL_INT(400, tb.code);
    TEST_ASSERT_NOT_NULL(strstr(tb.body.c_str(), "\"err\":-2"));
    TEST_ASSERT_EQUAL_UINT32(7, snapshotLastApiRx());                       // no valid packet yet
}
void test_api_unknown_cmd_200_no_heartbeat() {
    snapshotSetLastApiRx(7);
    std::string b = framed("{\"cmd\":\"FLY\"}");
    const ShimResponse& r = request(HTTP_POST, "/api", &b);
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING(errBody(ApiError::UNKNOWN_CMD, "NONE").c_str(), unframe(r.body).c_str());
    TEST_ASSERT_EQUAL_UINT32(7, snapshotLastApiRx());
}
void test_api_bad_param_counts_as_heartbeat() {
    snapshotSetLastApiRx(7);
    std::string b = framed("{\"cmd\":\"MOVE\",\"params\":{}}");               // pos missing
    int before = g_cmds;
    const ShimResponse& r = request(HTTP_POST, "/api", &b);
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_NOT_NULL(strstr(r.body.c_str(), "\"err\":-4,\"cmd\":\"MOVE\""));
    TEST_ASSERT_TRUE(snapshotLastApiRx() != 7);                               // valid packet
    TEST_ASSERT_EQUAL_INT(before, (int)g_cmds);                               // not queued
}
void test_api_get_state_local() {
    ProcData d; d.state = SafetyState::WAIT_ACK; d.err = ErrorCode::TEMP_HIGH; d.local = true;
    snapshotPublish(d);
    CycleStats cs; cs.cycle_max_us = 312; cs.period_max_us = 1010; cs.overruns = 2; cs.prio_lat_max_us = 1450;
    snapshotSetCycleStats(cs);
    int before = g_cmds;
    snapshotSetLastApiRx(7);
    std::string b = framed("{\"cmd\":\"GET_STATE\"}");
    const ShimResponse& r = request(HTTP_POST, "/api", &b);
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\",\"err\":0,\"cmd\":\"GET_STATE\",\"state\":\"WAIT_ACK\",\"sys_err\":2,"
        "\"local\":true,\"cycle_max_us\":312,\"period_max_us\":1010,\"overruns\":2,\"prio_lat_max_us\":1450}",
        unframe(r.body).c_str());
    TEST_ASSERT_EQUAL_INT(before, (int)g_cmds);                               // answered locally
    TEST_ASSERT_TRUE(snapshotLastApiRx() != 7);
}
void test_api_get_config_local() {
    Params p; paramsDefaults(p); p.measure_pos = 160.0f; strcpy(p.wifi_ssid, "net");
    SafetyFlags f; f.interlock = true;
    snapshotPublishConfig(p, f);
    char member[700]; paramsToJsonMember(p, f, member, sizeof member);
    std::string exp = std::string("{\"status\":\"ok\",\"err\":0,\"cmd\":\"GET_CONFIG\",") + member + "}";
    std::string b = framed("{\"cmd\":\"GET_CONFIG\"}");
    int before = g_cmds;
    const ShimResponse& r = request(HTTP_POST, "/api", &b);
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING(exp.c_str(), unframe(r.body).c_str());
    TEST_ASSERT_TRUE(crcFrameVerify(r.body.c_str(), r.body.size()));
    TEST_ASSERT_EQUAL_INT(before, (int)g_cmds);
}
void test_api_commands_go_through_the_queue() {
    std::string hv = framed("{\"cmd\":\"HV_ON\"}");
    const ShimResponse& a = request(HTTP_POST, "/api", &hv);
    TEST_ASSERT_EQUAL_INT(200, a.code);
    TEST_ASSERT_EQUAL_STRING(errBody(ApiError::PRECONDITION, "HV_ON").c_str(), unframe(a.body).c_str());
    std::string mv = framed("{\"cmd\":\"MOVE\",\"params\":{\"pos\":50}}");
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\",\"err\":0,\"cmd\":\"MOVE\"}", unframe(request(HTTP_POST, "/api", &mv).body).c_str());
    std::string lc = framed("{\"cmd\":\"MOVE\",\"params\":{\"pos\":150}}");
    const ShimResponse& l = request(HTTP_POST, "/api", &lc);
    TEST_ASSERT_EQUAL_INT(200, l.code);                                        // LOCAL_CONTROL is 200
    TEST_ASSERT_NOT_NULL(strstr(l.body.c_str(), "\"err\":-10"));
    assertCors(l);
}
void test_v1516_slow_reply_within_timeout_is_ok() {
    std::string mv = framed("{\"cmd\":\"MOVE\",\"params\":{\"pos\":77}}");
    const ShimResponse& r = request(HTTP_POST, "/api", &mv);                   // control task needs 50 ms
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"ok\",\"err\":0,\"cmd\":\"MOVE\"}", unframe(r.body).c_str());
}
void test_api_busy_when_control_task_does_not_answer() {
    g_reply = false;
    std::string b = framed("{\"cmd\":\"ACK\"}");
    const ShimResponse& r = request(HTTP_POST, "/api", &b);
    g_reply = true;
    TEST_ASSERT_EQUAL_INT(200, r.code);
    TEST_ASSERT_EQUAL_STRING(errBody(ApiError::BUSY, "ACK").c_str(), unframe(r.body).c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));               // stale ACK is consumed
}
void test_proc_while_control_task_publishes() {
    std::atomic<bool> pub{true};
    std::thread w([&] {                                                       // self-consistent snapshots
        uint32_t k = 0;
        while (pub) { ProcData d; d.ts_ms = ++k; d.measure.n = k; d.motion.pos = (float)(k % 200); snapshotPublish(d); }
    });
    int bad = 0;
    for (int i = 0; i < 2000; ++i) {
        const ShimResponse& r = request(HTTP_GET, "/proc");
        unsigned long ts = 0, n = 0;
        const char* a = strstr(r.body.c_str(), "\"ts\":"); const char* b = strstr(r.body.c_str(), "\"n\":");
        if (!a || !b) { ++bad; continue; }
        ts = strtoul(a + 5, nullptr, 10); n = strtoul(b + 4, nullptr, 10);
        if (ts != n) ++bad;
    }
    pub = false; w.join();
    TEST_ASSERT_EQUAL_INT(0, bad);
}
void test_v1517_add_html_sends_blocks_not_bytes() {
    // one sendContent() = one HTTP chunk = one TCP write on the device: never per character.
    std::string json;
    for (int i = 0; json.size() < 4000; ++i) json += (i % 7 == 0) ? "</" : (i % 5 == 0 ? "&<>" : "{\"k\":1},");
    json += "<";                                            // a '<' as the very last byte is sent too
    shimFiles()["/add.json"] = json;
    const ShimResponse& r = request(HTTP_GET, "/add.html");
    std::string script, pre;
    for (size_t i = 0; i < json.size(); ++i) {
        if (json[i] == '<' && i + 1 < json.size() && json[i + 1] == '/') { script += "<\\/"; ++i; } else script += json[i];
    }
    for (char c : json) { if (c == '&') pre += "&amp;"; else if (c == '<') pre += "&lt;"; else if (c == '>') pre += "&gt;"; else pre += c; }
    std::string exp = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>ADD</title>\n"
                      "<script type=\"application/ld+json\">\n" + script + "\n</script></head><body><pre>\n" +
                      pre + "\n</pre></body></html>\n";
    TEST_ASSERT_EQUAL_STRING(exp.c_str(), r.body.c_str());
    int maxChunks = 10 + 2 * (int)((json.size() + 63) / 64);
    if (r.chunks > maxChunks) {
        char m[96]; snprintf(m, sizeof m, "%d sendContent calls for %u bytes (max %d)", r.chunks, (unsigned)json.size(), maxChunks);
        TEST_FAIL_MESSAGE(m);
    }
    TEST_ASSERT_EQUAL_INT(0, shimOpenFiles());
}
void test_v1517_bad_param_is_an_error_also_for_get_state_and_get_config() {
    const char* bodies[] = {"{\"cmd\":\"GET_STATE\",\"params\":[1]}", "{\"cmd\":\"GET_CONFIG\",\"params\":[1]}"};
    const char* names[] = {"GET_STATE", "GET_CONFIG"};
    for (int k = 0; k < 2; ++k) {
        std::string b = framed(bodies[k]);
        snapshotSetLastApiRx(0);
        const ShimResponse& r = request(HTTP_POST, "/api", &b);
        TEST_ASSERT_EQUAL_INT(200, r.code);
        TEST_ASSERT_EQUAL_STRING(errBody(ApiError::BAD_PARAM, names[k]).c_str(), unframe(r.body).c_str());
        TEST_ASSERT_TRUE(snapshotLastApiRx() != 0 || millis() == 0);       // still a heartbeat
    }
}
// V1.5.25 (added by the reviewer; the file stays frozen for the implementer)
void test_v1525_idle_connection_is_closed_and_waiting_request_served() {
    std::string b = framed("{\"cmd\":\"GET_STATE\",\"params\":{}}");
    // 1. idle for less than HTTP_IDLE_DROP_MS: the request waits, nothing is closed
    int stops = shimClientStops();
    size_t before = srv().responses.size();
    srv().shimIdleClient(millis());
    srv().shimQueue(HTTP_POST, "/api", &b);
    httpServerLoop();
    TEST_ASSERT_EQUAL_UINT32(before, srv().responses.size());
    TEST_ASSERT_TRUE(srv().shimClientWaiting());
    TEST_ASSERT_EQUAL_INT(stops, shimClientStops());
    // 2. idle for HTTP_IDLE_DROP_MS: closed, the waiting request is answered in the same step, logged once
    srv().shimIdleClient(millis() - HTTP_IDLE_DROP_MS);
    httpServerLoop();
    TEST_ASSERT_EQUAL_UINT32(before + 1, srv().responses.size());
    TEST_ASSERT_EQUAL_INT(200, srv().responses.back().code);
    TEST_ASSERT_EQUAL_INT(stops + 1, shimClientStops());
    char log[LOG_LINES * LOG_LINE_LEN + 1]; logDump(log, sizeof log);
    TEST_ASSERT_NOT_NULL(strstr(log, "http: idle connection closed after 500 ms (1 so far)"));
    // 3. a client that has sent data is never closed
    srv().shimIdleClient(millis() - 3000, true);
    httpServerLoop();
    TEST_ASSERT_EQUAL_INT(stops + 1, shimClientStops());
    // 4. a second drop within HTTP_IDLE_LOG_MS: counted, not logged again
    srv().shimIdleClient(millis() - HTTP_IDLE_DROP_MS - 1);
    httpServerLoop();
    TEST_ASSERT_EQUAL_INT(stops + 2, shimClientStops());
    logDump(log, sizeof log);
    TEST_ASSERT_NULL(strstr(log, "(2 so far)"));
}
void test_zz_shutdown() { g_run = false; g_ctrl.join(); TEST_PASS(); }

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_init_registers_routes_and_begins);
    RUN_TEST(test_static_files_and_404);
    RUN_TEST(test_v1522_web_assets);
    RUN_TEST(test_add_html_embeds_and_escapes);
    RUN_TEST(test_llms_txt_exact);
    RUN_TEST(test_log_endpoint);
    RUN_TEST(test_proc_endpoint);
    RUN_TEST(test_options_preflight);
    RUN_TEST(test_api_syntax_errors_are_400);
    RUN_TEST(test_api_unknown_cmd_200_no_heartbeat);
    RUN_TEST(test_api_bad_param_counts_as_heartbeat);
    RUN_TEST(test_api_get_state_local);
    RUN_TEST(test_api_get_config_local);
    RUN_TEST(test_api_commands_go_through_the_queue);
    RUN_TEST(test_v1516_slow_reply_within_timeout_is_ok);
    RUN_TEST(test_api_busy_when_control_task_does_not_answer);
    RUN_TEST(test_proc_while_control_task_publishes);
    RUN_TEST(test_v1517_add_html_sends_blocks_not_bytes);
    RUN_TEST(test_v1517_bad_param_is_an_error_also_for_get_state_and_get_config);
    RUN_TEST(test_v1525_idle_connection_is_closed_and_waiting_request_served);
    RUN_TEST(test_zz_shutdown);
    return UNITY_END();
}
