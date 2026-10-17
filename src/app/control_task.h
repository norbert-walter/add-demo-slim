/**
 * @file control_task.h
 * @brief The one and only writer of the application state (FROZEN – Contract V1.3).
 *
 * Runs on Core 1 every CONTROL_PERIOD_MS. Reads HAL + stepper driver,
 * calls the pure logic (app_logic.h), writes outputs, publishes the snapshot.
 * Implementation: src/app/control_task.cpp (provided complete in Sprint 0b,
 * compiled and reviewed in Sprint I1).
 */
#pragma once
#include "logic/params.h"
#include <stdint.h>

/// Starts the task. `params` = values loaded from NVS (nvsLoadParams).
/// Returns false if hardware init failed (motionDrvInit).
bool controlTaskStart(const Params& params);

/// V1.5.25: reset CycleStats (cycle_max_us, period_max_us, overruns, prio_lat_max_us) in the next
/// cycle. Called once by setup() after "boot: ready", so start-up effects (display init, WLAN
/// start, flash access) do not count.
void controlResetCycleStats();

/// V1.5.25 diagnostics: breakdown of the LAST cycle with a work time > CYCLE_DIAG_CYCLE_US (2 ms)
/// or a start later than CYCLE_DIAG_PERIOD_US (2.5 ms) after the previous one (near the 3 ms limit). seq counts such cycles; the net task logs a
/// line when seq changed (rate-limited). Parts in us: adc = ADC sample + measurement, tick =
/// readInputs + appTick, cmd = commands, out = outputs, pub = replies + publish + statistics.
struct CycleDiag {
    uint32_t seq = 0;
    uint32_t total_us = 0, period_us = 0;
    uint32_t adc_us = 0, tick_us = 0, cmd_us = 0, out_us = 0, pub_us = 0;
};
void controlGetCycleDiag(CycleDiag& d);
