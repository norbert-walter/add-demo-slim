// FROZEN TESTS (Contract V1.5.14) – Sprint P1 must make these pass. Do not modify.
// Host test of the REAL src/system/system.cpp against shims of the ESP-IDF 5.x API
// (test/native/freertos_shim: esp_task_wdt.h, esp_system.h, esp_idf_version.h, Arduino.h).
// The IDF-4 call esp_task_wdt_init(timeout, panic) does not compile here.
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <string.h>
#include "system/system.cpp"   // implementation under test

void setUp() { Serial.clear(); shimWdt() = ShimWdt{}; ESP.restarts = 0; shimEspRestarts() = 0; }
void tearDown() {}

static bool has(const char* s) { return strstr(Serial.out.c_str(), s) != nullptr; }

void test_serial_and_banner() {
    shimResetReason() = ESP_RST_PWR_GLITCH;          // = 14
    systemInit();
    TEST_ASSERT_EQUAL_UINT32(115200, Serial.baud);
    TEST_ASSERT_TRUE_MESSAGE(has(FW_VERSION), Serial.out.c_str());
    TEST_ASSERT_TRUE_MESSAGE(has("v5.3.2-shim"), Serial.out.c_str());   // esp_get_idf_version()
    TEST_ASSERT_TRUE_MESSAGE(has("3.1.3"), Serial.out.c_str());         // Arduino core major.minor.patch
    TEST_ASSERT_TRUE_MESSAGE(has("14"), Serial.out.c_str());            // numeric reset reason
}
void test_watchdog_reconfigured_when_core_already_initialised() {
    shimWdt().initialised = true;                    // Arduino core 3.x usually initialises the TWDT
    systemInit();
    TEST_ASSERT_EQUAL_INT(1, shimWdt().reconfigure_calls);
    TEST_ASSERT_EQUAL_INT(0, shimWdt().init_calls);
    TEST_ASSERT_EQUAL_UINT32(WDT_TIMEOUT_S * 1000u, shimWdt().cfg.timeout_ms);
    TEST_ASSERT_TRUE(shimWdt().cfg.trigger_panic);
    TEST_ASSERT_EQUAL_UINT32(0, shimWdt().cfg.idle_core_mask);
}
void test_watchdog_initialised_when_not_yet_running() {
    shimWdt().initialised = false;
    systemInit();
    TEST_ASSERT_EQUAL_INT(1, shimWdt().reconfigure_calls);      // tried first ...
    TEST_ASSERT_EQUAL_INT(1, shimWdt().init_calls);             // ... then init after INVALID_STATE
    TEST_ASSERT_TRUE(shimWdt().initialised);
    TEST_ASSERT_EQUAL_UINT32(WDT_TIMEOUT_S * 1000u, shimWdt().cfg.timeout_ms);
    TEST_ASSERT_TRUE(shimWdt().cfg.trigger_panic);
}
void test_restart() {
    systemRestart();
    TEST_ASSERT_EQUAL_INT(1, ESP.restarts);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_serial_and_banner);
    RUN_TEST(test_watchdog_reconfigured_when_core_already_initialised);
    RUN_TEST(test_watchdog_initialised_when_not_yet_running);
    RUN_TEST(test_restart);
    return UNITY_END();
}
