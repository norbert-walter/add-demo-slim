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

/// Starts the task. `params` = values loaded from NVS (nvsLoadParams).
/// Returns false if hardware init failed (motionDrvInit).
bool controlTaskStart(const Params& params);

/// V1.5.25: reset CycleStats (cycle_max_us, period_max_us, overruns, prio_lat_max_us) in the next
/// cycle. Called once by setup() after "boot: ready", so start-up effects (display init, WLAN
/// start, flash access) do not count.
void controlResetCycleStats();
