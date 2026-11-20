/**
 * @file system.cpp
 * @brief Boot helpers and task watchdog – Sprint P1, task 3.
 *
 * V1.6.6 implementation (see system.h for the exact spec).
 */

#include <Arduino.h>
#include <esp_system.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include "esp_attr.h"
#include "config.h"
#include "system/system.h"
#include "comm/log_buffer.h"

// V1.6.6: boot counters in RTC memory (survives every reset except power-on)
RTC_NOINIT_ATTR static BootCounters g_bootCounters;

// V1.6.6: result of the last systemInit() call, returned by systemBootInfo()
static BootInfo g_bootInfo = {};

void systemInit() {
    // Step 0: FIRST statement — set brownout threshold before anything else
    REG_SET_FIELD(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_DBROWN_OUT_THRES, BROWNOUT_LEVEL);

    // Step 1: Serial.begin + boot banner (four lines as before + fifth line)
    Serial.begin(115200);

    Serial.printf("FW_VERSION %s\r\n", FW_VERSION);
    Serial.printf("ESP-IDF %s\r\n", esp_get_idf_version());
    Serial.printf("Arduino core %d.%d.%d\r\n",
                  ESP_ARDUINO_VERSION_MAJOR,
                  ESP_ARDUINO_VERSION_MINOR,
                  ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("Reset reason %lu\r\n", (unsigned long)esp_reset_reason());
    uint8_t bod_level = (uint8_t)REG_GET_FIELD(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_DBROWN_OUT_THRES);
    Serial.printf("Brownout level %u\r\n", bod_level);

    // Step 2: Boot counters
    uint8_t reason = (uint8_t)esp_reset_reason();
    bootCountUpdate(g_bootCounters, reason);

    // Step 3: Store result for systemBootInfo()
    g_bootInfo.reset_reason = reason;
    g_bootInfo.boots        = g_bootCounters.boots;
    g_bootInfo.brownouts    = g_bootCounters.brownouts;
    g_bootInfo.bod_level    = bod_level;

    // Step 4: Log line
    logLine("boot: reset %s, boots %lu, brownouts %lu, brownout level %u",
            resetReasonName(reason),
            (unsigned long)g_bootCounters.boots,
            (unsigned long)g_bootCounters.brownouts,
            bod_level);

    // Step 5: Watchdog – V1.5.14: esp_task_wdt_config_t + reconfigure first,
    // fallback to init on ESP_ERR_INVALID_STATE.
    esp_task_wdt_config_t cfg = {
        .timeout_ms     = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic  = true,
    };

    esp_err_t ret = esp_task_wdt_reconfigure(&cfg);
    if (ret == ESP_ERR_INVALID_STATE) {
        esp_task_wdt_init(&cfg);
    }
}

BootInfo systemBootInfo() {
    return g_bootInfo;
}

void systemRestart() {
    ESP.restart();
}