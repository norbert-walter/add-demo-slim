/**
 * @file hal_io.h
 * @brief Thin hardware layer for digital/analog I/O (FROZEN CONTRACT V1.5 §11).
 *
 * Implementation: src/hal/hal_io.cpp (Sprint H1, taken over from the
 * smoketest code). Only the control task (Core 1) calls these functions.
 *
 * The i2s output (buzzer) is written through the FastAccelStepper engine
 * (engine.i2sMuxSetBit), therefore halInit() needs the engine pointer from
 * motion_drv (call motionDrvInit() first). HV LED, shutter LED and heartbeat
 * LED are plain GPIOs, driven LOW in halInit() before anything else.
 *
 * V1.5.18 – exact behaviour (tested natively with GPIO/ADC/FastAccelStepper shims: test/native/test_hal_io):
 *  halInit(), in this order:
 *    1. for PIN_SHUTTER, PIN_HV_LED, PIN_HEARTBEAT_LED (in this order): pinMode(pin, OUTPUT);
 *       digitalWrite(pin, LOW)                      <- the very first hardware accesses
 *    2. pinMode(PIN_LIMIT_FWD / PIN_LIMIT_BWD / PIN_ESTOP, INPUT)   (external pull-ups)
 *    3. (void)analogReadMilliVolts(PIN_SENSOR_ADC), then analogSetPinAttenuation(PIN_SENSOR_ADC,
 *       ADC_11db)   (Arduino 3.x: the pin must be an ADC channel before the attenuation is set)
 *    4. engine = motionDrvEngine() (stored); if not NULL: engine->i2sMuxSetBit(buzzer slot, false)
 *    5. output cache "unknown": the next call of every output setter writes.
 *       Input state "not sampled yet": halEstop(), halLimitFwd(), halLimitBwd() return TRUE until
 *       the first halUpdateInputs() (fail-safe: E-stop and both limits count as active).
 *  halUpdateInputs(now): reads each input exactly once (digitalRead) and decodes it:
 *    limit active  = (level == LIMIT_ACTIVE_LEVEL); E-stop active = (level == HIGH) if active_high,
 *    else (level == LOW).
 *    First call after halInit: every state takes the decoded sample directly (no debounce).
 *    Limit switches (both directions): the state changes only after the decoded sample has
 *      differed from the state continuously for >= INPUT_DEBOUNCE_MS (time of the first differing
 *      sample = start; a sample equal to the state cancels; uint32 wrap-around safe).
 *    E-stop: an active sample sets the state active IMMEDIATELY (and cancels a pending release);
 *      release as for the limits (inactive continuously for >= INPUT_DEBOUNCE_MS).
 *    halSetEstopActiveHigh: only stores the polarity. If it differs from the polarity used by the
 *      previous halUpdateInputs, that call discards a pending E-stop release (the release debounce
 *      starts again with the first sample under the new polarity).
 *  halReadSensorSamples(buf, n): buf NULL or n == 0 -> nothing; else n x
 *    analogReadMilliVolts(PIN_SENSOR_ADC) into buf[0..n-1] (uint16_t, values > 65535 -> 65535).
 *  Output setters: shutter / HV LED / heartbeat: digitalWrite(pin, on ? HIGH : LOW);
 *    buzzer: engine->i2sMuxSetBit(I2SO_TO_SLOT(I2SO_BUZZER), on) - if the stored engine is NULL,
 *    nothing is written and the cache stays "unknown". Each setter writes only if the value
 *    differs from its cache or the cache is "unknown", then updates the cache.
 *  No delay(), no Serial, no heap, no locks: only the control task calls these functions.
 */
#pragma once

#include <stdint.h>

/// Configure GPIOs, ADC attenuation 11 dB. Outputs off.
void halInit();

/// V1.3: E-stop active level from the parameter estop_nc (true -> HIGH = active,
/// false -> LOW = active). Called by the control task every cycle BEFORE
/// halUpdateInputs(). Default before the first call: LOW = active.
/// On a change, halUpdateInputs() re-evaluates the current raw level with the new
/// polarity in the same call: "active" takes effect immediately, "inactive" only
/// after the release debounce (CONTRACT §11).
void halSetEstopActiveHigh(bool active_high);

/// Inputs decoded with the active levels (limits: config.h, E-stop: see above).
/// Call once per control cycle.
/// Limit switches: debounced (INPUT_DEBOUNCE_MS) in both directions.
/// E-stop: ACTIVATION is effective immediately (first sample), RELEASE is debounced.
void halUpdateInputs(uint32_t now_ms);
bool halEstop();      ///< true = E-stop active (pressed; with estop_nc also wire broken)
bool halLimitFwd();   ///< true = forward switch pressed
bool halLimitBwd();   ///< true = backward switch pressed

/// ADC_SAMPLES x analogReadMilliVolts(PIN_SENSOR_ADC) into buf (n entries). ~0.5 ms.
void halReadSensorSamples(uint16_t* buf, uint8_t n);

/// Output setters write the hardware only when the value differs from the last
/// written value (first call always writes). Cheap to call every cycle.
void halSetShutter(bool open);   ///< GPIO32 HIGH = open
void halSetHvLed(bool on);       ///< V1.2: GPIO PIN_HV_LED (digitalWrite, HIGH = on)
void halSetBuzzer(bool on);      ///< V1.3: active buzzer, i2s bit I2SO_TO_SLOT(I2SO_BUZZER); pattern from sound_logic
void halSetHeartbeat(bool on);   ///< V1.3: blue board LED GPIO PIN_HEARTBEAT_LED (HIGH = on)
