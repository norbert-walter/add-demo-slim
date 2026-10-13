/**
 * @file measure_logic.cpp
 * @brief Pure measurement chain implementation (CONTRACT §7).
 *
 * Pure C++17: no Arduino, no ESP-IDF, no heap allocation, no static mutable state.
 */

#include "measure_logic.h"
#include <algorithm>   // std::sort
#include <cmath>       // powf, fabsf, fmaxf

// ---------------------------------------------------------------------------
// Sensor curve: dist_mm = k_mm * (V)^exponent, V in volts
// Sentinel for mv <= 1.0 mV
// ---------------------------------------------------------------------------

float sensorMvToMm(float mv, const SensorCurve& c) {
    if (mv <= 1.0f) {
        return SENSOR_NO_SIGNAL_MM;
    }
    return c.k_mm * powf(mv / 1000.0f, c.exponent);
}

bool sensorInRange(float mm, const SensorCurve& c) {
    return (mm >= c.min_mm && mm <= c.max_mm);
}

// ---------------------------------------------------------------------------
// Trimmed mean: sort in-place, discard `discard` from each end
// ---------------------------------------------------------------------------

float medianMean(uint16_t* samples, size_t n, size_t discard) {
    if (n == 0 || n <= 2 * discard) {
        return 0.0f;
    }
    std::sort(samples, samples + n);
    double sum = 0.0;
    const size_t start = discard;
    const size_t end   = n - discard;  // exclusive
    for (size_t i = start; i < end; ++i) {
        sum += static_cast<double>(samples[i]);
    }
    return static_cast<float>(sum / static_cast<double>(end - start));
}

// ---------------------------------------------------------------------------
// First-order IIR low-pass: y = alpha * y + (1 - alpha) * x
// First call initialises y = x (no ramp-up).
// ---------------------------------------------------------------------------

float iirUpdate(Iir& f, float x) {
    if (!f.initialized) {
        f.y = x;
        f.initialized = true;
        return x;
    }
    f.y = f.alpha * f.y + (1.0f - f.alpha) * x;
    return f.y;
}

void iirReset(Iir& f) {
    f.initialized = false;
}

// ---------------------------------------------------------------------------
// 2-point linear calibration
// ---------------------------------------------------------------------------

bool calibValid(const CalibPoints& p) {
    const float raw_span  = fabsf(p.far_raw  - p.near_raw);
    const float ref_span  = fabsf(p.far_ref  - p.near_ref);
    return (raw_span >= 5.0f && ref_span >= 5.0f
            && p.near_ref >= 40.0f && p.near_ref <= 300.0f
            && p.far_ref  >= 40.0f && p.far_ref  <= 300.0f);
}

float calibApply(float raw_mm, const CalibPoints& p) {
    if (!calibValid(p)) {
        return raw_mm;
    }
    const float slope = (p.far_ref - p.near_ref) / (p.far_raw - p.near_raw);
    return p.near_ref + slope * (raw_mm - p.near_raw);
}

// ---------------------------------------------------------------------------
// Thickness and material detection
// ---------------------------------------------------------------------------

float thickness(float dist_mm, float base_dist_mm) {
    return fmaxf(0.0f, base_dist_mm - dist_mm);
}

bool onMaterial(float thick_mm, float threshold_mm) {
    return (thick_mm > threshold_mm);
}

// ---------------------------------------------------------------------------
// Stability detector (wrap-safe uint32 arithmetic)
// ---------------------------------------------------------------------------

void stabilityUpdate(StabilityTracker& s, uint32_t now_ms, float value) {
    if (!s.initialized) {
        s.ref         = value;
        s.since_ms    = now_ms;
        s.initialized = true;
    } else if (fabsf(value - s.ref) > s.tol_mm) {
        // Jump detected: restart window, reference stays the original stable value
        s.since_ms  = now_ms;
        // ref is NOT updated (V1.5.5: ref stays the first value of the stable period)
    }
    // else: within tolerance, nothing changes
}

bool stabilityIsStable(const StabilityTracker& s, uint32_t now_ms) {
    if (!s.initialized) {
        return false;
    }
    return (now_ms - s.since_ms) >= s.window_ms;
}

// ---------------------------------------------------------------------------
// Acquisition statistics (sum accumulated in double)
// ---------------------------------------------------------------------------

void statsReset(AcqStats& s) {
    s.sum = 0.0;
    s.n   = 0;
}

void statsAdd(AcqStats& s, float thick_mm, bool on_material, bool valid) {
    if (valid && on_material) {
        s.sum += static_cast<double>(thick_mm);
        ++s.n;
    }
}

float statsMean(const AcqStats& s) {
    if (s.n == 0) {
        return 0.0f;
    }
    return static_cast<float>(s.sum / static_cast<double>(s.n));
}