/**
 * @file nvs_config.h
 * @brief Persistence of Params in NVS via Preferences (FROZEN CONTRACT V1.6.0 §9, Sprint P1, K1).
 *
 * Namespace NVS_NAMESPACE. One key per Params field (short keys, max 15 chars):
 *   mpos, maint, tstart, tend, tspeed, shold, edge, base, craw1..craw6, cref1..cref6, estnc, ssid, pass,
 *   plus "ver" (uint8, schema version = 5; V1.6.0 calibration table. Version 4 (V1.5: 2-point
 *   calibration cnraw/cnref/cfraw/cfref) is still loaded, but WITHOUT the calibration; other
 *   versions -> defaults).
 * Safety flags (interlock, cooling_flow, temp) are NEVER stored.
 * Called only from setup() and from the network task (Core 0) – never from
 * the control task (flash writes take milliseconds).
 *
 * V1.5.14 – exact behaviour (tested natively with a Preferences shim: test/native/test_nvs_config):
 *  Types: floats with putFloat/getFloat, "ver" with putUChar/getUChar, estnc with
 *  putBool/getBool, ssid/pass with putString/getString(key, char* buf, sizeof(field)).
 *  Key -> field: mpos measure_pos, maint maint_pos, tstart traverse_start, tend traverse_end,
 *  tspeed traverse_speed, shold shutter_hold, edge edge_threshold, base base_dist,
 *  crawI calib.raw[I-1], crefI calib.ref[I-1] (I = 1..6, V1.6.0),
 *  estnc estop_nc, ssid wifi_ssid, pass wifi_pass.
 *  Load: paramsDefaults(p) first. begin(NVS_NAMESPACE, true) (read-only); begin fails or
 *    getUChar("ver", 0) not 4 and not 5 -> keep the defaults. Otherwise every value read is applied
 *    with paramSet() under its SET key name (measure_pos, ..., estop_nc, wifi_ssid, wifi_pass and,
 *    ONLY for ver 5, calib_ref_1..calib_ref_6); a value paramSet rejects (range, NaN, string rules)
 *    keeps its default. A missing key keeps its default. ver 5: craw1..craw6 (no SET key) are taken
 *    if finite. ver 4: the old keys cnraw/cnref/cfraw/cfref are ignored (calibration = default).
 *    Finally: !calibValid(p.calib) -> p.calib = CalibTable{}. end() before return.
 *  Save: begin(NVS_NAMESPACE, false); failure -> return false. Write "ver" = 5 and all 23 keys
 *    (V1.6.0; the old keys cnraw/cnref/cfraw/cfref are neither written nor removed).
 *    A put returning 0 is an error, EXCEPT putString of an empty string (Preferences returns
 *    strlen = 0 there). end() always after a successful begin. Returns true only if begin and
 *    every put succeeded.
 *  Erase: begin(NVS_NAMESPACE, false); clear(); end().
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
