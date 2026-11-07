// FROZEN TESTS (Contract V1.5.6, calibration V1.6.0 / Sprint K1) – Sprint L2 must make these pass. Do not modify.
// Covers: src/logic/measure_logic.cpp (CONTRACT §7)
#include <unity.h>
#include <math.h>
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

// ---- calibration table (V1.6.0, 6 points) -----------------------------------------
static CalibTable tbl(const float* raw, const float* ref) {
    CalibTable t;
    for (int i = 0; i < CALIB_POINTS; ++i) { t.raw[i] = raw[i]; t.ref[i] = ref[i]; }
    return t;
}
// measured residual shape of the demo sensor (A1, 07.11.2026): raw readings at the six steps
static const float RAW_DEMO[6] = {65.3f, 60.2f, 54.8f, 49.6f, 44.7f, 40.5f};
static const float REF_DEMO[6] = {65.0f, 60.0f, 55.0f, 50.0f, 45.0f, 40.0f};

void test_calib_points_constant() {
    TEST_ASSERT_EQUAL_INT(6, CALIB_POINTS);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, CALIB_MIN_STEP_MM);
}
void test_calib_defaults_are_demo_identity() {
    CalibTable t;
    const float d[6] = {65.0f, 60.0f, 55.0f, 50.0f, 45.0f, 40.0f};
    for (int i = 0; i < 6; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(d[i], t.raw[i]);
        TEST_ASSERT_EQUAL_FLOAT(d[i], t.ref[i]);
    }
    TEST_ASSERT_TRUE(calibValid(t));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.4f, calibApply(123.4f, t));   // identity, also outside the table
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 52.5f, calibApply(52.5f, t));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, calibApply(10.0f, t));
}
void test_calib_hits_every_point_exactly() {
    CalibTable t = tbl(RAW_DEMO, REF_DEMO);
    TEST_ASSERT_TRUE(calibValid(t));
    for (int i = 0; i < 6; ++i) TEST_ASSERT_FLOAT_WITHIN(0.001f, REF_DEMO[i], calibApply(RAW_DEMO[i], t));
}
void test_calib_interpolates_each_segment() {
    CalibTable t = tbl(RAW_DEMO, REF_DEMO);
    // midpoint of every raw segment maps to the midpoint of the ref segment
    for (int i = 0; i < 5; ++i) {
        float rm = 0.5f * (RAW_DEMO[i] + RAW_DEMO[i + 1]);
        float em = 0.5f * (REF_DEMO[i] + REF_DEMO[i + 1]);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, em, calibApply(rm, t));
    }
    // a quarter into segment 49.6..54.8 (ref 50..55)
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 51.25f, calibApply(49.6f + 0.25f * 5.2f, t));
}
void test_calib_extrapolates_with_end_segments() {
    CalibTable t = tbl(RAW_DEMO, REF_DEMO);
    // below: segment 40.5..44.7 -> 40..45, slope 5/4.2
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f - 4.5f * 5.0f / 4.2f, calibApply(36.0f, t));
    // above: segment 60.2..65.3 -> 60..65, slope 5/5.1
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 65.0f + 34.7f * 5.0f / 5.1f, calibApply(100.0f, t));
    // the lower boom beside the material (about 186 mm) stays plausible
    float boom = calibApply(186.0f, t);
    TEST_ASSERT_TRUE(boom > 170.0f && boom < 200.0f);
}
void test_calib_order_of_points_irrelevant() {
    // the same table stored in another order gives the same result (CALIB point i can be any step)
    const float raw[6] = {49.6f, 65.3f, 40.5f, 54.8f, 44.7f, 60.2f};
    const float ref[6] = {50.0f, 65.0f, 40.0f, 55.0f, 45.0f, 60.0f};
    CalibTable a = tbl(RAW_DEMO, REF_DEMO), b = tbl(raw, ref);
    TEST_ASSERT_TRUE(calibValid(b));
    const float xs[] = {30.0f, 40.5f, 42.0f, 47.3f, 52.0f, 57.7f, 63.0f, 70.0f, 186.0f};
    for (float x : xs) TEST_ASSERT_FLOAT_WITHIN(0.001f, calibApply(x, a), calibApply(x, b));
}
void test_calib_ascending_storage_order() {
    // stored from near to far (ascending): same result as the default far-to-near order
    float raw[6], ref[6];
    for (int i = 0; i < 6; ++i) { raw[i] = RAW_DEMO[5 - i]; ref[i] = REF_DEMO[5 - i]; }
    CalibTable a = tbl(RAW_DEMO, REF_DEMO), b = tbl(raw, ref);
    TEST_ASSERT_TRUE(calibValid(b));
    for (float x = 35.0f; x < 70.0f; x += 0.7f) TEST_ASSERT_FLOAT_WITHIN(0.001f, calibApply(x, a), calibApply(x, b));
}
void test_calib_invalid_cases() {
    CalibTable t = tbl(RAW_DEMO, REF_DEMO);
    t.raw[2] = 60.0f;                                   // raw of ref 55 only 0.2 mm below raw of ref 60 (< 1 mm)
    TEST_ASSERT_FALSE(calibValid(t));
    TEST_ASSERT_EQUAL_FLOAT(77.0f, calibApply(77.0f, t));   // invalid -> raw passes through
    t = tbl(RAW_DEMO, REF_DEMO); t.raw[1] = 66.0f;      // ref 60 has a larger raw than ref 65 -> falling
    TEST_ASSERT_FALSE(calibValid(t));
    TEST_ASSERT_EQUAL_FLOAT(52.0f, calibApply(52.0f, t));
    t = tbl(RAW_DEMO, REF_DEMO); t.ref[3] = 55.0f;      // two equal refs
    TEST_ASSERT_FALSE(calibValid(t));
    t = tbl(RAW_DEMO, REF_DEMO); t.ref[5] = 39.9f;      // ref out of range
    TEST_ASSERT_FALSE(calibValid(t));
    t = CalibTable{}; t.ref[0] = 300.1f;
    TEST_ASSERT_FALSE(calibValid(t));
}
void test_calib_nan_and_inf_invalid() {
    const float bad[] = {NAN, INFINITY, -INFINITY};
    for (float v : bad) {
        for (int i = 0; i < 6; ++i) {
            CalibTable t; t.raw[i] = v;
            TEST_ASSERT_FALSE(calibValid(t));
            TEST_ASSERT_EQUAL_FLOAT(51.0f, calibApply(51.0f, t));
            t = CalibTable{}; t.ref[i] = v;
            TEST_ASSERT_FALSE(calibValid(t));
        }
    }
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
    // steps of exactly 1.0 mm (refs and raws) are valid, 0.99 mm is not
    const float raw[6] = {50.0f, 49.0f, 48.0f, 47.0f, 46.0f, 45.0f};
    const float ref[6] = {50.0f, 49.0f, 48.0f, 47.0f, 46.0f, 45.0f};
    CalibTable t = tbl(raw, ref);
    TEST_ASSERT_TRUE(calibValid(t));
    t.raw[1] = 49.01f;                                   // raw step 50 - 49.01 = 0.99
    TEST_ASSERT_FALSE(calibValid(t));
    t = tbl(raw, ref); t.ref[1] = 49.01f;                // ref step 0.99
    TEST_ASSERT_FALSE(calibValid(t));
    // refs 40 and 300 inclusive
    const float r2[6] = {300.0f, 250.0f, 200.0f, 150.0f, 100.0f, 40.0f};
    t = tbl(r2, r2);
    TEST_ASSERT_TRUE(calibValid(t));
    t.ref[5] = 39.99f;
    TEST_ASSERT_FALSE(calibValid(t));
}
void test_calib_fixes_demo_bow() {
    // V1.6.0 reason: the 2-point fit left up to 0.87 mm; the table brings every step within 0.01 mm
    // and a value between two steps stays within the linear interpolation
    CalibTable t = tbl(RAW_DEMO, REF_DEMO);
    for (int i = 0; i < 6; ++i) TEST_ASSERT_FLOAT_WITHIN(0.01f, REF_DEMO[i], calibApply(RAW_DEMO[i], t));
    float y = calibApply(57.5f, t);
    TEST_ASSERT_TRUE(y > 55.0f && y < 60.0f);
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

// V1.5.6: after a jump the tracker settles at the NEW level (ref = value on restart)
void test_stability_settles_at_new_level_after_jump() {
    StabilityTracker s;
    stabilityUpdate(s, 0, 100.0f);
    stabilityUpdate(s, 200, 110.0f);                     // jump -> restart at 110
    for (uint32_t t = 220; t <= 800; t += 20) stabilityUpdate(s, t, 110.1f);   // new level
    TEST_ASSERT_TRUE(stabilityIsStable(s, 700));         // 500 ms after the jump
    TEST_ASSERT_TRUE(stabilityIsStable(s, 800));
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
    RUN_TEST(test_calib_points_constant);
    RUN_TEST(test_calib_defaults_are_demo_identity);
    RUN_TEST(test_calib_hits_every_point_exactly);
    RUN_TEST(test_calib_interpolates_each_segment);
    RUN_TEST(test_calib_extrapolates_with_end_segments);
    RUN_TEST(test_calib_order_of_points_irrelevant);
    RUN_TEST(test_calib_ascending_storage_order);
    RUN_TEST(test_calib_invalid_cases);
    RUN_TEST(test_calib_nan_and_inf_invalid);
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
    RUN_TEST(test_calib_fixes_demo_bow);
    RUN_TEST(test_stability_uses_struct_fields);
    RUN_TEST(test_stability_tolerance_is_inclusive);
    RUN_TEST(test_stability_reference_does_not_drift);
    RUN_TEST(test_stats_sum_in_double);
    RUN_TEST(test_stability_settles_at_new_level_after_jump);
    return UNITY_END();
}
