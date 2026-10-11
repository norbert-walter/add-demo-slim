/**
 * @file hal_io.h
 * @brief Thin hardware layer for digital/analog I/O (FROZEN CONTRACT V1.4 §11).
 *
 * Implementation: src/hal/hal_io.cpp (Sprint S2.5, taken over from the
 * smoketest code). Only the control task (Core 1) calls these functions.
 *
 * The i2s output (buzzer) is written through the FastAccelStepper engine
 * (engine.i2sMuxSetBit), therefore halInit() needs the engine pointer from
 * motion_drv (call motionDrvInit() first). HV LED, shutter LED and heartbeat
 * LED are plain GPIOs, driven LOW in halInit() before anything else.
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
