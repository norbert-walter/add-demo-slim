/**
 * @file net_task.h
 * @brief Network task: WLAN, HTTP server, NVS persistence, delayed reboot (V1.5.20, Sprint I1).
 *
 * Implementation: src/app/net_task.cpp. Runs on NET_TASK_CORE (Core 0) every NET_TASK_PERIOD_MS.
 * It is the ONLY task that calls wifiLoop(), httpServerLoop() (and through it
 * cmdQueueSendAndWait()), nvsSaveParams() and systemRestart().
 *
 * V1.5.20 – exact behaviour (tested natively: test/native/test_net_task):
 *  netTaskStep(now), in this order:
 *    1. wifiLoop(now)
 *    2. httpServerLoop()
 *    3. if snapshotTakePersistRequest(): Params p; SafetyFlags f; snapshotGetConfig(p, f);
 *       logLine("nvs: saved") if nvsSaveParams(p) returned true, else logLine("nvs: save FAILED")
 *    4. if snapshotTakeRebootRequest() and no reboot is pending: reboot pending, due at
 *       now + REBOOT_DELAY_MS; logLine("reboot: in %u ms", REBOOT_DELAY_MS). Further requests while
 *       pending are taken (cleared) but do not move the due time.
 *    5. if a reboot is pending and (int32_t)(now - due) >= 0 (wrap-around safe): pending = false,
 *       systemRestart(). (A pending flag, not due == 0, marks the state: now + 700 may be 0.)
 *    Step 3 comes before step 5: a SET (persist) and a REBOOT in the same step are saved first.
 *  netTaskStart(): xTaskCreatePinnedToCore(<task function>, "net", NET_TASK_STACK, nullptr,
 *    NET_TASK_PRIO, nullptr, NET_TASK_CORE); returns true if that returned pdPASS.
 *    The task function loops forever: netTaskStep(millis()); vTaskDelay(pdMS_TO_TICKS(NET_TASK_PERIOD_MS)).
 *
 * src/main.cpp (V1.5.20, tested natively in the same suite), setup() in this order:
 *    1. systemInit()
 *    2. SPIFFS.begin(true)  (format on failure); false -> logLine("spiffs: mount failed"), continue
 *    3. Params p; nvsLoadParams(p)
 *    4. cmdQueueInit()
 *    5. controlTaskStart(p); false -> logLine("FATAL: control task start failed"), continue
 *    6. displayStart();      false -> logLine("display: start failed"), continue
 *    7. wifiInit(p.wifi_ssid, p.wifi_pass)
 *    8. httpServerInit()
 *    9. netTaskStart();      false -> logLine("FATAL: net task start failed")
 *   10. logLine("boot: ready, FW %s", FW_VERSION)
 *  loop(): vTaskDelay(portMAX_DELAY) (the Arduino loop task is not used).
 *  The device keeps running after a failed step: the web server and /log show what failed.
 */
#pragma once
#include <stdint.h>

void netTaskStep(uint32_t now_ms);
bool netTaskStart();
