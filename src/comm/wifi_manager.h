/**
 * @file wifi_manager.h
 * @brief WiFi: access point always on + optional router connection (FROZEN CONTRACT V1.5 §12.1, Sprint P2).
 *
 * Mode WIFI_AP_STA from the start:
 *  - AP "ADD-Demo" / WIFI_AP_PASS, 192.168.4.1 – ALWAYS active, so the tablet can
 *    reach the device directly even without a router.
 *  - Station: if params.wifi_ssid is not empty the device logs into that network
 *    (DHCP) and is reachable there too (and can use the internet via the router).
 *    Connection management is the pure state machine in logic/wifi_logic.h
 *    (staStep every 100 ms): 3 attempts x 10 s, then 2 min pause, forever;
 *    a lost connection starts the attempts again.
 *    Note: while the station is searching, the ESP32 radio may switch channels and
 *    AP clients can notice short interruptions – the 2 min pause keeps this rare.
 *  - New wifi_ssid / wifi_pass (SET) take effect after REBOOT; after every boot
 *    the login happens automatically.
 *  - mDNS "add-demo" (http://add-demo.local) on both interfaces.
 * wifiLoop() is called from the network task and never blocks > 50 ms.
 * Every state change is written to the log (log_buffer), e.g.
 * "sta: connected 192.168.1.57" / "sta: attempt 2/3 failed" / "sta: backoff 120 s".
 *
 * V1.5.16 – exact behaviour (tested natively with WiFi/MDNS shims: test/native/test_wifi_manager):
 *  wifiInit: copies ssid and pass into file-static buffers (sizes as Params::wifi_ssid /
 *    wifi_pass, truncated), WiFi.mode(WIFI_AP_STA), WiFi.setAutoReconnect(false) (the state
 *    machine reconnects, not the core), WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS),
 *    MDNS.begin("add-demo"), MDNS.addService("http", "tcp", HTTP_PORT),
 *    logLine("ap: %s %s", WIFI_AP_SSID, <AP IP>). StaConfig = defaults (3 x 10 s, 120 s).
 *  wifiLoop(now): the FIRST call steps immediately, afterwards only if (now - last step) >= 100
 *    (uint32 wrap-around safe); otherwise it returns at once. One step:
 *      link_up = (WiFi.status() == WL_CONNECTED);
 *      action = staStep(status, ssid[0] != 0, link_up, now, cfg);
 *      BEGIN -> WiFi.begin(ssid, pass); DISCONNECT -> WiFi.disconnect(false) (AP stays on).
 *    Log lines after the step, exactly (only when something changed):
 *      CONNECTED -> CONNECTING (link lost):      "sta: connection lost"
 *      CONNECTING with a new attempt number n:   "sta: attempt %u/%u"  (n, cfg.attempts)
 *      entering CONNECTED:                       "sta: connected %s"   (WiFi.localIP())
 *      entering BACKOFF:                         "sta: backoff %lu s"  (cfg.backoff_ms / 1000)
 *      entering OFF from another state:          "sta: off"
 *    The StaStatus is written under a file-static portMUX (taskENTER/EXIT_CRITICAL);
 *    wifiStaStatus() copies it under the same lock (the display task reads it on Core 1).
 *    V1.5.17: the critical sections contain ONLY the copy of the StaStatus (read before staStep,
 *    write after it). WiFi.status() is read BEFORE the first critical section, staStep runs on
 *    a local copy outside it, and no WiFi.* call or logLine happens inside a critical section
 *    (on the ESP32 they use FreeRTOS/lwIP and must not run with interrupts masked).
 *  wifiGetApIp: WiFi.softAPIP().toString() with snprintf into buf (cap > 0, always terminated).
 *  wifiGetStaIp: WiFi.localIP().toString() if the station state is CONNECTED, else "".
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "logic/wifi_logic.h"

void wifiInit(const char* ssid, const char* pass);
void wifiLoop(uint32_t now_ms);
void wifiGetApIp(char* buf, size_t cap);    ///< "192.168.4.1"
void wifiGetStaIp(char* buf, size_t cap);   ///< "" while not connected
StaStatus wifiStaStatus();                  ///< copy of the station state (for display / log)
