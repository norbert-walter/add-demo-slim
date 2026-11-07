/**
 * @file measure_logic.h
 * @brief Pure measurement chain (FROZEN CONTRACT V1.6.0 §7). No hardware access.
 *
 * Implementation: src/logic/measure_logic.cpp (Sprint L2).
 *
 * Signal chain, executed by the control task at MEASURE_RATE_HZ (50 Hz):
 *   16 x analogReadMilliVolts()  -> medianMean() (discard 4 low + 4 high)
 *   -> sensorMvToMm()            -> iirUpdate()            = raw_mm (uncalibrated)
 *   -> calibApply()              = dist_mm (calibrated)
 *   -> thickness(dist, base)     -> onMaterial(thick, edge_threshold)
 */
#pragma once

#include "types.h"

/// Sharp GP2Y0A41SK0F: dist_mm = k_mm * V^exponent with V in volt.
/// V1.4.2: fitted to the six stair steps measured in smoketest 0a (2026-10-11) with the
/// final wiring (11 kOhm load Vo-GND and 47 uF Vcc-GND at the sensor, R20 removed):
/// 1.972 V/65 mm, 2.091/60, 2.236/55, 2.403/50, 2.588/45, 2.777 V/40 mm, max. error
/// 0.48 mm. (V1.2 curve 121.5 * V^-1.142 read 2..9 mm too short with the load.)
/// Outside 40..65 mm the curve is only indicative (lower boom: 0.93 V reads ~186 mm).
struct SensorCurve {
    float k_mm     = 168.8f;
    float exponent = -1.398f;
    float min_mm   = 35.0f;   ///< V1.4.2: valid range of the sensor (40 mm step must not sit on the limit)
    float max_mm   = 300.0f;
};

/// Sentinel distance returned for voltages <= 1 mV (no signal).
constexpr float SENSOR_NO_SIGNAL_MM = 9999.0f;

/// Converts millivolt to millimetre. mv <= 1.0 -> SENSOR_NO_SIGNAL_MM. No clamping.
float sensorMvToMm(float mv, const SensorCurve& c);

/// min_mm <= mm <= max_mm
bool sensorInRange(float mm, const SensorCurve& c);

/**
 * Sorts samples ascending IN PLACE, discards `discard` values at each end and
 * returns the mean of the remaining n - 2*discard values.
 * If n == 0 or n <= 2*discard: returns 0.0f and leaves samples unchanged.
 */
float medianMean(uint16_t* samples, size_t n, size_t discard);

/// First-order IIR low pass: y = alpha * y + (1 - alpha) * x.
/// The first call after construction/reset sets y = x (no start-up ramp).
struct Iir {
    float alpha       = 0.8f;
    float y           = 0.0f;
    bool  initialized = false;
};
float iirUpdate(Iir& f, float x);  ///< returns the new y
void  iirReset(Iir& f);            ///< next update re-initialises

/**
 * V1.6.0 (Sprint K1) – calibration table, one point per step of the stair body.
 * Point i (1..CALIB_POINTS) = step centre at axis 95 + 20 * i mm (115, 135 ... 215 mm).
 * raw[i-1] = uncalibrated reading (raw_mm) captured with CALIB point i,
 * ref[i-1] = true distance of that step (SET calib_ref_i).
 * Defaults: identity, raw = ref = 65, 60, 55, 50, 45, 40 mm (points 1..6).
 * The 2-point calibration (near/far, CalibPoints) of V1.5 is gone.
 */
constexpr int   CALIB_POINTS      = 6;
constexpr float CALIB_MIN_STEP_MM = 1.0f;   ///< minimum distance of neighbouring points (raw and ref)

struct CalibTable {
    float raw[CALIB_POINTS] = {65.0f, 60.0f, 55.0f, 50.0f, 45.0f, 40.0f};
    float ref[CALIB_POINTS] = {65.0f, 60.0f, 55.0f, 50.0f, 45.0f, 40.0f};
};

/**
 * Valid if ALL of:
 *  - every raw and every ref is finite,
 *  - every ref is within 40..300 mm (inclusive),
 *  - with the points sorted by ref ascending, the refs AND the raws are strictly increasing
 *    and every two neighbours differ by at least CALIB_MIN_STEP_MM in raw and in ref
 *    (exactly 1.0 mm is valid). The order of the points in the table does not matter.
 * No heap, no static state (sort a local copy, e.g. insertion sort of 6 indices).
 */
bool calibValid(const CalibTable& t);

/**
 * Piecewise linear map raw -> dist through the points sorted by raw:
 *  - raw between two neighbouring points: linear interpolation on that segment,
 *  - raw exactly at a point: its ref,
 *  - raw below the smallest / above the largest point: extrapolation with the first / last segment.
 * If !calibValid(t): returns raw_mm unchanged. No heap, no static state.
 */
float calibApply(float raw_mm, const CalibTable& t);

/// max(0, base_dist_mm - dist_mm)
float thickness(float dist_mm, float base_dist_mm);

/// thick_mm > threshold_mm (strictly greater)
bool onMaterial(float thick_mm, float threshold_mm);

/**
 * Stability detector for calibration capture.
 * update(): if not initialised or |value - ref| > tol_mm -> RESTART: ref = value AND
 *           since = now (both!). Otherwise nothing changes: ref keeps the value of the last
 *           restart and does not follow small changes (V1.5.5). After a jump to a new
 *           level the tracker therefore becomes stable again at the new level (V1.5.6).
 * isStable(): initialised && (now - since) >= window_ms (uint32 wrap-safe).
 */
struct StabilityTracker {
    uint32_t window_ms   = 500;
    float    tol_mm      = 0.5f;
    float    ref         = 0.0f;
    uint32_t since_ms    = 0;
    bool     initialized = false;
};
void stabilityUpdate(StabilityTracker& s, uint32_t now_ms, float value);
bool stabilityIsStable(const StabilityTracker& s, uint32_t now_ms);

/// V1.5.5: all functions use the fields of the structs passed in (SensorCurve, Iir,
/// StabilityTracker); the defaults must never be hard-coded in measure_logic.cpp.

/// Running mean of thickness during an acquisition (traverse run / measure hold).
/// The sum is accumulated in double (AcqStats::sum).
struct AcqStats {
    double   sum = 0.0;
    uint32_t n   = 0;
};
void  statsReset(AcqStats& s);
/// Adds thick only if valid && on_material ("measures only on material, ignores gaps").
void  statsAdd(AcqStats& s, float thick_mm, bool on_material, bool valid);
float statsMean(const AcqStats& s);  ///< 0.0f if n == 0
