#pragma once
// V1.5.16 host shim of the Arduino-ESP32 WiFi class (signatures of core 3.x for the calls the
// project may use). Records calls; the test sets status() and the IPs.
#include "Arduino.h"
#include "IPAddress.h"
#include <string>
#include <vector>
typedef enum { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 } wifi_mode_t;
typedef enum { WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL = 1, WL_SCAN_COMPLETED = 2, WL_CONNECTED = 3,
               WL_CONNECT_FAILED = 4, WL_CONNECTION_LOST = 5, WL_DISCONNECTED = 6 } wl_status_t;
struct ShimWiFi {
    std::mutex m;
    wifi_mode_t mode = WIFI_OFF;
    std::string ap_ssid, ap_pass, sta_ssid, sta_pass;
    int begins = 0, disconnects = 0, disconnect_wifioff = 0, soft_aps = 0;
    bool auto_reconnect = true;
    bool sleep = true;                    // V1.5.24: modem sleep (Arduino default on)
    std::vector<std::string> calls;       // V1.5.24: order of mode/setSleep/setAutoReconnect/softAP
    std::atomic<int> in_critical{0};      // V1.5.17: WiFi calls made inside taskENTER/EXIT_CRITICAL
    wl_status_t status = WL_DISCONNECTED;
    IPAddress ap_ip{192, 168, 4, 1}, sta_ip{192, 168, 1, 57};
};
inline ShimWiFi& shimWiFi() { static ShimWiFi w; if (shimCritDepth() > 0) ++w.in_critical; return w; }
class WiFiClass {
public:
    bool mode(wifi_mode_t m) { shimWiFi().mode = m; shimWiFi().calls.push_back("mode"); return true; }
    bool setSleep(bool enable) { shimWiFi().sleep = enable; shimWiFi().calls.push_back(enable ? "setSleep 1" : "setSleep 0"); return true; }
    wifi_mode_t getMode() { return shimWiFi().mode; }
    bool softAP(const char* ssid, const char* pass = nullptr, int channel = 1, int hidden = 0, int max_conn = 4) {
        ShimWiFi& w = shimWiFi(); w.ap_ssid = ssid ? ssid : ""; w.ap_pass = pass ? pass : ""; ++w.soft_aps; w.calls.push_back("softAP"); return true;
    }
    IPAddress softAPIP() { return shimWiFi().ap_ip; }
    wl_status_t begin(const char* ssid, const char* pass = nullptr) {
        ShimWiFi& w = shimWiFi(); w.sta_ssid = ssid ? ssid : ""; w.sta_pass = pass ? pass : ""; ++w.begins;
        return w.status;
    }
    bool disconnect(bool wifioff = false, bool eraseap = false) {
        ShimWiFi& w = shimWiFi(); ++w.disconnects; if (wifioff) ++w.disconnect_wifioff; return true;
    }
    bool setAutoReconnect(bool v) { shimWiFi().auto_reconnect = v; shimWiFi().calls.push_back("setAutoReconnect"); return true; }
    wl_status_t status() { return shimWiFi().status; }
    IPAddress localIP() { return shimWiFi().sta_ip; }
    bool isConnected() { return shimWiFi().status == WL_CONNECTED; }
};
inline WiFiClass WiFi;
