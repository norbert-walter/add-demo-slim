#pragma once
// V1.5.14 host shim of the ESP-IDF 5.x task watchdog API (esp_task_wdt.h).
// The IDF-4 signature esp_task_wdt_init(uint32_t, bool) does NOT exist here on purpose.
#include "esp_err.h"
typedef struct {
    uint32_t timeout_ms;
    uint32_t idle_core_mask;
    bool     trigger_panic;
} esp_task_wdt_config_t;
typedef void* TaskHandle_t;
struct ShimWdt {
    bool initialised = false;
    int  init_calls = 0, reconfigure_calls = 0, add_calls = 0;
    esp_task_wdt_config_t cfg{};
};
inline ShimWdt& shimWdt() { static ShimWdt w; return w; }
inline esp_err_t esp_task_wdt_init(const esp_task_wdt_config_t* c) {
    ShimWdt& w = shimWdt(); ++w.init_calls;
    if (w.initialised) return ESP_ERR_INVALID_STATE;
    w.initialised = true; w.cfg = *c; return ESP_OK;
}
inline esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t* c) {
    ShimWdt& w = shimWdt(); ++w.reconfigure_calls;
    if (!w.initialised) return ESP_ERR_INVALID_STATE;
    w.cfg = *c; return ESP_OK;
}
inline esp_err_t esp_task_wdt_add(TaskHandle_t) { ++shimWdt().add_calls; return ESP_OK; }
inline esp_err_t esp_task_wdt_reset(void) { return ESP_OK; }
inline esp_err_t esp_task_wdt_deinit(void) { shimWdt().initialised = false; return ESP_OK; }
