/**
 * @file nvs_config.h
 * @brief Persistence of Params in NVS via Preferences (FROZEN CONTRACT V1.5 §9, Sprint P1).
 *
 * Namespace NVS_NAMESPACE. One key per Params field (short keys, max 15 chars):
 *   mpos, maint, tstart, tend, tspeed, shold, edge, base, cnraw, cnref, cfraw, cfref, estnc, ssid, pass,
 *   plus "ver" (uint8, schema version = 4; V1.3 speeds in mm/s, V1.5 measuring zone > 60 mm and
 *   shutter_hold "shold" – older data -> defaults).
 * Safety flags (interlock, cooling_flow, temp) are NEVER stored.
 * Called only from setup() and from the network task (Core 0) – never from
 * the control task (flash writes take milliseconds).
 *
 * V1.5.14 – exact behaviour (tested natively with a Preferences shim: test/native/test_nvs_config):
 *  Types: floats with putFloat/getFloat, "ver" with putUChar/getUChar, estnc with
 *  putBool/getBool, ssid/pass with putString/getString(key, char* buf, sizeof(field)).
 *  Key -> field: mpos measure_pos, maint maint_pos, tstart traverse_start, tend traverse_end,
 *  tspeed traverse_speed, shold shutter_hold, edge edge_threshold, base base_dist,
 *  cnraw calib.near_raw, cnref calib.near_ref, cfraw calib.far_raw, cfref calib.far_ref,
 *  estnc estop_nc, ssid wifi_ssid, pass wifi_pass.
 *  Load: paramsDefaults(p) first. begin(NVS_NAMESPACE, true) (read-only); begin fails or
 *    getUChar("ver", 0) != 4 -> keep the defaults. Otherwise every value read is applied with
 *    paramSet() under its SET key name (measure_pos, ..., calib_near_ref, calib_far_ref,
 *    estop_nc, wifi_ssid, wifi_pass); a value paramSet rejects (range, NaN, string rules) keeps
 *    its default. A missing key keeps its default. near_raw / far_raw (no SET key) are taken
 *    if finite. Finally: !calibValid(p.calib) -> p.calib = CalibPoints{}. end() before return.
 *  Save: begin(NVS_NAMESPACE, false); failure -> return false. Write "ver" = 4 and all 15 keys.
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
