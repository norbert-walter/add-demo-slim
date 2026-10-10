/**
 * @file control_task.h
 * @brief The one and only writer of the application state (FROZEN – Contract V1.3).
 *
 * Runs on Core 1 every CONTROL_PERIOD_MS. Reads HAL + stepper driver,
 * calls the pure logic (app_logic.h), writes outputs, publishes the snapshot.
 * Implementation: src/app/control_task.cpp (provided complete in Sprint 0b,
 * compiled and reviewed in Sprint S2.6).
 */
#pragma once
#include "logic/params.h"

/// Starts the task. `params` = values loaded from NVS (nvsLoadParams).
/// Returns false if hardware init failed (motionDrvInit).
bool controlTaskStart(const Params& params);
