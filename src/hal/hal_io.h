/**
 * @file hal_io.h
 * @brief Thin hardware layer for digital/analog I/O (FROZEN CONTRACT §11).
 *
 * Implementation: src/hal/hal_io.cpp (Sprint S2.5, taken over from the
 * smoketest code). Only the control task (Core 1) calls these functions.
 *
 * The i2s outputs (HV LED, buzzer) are written through the FastAccelStepper
 * engine (engine.i2sMuxSetBit), therefore halInit() needs the engine pointer
 * from motion_drv (call motionDrvInit() first).
 */
#pragma once

#include <stdint.h>

/// Configure GPIOs, ADC attenuation 11 dB. Outputs off.
void halInit();

/// Inputs decoded with the active levels from config.h. Call once per control cycle.
/// Limit switches: debounced (INPUT_DEBOUNCE_MS) in both directions.
/// E-stop: ACTIVATION is effective immediately (first sample), RELEASE is debounced.
void halUpdateInputs(uint32_t now_ms);
bool halEstop();      ///< true = E-stop active (pressed OR wire broken)
bool halLimitFwd();   ///< true = forward switch pressed
bool halLimitBwd();   ///< true = backward switch pressed

/// ADC_SAMPLES x analogReadMilliVolts(PIN_SENSOR_ADC) into buf (n entries). ~0.5 ms.
void halReadSensorSamples(uint16_t* buf, uint8_t n);

void halSetShutter(bool open);   ///< GPIO32 HIGH = open
void halSetHvLed(bool on);       ///< i2s slot I2SO_TO_SLOT(I2SO_HV_LED)
void halBeep(uint32_t now_ms, uint32_t duration_ms);   ///< non-blocking start
void halUpdateBuzzer(uint32_t now_ms);                  ///< switches buzzer off after duration
