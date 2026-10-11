/**
 * @file nvs_config.h
 * @brief Persistence of Params in NVS via Preferences (FROZEN CONTRACT V1.4 §9, Sprint S1).
 *
 * Namespace NVS_NAMESPACE. One key per Params field (short keys, max 15 chars):
 *   mpos, maint, tstart, tend, tspeed, edge, base, cnraw, cnref, cfraw, cfref, estnc, ssid, pass,
 *   plus "ver" (uint8, schema version = 3; V1.3 speeds in mm/s – older data -> defaults).
 * Safety flags (interlock, cooling_flow, temp) are NEVER stored.
 * Called only from setup() and from the network task (Core 0) – never from
 * the control task (flash writes take milliseconds).
 */
#pragma once
#include "logic/params.h"

/// Loads params; missing keys / wrong "ver" -> defaults (paramsDefaults).
/// Values outside the CONTRACT §9 ranges are replaced by their defaults.
void nvsLoadParams(Params& p);

/// Saves all params. Returns false on NVS error.
bool nvsSaveParams(const Params& p);

/// Erases the namespace (factory reset; used by smoketest / service only).
void nvsErase();
