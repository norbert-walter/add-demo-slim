/**
 * @file system.h
 * @brief Boot helpers and task watchdog (FROZEN CONTRACT §12, implemented in Sprint S1).
 *
 * ESP-IDF 5.x API (Arduino Core 3.x): the task watchdog is configured with
 * esp_task_wdt_config_t + esp_task_wdt_reconfigure() (the core may already have
 * initialised it) or esp_task_wdt_init() if not yet initialised.
 * Do NOT use the IDF-4 signature esp_task_wdt_init(timeout, panic).
 */
#pragma once
#include <stdint.h>

/// Serial 115200, boot banner with FW_VERSION, ESP-IDF and Arduino core
/// version, reset reason; task watchdog WDT_TIMEOUT_S with panic = true.
void systemInit();

/// Restart helper used by the network task (after the HTTP reply was sent).
void systemRestart();
