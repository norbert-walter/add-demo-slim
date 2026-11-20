#pragma once
// V1.6.6 host shim of gpio_set_drive_capability (driver/gpio.h). Logged into shimHwLog() as
// "drive <pin> <cap>".
#include "Arduino.h"
#include "esp_err.h"
typedef int gpio_num_t;
typedef enum { GPIO_DRIVE_CAP_0 = 0, GPIO_DRIVE_CAP_1 = 1, GPIO_DRIVE_CAP_2 = 2, GPIO_DRIVE_CAP_DEFAULT = 2,
               GPIO_DRIVE_CAP_3 = 3 } gpio_drive_cap_t;
inline esp_err_t gpio_set_drive_capability(gpio_num_t pin, gpio_drive_cap_t cap) {
    shimHw("drive %d %d", (int)pin, (int)cap);
    return ESP_OK;
}
