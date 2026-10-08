/**
 * @file http_server.h
 * @brief HTTP endpoints (FROZEN CONTRACT V1.1 §5, Sprint S3). Runs in the network task (Core 0).
 *
 * Library: WebServer (Arduino core). All handlers return quickly.
 *   GET  /            -> SPIFFS /www/index.html
 *   GET  /settings    -> SPIFFS /www/settings.html
 *   GET  /add         -> SPIFFS /add.json  (application/json, Cache-Control: no-store)
 *   GET  /add.html    -> /add.json embedded per ADD spec §5.3
 *   GET  /llms.txt    -> generated index (ADD spec §5.4)
 *   GET  /doc         -> SPIFFS /doc.md (text/markdown)
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
 */
#pragma once

void httpServerInit();   ///< mounts nothing; SPIFFS is mounted in setup()
void httpServerLoop();   ///< server.handleClient()
