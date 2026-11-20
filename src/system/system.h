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
 *
 * V1.6.6 (Sprint B1) – brownout threshold, reset reason, boot counters. systemInit() in this order:
 *  0. FIRST statement, before Serial.begin: set the brownout threshold
 *       REG_SET_FIELD(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_DBROWN_OUT_THRES, BROWNOUT_LEVEL);
 *     (headers "soc/soc.h" and "soc/rtc_cntl_reg.h"; the field ESP-IDF itself sets at start-up
 *     from the sdkconfig - level 0 in the Arduino libraries. Detector, interrupt and restart stay
 *     as ESP-IDF configured them.)
 *  1. Serial.begin and the four banner lines as before (V1.5.14), then a fifth banner line
 *       Serial.printf("Brownout level %u\r\n", <level read back>)
 *     with the level READ BACK from the register:
 *       REG_GET_FIELD(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_DBROWN_OUT_THRES)
 *  2. Boot counters: a file-static  RTC_NOINIT_ATTR BootCounters  (header "esp_attr.h"; RTC memory
 *     keeps it over every reset except power-on). bootCountUpdate(counters, reason) with
 *     reason = (uint8_t)esp_reset_reason(). Never initialise that variable in code.
 *  3. The result is stored for systemBootInfo(): reset_reason = reason, boots and brownouts from
 *     the counters, bod_level = the level read back in step 1.
 *  4. logLine("boot: reset %s, boots %lu, brownouts %lu, brownout level %u",
 *             resetReasonName(reason), boots, brownouts, bod_level)   (comm/log_buffer.h)
 *  5. Watchdog as before (V1.5.14).
 *  systemBootInfo(): returns the BootInfo stored in step 3 (all 0 before systemInit).
 */
#pragma once
#include <stdint.h>
#include "logic/proc_json.h"   // BootInfo

/// Serial 115200, boot banner with FW_VERSION, ESP-IDF and Arduino core
/// version, reset reason; task watchdog WDT_TIMEOUT_S with panic = true.
void systemInit();

/// V1.6.6: reset reason, boot counters and brownout level of this start (for GET_STATE).
BootInfo systemBootInfo();

/// Restart helper used by the network task: called REBOOT_DELAY_MS after
/// snapshotTakeRebootRequest() returned true, so that the HTTP reply is sent and
/// the REBOOT buzzer pattern has finished (V1.3).
void systemRestart();
