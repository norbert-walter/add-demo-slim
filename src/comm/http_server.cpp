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

/// V1.5.22: a font file, cacheable for one day (Cache-Control: max-age=86400).
static void sendFont(const char* path) {
    if (!SPIFFS.exists(path)) {
        notFound();
        return;
    }
    File f = SPIFFS.open(path, "r");
    server.sendHeader("Cache-Control", "max-age=86400");
    cors();
    server.streamFile(f, "font/woff2");
    f.close();
}

/// Read a file in blocks and escape "</" as "<\/". Block-wise sendContent (V1.5.17).
static void sendScriptJson(File& f) {
    uint8_t buf[64];
    char out[2048];  // plenty for worst case: every byte → 3 chars ('</' → '<\/')
    int oi = 0;        // output index
    bool pending_lt = false;

    auto flush_out = [&]() {
        if (oi > 0) { server.sendContent(out, oi); oi = 0; }
    };

    while (f.available()) {
        size_t n = f.read(buf, sizeof buf);
        for (size_t i = 0; i < n; ++i) {
            if (pending_lt && buf[i] == '/') {
                // '</' across block boundary → '<\/'
                out[oi++] = '<'; out[oi++] = '\\'; out[oi++] = '/';
                pending_lt = false;
            } else {
                // flush any pending '<' from previous block
                if (pending_lt) {
                    out[oi++] = '<';
                    pending_lt = false;
                }
                if (buf[i] == '<') {
                    pending_lt = true;
                } else {
                    out[oi++] = (char)buf[i];
                }
            }
        }
        flush_out();
    }
    // A '<' as the very last byte of the file is sent too
    if (pending_lt) server.sendContent("<");
}

/// Read a file in blocks and HTML-escape & < >. Block-wise sendContent (V1.5.17).
static void sendHtmlEscaped(File& f) {
    uint8_t buf[64];
    char out[2048];  // worst case: '&' → '&amp;' = 5 chars per input byte
    int oi = 0;

    auto flush_out = [&]() {
        if (oi > 0) { server.sendContent(out, oi); oi = 0; }
    };

    while (f.available()) {
        size_t n = f.read(buf, sizeof buf);
        for (size_t i = 0; i < n; ++i) {
            char c = (char)buf[i];
            if      (c == '&') { out[oi++] = '&'; out[oi++] = 'a'; out[oi++] = 'm'; out[oi++] = 'p'; out[oi++] = ';'; }
            else if (c == '<') { out[oi++] = '&'; out[oi++] = 'l'; out[oi++] = 't'; out[oi++] = ';'; }
            else if (c == '>') { out[oi++] = '&'; out[oi++] = 'g'; out[oi++] = 't'; out[oi++] = ';'; }
            else               { out[oi++] = c; }
        }
        flush_out();
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
    // Static file routes
    server.on("/", HTTP_GET, []() { sendFile("/www/index.html", "text/html", false); });
    server.on("/settings", HTTP_GET, []() { sendFile("/www/settings.html", "text/html", false); });
    server.on("/add", HTTP_GET, []() { sendFile("/add.json", "application/json", true); });
    server.on("/doc", HTTP_GET, []() { sendFile("/doc.md", "text/markdown", false); });
    // V1.5.22: web UI assets (Sprint W1); fonts may be cached by the browser
    server.on("/app.js", HTTP_GET, []() { sendFile("/www/app.js", "application/javascript", false); });
    server.on("/style.css", HTTP_GET, []() { sendFile("/www/style.css", "text/css", false); });
    server.on("/font-u400.woff2", HTTP_GET, []() { sendFont("/www/font-u400.woff2"); });
    server.on("/font-u700.woff2", HTTP_GET, []() { sendFont("/www/font-u700.woff2"); });
    server.on("/font-m400.woff2", HTTP_GET, []() { sendFont("/www/font-m400.woff2"); });
    server.on("/font-m700.woff2", HTTP_GET, []() { sendFont("/www/font-m700.woff2"); });

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