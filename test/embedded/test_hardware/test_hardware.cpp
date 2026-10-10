/**
 * @file test_hardware.cpp
 * @brief On-target tests (Sprint S5). Uses the REAL firmware modules (HAL,
 *        motion driver, NVS, logic). Run only after user approval:
 *            pio test -e esp32-test
 *
 * SAFETY: no test moves the axis further than 2 mm. The motor test is
 * skipped unless the E-stop is released AND the backward switch is free.
 */
#include <Arduino.h>
#include <unity.h>
#include <string.h>
#include "config.h"
#include "hal/hal_io.h"
#include "hal/motion_drv.h"
#include "system/nvs_config.h"
#include "logic/crc32.h"
#include "logic/measure_logic.h"
#include "logic/proc_json.h"

void setUp() {}
void tearDown() { halSetShutter(false); halSetHvLed(false); }

void test_crc_matches_reference_on_target() {
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32Ieee((const uint8_t*)"123456789", 9));
}
void test_inputs_readable_and_estop_released() {
    halUpdateInputs(millis());
    delay(INPUT_DEBOUNCE_MS * 3);
    for (int i = 0; i < 5; ++i) { halUpdateInputs(millis()); delay(2); }
    TEST_ASSERT_FALSE_MESSAGE(halEstop(), "E-stop active or wire broken - release it for this test");
}
void test_sensor_in_range() {
    uint16_t s[ADC_SAMPLES];
    halReadSensorSamples(s, ADC_SAMPLES);
    float mm = sensorMvToMm(medianMean(s, ADC_SAMPLES, ADC_DISCARD), SensorCurve{});
    TEST_ASSERT_TRUE_MESSAGE(sensorInRange(mm, SensorCurve{}), "sensor reading outside 40..300 mm");
}
void test_outputs_switch_without_fault() {
    halSetShutter(true);  delay(300); halSetShutter(false);
    halSetHvLed(true);    delay(300); halSetHvLed(false);
    halSetBuzzer(true);   delay(100); halSetBuzzer(false);
    halSetHeartbeat(true); delay(300); halSetHeartbeat(false);
    TEST_PASS();
}
// V1.3: changing the E-stop polarity must re-evaluate immediately (CONTRACT §11).
// Works for both wirings: the configured polarity is read from NVS (estop_nc).
// PRECONDITION: E-stop NOT pressed and wired as configured (else the test stops with a message).
// With the released E-stop, the opposite polarity must read as active at once,
// the configured one must read inactive only after the release debounce.
void test_estop_polarity_change() {
    Params p; nvsLoadParams(p);
    const bool cfg = p.estop_nc;
    halSetEstopActiveHigh(cfg);
    for (int i = 0; i < INPUT_DEBOUNCE_MS + 10; ++i) { halUpdateInputs(millis()); delay(1); }
    TEST_ASSERT_FALSE_MESSAGE(halEstop(),
        "precondition: E-stop released and wired as configured (estop_nc) - check wiring/parameter");
    halSetEstopActiveHigh(!cfg);                 // opposite polarity for a released E-stop
    halUpdateInputs(millis());
    TEST_ASSERT_TRUE_MESSAGE(halEstop(), "polarity change must activate E-stop in the same call");
    halSetEstopActiveHigh(cfg);
    halUpdateInputs(millis());
    TEST_ASSERT_TRUE_MESSAGE(halEstop(), "release must be debounced");
    for (int i = 0; i < INPUT_DEBOUNCE_MS + 5; ++i) { halUpdateInputs(millis()); delay(1); }
    TEST_ASSERT_FALSE(halEstop());
}
void test_nvs_roundtrip() {
    Params p; paramsDefaults(p);
    p.measure_pos = 111.0f; strcpy(p.wifi_ssid, "TestNet");
    TEST_ASSERT_TRUE(nvsSaveParams(p));
    Params q; nvsLoadParams(q);
    TEST_ASSERT_EQUAL_FLOAT(111.0f, q.measure_pos);
    TEST_ASSERT_EQUAL_STRING("TestNet", q.wifi_ssid);
    paramsDefaults(p);
    TEST_ASSERT_TRUE(nvsSaveParams(p));
}
void test_motor_moves_2mm_and_back() {
    halUpdateInputs(millis());
    if (halEstop() || halLimitBwd() || halLimitFwd()) TEST_IGNORE_MESSAGE("axis not free - skipped");
    DrvRequest r; r.op = DrvOp::ZERO_HERE; motionDrvApply(r);
    r.op = DrvOp::MOVE_TO; r.target_steps = (int32_t)(2.0f * STEPS_PER_MM); r.speed_hz = (uint32_t)(5.0f * STEPS_PER_MM);
    motionDrvApply(r);
    uint32_t t0 = millis(); while (motionDrvRunning() && millis() - t0 < 3000) delay(5);
    TEST_ASSERT_EQUAL_INT32(r.target_steps, motionDrvPos());
    r.target_steps = 0; motionDrvApply(r);
    t0 = millis(); while (motionDrvRunning() && millis() - t0 < 3000) delay(5);
    TEST_ASSERT_EQUAL_INT32(0, motionDrvPos());
}
void test_proc_json_fits() {
    ProcData d; char b[PROC_JSON_MAX];
    TEST_ASSERT_TRUE(procToJson(d, b, sizeof(b)) > 0);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    bool drv_ok = motionDrvInit();
    halInit();
    UNITY_BEGIN();
    if (!drv_ok) { TEST_MESSAGE("motionDrvInit failed"); UNITY_END(); return; }
    RUN_TEST(test_crc_matches_reference_on_target);
    RUN_TEST(test_inputs_readable_and_estop_released);
    RUN_TEST(test_sensor_in_range);
    RUN_TEST(test_outputs_switch_without_fault);
    RUN_TEST(test_estop_polarity_change);
    RUN_TEST(test_nvs_roundtrip);
    RUN_TEST(test_motor_moves_2mm_and_back);
    RUN_TEST(test_proc_json_fits);
    UNITY_END();
}
void loop() { delay(1000); }
