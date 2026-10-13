// FROZEN TESTS (Contract V1.5.5) – Sprint L2 must make these pass. Do not modify.
// Covers: src/logic/measure_logic.cpp (CONTRACT §7)
#include <unity.h>
#include "logic/measure_logic.h"

static SensorCurve C;
void setUp() { C = SensorCurve{}; }
void tearDown() {}

// ---- sensor curve (millimetre, NOT centimetre) ------------------------------
// V1.4.2: default curve fitted to the demo hardware (smoketest 0a): 168.8 * V^-1.398
void test_curve_1v_is_168_8_mm() { TEST_ASSERT_FLOAT_WITHIN(0.05f, 168.8f, sensorMvToMm(1000.0f, C)); }
void test_curve_2v() { TEST_ASSERT_FLOAT_WITHIN(0.2f, 64.05f, sensorMvToMm(2000.0f, C)); }
void test_curve_matches_measured_demo_points() {
    // measured on the demo axis (smoketest 0a 2026-10-11, step centres 115..215 mm,
    // 11 kOhm load at the sensor output): volt -> distance in mm
    const float v[] = {1.972f, 2.091f, 2.236f, 2.403f, 2.588f, 2.777f};
    const float d[] = {65.0f, 60.0f, 55.0f, 50.0f, 45.0f, 40.0f};
    for (int i = 0; i < 6; ++i) TEST_ASSERT_FLOAT_WITHIN(1.0f, d[i], sensorMvToMm(v[i] * 1000.0f, C));
}
void test_curve_far_end_0v4_out_of_range() {
    float mm = sensorMvToMm(400.0f, C);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 607.70f, mm);
    TEST_ASSERT_FALSE(sensorInRange(mm, C));
}
void test_curve_monotonic_decreasing() {
    float prev = sensorMvToMm(300.0f, C);
    for (int mv = 350; mv <= 3100; mv += 50) {
        float d = sensorMvToMm((float)mv, C);
        TEST_ASSERT_TRUE(d < prev);
        prev = d;
    }
}
void test_curve_no_signal() {
    TEST_ASSERT_EQUAL_FLOAT(SENSOR_NO_SIGNAL_MM, sensorMvToMm(0.0f, C));
    TEST_ASSERT_EQUAL_FLOAT(SENSOR_NO_SIGNAL_MM, sensorMvToMm(1.0f, C));
    TEST_ASSERT_EQUAL_FLOAT(SENSOR_NO_SIGNAL_MM, sensorMvToMm(-5.0f, C));
}
void test_range() {
    TEST_ASSERT_TRUE(sensorInRange(35.0f, C));     // V1.4.2: 35 mm (was 40)
    TEST_ASSERT_TRUE(sensorInRange(40.0f, C));
    TEST_ASSERT_TRUE(sensorInRange(300.0f, C));
    TEST_ASSERT_FALSE(sensorInRange(34.9f, C));
    TEST_ASSERT_FALSE(sensorInRange(300.1f, C));
    TEST_ASSERT_FALSE(sensorInRange(SENSOR_NO_SIGNAL_MM, C));
}

// ---- median mean -------------------------------------------------------------
void test_median_constant() {
    uint16_t s[16]; for (int i = 0; i < 16; ++i) s[i] = 1500;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1500.0f, medianMean(s, 16, 4));
}
void test_median_rejects_outliers() {
    uint16_t s[16]; for (int i = 0; i < 16; ++i) s[i] = 2000;
    s[0] = 0; s[3] = 3300; s[7] = 0; s[15] = 3300;   // 4 outliers, 2 per side
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2000.0f, medianMean(s, 16, 4));
}
void test_median_mean_of_middle() {
    uint16_t s[16] = {16, 1, 15, 2, 14, 3, 13, 4, 12, 5, 11, 6, 10, 7, 9, 8};
    // sorted 1..16, discard 1-4 and 13-16 -> mean of 5..12 = 8.5
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.5f, medianMean(s, 16, 4));
    TEST_ASSERT_EQUAL_UINT16(1, s[0]);    // sorted in place
    TEST_ASSERT_EQUAL_UINT16(16, s[15]);
}
void test_median_degenerate() {
    uint16_t s[8] = {5, 4, 3, 2, 1, 9, 9, 9};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, medianMean(s, 8, 4));
    TEST_ASSERT_EQUAL_UINT16(5, s[0]);    // unchanged
    TEST_ASSERT_EQUAL_FLOAT(0.0f, medianMean(s, 0, 0));
}

// ---- IIR ----------------------------------------------------------------------
void test_iir_first_sample_initialises() {
    Iir f; TEST_ASSERT_EQUAL_FLOAT(100.0f, iirUpdate(f, 100.0f));
}
void test_iir_weighting() {
    Iir f; iirUpdate(f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f, iirUpdate(f, 500.0f));  // 0.8*100 + 0.2*500
}
void test_iir_converges() {
    Iir f; iirUpdate(f, 0.0f);
    float y = 0; for (int i = 0; i < 50; ++i) y = iirUpdate(f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, y);
}
void test_iir_reset() {
    Iir f; iirUpdate(f, 10.0f); iirUpdate(f, 10.0f);
    iirReset(f);
    TEST_ASSERT_EQUAL_FLOAT(77.0f, iirUpdate(f, 77.0f));
}

// ---- calibration ------------------------------------------------------------------
void test_calib_default_identity() {
    CalibPoints p;
    TEST_ASSERT_TRUE(calibValid(p));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.4f, calibApply(123.4f, p));
}
void test_calib_defaults_are_demo_identity() {
    CalibPoints p;
    TEST_ASSERT_EQUAL_FLOAT(40.0f, p.near_ref); TEST_ASSERT_EQUAL_FLOAT(40.0f, p.near_raw);
    TEST_ASSERT_EQUAL_FLOAT(65.0f, p.far_ref);  TEST_ASSERT_EQUAL_FLOAT(65.0f, p.far_raw);
}
void test_calib_offset() {
    CalibPoints p; p.near_raw = 52; p.near_ref = 50; p.far_raw = 202; p.far_ref = 200;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, calibApply(102.0f, p));
}
void test_calib_scale_and_points() {
    CalibPoints p; p.near_raw = 60; p.near_ref = 50; p.far_raw = 180; p.far_ref = 200;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, calibApply(60.0f, p));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, calibApply(180.0f, p));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 125.0f, calibApply(120.0f, p));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, calibApply(40.0f, p));   // extrapolation
}
void test_calib_invalid_cases() {
    CalibPoints p;
    p.near_raw = 100; p.far_raw = 103;                 // raw span < 5
    TEST_ASSERT_FALSE(calibValid(p));
    TEST_ASSERT_EQUAL_FLOAT(77.0f, calibApply(77.0f, p));
    p = CalibPoints{}; p.far_ref = 43;                 // ref span < 5
    TEST_ASSERT_FALSE(calibValid(p));
    p = CalibPoints{}; p.near_ref = 30;                // ref out of range
    TEST_ASSERT_FALSE(calibValid(p));
    p = CalibPoints{}; p.far_ref = 301;
    TEST_ASSERT_FALSE(calibValid(p));
}
void test_calib_reversed_points_valid() {
    CalibPoints p; p.near_raw = 200; p.near_ref = 200; p.far_raw = 50; p.far_ref = 50;
    TEST_ASSERT_TRUE(calibValid(p));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, calibApply(100.0f, p));
}

// ---- thickness / material ----------------------------------------------------------
void test_thickness() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.5f, thickness(187.5f, 200.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, thickness(205.0f, 200.0f));  // never negative
}
void test_on_material_strict() {
    TEST_ASSERT_TRUE(onMaterial(3.1f, 3.0f));
    TEST_ASSERT_FALSE(onMaterial(3.0f, 3.0f));
    TEST_ASSERT_FALSE(onMaterial(0.0f, 3.0f));
}

// ---- stability ---------------------------------------------------------------------
void test_stability_needs_window() {
    StabilityTracker s;
    stabilityUpdate(s, 1000, 100.0f);
    TEST_ASSERT_FALSE(stabilityIsStable(s, 1000));
    stabilityUpdate(s, 1200, 100.3f);
    TEST_ASSERT_FALSE(stabilityIsStable(s, 1499));
    TEST_ASSERT_TRUE(stabilityIsStable(s, 1500));
}
void test_stability_restarts_on_jump() {
    StabilityTracker s;
    stabilityUpdate(s, 0, 100.0f);
    stabilityUpdate(s, 400, 100.6f);   // > 0.5 -> restart at 400
    TEST_ASSERT_FALSE(stabilityIsStable(s, 800));
    TEST_ASSERT_TRUE(stabilityIsStable(s, 900));
}
void test_stability_uninitialised() {
    StabilityTracker s;
    TEST_ASSERT_FALSE(stabilityIsStable(s, 100000));
}
void test_stability_wraparound() {
    StabilityTracker s;
    stabilityUpdate(s, 0xFFFFFF00u, 50.0f);
    TEST_ASSERT_TRUE(stabilityIsStable(s, 0x00000200u));   // 768 ms later
}

// ---- acquisition statistics ---------------------------------------------------------
void test_stats_only_valid_on_material() {
    AcqStats a;
    statsAdd(a, 10.0f, true, true);
    statsAdd(a, 20.0f, true, true);
    statsAdd(a, 99.0f, false, true);   // gap
    statsAdd(a, 99.0f, true, false);   // shutter closed
    TEST_ASSERT_EQUAL_UINT32(2, a.n);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, statsMean(a));
}
void test_stats_reset_and_empty() {
    AcqStats a;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, statsMean(a));
    statsAdd(a, 5.0f, true, true);
    statsReset(a);
    TEST_ASSERT_EQUAL_UINT32(0, a.n);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, statsMean(a));
}

// ---- V1.5.5: every parameter of the structs is used (no hard-coded defaults) ----------
void test_curve_uses_struct_fields() {
    SensorCurve c; c.k_mm = 100.0f; c.exponent = -1.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, sensorMvToMm(2000.0f, c));   // 100 * 2^-1
    c.exponent = -2.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, sensorMvToMm(2000.0f, c));   // 100 * 2^-2
}
void test_range_uses_struct_fields() {
    SensorCurve c; c.min_mm = 50.0f; c.max_mm = 60.0f;
    TEST_ASSERT_FALSE(sensorInRange(49.9f, c));
    TEST_ASSERT_TRUE(sensorInRange(50.0f, c));
    TEST_ASSERT_TRUE(sensorInRange(60.0f, c));
    TEST_ASSERT_FALSE(sensorInRange(60.1f, c));
    TEST_ASSERT_FALSE(sensorInRange(40.0f, c));
}
void test_median_other_sizes() {
    uint16_t a[5] = {50, 10, 40, 20, 30};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, medianMean(a, 5, 1));       // mean of 20,30,40
    uint16_t b[4] = {4, 1, 3, 2};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, medianMean(b, 4, 0));        // no discard
    uint16_t c[3] = {7, 9, 8};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, medianMean(c, 3, 1));        // n = 2*discard + 1
}
void test_median_large_values_no_overflow() {
    uint16_t s[16]; for (int i = 0; i < 16; ++i) s[i] = 60000;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 60000.0f, medianMean(s, 16, 4));
}
void test_iir_uses_alpha() {
    Iir f; f.alpha = 0.5f;
    iirUpdate(f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.0f, iirUpdate(f, 500.0f));    // 0.5*100 + 0.5*500
    Iir g; g.alpha = 0.0f;
    iirUpdate(g, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, iirUpdate(g, 7.0f));        // no filtering
}
void test_calib_boundaries() {
    CalibPoints p; p.near_raw = 100; p.far_raw = 105; p.near_ref = 100; p.far_ref = 105;
    TEST_ASSERT_TRUE(calibValid(p));                     // spans exactly 5 mm are valid
    p.far_raw = 104.9f;
    TEST_ASSERT_FALSE(calibValid(p));
    p = CalibPoints{}; p.near_ref = 40; p.far_ref = 300; p.near_raw = 40; p.far_raw = 300;
    TEST_ASSERT_TRUE(calibValid(p));                     // refs 40 and 300 inclusive
    p.far_ref = 300.1f;
    TEST_ASSERT_FALSE(calibValid(p));
}
void test_stability_uses_struct_fields() {
    StabilityTracker s; s.window_ms = 200; s.tol_mm = 2.0f;
    stabilityUpdate(s, 0, 100.0f);
    stabilityUpdate(s, 100, 101.9f);                     // within 2.0 -> no restart
    TEST_ASSERT_TRUE(stabilityIsStable(s, 200));
    stabilityUpdate(s, 250, 102.1f);                     // 2.1 from the reference -> restart
    TEST_ASSERT_FALSE(stabilityIsStable(s, 449));
    TEST_ASSERT_TRUE(stabilityIsStable(s, 450));
}
void test_stability_tolerance_is_inclusive() {
    StabilityTracker s;
    stabilityUpdate(s, 0, 100.0f);
    stabilityUpdate(s, 300, 100.5f);                     // exactly tol -> no restart
    TEST_ASSERT_TRUE(stabilityIsStable(s, 500));
}
void test_stability_reference_does_not_drift() {
    StabilityTracker s;
    stabilityUpdate(s, 0, 100.0f);
    stabilityUpdate(s, 300, 100.4f);                     // within tol of the reference 100.0
    stabilityUpdate(s, 450, 100.8f);                     // 0.8 from the reference -> restart
    TEST_ASSERT_FALSE(stabilityIsStable(s, 800));
    TEST_ASSERT_TRUE(stabilityIsStable(s, 950));
}
void test_stats_sum_in_double() {
    AcqStats a;
    statsAdd(a, 2.0e7f, true, true);
    for (int i = 0; i < 10; ++i) statsAdd(a, 1.0f, true, true);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, (2.0e7f + 10.0f) / 11.0f, statsMean(a));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_curve_1v_is_168_8_mm);
    RUN_TEST(test_curve_2v);
    RUN_TEST(test_curve_matches_measured_demo_points);
    RUN_TEST(test_curve_far_end_0v4_out_of_range);
    RUN_TEST(test_curve_monotonic_decreasing);
    RUN_TEST(test_curve_no_signal);
    RUN_TEST(test_range);
    RUN_TEST(test_median_constant);
    RUN_TEST(test_median_rejects_outliers);
    RUN_TEST(test_median_mean_of_middle);
    RUN_TEST(test_median_degenerate);
    RUN_TEST(test_iir_first_sample_initialises);
    RUN_TEST(test_iir_weighting);
    RUN_TEST(test_iir_converges);
    RUN_TEST(test_iir_reset);
    RUN_TEST(test_calib_default_identity);
    RUN_TEST(test_calib_defaults_are_demo_identity);
    RUN_TEST(test_calib_offset);
    RUN_TEST(test_calib_scale_and_points);
    RUN_TEST(test_calib_invalid_cases);
    RUN_TEST(test_calib_reversed_points_valid);
    RUN_TEST(test_thickness);
    RUN_TEST(test_on_material_strict);
    RUN_TEST(test_stability_needs_window);
    RUN_TEST(test_stability_restarts_on_jump);
    RUN_TEST(test_stability_uninitialised);
    RUN_TEST(test_stability_wraparound);
    RUN_TEST(test_stats_only_valid_on_material);
    RUN_TEST(test_stats_reset_and_empty);
    RUN_TEST(test_curve_uses_struct_fields);
    RUN_TEST(test_range_uses_struct_fields);
    RUN_TEST(test_median_other_sizes);
    RUN_TEST(test_median_large_values_no_overflow);
    RUN_TEST(test_iir_uses_alpha);
    RUN_TEST(test_calib_boundaries);
    RUN_TEST(test_stability_uses_struct_fields);
    RUN_TEST(test_stability_tolerance_is_inclusive);
    RUN_TEST(test_stability_reference_does_not_drift);
    RUN_TEST(test_stats_sum_in_double);
    return UNITY_END();
}
