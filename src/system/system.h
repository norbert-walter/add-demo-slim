/**
 * @file system.h
 * @brief Boot helpers and task watchdog (FROZEN CONTRACT V1.5 §12, implemented in Sprint P1).
 *
 * ESP-IDF 5.x API (Arduino Core 3.x): the task watchdog is configured with
 * esp_task_wdt_config_t + esp_task_wdt_reconfigure() (the core may already have
 * initialised it) or esp_task_wdt_init() if not yet initialised.
 * Do NOT use the IDF-4 signature esp_task_wdt_init(timeout, panic).
 *
 * V1.5.14 – exact behaviour (tested natively with shims: test/native/test_system):
 *  systemInit: Serial.begin(115200); banner lines via Serial.printf/println containing
 *    FW_VERSION, esp_get_idf_version() (esp_idf_version.h), the Arduino core version
 *    (ESP_ARDUINO_VERSION_MAJOR/MINOR/PATCH) and the numeric esp_reset_reason() (esp_system.h).
 *    Watchdog: esp_task_wdt_config_t cfg = {timeout_ms = WDT_TIMEOUT_S * 1000,
 *    idle_core_mask = 0, trigger_panic = true}; esp_task_wdt_reconfigure(&cfg); if that returns
 *    ESP_ERR_INVALID_STATE -> esp_task_wdt_init(&cfg). (esp_task_wdt.h)
 *  systemRestart: ESP.restart().
 */
#pragma once
#include <stdint.h>

/// Serial 115200, boot banner with FW_VERSION, ESP-IDF and Arduino core
/// version, reset reason; task watchdog WDT_TIMEOUT_S with panic = true.
void systemInit();

/// Restart helper used by the network task: called REBOOT_DELAY_MS after
/// snapshotTakeRebootRequest() returned true, so that the HTTP reply is sent and
/// the REBOOT buzzer pattern has finished (V1.3).
void systemRestart();
