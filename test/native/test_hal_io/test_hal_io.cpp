// FROZEN TESTS (Contract V1.5.18) – Sprint H1 must make these pass. Do not modify.
// Host test of the REAL src/hal/hal_io.cpp with the GPIO/ADC shims (Arduino.h) and the
// FastAccelStepper shim (test/native/freertos_shim). Every hardware access is recorded in
// shimHwLog() in call order. motionDrvEngine() is provided by this test (hal_io must not need
// motion_drv.cpp to be tested).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <string.h>
#include <string>
#include <vector>
#include <FastAccelStepper.h>
#include "hal/hal_io.cpp"   // implementation under test

static FastAccelStepperEngine g_engine;
static bool g_have_engine = true;
FastAccelStepperEngine* motionDrvEngine() { return g_have_engine ? &g_engine : nullptr; }

static ShimGpio& io() { return shimGpio(); }
static int count(const char* entry) { int n = 0; for (auto& e : shimHwLog()) if (e == entry) ++n; return n; }
static int indexOf(const char* entry) { for (size_t i = 0; i < shimHwLog().size(); ++i) if (shimHwLog()[i] == entry) return (int)i; return -1; }
static void released() {                       // all inputs inactive with the default wiring
    io().in[PIN_LIMIT_FWD] = !LIMIT_ACTIVE_LEVEL; io().in[PIN_LIMIT_BWD] = !LIMIT_ACTIVE_LEVEL;
    io().in[PIN_ESTOP] = HIGH;                 // estop_nc = false: active LOW
}
static uint32_t T = 1000;
static void freshInit() {                      // halInit + first sample, all released
    g_have_engine = true; halSetEstopActiveHigh(false);
    io().reset(); shimFas().reset(); released(); shimHwLog().clear();
    halInit();
    T += 1000; halUpdateInputs(T);
}

void setUp() {}
void tearDown() {}

void test_init_outputs_low_before_anything_else() {
    io().reset(); shimFas().reset(); shimHwLog().clear(); g_have_engine = true;
    halInit();
    const std::vector<std::string> first = {"pinMode 32 3", "dw 32 0", "pinMode 4 3", "dw 4 0", "pinMode 2 3", "dw 2 0"};
    TEST_ASSERT_TRUE(shimHwLog().size() >= first.size());
    for (size_t i = 0; i < first.size(); ++i) TEST_ASSERT_EQUAL_STRING(first[i].c_str(), shimHwLog()[i].c_str());
    TEST_ASSERT_EQUAL_INT(INPUT, io().mode[PIN_LIMIT_FWD]);
    TEST_ASSERT_EQUAL_INT(INPUT, io().mode[PIN_LIMIT_BWD]);
    TEST_ASSERT_EQUAL_INT(INPUT, io().mode[PIN_ESTOP]);
    int adc = indexOf("adc 34"), att = indexOf("atten 34 3");
    TEST_ASSERT_TRUE_MESSAGE(adc > 5 && att > adc, "ADC read first, then attenuation 11 dB");
    TEST_ASSERT_EQUAL_INT(1, count("fas bit 7 0"));                 // buzzer off
    TEST_ASSERT_EQUAL_INT(1, (int)shimFas().bit_writes);            // and nothing else on the mux
    TEST_ASSERT_EQUAL_INT(3, count("dw 32 0") + count("dw 4 0") + count("dw 2 0"));
    TEST_ASSERT_EQUAL_STRING("", Serial.out.c_str());
}
void test_inputs_fail_safe_until_first_update() {
    io().reset(); released(); shimHwLog().clear();
    halInit();
    TEST_ASSERT_TRUE(halEstop()); TEST_ASSERT_TRUE(halLimitFwd()); TEST_ASSERT_TRUE(halLimitBwd());
    halUpdateInputs(T += 10);                                          // first sample: taken directly
    TEST_ASSERT_FALSE(halEstop()); TEST_ASSERT_FALSE(halLimitFwd()); TEST_ASSERT_FALSE(halLimitBwd());
    io().in[PIN_LIMIT_BWD] = LIMIT_ACTIVE_LEVEL; halInit(); halUpdateInputs(T += 10);
    TEST_ASSERT_TRUE(halLimitBwd());                                   // first sample active: at once
    TEST_ASSERT_FALSE(halLimitFwd());
}
void test_each_input_read_once_per_update() {
    freshInit();
    int f = io().reads[PIN_LIMIT_FWD], b = io().reads[PIN_LIMIT_BWD], e = io().reads[PIN_ESTOP];
    halUpdateInputs(T += 1);
    TEST_ASSERT_EQUAL_INT(f + 1, io().reads[PIN_LIMIT_FWD]);
    TEST_ASSERT_EQUAL_INT(b + 1, io().reads[PIN_LIMIT_BWD]);
    TEST_ASSERT_EQUAL_INT(e + 1, io().reads[PIN_ESTOP]);
    int before = (int)shimHwLog().size();
    (void)halEstop(); (void)halLimitFwd(); (void)halLimitBwd();
    TEST_ASSERT_EQUAL_INT(before, (int)shimHwLog().size());            // getters do not touch hardware
}
void test_limit_debounce_both_directions() {
    freshInit();
    const uint32_t t = T;
    io().in[PIN_LIMIT_FWD] = LIMIT_ACTIVE_LEVEL;
    halUpdateInputs(t + 1); TEST_ASSERT_FALSE(halLimitFwd());
    halUpdateInputs(t + 1 + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_FALSE(halLimitFwd());
    halUpdateInputs(t + 1 + INPUT_DEBOUNCE_MS); TEST_ASSERT_TRUE(halLimitFwd());
    TEST_ASSERT_FALSE(halLimitBwd());                                  // independent
    io().in[PIN_LIMIT_FWD] = !LIMIT_ACTIVE_LEVEL;                      // release is debounced too
    halUpdateInputs(t + 20); TEST_ASSERT_TRUE(halLimitFwd());
    halUpdateInputs(t + 20 + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_TRUE(halLimitFwd());
    halUpdateInputs(t + 20 + INPUT_DEBOUNCE_MS); TEST_ASSERT_FALSE(halLimitFwd());
    T = t + 100;
}
void test_limit_bounce_restarts_debounce() {
    freshInit();
    const uint32_t t = T;
    io().in[PIN_LIMIT_BWD] = LIMIT_ACTIVE_LEVEL;  halUpdateInputs(t + 1);
    io().in[PIN_LIMIT_BWD] = !LIMIT_ACTIVE_LEVEL; halUpdateInputs(t + 3);     // bounce cancels
    io().in[PIN_LIMIT_BWD] = LIMIT_ACTIVE_LEVEL;  halUpdateInputs(t + 4);     // new start
    halUpdateInputs(t + 1 + INPUT_DEBOUNCE_MS); TEST_ASSERT_FALSE(halLimitBwd());
    halUpdateInputs(t + 4 + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_FALSE(halLimitBwd());
    halUpdateInputs(t + 4 + INPUT_DEBOUNCE_MS); TEST_ASSERT_TRUE(halLimitBwd());
    T = t + 100;
}
void test_estop_activation_immediate_release_debounced() {
    freshInit();
    const uint32_t t = T;
    io().in[PIN_ESTOP] = LOW;                                          // default wiring: active LOW
    halUpdateInputs(t + 1); TEST_ASSERT_TRUE(halEstop());              // same call, no debounce
    io().in[PIN_ESTOP] = HIGH;
    halUpdateInputs(t + 2); TEST_ASSERT_TRUE(halEstop());
    halUpdateInputs(t + 2 + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_TRUE(halEstop());
    io().in[PIN_ESTOP] = LOW;  halUpdateInputs(t + 2 + INPUT_DEBOUNCE_MS);   // pressed again: cancels
    io().in[PIN_ESTOP] = HIGH; halUpdateInputs(t + 3 + INPUT_DEBOUNCE_MS);
    halUpdateInputs(t + 3 + 2 * INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_TRUE(halEstop());
    halUpdateInputs(t + 3 + 2 * INPUT_DEBOUNCE_MS); TEST_ASSERT_FALSE(halEstop());
    io().in[PIN_ESTOP] = LOW; halUpdateInputs(t + 4 + 2 * INPUT_DEBOUNCE_MS);
    TEST_ASSERT_TRUE(halEstop());                                      // a single sample is enough
    T = t + 100;
}
void test_estop_nc_wiring_active_high() {
    freshInit();
    halSetEstopActiveHigh(true);
    io().in[PIN_ESTOP] = LOW; halUpdateInputs(T += 1);                 // closed contact = normal
    for (int i = 0; i < 10; ++i) halUpdateInputs(T += 1);
    TEST_ASSERT_FALSE(halEstop());
    io().in[PIN_ESTOP] = HIGH; halUpdateInputs(T += 1);                // open / broken wire
    TEST_ASSERT_TRUE(halEstop());
    halSetEstopActiveHigh(false);
}
void test_polarity_change_reevaluated_in_same_call() {
    freshInit();                                                       // released, active LOW
    for (int i = 0; i < 10; ++i) halUpdateInputs(T += 1);
    TEST_ASSERT_FALSE(halEstop());
    halSetEstopActiveHigh(true);                                       // HIGH level now means active
    TEST_ASSERT_FALSE(halEstop());                                     // the setter itself decodes nothing
    halUpdateInputs(T += 1); TEST_ASSERT_TRUE(halEstop());             // at once
    halSetEstopActiveHigh(false);
    halUpdateInputs(T += 1); TEST_ASSERT_TRUE(halEstop());             // release debounced
    const uint32_t t = T;
    halUpdateInputs(t + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_TRUE(halEstop());
    halUpdateInputs(t + INPUT_DEBOUNCE_MS); TEST_ASSERT_FALSE(halEstop());
    T = t + 100;
}
void test_polarity_change_restarts_pending_release() {
    freshInit();
    const uint32_t t = T;
    io().in[PIN_ESTOP] = LOW; halUpdateInputs(t + 1); TEST_ASSERT_TRUE(halEstop());
    io().in[PIN_ESTOP] = HIGH; halUpdateInputs(t + 2);                 // release pending since t+2
    halSetEstopActiveHigh(true); io().in[PIN_ESTOP] = LOW;             // other wiring, also inactive
    halUpdateInputs(t + 4);                                            // polarity change: restart at t+4
    halUpdateInputs(t + 2 + INPUT_DEBOUNCE_MS); TEST_ASSERT_TRUE(halEstop());
    halUpdateInputs(t + 4 + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_TRUE(halEstop());
    halUpdateInputs(t + 4 + INPUT_DEBOUNCE_MS); TEST_ASSERT_FALSE(halEstop());
    halSetEstopActiveHigh(false); io().in[PIN_ESTOP] = HIGH;
    T = t + 100;
}
void test_debounce_across_uint32_wraparound() {
    freshInit();
    uint32_t t = 0xFFFFFFFFu - 2;
    io().in[PIN_LIMIT_FWD] = LIMIT_ACTIVE_LEVEL;
    halUpdateInputs(t);                                                // start just before the wrap
    halUpdateInputs(t + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_FALSE(halLimitFwd());   // wrapped, 4 ms
    halUpdateInputs(t + INPUT_DEBOUNCE_MS);     TEST_ASSERT_TRUE(halLimitFwd());
    io().in[PIN_ESTOP] = LOW;  halUpdateInputs(t + 10); TEST_ASSERT_TRUE(halEstop());
    io().in[PIN_ESTOP] = HIGH; halUpdateInputs(t + 11);
    halUpdateInputs(t + 11 + INPUT_DEBOUNCE_MS - 1); TEST_ASSERT_TRUE(halEstop());
    halUpdateInputs(t + 11 + INPUT_DEBOUNCE_MS); TEST_ASSERT_FALSE(halEstop());
    T = 1000;
}
void test_sensor_samples() {
    freshInit();
    uint16_t b[20]; for (auto& v : b) v = 7;
    io().mv[PIN_SENSOR_ADC] = 1234;
    int r = io().adc_reads[PIN_SENSOR_ADC];
    halReadSensorSamples(b, ADC_SAMPLES);
    TEST_ASSERT_EQUAL_INT(r + ADC_SAMPLES, io().adc_reads[PIN_SENSOR_ADC]);
    for (int i = 0; i < ADC_SAMPLES; ++i) TEST_ASSERT_EQUAL_UINT16(1234, b[i]);
    TEST_ASSERT_EQUAL_UINT16(7, b[ADC_SAMPLES]);                       // nothing behind n
    halReadSensorSamples(b, 0);
    halReadSensorSamples(nullptr, 5);
    TEST_ASSERT_EQUAL_INT(r + ADC_SAMPLES, io().adc_reads[PIN_SENSOR_ADC]);
    io().mv[PIN_SENSOR_ADC] = 70000;
    halReadSensorSamples(b, 1);
    TEST_ASSERT_EQUAL_UINT16(65535, b[0]);
}
void test_outputs_write_only_on_change() {
    freshInit();
    struct { void (*set)(bool); int pin; } o[3] = {{halSetShutter, PIN_SHUTTER}, {halSetHvLed, PIN_HV_LED}, {halSetHeartbeat, PIN_HEARTBEAT_LED}};
    for (auto& x : o) {
        int w = io().writes[x.pin];
        x.set(false); TEST_ASSERT_EQUAL_INT_MESSAGE(w + 1, io().writes[x.pin], "first call after halInit writes");
        TEST_ASSERT_EQUAL_INT(LOW, io().out[x.pin]);
        x.set(false); TEST_ASSERT_EQUAL_INT(w + 1, io().writes[x.pin]);
        x.set(true);  TEST_ASSERT_EQUAL_INT(w + 2, io().writes[x.pin]);
        TEST_ASSERT_EQUAL_INT(HIGH, io().out[x.pin]);
        for (int i = 0; i < 100; ++i) x.set(true);
        TEST_ASSERT_EQUAL_INT(w + 2, io().writes[x.pin]);
        x.set(false); TEST_ASSERT_EQUAL_INT(w + 3, io().writes[x.pin]);
    }
    halSetShutter(true);                                               // pins are not mixed up
    TEST_ASSERT_EQUAL_INT(HIGH, io().out[PIN_SHUTTER]);
    TEST_ASSERT_EQUAL_INT(LOW, io().out[PIN_HV_LED]);
    TEST_ASSERT_EQUAL_INT(LOW, io().out[PIN_HEARTBEAT_LED]);
}
void test_buzzer_bit_only_on_change() {
    freshInit();
    int n = shimFas().bit_writes;
    halSetBuzzer(false); TEST_ASSERT_EQUAL_INT(n + 1, shimFas().bit_writes);   // first call writes
    halSetBuzzer(false); TEST_ASSERT_EQUAL_INT(n + 1, shimFas().bit_writes);
    halSetBuzzer(true);
    TEST_ASSERT_EQUAL_STRING("fas bit 7 1", shimHwLog().back().c_str());
    for (int i = 0; i < 50; ++i) halSetBuzzer(true);
    TEST_ASSERT_EQUAL_INT(n + 2, shimFas().bit_writes);
    halSetBuzzer(false);
    TEST_ASSERT_EQUAL_STRING("fas bit 7 0", shimHwLog().back().c_str());
    TEST_ASSERT_EQUAL_HEX32(0, shimFas().bits);
}
void test_setters_write_again_after_halInit() {
    freshInit();
    halSetShutter(true); halSetHvLed(true); halSetHeartbeat(true); halSetBuzzer(true);
    halInit();                                                         // hardware LOW / buzzer off again
    TEST_ASSERT_EQUAL_INT(LOW, io().out[PIN_SHUTTER]);
    TEST_ASSERT_EQUAL_HEX32(0, shimFas().bits);
    halSetShutter(true); halSetHvLed(true); halSetHeartbeat(true); halSetBuzzer(true);   // must write
    TEST_ASSERT_EQUAL_INT(HIGH, io().out[PIN_SHUTTER]);
    TEST_ASSERT_EQUAL_INT(HIGH, io().out[PIN_HV_LED]);
    TEST_ASSERT_EQUAL_INT(HIGH, io().out[PIN_HEARTBEAT_LED]);
    TEST_ASSERT_EQUAL_HEX32(1u << I2SO_TO_SLOT(I2SO_BUZZER), shimFas().bits);
}
void test_buzzer_without_engine_writes_nothing() {
    io().reset(); shimFas().reset(); released(); shimHwLog().clear();
    g_have_engine = false;
    halInit();
    TEST_ASSERT_EQUAL_INT(0, shimFas().bit_writes);
    halSetBuzzer(true); halSetBuzzer(false); halSetBuzzer(true);
    TEST_ASSERT_EQUAL_INT(0, shimFas().bit_writes);
    g_have_engine = true;                                              // available later: still not used
    halSetBuzzer(false); halSetBuzzer(true);
    TEST_ASSERT_EQUAL_INT(0, shimFas().bit_writes);
    halInit();                                                         // engine is taken in halInit
    halSetBuzzer(true);
    TEST_ASSERT_EQUAL_STRING("fas bit 7 1", shimHwLog().back().c_str());
}
void test_no_serial_output() {
    TEST_ASSERT_EQUAL_STRING("", Serial.out.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_init_outputs_low_before_anything_else);
    RUN_TEST(test_inputs_fail_safe_until_first_update);
    RUN_TEST(test_each_input_read_once_per_update);
    RUN_TEST(test_limit_debounce_both_directions);
    RUN_TEST(test_limit_bounce_restarts_debounce);
    RUN_TEST(test_estop_activation_immediate_release_debounced);
    RUN_TEST(test_estop_nc_wiring_active_high);
    RUN_TEST(test_polarity_change_reevaluated_in_same_call);
    RUN_TEST(test_polarity_change_restarts_pending_release);
    RUN_TEST(test_debounce_across_uint32_wraparound);
    RUN_TEST(test_sensor_samples);
    RUN_TEST(test_outputs_write_only_on_change);
    RUN_TEST(test_buzzer_bit_only_on_change);
    RUN_TEST(test_setters_write_again_after_halInit);
    RUN_TEST(test_buzzer_without_engine_writes_nothing);
    RUN_TEST(test_no_serial_output);
    return UNITY_END();
}
