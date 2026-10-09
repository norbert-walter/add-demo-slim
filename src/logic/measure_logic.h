/**
 * @file measure_logic.h
 * @brief Pure measurement chain (FROZEN CONTRACT §7). No hardware access.
 *
 * Implementation: src/logic/measure_logic.cpp (Sprint S2.2).
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
/// V1.2: fitted to the six measured points of the demo (40..65 mm at 2.63..1.72 V,
/// max. error 0.6 mm). Datasheet fit 120.8 * V^-1.058 was ~3 mm too high there.
/// Outside 40..65 mm the curve is only indicative (135 mm reads ~189 mm).
struct SensorCurve {
    float k_mm     = 121.5f;
    float exponent = -1.142f;
    float min_mm   = 40.0f;   ///< valid range of the sensor
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

/// 2-point calibration. raw = uncalibrated reading, ref = true distance of the target.
struct CalibPoints {             ///< V1.2 defaults: identity at the demo's working range
    float near_raw = 40.0f;
    float near_ref = 40.0f;
    float far_raw  = 65.0f;
    float far_ref  = 65.0f;
};

/**
 * Valid if |far_raw - near_raw| >= 5 mm, |far_ref - near_ref| >= 5 mm and both
 * refs within 40..300 mm.
 */
bool calibValid(const CalibPoints& p);

/// Linear map through (near_raw, near_ref) and (far_raw, far_ref), extrapolates
/// outside. If !calibValid(p) returns raw unchanged.
float calibApply(float raw_mm, const CalibPoints& p);

/// max(0, base_dist_mm - dist_mm)
float thickness(float dist_mm, float base_dist_mm);

/// thick_mm > threshold_mm (strictly greater)
bool onMaterial(float thick_mm, float threshold_mm);

/**
 * Stability detector for calibration capture.
 * update(): if not initialised or |value - ref| > tol_mm -> ref = value, since = now.
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

/// Running mean of thickness during an acquisition (traverse run / measure hold).
struct AcqStats {
    double   sum = 0.0;
    uint32_t n   = 0;
};
void  statsReset(AcqStats& s);
/// Adds thick only if valid && on_material ("measures only on material, ignores gaps").
void  statsAdd(AcqStats& s, float thick_mm, bool on_material, bool valid);
float statsMean(const AcqStats& s);  ///< 0.0f if n == 0
