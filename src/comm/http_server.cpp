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

/// V1.5.25: the Arduino WebServer serves one connection at a time and waits up to 5 s
/// (HTTP_MAX_DATA_WAIT, fixed in the core) for a connected client to send its request. Browsers
/// open spare connections in advance and send nothing on them, so every other client - the
/// heartbeat too - was blocked for up to 5 s each and the comm watchdog fired. A connection that
/// has sent nothing after HTTP_IDLE_DROP_MS is closed here; the browser simply opens a new one.
class AddWebServer : public WebServer {
public:
    explicit AddWebServer(int port) : WebServer(port) {}
    /// true if the current client was closed because it sent nothing for idle_ms.
    bool dropIdleClient(uint32_t now_ms, uint32_t idle_ms) {
        if (_currentStatus != HC_WAIT_READ || _currentClient.available()) return false;
        if ((uint32_t)(now_ms - (uint32_t)_statusChange) < idle_ms) return false;
        _currentClient.stop();
        _currentClient = NetworkClient();
        _currentStatus = HC_NONE;
        return true;
    }
};

static AddWebServer server(HTTP_PORT);
static uint32_t s_idle_drops = 0;        // V1.5.25: closed idle connections since boot
static uint32_t s_idle_logged = 0;       // value of s_idle_drops in the last log line
static uint32_t s_idle_log_ms = 0;
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

/// V1.6.5: file route helper (gzip copy when accepted).
static void sendFile(const char* path, const char* type, const char* cache, bool pragma = false) {
    // 1. Determine whether the client accepts gzip and a pre-compressed copy exists.
    bool useGz = server.header("Accept-Encoding").indexOf("gzip") >= 0 && SPIFFS.exists(String(path) + ".gz");

    // 2. If no gzip and the plain file is missing -> 404 (no further headers).
    if (!useGz && !SPIFFS.exists(path)) {
        notFound();
        return;
    }

    // 3. Open the file (gzip copy or plain), send headers, stream, close.
    File f = useGz ? SPIFFS.open(String(path) + ".gz", "r") : SPIFFS.open(path, "r");
    if (cache) server.sendHeader("Cache-Control", cache);
    if (pragma) server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Vary", "Accept-Encoding");
    cors();
    server.streamFile(f, type);
    f.close();
}

/// V1.6.5: cached font / favicon helper (no gzip lookup).
static void sendCached(const char* path, const char* type) {
    if (!SPIFFS.exists(path)) {
        notFound();
        return;
    }
    File f = SPIFFS.open(path, "r");
    server.sendHeader("Cache-Control", "max-age=86400");
    cors();
    server.streamFile(f, type);
    f.close();
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

static void handleAddHtml() {
    sendFile("/add.html", "text/html; charset=utf-8", "no-store, no-cache, must-revalidate", true);
}

static void handleLlms() {
    cors();
    server.send(200, "text/plain",
        "# AI Device Description (ADD)\n"
        "- ADD (JSON): [Device Description](/add)\n"
        "- ADD (HTML): [Device Description for Cloud-AI](/add.html)\n"
        "- OpenAPI (JSON): [HTTP API, technical reference](/openapi.json)\n"
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

    // 3. Handle based on command type — BAD_PARAM before GET_STATE/GET_CONFIG (V1.5.17)
    if (parseErr != ApiError::OK && parseErr != ApiError::BAD_PARAM) {
        // Parse error: HTTP 400 for BAD_CRC/BAD_JSON, else 200
        int code = (parseErr == ApiError::BAD_CRC || parseErr == ApiError::BAD_JSON) ? 400 : 200;
        reply(code, cmd.id, parseErr, nullptr);
    } else if (parseErr == ApiError::BAD_PARAM) {
        // Bad params -> respond with error (also for GET_STATE / GET_CONFIG, V1.5.17)
        reply(200, cmd.id, parseErr, nullptr);
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
    // V1.6.5: collect Accept-Encoding before routes so sendFile can check it.
    const char* accept_keys[] = {"Accept-Encoding"};
    server.collectHeaders(accept_keys, 1);

    // Static file routes
    server.on("/", HTTP_GET, []() { sendFile("/www/index.html", "text/html", nullptr); });
    server.on("/settings", HTTP_GET, []() { sendFile("/www/settings.html", "text/html", nullptr); });
    server.on("/add", HTTP_GET, []() { sendFile("/add.json", "application/json", "no-store"); });
    server.on("/openapi.json", HTTP_GET, []() { sendFile("/openapi.json", "application/json", "no-store"); });
    server.on("/doc", HTTP_GET, []() { sendFile("/doc.html", "text/html", nullptr); });   // V1.5.27: HTML
    // V1.5.22: web UI assets (Sprint W1)
    server.on("/app.js", HTTP_GET, []() { sendFile("/www/app.js", "application/javascript", nullptr); });
    server.on("/style.css", HTTP_GET, []() { sendFile("/www/style.css", "text/css", nullptr); });
    // V1.6.5: fonts and favicon (sendCached — no gzip lookup)
    server.on("/font-u400.woff2", HTTP_GET, []() { sendCached("/www/font-u400.woff2", "font/woff2"); });
    server.on("/font-u700.woff2", HTTP_GET, []() { sendCached("/www/font-u700.woff2", "font/woff2"); });
    server.on("/font-m400.woff2", HTTP_GET, []() { sendCached("/www/font-m400.woff2", "font/woff2"); });
    server.on("/font-m700.woff2", HTTP_GET, []() { sendCached("/www/font-m700.woff2", "font/woff2"); });
    server.on("/favicon.ico", HTTP_GET, []() { sendCached("/www/favicon.ico", "image/x-icon"); });

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
    // V1.5.25: close an idle connection first, so that a waiting request is served in this step
    const uint32_t now = millis();
    if (server.dropIdleClient(now, HTTP_IDLE_DROP_MS)) ++s_idle_drops;
    if (s_idle_drops != s_idle_logged && (s_idle_logged == 0 || (uint32_t)(now - s_idle_log_ms) >= HTTP_IDLE_LOG_MS)) {
        logLine("http: idle connection closed after %u ms (%lu so far)", (unsigned)HTTP_IDLE_DROP_MS,
                (unsigned long)s_idle_drops);
        s_idle_logged = s_idle_drops;
        s_idle_log_ms = now;
    }
    server.handleClient();
}