/**
 * @file system.cpp
 * @brief Boot helpers and task watchdog – Sprint P1, task 3.
 *
 * V1.5.14 implementation (see system.h for the exact spec).
 */

#include <Arduino.h>
#include <esp_system.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "system/system.h"

void systemInit() {
    Serial.begin(115200);

    // Boot banner – V1.5.14: FW_VERSION, esp_get_idf_version(), Arduino core version,
    // numeric esp_reset_reason().
    Serial.printf("FW_VERSION %s\r\n", FW_VERSION);
    Serial.printf("ESP-IDF %s\r\n", esp_get_idf_version());
    Serial.printf("Arduino core %d.%d.%d\r\n",
                  ESP_ARDUINO_VERSION_MAJOR,
                  ESP_ARDUINO_VERSION_MINOR,
                  ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("Reset reason %lu\r\n", (unsigned long)esp_reset_reason());

    // Task watchdog – V1.5.14: esp_task_wdt_config_t + reconfigure first,
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

void systemRestart() {
    ESP.restart();
}