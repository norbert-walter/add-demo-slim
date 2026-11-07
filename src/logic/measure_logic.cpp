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
// 6-point calibration table (V1.6.0, Sprint K1)
// ---------------------------------------------------------------------------

/// Insertion-sort indices so that t.ref[idx[i]] is ascending.
static void sort_idx_by_ref(const CalibTable& t, int* idx) {
    for (int i = 0; i < CALIB_POINTS; ++i) idx[i] = i;
    for (int i = 1; i < CALIB_POINTS; ++i) {
        const int key = idx[i];
        const float kref = t.ref[key];
        int j = i - 1;
        while (j >= 0 && t.ref[idx[j]] > kref) {
            idx[j + 1] = idx[j];
            --j;
        }
        idx[j + 1] = key;
    }
}

/// Insertion-sort indices so that t.raw[idx[i]] is ascending.
static void sort_idx_by_raw(const CalibTable& t, int* idx) {
    for (int i = 0; i < CALIB_POINTS; ++i) idx[i] = i;
    for (int i = 1; i < CALIB_POINTS; ++i) {
        const int key = idx[i];
        const float kraw = t.raw[key];
        int j = i - 1;
        while (j >= 0 && t.raw[idx[j]] > kraw) {
            idx[j + 1] = idx[j];
            --j;
        }
        idx[j + 1] = key;
    }
}

bool calibValid(const CalibTable& t) {
    int idx[CALIB_POINTS];

    // All values finite, refs in [40..300]
    for (int i = 0; i < CALIB_POINTS; ++i) {
        if (!std::isfinite(t.raw[i]) || !std::isfinite(t.ref[i])) return false;
        if (t.ref[i] < 40.0f || t.ref[i] > 300.0f) return false;
    }

    // Sort by ref ascending and check neighbours
    sort_idx_by_ref(t, idx);
    for (int i = 1; i < CALIB_POINTS; ++i) {
        const int prev = idx[i - 1];
        const int curr = idx[i];
        if (t.ref[curr] <= t.ref[prev]) return false;          // refs must be strictly increasing
        if ((t.ref[curr] - t.ref[prev]) < CALIB_MIN_STEP_MM)  return false;
        if ((t.raw[curr] - t.raw[prev]) < CALIB_MIN_STEP_MM)  return false;
    }
    return true;
}

float calibApply(float raw_mm, const CalibTable& t) {
    int idx[CALIB_POINTS];
    sort_idx_by_raw(t, idx);

    if (!calibValid(t)) return raw_mm;          // invalid → pass through

    // --- below first point: extrapolate with first segment ----------------
    if (raw_mm <= t.raw[idx[0]]) {
        const float r0 = t.raw[idx[0]], r1 = t.raw[idx[1]];
        const float d0 = t.ref[idx[0]], d1 = t.ref[idx[1]];
        return d0 + (d1 - d0) * (raw_mm - r0) / (r1 - r0);
    }

    // --- between points: find segment and interpolate ---------------------
    for (int i = 0; i < CALIB_POINTS - 1; ++i) {
        const int prev = idx[i];
        const int curr = idx[i + 1];
        const float rp = t.raw[prev], rc = t.raw[curr];
        if (raw_mm <= rc) {
            const float dp = t.ref[prev], dc = t.ref[curr];
            return dp + (dc - dp) * (raw_mm - rp) / (rc - rp);
        }
    }

    // --- above last point: extrapolate with last segment ------------------
    const int prev = idx[CALIB_POINTS - 2];
    const int curr = idx[CALIB_POINTS - 1];
    return t.ref[curr] + (t.ref[curr] - t.ref[prev]) * (raw_mm - t.raw[curr]) / (t.raw[curr] - t.raw[prev]);
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
        // Jump detected: restart window, both ref and since reset to current reading
        s.ref         = value;
        s.since_ms    = now_ms;
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