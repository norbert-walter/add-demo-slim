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
