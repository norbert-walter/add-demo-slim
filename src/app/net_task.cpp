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

static bool reboot_pending = false;
static uint32_t reboot_at = 0;

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
    // 1. wifiLoop(now)
    wifiLoop(now_ms);

    // 2. httpServerLoop()
    httpServerLoop();

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
        logLine("reboot: in %u ms", REBOOT_DELAY_MS);
    }

    // 5. Reboot execution (wrap-around safe signed comparison)
    if (reboot_pending && (int32_t)(now_ms - reboot_at) >= 0) {
        reboot_pending = false;
        systemRestart();
    }
}