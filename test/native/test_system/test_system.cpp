// FROZEN TESTS (Contract V1.5.14, V1.6.6 Sprint B1) – the implementation must make these pass. Do not modify.
// Host test of the REAL src/system/system.cpp against shims of the ESP-IDF 5.x API
// (test/native/freertos_shim: esp_task_wdt.h, esp_system.h, esp_idf_version.h, Arduino.h).
// The IDF-4 call esp_task_wdt_init(timeout, panic) does not compile here.
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <string.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include "system/system.cpp"   // implementation under test

// V1.6.6: logLine is provided here (comm/log_buffer.cpp is not linked) and records the lines.
static std::vector<std::string> g_log;
void logLine(const char* fmt, ...) {
    char b[160]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    g_log.push_back(b);
}

void setUp() {
    Serial.clear(); shimWdt() = ShimWdt{}; ESP.restarts = 0; shimEspRestarts() = 0; g_log.clear();
    for (int i = 0; i < 16; ++i) shimRegs()[i] = 0;
    shimRegWrites() = 0; shimRegStuck() = 0;
}
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

// ---- V1.6.6 (Sprint B1) ----
void test_v166_brownout_level_set_first_and_read_back() {
    shimRegs()[RTC_CNTL_BROWN_OUT_REG] = (1u << 30) | (1u << 26);   // other bits (enable, rst_ena ...) set
    shimResetReason() = ESP_RST_POWERON;
    systemInit();
    uint32_t r = shimRegs()[RTC_CNTL_BROWN_OUT_REG];
    TEST_ASSERT_EQUAL_UINT32(BROWNOUT_LEVEL, (r >> 27) & 7u);
    TEST_ASSERT_EQUAL_UINT32((1u << 30) | (1u << 26), r & ~(7u << 27));       // other bits untouched
    TEST_ASSERT_EQUAL_UINT32(115200, Serial.baud);                           // Serial started as well
    char line[32]; snprintf(line, sizeof line, "Brownout level %u", (unsigned)BROWNOUT_LEVEL);
    TEST_ASSERT_TRUE_MESSAGE(has(line), Serial.out.c_str());
    TEST_ASSERT_EQUAL_UINT8(BROWNOUT_LEVEL, systemBootInfo().bod_level);
}
void test_v166_level_is_read_back_not_assumed() {
    // a field that does not take the value: the level READ BACK (2) is reported, not BROWNOUT_LEVEL
    shimRegs()[RTC_CNTL_BROWN_OUT_REG] = 2u << 27;
    shimRegStuck() = 7u << 27;
    shimResetReason() = ESP_RST_POWERON;
    systemInit();
    TEST_ASSERT_TRUE(shimRegWrites() >= 1);                          // it was written ...
    TEST_ASSERT_EQUAL_UINT8(2, systemBootInfo().bod_level);          // ... but reads back 2
    TEST_ASSERT_TRUE_MESSAGE(has("Brownout level 2\r\n"), Serial.out.c_str());
    char exp[96]; snprintf(exp, sizeof exp, "boot: reset POWERON, boots 1, brownouts 0, brownout level 2");
    TEST_ASSERT_EQUAL_STRING(exp, g_log.back().c_str());
}
void test_v166_boot_info_and_log_line() {
    shimResetReason() = ESP_RST_POWERON;  systemInit();
    BootInfo b = systemBootInfo();
    TEST_ASSERT_EQUAL_UINT8(ESP_RST_POWERON, b.reset_reason);
    TEST_ASSERT_EQUAL_UINT32(1, b.boots);
    TEST_ASSERT_EQUAL_UINT32(0, b.brownouts);
    shimResetReason() = ESP_RST_BROWNOUT; systemInit();      // RTC memory kept (static on the host)
    shimResetReason() = ESP_RST_SW;       systemInit();
    shimResetReason() = ESP_RST_BROWNOUT; systemInit();
    b = systemBootInfo();
    TEST_ASSERT_EQUAL_UINT8(ESP_RST_BROWNOUT, b.reset_reason);
    TEST_ASSERT_EQUAL_UINT32(4, b.boots);
    TEST_ASSERT_EQUAL_UINT32(2, b.brownouts);
    char exp[96];
    snprintf(exp, sizeof exp, "boot: reset BROWNOUT, boots 4, brownouts 2, brownout level %u", (unsigned)BROWNOUT_LEVEL);
    TEST_ASSERT_EQUAL_STRING(exp, g_log.back().c_str());
    TEST_ASSERT_EQUAL_INT(4, (int)g_log.size());             // one line per start
    shimResetReason() = ESP_RST_POWERON;  systemInit();       // power-on: counters start again
    TEST_ASSERT_EQUAL_UINT32(1, systemBootInfo().boots);
    TEST_ASSERT_EQUAL_UINT32(0, systemBootInfo().brownouts);
}
static uint32_t g_level_at_begin = 99;
static void atSerialBegin() { g_level_at_begin = (shimRegs()[RTC_CNTL_BROWN_OUT_REG] >> 27) & 7u; }
void test_v166_level_set_before_serial() {
    // step 0: the register is written before anything else, in particular before Serial.begin
    shimRegs()[RTC_CNTL_BROWN_OUT_REG] = 0;
    shimResetReason() = ESP_RST_POWERON;
    shimSerialBeginHook() = atSerialBegin;
    systemInit();
    shimSerialBeginHook() = nullptr;
    TEST_ASSERT_EQUAL_UINT32(BROWNOUT_LEVEL, g_level_at_begin);
    char line[32]; snprintf(line, sizeof line, "Brownout level %u\r\n", (unsigned)BROWNOUT_LEVEL);
    TEST_ASSERT_TRUE_MESSAGE(has(line), Serial.out.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_serial_and_banner);
    RUN_TEST(test_watchdog_reconfigured_when_core_already_initialised);
    RUN_TEST(test_watchdog_initialised_when_not_yet_running);
    RUN_TEST(test_restart);
    RUN_TEST(test_v166_brownout_level_set_first_and_read_back);
    RUN_TEST(test_v166_level_is_read_back_not_assumed);
    RUN_TEST(test_v166_boot_info_and_log_line);
    RUN_TEST(test_v166_level_set_before_serial);
    return UNITY_END();
}
