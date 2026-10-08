/**
 * @file wifi_manager.h
 * @brief WiFi STA with AP fallback (Sprint S1).
 *
 * wifiInit(): if ssid is empty -> AP mode (WIFI_AP_SSID / WIFI_AP_PASS, 192.168.4.1).
 *   Else STA connect with WIFI_CONNECT_TIMEOUT_MS; on failure -> AP mode.
 * wifiLoop(): called from the network task. In STA mode check every 10 s,
 *   reconnect; after 3 failed reconnects switch to AP mode. Never blocks > 50 ms.
 * mDNS name "add-demo" (http://add-demo.local) in both modes.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

void wifiInit(const char* ssid, const char* pass);
void wifiLoop(uint32_t now_ms);
bool wifiIsUp();
void wifiGetIp(char* buf, size_t cap);   ///< "192.168.4.1"
bool wifiIsAp();
