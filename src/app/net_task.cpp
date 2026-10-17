/**
 * @file net_task.cpp
 * @brief Network task implementation (V1.5.20, Sprint I1).
 */
#include <Arduino.h>
#include "config.h"
#include "logic/params.h"
#include "app/snapshot.h"
#include "comm/wifi_manager.h"
#include "comm/http_server.h"
#include "comm/log_buffer.h"
#include "system/nvs_config.h"
#include "system/system.h"
#include "app/net_task.h"
#include "app/command_queue.h"   // V1.5.23: serial console
#include "logic/types.h"
#include "logic/wifi_logic.h"
#include <stdio.h>
#include <string.h>

static bool reboot_pending = false;
static uint32_t reboot_at = 0;

// ---- V1.5.23: serial console ------------------------------------------------------------------
static char   s_line[CONSOLE_LINE_MAX + 1];
static size_t s_len = 0;
static bool   s_overflow = false;

static void consoleReply(const char* json) {
    Serial.print(json);
    Serial.print("\n");
}

/// JSON string content: '"' and '\\' escaped, control characters as '?'.
static void jsonCopy(char* out, size_t cap, const char* in) {
    size_t o = 0;
    for (; *in && o + 2 < cap; ++in) {
        char c = *in;
        if (c == '"' || c == '\\') { out[o++] = '\\'; out[o++] = c; }
        else out[o++] = ((unsigned char)c < 0x20) ? '?' : c;
    }
    out[o] = 0;
}

static ApiError consoleSend(CmdId id, const char* key, const char* str) {
    Command c;
    c.id = id;
    c.source = CmdSource::API;
    c.want_reply = true;
    if (key) {
        snprintf(c.key, sizeof c.key, "%s", key);
        c.vtype = ValueType::STRING;
        snprintf(c.str, sizeof c.str, "%s", str);
    }
    return cmdQueueSendAndWait(c, API_REPLY_TIMEOUT_MS);
}

static void errorReply(ApiError e, const char* key) {
    char b[160];
    if (key) snprintf(b, sizeof b, "{\"error\":\"%s\",\"err\":%d,\"key\":\"%s\"}", apiErrorMessage(e), (int)e, key);
    else snprintf(b, sizeof b, "{\"error\":\"%s\",\"err\":%d}", apiErrorMessage(e), (int)e);
    consoleReply(b);
}

/// SET ssid, SET pass, REBOOT and the answer (ssid == nullptr: "wifi off").
static void wifiCommand(const char* ssid, const char* pass) {
    ApiError e = consoleSend(CmdId::SET, "wifi_ssid", ssid ? ssid : "");
    if (e != ApiError::OK) { errorReply(e, "wifi_ssid"); return; }
    e = consoleSend(CmdId::SET, "wifi_pass", ssid ? pass : "");
    if (e != ApiError::OK) { errorReply(e, "wifi_pass"); return; }
    ApiError r = consoleSend(CmdId::REBOOT, nullptr, nullptr);
    char b[200], head[120];
    if (ssid) { char js[80]; jsonCopy(js, sizeof js, ssid); snprintf(head, sizeof head, "{\"wifi\":\"saved\",\"ssid\":\"%s\"", js); }
    else snprintf(head, sizeof head, "{\"wifi\":\"off\"");
    if (r == ApiError::OK) snprintf(b, sizeof b, "%s,\"reboot\":true}", head);
    else snprintf(b, sizeof b, "%s,\"reboot\":false,\"err\":%d}", head, (int)r);
    consoleReply(b);
}

static void consoleExecute(const char* line) {
    while (*line == ' ') ++line;
    if (!*line) return;
    if (strcmp(line, "help") == 0) {
        consoleReply("{\"help\":\"help | ver | status | reboot | wifi <ssid> <password> | wifi \\\"<ssid>\\\" <password> | wifi off\"}");
    } else if (strcmp(line, "ver") == 0) {
        consoleReply("{\"fw\":\"" FW_VERSION "\"}");
    } else if (strcmp(line, "status") == 0) {
        ProcData d; snapshotGet(d);
        Params p; SafetyFlags f; snapshotGetConfig(p, f);
        StaStatus st = wifiStaStatus();
        char ap[32], sta[32], ssid[80], b[320];
        wifiGetApIp(ap, sizeof ap);
        wifiGetStaIp(sta, sizeof sta);
        jsonCopy(ssid, sizeof ssid, p.wifi_ssid);
        snprintf(b, sizeof b, "{\"state\":\"%s\",\"err\":%d,\"local\":%s,\"ap\":\"%s\",\"sta\":\"%s\",\"sta_ip\":\"%s\",\"ssid\":\"%s\",\"pass_set\":%s}",
                 safetyStateName(d.state), (int)d.err, d.local ? "true" : "false", ap, staStateName(st.state), sta, ssid,
                 p.wifi_pass[0] ? "true" : "false");
        consoleReply(b);
    } else if (strcmp(line, "reboot") == 0) {
        ApiError e = consoleSend(CmdId::REBOOT, nullptr, nullptr);
        if (e == ApiError::OK) consoleReply("{\"reboot\":true}"); else errorReply(e, nullptr);
    } else if (strcmp(line, "wifi off") == 0) {
        wifiCommand(nullptr, nullptr);
    } else if (strcmp(line, "wifi") == 0) {
        consoleReply("{\"error\":\"usage: wifi <ssid> <password> | wifi off\"}");
    } else if (strncmp(line, "wifi ", 5) == 0) {
        const char* a = line + 5;
        char ssid[CONSOLE_LINE_MAX + 1];
        const char* pass = nullptr;
        if (*a == '"') {
            const char* q = strchr(a + 1, '"');
            if (q && q[1] == ' ') {
                size_t n = (size_t)(q - a - 1);
                memcpy(ssid, a + 1, n); ssid[n] = 0;
                pass = q + 2;
            }
        } else {
            const char* sp = strchr(a, ' ');
            if (sp) {
                size_t n = (size_t)(sp - a);
                memcpy(ssid, a, n); ssid[n] = 0;
                pass = sp + 1;
            }
        }
        if (!pass || !*pass) { consoleReply("{\"error\":\"usage: wifi <ssid> <password> | wifi off\"}"); return; }
        wifiCommand(ssid, pass);
    } else {
        consoleReply("{\"error\":\"unknown command, try help\"}");
    }
}

/// Step 0 of netTaskStep: read up to CONSOLE_READ_MAX bytes, execute at most one complete line.
static void consoleStep() {
    for (int i = 0; i < CONSOLE_READ_MAX && Serial.available() > 0; ++i) {
        int c = Serial.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n') {
            bool over = s_overflow;
            s_line[s_len] = 0;
            s_len = 0;
            s_overflow = false;
            if (over) consoleReply("{\"error\":\"line too long\"}");
            else consoleExecute(s_line);
            return;                                   // at most one command per step
        }
        if (s_len < CONSOLE_LINE_MAX) s_line[s_len++] = (char)c;
        else s_overflow = true;
    }
}

// ---- V1.5.24: comm diagnostics -------------------------------------------------------------------
static bool     s_rx_seen = false;
static uint32_t s_prev_rx = 0;
static bool     s_prev_armed = false;
static uint32_t s_max_gap = 0;
static bool     s_timeout_logged = false;

static void commDiagnostics(uint32_t now_ms) {
    ProcData d;
    snapshotGet(d);
    uint32_t rx = snapshotLastApiRx();
    bool armed = d.safety.comm_armed;
    if (!armed) s_prev_armed = false;               // a gap counts only if armed the whole time
    if (!s_rx_seen || rx != s_prev_rx) {
        if (s_rx_seen && s_prev_armed && armed) {
            uint32_t gap = rx - s_prev_rx;
            if (gap > s_max_gap) s_max_gap = gap;
            if (gap > COMM_GAP_LOG_MS) logLine("comm: /api gap %u ms", (unsigned)gap);
        }
        s_prev_rx = rx;
        s_prev_armed = armed;
        s_rx_seen = true;
    }
    bool timeout = d.err == ErrorCode::COMM_TIMEOUT;
    if (timeout && !s_timeout_logged) {
        logLine("comm: timeout, last /api packet %u ms ago, longest gap while armed %u ms",
                (unsigned)(now_ms - rx), (unsigned)s_max_gap);
    }
    s_timeout_logged = timeout;
    if (!armed && !timeout) s_max_gap = 0;
}

static void netTask(void* parameter) {
    for (;;) {
        netTaskStep(millis());
        vTaskDelay(pdMS_TO_TICKS(NET_TASK_PERIOD_MS));
    }
}

bool netTaskStart() {
    return xTaskCreatePinnedToCore(netTask, "net", NET_TASK_STACK, nullptr,
                                   NET_TASK_PRIO, nullptr, NET_TASK_CORE) == pdPASS;
}

void netTaskStep(uint32_t now_ms) {
    // 0. serial console (V1.5.23)
    consoleStep();

    // 1. wifiLoop(now)
    wifiLoop(now_ms);

    // 2. httpServerLoop()
    httpServerLoop();

    // 2a. comm diagnostics (V1.5.24)
    commDiagnostics(now_ms);

    // 3. Persist request: save config copy to NVS
    if (snapshotTakePersistRequest()) {
        Params p;
        SafetyFlags f;
        snapshotGetConfig(p, f);
        if (nvsSaveParams(p)) {
            logLine("nvs: saved");
        } else {
            logLine("nvs: save FAILED");
        }
    }

    // 4. Reboot request scheduling (only if not already pending)
    if (snapshotTakeRebootRequest() && !reboot_pending) {
        reboot_pending = true;
        reboot_at = now_ms + REBOOT_DELAY_MS;
        logLine("reboot: in %u ms", (unsigned)REBOOT_DELAY_MS);
    }

    // 5. Reboot execution (wrap-around safe signed comparison)
    if (reboot_pending && (int32_t)(now_ms - reboot_at) >= 0) {
        reboot_pending = false;
        systemRestart();
    }
}