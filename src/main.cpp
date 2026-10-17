/**
 * @file main.cpp
 * @brief Entry point: setup() and loop() (V1.5.20, Sprint I1).
 */
#include <Arduino.h>
#include <SPIFFS.h>
#include "config.h"
#include "logic/params.h"
#include "system/system.h"
#include "system/nvs_config.h"
#include "app/command_queue.h"
#include "app/control_task.h"
#include "app/net_task.h"
#include "display/display.h"
#include "comm/wifi_manager.h"
#include "comm/http_server.h"
#include "comm/log_buffer.h"

void setup() {
    // 1. systemInit
    systemInit();

    // 2. SPIFFS mount (format on failure)
    if (!SPIFFS.begin(true)) {
        logLine("spiffs: mount failed");
    }

    // 3. Load params from NVS
    Params p;
    nvsLoadParams(p);

    // 4. Initialize command queue
    cmdQueueInit();

    // 5. Start control task
    if (!controlTaskStart(p)) {
        logLine("FATAL: control task start failed");
    }

    // 6. Start display
    if (!displayStart()) {
        logLine("display: start failed");
    }

    // 7. Initialize WiFi
    wifiInit(p.wifi_ssid, p.wifi_pass);

    // 8. Initialize HTTP server
    httpServerInit();

    // 9. Start network task
    if (!netTaskStart()) {
        logLine("FATAL: net task start failed");
    }

    // 10. Boot ready
    logLine("boot: ready, FW %s", FW_VERSION);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}