/**
 * @file wifi_manager.cpp
 * @brief WiFi manager: AP always on + optional STA router connection (Sprint P2).
 *
 * See wifi_manager.h for the full specification (V1.5.16 block there is THE contract).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "logic/params.h"
#include "comm/log_buffer.h"
#include "comm/wifi_manager.h"

// ---------------------------------------------------------------------------
// File-static state
// ---------------------------------------------------------------------------
static char ssid_buf[sizeof(Params::wifi_ssid)];
static char pass_buf[sizeof(Params::wifi_pass)];

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static StaStatus status;
static StaConfig cfg;
static bool stepped_once = false;
static uint32_t last_step_time = 0;

// ---------------------------------------------------------------------------
// wifiInit – V1.5.16: copy creds, AP+STA mode, mDNS, log AP IP
// ---------------------------------------------------------------------------
void wifiInit(const char* ssid, const char* pass) {
    // Copy ssid and pass into file-static buffers (sizes as Params::wifi_ssid / wifi_pass, truncated)
    strncpy(ssid_buf, ssid, sizeof(ssid_buf) - 1);
    ssid_buf[sizeof(ssid_buf) - 1] = '\0';

    strncpy(pass_buf, pass, sizeof(pass_buf) - 1);
    pass_buf[sizeof(pass_buf) - 1] = '\0';

    // WiFi.mode(WIFI_AP_STA)
    WiFi.mode(WIFI_AP_STA);

    // V1.5.24: modem sleep off – with sleep the ESP answers in 75 ... 140 ms and single requests
    // took over 3 s (lost beacons, TCP retransmissions), which tripped the 5 s comm watchdog
    WiFi.setSleep(false);

    // V1.5.25: do not let the WiFi driver write the station config to flash on every WiFi.begin()
    // (the credentials live in our own NVS keys). A flash write stops the other core for
    // milliseconds -> control task overruns on every connection attempt.
    WiFi.persistent(false);

    // WiFi.setAutoReconnect(false) (the state machine reconnects, not the core)
    WiFi.setAutoReconnect(false);

    // WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS)
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

    // MDNS.begin("add-demo")
    MDNS.begin("add-demo");

    // MDNS.addService("http", "tcp", HTTP_PORT)
    MDNS.addService("http", "tcp", HTTP_PORT);

    // logLine("ap: %s %s", WIFI_AP_SSID, <AP IP>)
    char ap_ip_str[16];
    snprintf(ap_ip_str, sizeof(ap_ip_str), "%s", WiFi.softAPIP().toString().c_str());
    logLine("ap: %s %s", WIFI_AP_SSID, ap_ip_str);

    // StaConfig = defaults (3 x 10 s, 120 s)
    cfg = StaConfig{};
}

// ---------------------------------------------------------------------------
// wifiLoop – V1.5.16: step every 100 ms, execute action, log changes
// ---------------------------------------------------------------------------
void wifiLoop(uint32_t now_ms) {
    // FIRST call steps immediately, afterwards only if (now - last_step) >= 100
    // (uint32 wrap-around safe); otherwise it returns at once.
    if (stepped_once) {
        if ((now_ms - last_step_time) < 100u) {
            return;
        }
    }

    // V1.5.17: read link_up BEFORE any critical section
    bool link_up = (WiFi.status() == WL_CONNECTED);

    taskENTER_CRITICAL(&mux);

    StaStatus local_s = status;       // copy of current status
    StaStatus prev_s = status;        // second copy "prev" for change detection

    // action = staStep(status, ssid[0] != 0, link_up, now, cfg)
    StaAction action = staStep(local_s, ssid_buf[0] != '\0', link_up, now_ms, cfg);

    // Write the new status back under the same lock
    status = local_s;

    taskEXIT_CRITICAL(&mux);

    // Execute action OUTSIDE critical section
    // BEGIN -> WiFi.begin(ssid, pass); DISCONNECT -> WiFi.disconnect(false) (AP stays on).
    if (action == StaAction::BEGIN) {
        WiFi.begin(ssid_buf, pass_buf);
    } else if (action == StaAction::DISCONNECT) {
        WiFi.disconnect(false);
    }

    // Log lines after the step, exactly (only when something changed):
    if (prev_s.state != local_s.state) {
        // CONNECTED -> CONNECTING (link lost):      "sta: connection lost"
        if (prev_s.state == StaState::CONNECTED && local_s.state == StaState::CONNECTING) {
            logLine("sta: connection lost");
        }
        // entering CONNECTED:                       "sta: connected %s"   (WiFi.localIP())
        else if (local_s.state == StaState::CONNECTED) {
            char ip[16];
            snprintf(ip, sizeof(ip), "%s", WiFi.localIP().toString().c_str());
            logLine("sta: connected %s", ip);
        }
        // entering BACKOFF:                         "sta: backoff %lu s"  (cfg.backoff_ms / 1000)
        else if (local_s.state == StaState::BACKOFF) {
            logLine("sta: backoff %lu s", (unsigned long)(cfg.backoff_ms / 1000));
        }
        // entering OFF from another state:          "sta: off"
        else if (local_s.state == StaState::OFF && prev_s.state != StaState::OFF) {
            logLine("sta: off");
        }
    }
    // CONNECTING with a new attempt number n (state may not change):   "sta: attempt %u/%u"  (n, cfg.attempts)
    if (local_s.state == StaState::CONNECTING && (prev_s.state != StaState::CONNECTING || local_s.attempt > prev_s.attempt)) {
        logLine("sta: attempt %u/%u", local_s.attempt, cfg.attempts);
    }

    last_step_time = now_ms;
    stepped_once = true;
}

// ---------------------------------------------------------------------------
// wifiGetApIp – V1.5.16: WiFi.softAPIP().toString() with snprintf into buf
// ---------------------------------------------------------------------------
void wifiGetApIp(char* buf, size_t cap) {
    if (cap > 0 && buf != nullptr) {
        snprintf(buf, cap, "%s", WiFi.softAPIP().toString().c_str());
    }
}

// ---------------------------------------------------------------------------
// wifiGetStaIp – V1.5.16: localIP().toString() if CONNECTED, else ""
// ---------------------------------------------------------------------------
void wifiGetStaIp(char* buf, size_t cap) {
    if (cap > 0 && buf != nullptr) {
        taskENTER_CRITICAL(&mux);
        StaStatus s = status;
        taskEXIT_CRITICAL(&mux);

        if (s.state == StaState::CONNECTED) {
            snprintf(buf, cap, "%s", WiFi.localIP().toString().c_str());
        } else {
            buf[0] = '\0';
        }
    }
}

// ---------------------------------------------------------------------------
// wifiStaStatus – V1.5.16: copy of the station state (for display / log)
// ---------------------------------------------------------------------------
StaStatus wifiStaStatus() {
    taskENTER_CRITICAL(&mux);
    StaStatus s = status;
    taskEXIT_CRITICAL(&mux);
    return s;
}
