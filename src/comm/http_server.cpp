/**
 * @file http_server.cpp
 * @brief HTTP endpoint handlers (FROZEN CONTRACT V1.5 §5, Sprint P2).
 *
 * Runs in the network task (Core 0). Task 2: all routes EXCEPT POST /api.
 */
#include <Arduino.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <string.h>
#include "config.h"
#include "logic/api_parse.h"
#include "logic/proc_json.h"
#include "logic/params.h"
#include "app/snapshot.h"
#include "app/command_queue.h"
#include "comm/log_buffer.h"
#include "comm/http_server.h"

static WebServer server(HTTP_PORT);
static char response[1024];  // used in task 3

// ---------------------------------------------------------------------------
// Static helpers (V1.5.16)
// ---------------------------------------------------------------------------

/// Send the Access-Control-Allow-Origin header on every response.
static void cors() {
    server.sendHeader("Access-Control-Allow-Origin", "*", true);
}

/// 404 answer: cors + {"error":"not found"}.
static void notFound() {
    cors();
    server.send(404, "application/json", "{\"error\":\"not found\"}");
}

/// Stream a SPIFFS file. If noStore is true, add Cache-Control: no-store header.
static void sendFile(const char* path, const char* type, bool noStore) {
    if (!SPIFFS.exists(path)) {
        notFound();
        return;
    }
    File f = SPIFFS.open(path, "r");
    if (noStore) server.sendHeader("Cache-Control", "no-store");
    cors();
    server.streamFile(f, type);
    f.close();
}

/// Read a file in blocks and escape "</" as "<\/".
static void sendScriptJson(File& f) {
    uint8_t buf[64];
    bool prev_lt = false;
    while (f.available()) {
        size_t n = f.read(buf, sizeof buf);
        for (size_t i = 0; i < n; ++i) {
            if (prev_lt && buf[i] == '/') {
                server.sendContent("<\\/");
                prev_lt = false;
            } else {
                // flush any pending '<' from previous block
                if (prev_lt) {
                    server.sendContent("<");
                    prev_lt = false;
                }
                if (buf[i] == '<') {
                    prev_lt = true;
                } else {
                    char c = (char)buf[i];
                    String s; s += c;
                    server.sendContent(s);
                }
            }
        }
    }
}

/// Read a file in blocks and HTML-escape & < >.
static void sendHtmlEscaped(File& f) {
    uint8_t buf[64];
    while (f.available()) {
        size_t n = f.read(buf, sizeof buf);
        for (size_t i = 0; i < n; ++i) {
            char c = (char)buf[i];
            if (c == '&')      server.sendContent("&amp;");
            else if (c == '<') server.sendContent("&lt;");
            else if (c == '>') server.sendContent("&gt;");
            else { String s; s += c; server.sendContent(s); }
        }
    }
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

static void handleAddHtml() {
    const char* type = "text/html; charset=utf-8";

    if (!SPIFFS.exists("/add.json")) {
        notFound();
        return;
    }

    cors();
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, type, "");

    // First open: script JSON with "</" -> "<\/"
    {
        File f = SPIFFS.open("/add.json", "r");
        server.sendContent("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>ADD</title>\n");
        server.sendContent("<script type=\"application/ld+json\">\n");
        sendScriptJson(f);
        f.close();
    }

    // Second open: HTML-escaped JSON in <pre>
    {
        File f = SPIFFS.open("/add.json", "r");
        server.sendContent("\n</script></head><body><pre>\n");
        sendHtmlEscaped(f);
        f.close();
    }

    server.sendContent("\n</pre></body></html>\n");
    server.sendContent("");  // finish chunked transfer
}

static void handleLlms() {
    cors();
    server.send(200, "text/plain",
        "# AI Device Description (ADD)\n"
        "- ADD (JSON): [Device Description](/add)\n"
        "- ADD (HTML): [Device Description for Cloud-AI](/add.html)\n"
        "- Documentation: [Device manual](/doc)\n");
}

static void handleLog() {
    static char buf[LOG_LINES * LOG_LINE_LEN + 1];
    logDump(buf, sizeof buf);
    cors();
    server.send(200, "text/plain", buf);
}

static void handleProc() {
    ProcData d;
    snapshotGet(d);
    char out[PROC_JSON_MAX];
    procToJson(d, out, sizeof out);
    cors();
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", out);
}

static void handleOptions() {
    cors();
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
}

// ---------------------------------------------------------------------------
// Task 3: POST /api handler (V1.5.16)
// ---------------------------------------------------------------------------

/// Build an API response: responseToJson → cors → send.
static void reply(int code, CmdId id, ApiError err, const char* extra) {
    responseToJson(id, err, nullptr, extra, response, sizeof(response));
    cors();
    server.send(code, "application/json", response);
}

/// POST /api handler (V1.5.16 block, step by step).
static void handleApi() {
    // 1. Read body: no body or length > API_BODY_MAX -> BAD_JSON (HTTP 400)
    if (!server.hasArg("plain") || server.arg("plain").length() > API_BODY_MAX) {
        reply(400, CmdId::NONE, ApiError::BAD_JSON, nullptr);
        return;
    }

    String bodyStr = server.arg("plain");
    const char* body = bodyStr.c_str();
    size_t length = bodyStr.length();

    // 2. Parse request
    Command cmd;
    ApiError parseErr = apiParseRequest(body, length, cmd);

    if (parseErr == ApiError::OK || parseErr == ApiError::BAD_PARAM) {
        // Valid packet: CRC ok, known cmd -> heartbeat
        snapshotSetLastApiRx(millis());
    }

    // 3. Handle based on command type
    if (parseErr != ApiError::OK && parseErr != ApiError::BAD_PARAM) {
        // Parse error: HTTP 400 for BAD_CRC/BAD_JSON, else 200
        int code = (parseErr == ApiError::BAD_CRC || parseErr == ApiError::BAD_JSON) ? 400 : 200;
        reply(code, cmd.id, parseErr, nullptr);
    } else if (cmd.id == CmdId::GET_STATE) {
        // Local response with stateExtraToJson
        ProcData d;
        snapshotGet(d);
        CycleStats cs;
        snapshotGetCycleStats(cs);
        static char extra[600];
        stateExtraToJson(d.state, d.err, d.local, cs, extra, sizeof(extra));
        reply(200, cmd.id, ApiError::OK, extra);
    } else if (cmd.id == CmdId::GET_CONFIG) {
        // Local response with paramsToJsonMember
        Params p;
        SafetyFlags f;
        snapshotGetConfig(p, f);
        static char extra[600];
        paramsToJsonMember(p, f, extra, sizeof(extra));
        reply(200, cmd.id, ApiError::OK, extra);
    } else if (parseErr == ApiError::BAD_PARAM) {
        // Bad params but valid packet -> respond locally, don't queue
        reply(200, cmd.id, parseErr, nullptr);
    } else {
        // OK -> queue and wait for reply
        ApiError err = cmdQueueSendAndWait(cmd, API_REPLY_TIMEOUT_MS);
        reply(200, cmd.id, err, nullptr);
    }
}

// ---------------------------------------------------------------------------
// Public API (V1.5.16)
// ---------------------------------------------------------------------------

void httpServerInit() {
    // Static file routes
    server.on("/", HTTP_GET, []() { sendFile("/www/index.html", "text/html", false); });
    server.on("/settings", HTTP_GET, []() { sendFile("/www/settings.html", "text/html", false); });
    server.on("/add", HTTP_GET, []() { sendFile("/add.json", "application/json", true); });
    server.on("/doc", HTTP_GET, []() { sendFile("/doc.md", "text/markdown", false); });

    // Dynamic handlers
    server.on("/add.html", HTTP_GET, handleAddHtml);
    server.on("/llms.txt", HTTP_GET, handleLlms);
    server.on("/log", HTTP_GET, handleLog);
    server.on("/proc", HTTP_GET, handleProc);

    // POST /api (task 3) — before OPTIONS route
    server.on("/api", HTTP_POST, handleApi);

    // OPTIONS /api (CORS preflight)
    server.on("/api", HTTP_OPTIONS, handleOptions);

    server.onNotFound(notFound);
    server.begin();
}

void httpServerLoop() {
    server.handleClient();
}