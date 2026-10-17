/**
 * @file http_server.h
 * @brief HTTP endpoints (FROZEN CONTRACT V1.5 §5, Sprint P2). Runs in the network task (Core 0).
 *
 * Library: WebServer (Arduino core). All handlers return quickly.
 *   GET  /            -> SPIFFS /www/index.html
 *   GET  /settings    -> SPIFFS /www/settings.html
 *   GET  /add         -> SPIFFS /add.json  (application/json, Cache-Control: no-store)
 *   GET  /add.html    -> /add.json embedded per ADD spec §5.3
 *   GET  /llms.txt    -> generated index (ADD spec §5.4)
 *   GET  /doc         -> SPIFFS /doc.md (text/markdown)
 *   V1.5.22 (web UI assets, Sprint W1):
 *   GET  /app.js      -> SPIFFS /www/app.js (application/javascript)
 *   GET  /style.css   -> SPIFFS /www/style.css (text/css)
 *   GET  /font-u400.woff2, /font-u700.woff2, /font-m400.woff2, /font-m700.woff2 -> SPIFFS /www/<same name>
 *        (font/woff2, Cache-Control: max-age=86400; SPIFFS names are limited to 31 characters)
 *   GET  /log         -> logDump() (text/plain)
 *   GET  /proc        -> snapshotGet() + procToJson() (Cache-Control: no-store)
 *   POST /api         -> body max API_BODY_MAX bytes, then:
 *        1. apiParseRequest(); error -> response with that error (HTTP 400 for BAD_CRC/BAD_JSON, else 200)
 *        2. valid packet (CRC ok, known cmd) -> snapshotSetLastApiRx(millis())
 *        3. GET_STATE  -> reply locally: extra = stateExtraToJson(snapshot.state, snapshot.err,
 *                          snapshot.local, CycleStats from snapshotGetCycleStats())   (V1.1)
 *           GET_CONFIG -> reply locally: extra = paramsToJsonMember(snapshotGetConfig())
 *           others     -> cmdQueueSendAndWait(cmd, API_REPLY_TIMEOUT_MS)
 *        4. responseToJson(); Content-Type application/json; HTTP 200
 *           (also for LOCAL_CONTROL = -10: the device is fine, the operator has priority)
 *   Every response: Access-Control-Allow-Origin: *  (dashboards on other hosts)
 *   Unknown path -> 404 {"error":"not found"}
 *
 * V1.5.16 – exact behaviour (tested natively with WebServer/SPIFFS shims: test/native/test_http_server):
 *  - One file-static WebServer server(HTTP_PORT). httpServerInit registers every route with
 *    server.on(path, method, handler) and server.onNotFound(), then server.begin().
 *    httpServerLoop() only calls server.handleClient().
 *  - Every response first sends the header Access-Control-Allow-Origin: * (server.sendHeader).
 *  - OPTIONS /api (CORS preflight of browsers on other hosts): 204, empty body, headers
 *    Access-Control-Allow-Methods: GET, POST, OPTIONS and Access-Control-Allow-Headers: Content-Type.
 *  - Files: if SPIFFS.exists(path): File f = SPIFFS.open(path, "r"); server.streamFile(f, type);
 *    f.close(). Missing file -> the 404 answer. Types: "/" and "/settings" text/html,
 *    "/add" application/json (+ Cache-Control: no-store), "/doc" text/markdown.
 *  - /add.html (from /add.json; missing -> 404): HTTP 200, Content-Type
 *    "text/html; charset=utf-8", headers Cache-Control: no-store, no-cache, must-revalidate and
 *    Pragma: no-cache. Sent in chunks (server.setContentLength(CONTENT_LENGTH_UNKNOWN);
 *    server.send(200, type, ""); server.sendContent(...) ...; server.sendContent("") to finish):
 *      <!DOCTYPE html><html><head><meta charset="utf-8"><title>ADD</title>
 *      <script type="application/ld+json">
 *      JSON, with every "</" written as "<\/"
 *      </script></head><body><pre>
 *      JSON, with & < > written as &amp; &lt; &gt;
 *      </pre></body></html>
 *    (each line above ends with '\n'; the JSON is copied byte for byte otherwise.)
 *    V1.5.17: the JSON is read in blocks (e.g. 256 bytes) and sent block-wise: at most 2
 *    sendContent calls per started 64 bytes of JSON (one sendContent = one HTTP chunk = one TCP
 *    write on the device; never one call per character). A '<' held back at the end of a block
 *    belongs to the next block; a '<' as the very last byte of the file is sent as well.
 *  - /llms.txt: text/plain, exactly these 4 lines, each ending with '\n':
 *      # AI Device Description (ADD)
 *      - ADD (JSON): [Device Description](/add)
 *      - ADD (HTML): [Device Description for Cloud-AI](/add.html)
 *      - Documentation: [Device manual](/doc)
 *  - /log: text/plain, logDump() into a static buffer of LOG_LINES * LOG_LINE_LEN + 1 bytes.
 *  - /proc: snapshotGet + procToJson into char[PROC_JSON_MAX]; application/json, Cache-Control: no-store.
 *  - POST /api: body = server.arg("plain"). No body or length > API_BODY_MAX -> BAD_JSON.
 *    Otherwise apiParseRequest(body, length, cmd). Packet is "valid" if the result is OK or
 *    BAD_PARAM (CRC ok, known cmd) -> snapshotSetLastApiRx(millis()). Every error (V1.5.17: also BAD_PARAM
 *    for GET_STATE / GET_CONFIG) -> responseToJson
 *    (cmd.id, err, nullptr, nullptr, ...) with HTTP 400 for BAD_CRC/BAD_JSON, else 200.
 *    OK: GET_STATE / GET_CONFIG answered locally (extra as listed above, ProcData from
 *    snapshotGet), all other commands: err = cmdQueueSendAndWait(cmd, API_REPLY_TIMEOUT_MS),
 *    response responseToJson(cmd.id, err, nullptr, nullptr, ...), HTTP 200.
 *    Response buffer: static char[1024]; Content-Type application/json.
 *  - 404 answer: HTTP 404, application/json, body {"error":"not found"}.
 */
#pragma once

void httpServerInit();   ///< mounts nothing; SPIFFS is mounted in setup()
void httpServerLoop();   ///< server.handleClient()
