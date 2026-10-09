// FROZEN TESTS (Contract V1.2) – Sprint S2.3 must make these pass. Do not modify.
// Covers: src/logic/motion_seq.cpp (CONTRACT §8)
#include <unity.h>
#include "logic/motion_seq.h"
#include "../fake_axis.h"

static MotionConfig C;
static MotionSeq S;
static FakeAxis A;
static uint32_t now;

void setUp() { C = MotionConfig{}; S = MotionSeq{}; A = FakeAxis{}; now = 1000; A.phys_steps = mmToSteps(100.0f, C); }
void tearDown() {}

#define API(e, x) TEST_ASSERT_EQUAL_INT((int)(e), (int)(x))
#define OP(e, x)  TEST_ASSERT_EQUAL_INT((int)(e), (int)(x))

// Runs the sequencer for `ms` milliseconds (1 ms cycles). Collects events.
static MotionEvents runFor(uint32_t ms) {
    MotionEvents acc;
    for (uint32_t i = 0; i < ms; ++i) {
        now++;
        A.step(1);
        DrvRequest r = motionTick(S, A.inputs(now), C);
        A.apply(r);
        MotionEvents e = motionTakeEvents(S);
        acc.homing_done |= e.homing_done; acc.homing_failed |= e.homing_failed;
        acc.motor_fault |= e.motor_fault; acc.acq_started |= e.acq_started; acc.acq_ended |= e.acq_ended;
    }
    return acc;
}
static void start(ApiError exp, ApiError got, const DrvRequest& r) { API(exp, got); A.apply(r); }
static void home() {
    DrvRequest r;
    start(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r), r);
    MotionEvents e = runFor(30000);
    TEST_ASSERT_TRUE(e.homing_done);
    TEST_ASSERT_TRUE(S.homed);
}

// ---- conversions ----------------------------------------------------------------
void test_conversions() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 76.190f, C.steps_per_mm);
    TEST_ASSERT_EQUAL_INT32(9143, mmToSteps(120.0f, C));      // 120 * 76.190 = 9142.86
    TEST_ASSERT_EQUAL_INT32(0, mmToSteps(0.0f, C));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 120.0f, stepsToMm(9143, C));
    TEST_ASSERT_EQUAL_UINT32(254, mmPerMinToHz(200.0f, C));   // 200/60*76.19 = 253.97
    TEST_ASSERT_EQUAL_UINT32(762, mmPerMinToHz(600.0f, C));
}

// ---- homing -----------------------------------------------------------------------
void test_homing_start_requests_run_backward() {
    DrvRequest r;
    API(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r));
    OP(DrvOp::RUN_BACKWARD, r.op);
    TEST_ASSERT_EQUAL_UINT32(mmPerMinToHz(C.homing_speed, C), r.speed_hz);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::HOME_SEEK);
    TEST_ASSERT_FALSE(S.homed);
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::HOMING);
}
void test_homing_full_sequence_sets_zero_2mm_off_switch() {
    home();
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    TEST_ASSERT_EQUAL_INT32(0, A.drvPos());
    TEST_ASSERT_FALSE(A.limitBwd());
    TEST_ASSERT_FLOAT_WITHIN(0.1, C.pulloff_mm, A.phys_steps / C.steps_per_mm);
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::IDLE);
}
void test_homing_when_already_on_switch() {
    A.phys_steps = 0;   // switch pressed
    DrvRequest r;
    API(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r));
    OP(DrvOp::ZERO_HERE, r.op);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::HOME_LATCH);
    A.apply(r);
    MotionEvents e = runFor(2000);
    TEST_ASSERT_TRUE(e.homing_done);
    TEST_ASSERT_EQUAL_INT32(0, A.drvPos());
}
void test_homing_timeout() {
    A.block = true;   // axis does not move, switch never reached
    DrvRequest r;
    start(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r), r);
    MotionEvents e = runFor(C.homing_timeout_ms - 5);
    TEST_ASSERT_FALSE(e.homing_failed);
    e = runFor(10);
    TEST_ASSERT_TRUE(e.homing_failed);
    TEST_ASSERT_FALSE(S.homed);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::ERROR);
    TEST_ASSERT_TRUE(A.force_stops >= 1);
}
void test_homing_fwd_limit_fails() {
    DrvRequest r;
    start(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r), r);
    A.phys_steps = A.fwd_limit_steps + 10;   // wiring error: fwd switch while seeking
    MotionEvents e = runFor(1);
    TEST_ASSERT_TRUE(e.homing_failed);
    TEST_ASSERT_FALSE(e.motor_fault);
}
void test_homing_busy_when_not_idle() {
    DrvRequest r;
    start(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r), r);
    API(ApiError::BUSY, motionStartHoming(S, A.inputs(now), C, r));
    OP(DrvOp::NONE, r.op);
}

// ---- move ----------------------------------------------------------------------------
void test_move_requires_homing() {
    DrvRequest r;
    API(ApiError::PRECONDITION, motionStartMove(S, 50, 600, A.inputs(now), C, r));
    OP(DrvOp::NONE, r.op);
}
void test_move_range_and_speed_checks() {
    home();
    DrvRequest r;
    API(ApiError::BAD_PARAM, motionStartMove(S, -0.1f, 600, A.inputs(now), C, r));
    API(ApiError::BAD_PARAM, motionStartMove(S, 248.1f, 600, A.inputs(now), C, r));   // V1.2 axis 0..248
    API(ApiError::BAD_PARAM, motionStartMove(S, 100, 9.9f, A.inputs(now), C, r));
    API(ApiError::BAD_PARAM, motionStartMove(S, 100, 3000.1f, A.inputs(now), C, r));
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    MotionSeq s2 = S;
    API(ApiError::OK, motionStartMove(s2, 248.0f, 3000.0f, A.inputs(now), C, r)); // upper boundary ok
    API(ApiError::OK, motionStartMove(S, 0.0f, 10.0f, A.inputs(now), C, r));     // boundaries ok
}
void test_move_executes_and_returns_idle() {
    home();
    DrvRequest r;
    API(ApiError::OK, motionStartMove(S, 120.0f, 3000.0f, A.inputs(now), C, r));
    OP(DrvOp::MOVE_TO, r.op);
    TEST_ASSERT_EQUAL_INT32(9143, r.target_steps);
    TEST_ASSERT_EQUAL_UINT32(mmPerMinToHz(3000.0f, C), r.speed_hz);
    A.apply(r);
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::MOVING);
    API(ApiError::BUSY, motionStartMove(S, 10.0f, 3000.0f, A.inputs(now), C, r));
    runFor(3000);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    TEST_ASSERT_EQUAL_INT32(9143, A.drvPos());
}
void test_move_settles_even_if_driver_stopped_elsewhere() {
    home();
    DrvRequest r;
    start(ApiError::OK, motionStartMove(S, 120.0f, 3000.0f, A.inputs(now), C, r), r);
    runFor(50);
    A.running = false;   // driver stopped early (e.g. external stop)
    runFor(C.settle_ms + 5);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
}

// ---- traverse --------------------------------------------------------------------------
void test_traverse_param_checks() {
    home();
    DrvRequest r;
    API(ApiError::BAD_PARAM, motionStartTraverse(S, 20.0f, 100, 200, A.inputs(now), C, r));  // start in park zone (<= 20)
    API(ApiError::BAD_PARAM, motionStartTraverse(S, 100, 20.0f, 200, A.inputs(now), C, r));  // end in park zone
    API(ApiError::BAD_PARAM, motionStartTraverse(S, 25, 248.5f, 200, A.inputs(now), C, r));
    API(ApiError::BAD_PARAM, motionStartTraverse(S, 100, 100.5f, 200, A.inputs(now), C, r)); // < 1 mm
    API(ApiError::BAD_PARAM, motionStartTraverse(S, 25, 230, 5, A.inputs(now), C, r));
    TEST_ASSERT_TRUE(S.mode == OpMode::NONE);
    MotionSeq unhomed;
    API(ApiError::PRECONDITION, motionStartTraverse(unhomed, 60, 240, 200, A.inputs(now), C, r));
}
void test_traverse_full_run() {
    home();
    DrvRequest r;
    API(ApiError::OK, motionStartTraverse(S, 25.0f, 75.0f, 600.0f, A.inputs(now), C, r));
    OP(DrvOp::MOVE_TO, r.op);
    TEST_ASSERT_EQUAL_INT32(mmToSteps(25.0f, C), r.target_steps);
    TEST_ASSERT_EQUAL_UINT32(mmPerMinToHz(C.approach_speed, C), r.speed_hz);
    TEST_ASSERT_TRUE(S.mode == OpMode::TRAVERSE);
    A.apply(r);
    MotionEvents e = runFor(1500);   // approach 25 mm at 1500 mm/min ~ 1 s
    TEST_ASSERT_TRUE(e.acq_started);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::TRAV_RUN);
    TEST_ASSERT_TRUE(motionAcquiring(S));
    TEST_ASSERT_EQUAL_DOUBLE(mmPerMinToHz(600.0f, C), A.speed_hz);
    e = runFor(6000);                // 50 mm at 600 mm/min = 5 s
    TEST_ASSERT_TRUE(e.acq_ended);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    TEST_ASSERT_TRUE(S.mode == OpMode::NONE);
    TEST_ASSERT_FALSE(motionAcquiring(S));
    TEST_ASSERT_EQUAL_INT32(mmToSteps(75.0f, C), A.drvPos());
}
void test_traverse_backwards_allowed() {
    home();
    DrvRequest r;
    start(ApiError::OK, motionStartTraverse(S, 80.0f, 30.0f, 1200.0f, A.inputs(now), C, r), r);
    MotionEvents e = runFor(10000);
    TEST_ASSERT_TRUE(e.acq_ended);
    TEST_ASSERT_EQUAL_INT32(mmToSteps(30.0f, C), A.drvPos());
}

// ---- measure -------------------------------------------------------------------------------
void test_measure_holds_until_stop() {
    home();
    DrvRequest r;
    API(ApiError::BAD_PARAM, motionStartMeasure(S, 20.0f, A.inputs(now), C, r));   // park zone boundary
    API(ApiError::BAD_PARAM, motionStartMeasure(S, 248.1f, A.inputs(now), C, r));
    start(ApiError::OK, motionStartMeasure(S, 50.0f, A.inputs(now), C, r), r);
    TEST_ASSERT_TRUE(S.mode == OpMode::MEASURE);
    MotionEvents e = runFor(5000);
    TEST_ASSERT_TRUE(e.acq_started);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::MEAS_HOLD);
    e = runFor(5000);
    TEST_ASSERT_FALSE(e.acq_ended);                 // holds indefinitely
    motionStop(S, r);
    OP(DrvOp::STOP, r.op);
    e = motionTakeEvents(S);
    TEST_ASSERT_TRUE(e.acq_ended);
    TEST_ASSERT_TRUE(S.mode == OpMode::NONE);
    TEST_ASSERT_TRUE(S.homed);                      // controlled stop keeps reference
}

// ---- stop / emergency ------------------------------------------------------------------------
void test_stop_when_idle_is_noop() {
    DrvRequest r; r.op = DrvOp::MOVE_TO;
    motionStop(S, r);
    OP(DrvOp::NONE, r.op);
}
void test_stop_during_homing_leaves_unhomed() {
    DrvRequest r;
    start(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r), r);
    runFor(100);
    motionStop(S, r);
    OP(DrvOp::STOP, r.op);
    TEST_ASSERT_FALSE(S.homed);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
}
void test_emergency_while_moving_loses_reference() {
    home();
    DrvRequest r;
    start(ApiError::OK, motionStartTraverse(S, 25, 200, 600, A.inputs(now), C, r), r);
    runFor(1500);
    TEST_ASSERT_TRUE(motionAcquiring(S));
    motionEmergencyStop(S, A.inputs(now), r);
    OP(DrvOp::FORCE_STOP, r.op);
    TEST_ASSERT_FALSE(S.homed);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    TEST_ASSERT_TRUE(S.mode == OpMode::NONE);
    TEST_ASSERT_TRUE(motionTakeEvents(S).acq_ended);
}
void test_emergency_while_idle_keeps_reference() {
    home();
    DrvRequest r;
    motionEmergencyStop(S, A.inputs(now), r);
    OP(DrvOp::FORCE_STOP, r.op);
    TEST_ASSERT_TRUE(S.homed);
}

// ---- unexpected limits -------------------------------------------------------------------------
void test_fwd_limit_during_move_is_motor_fault() {
    home();
    DrvRequest r;
    start(ApiError::OK, motionStartMove(S, 248.0f, 3000.0f, A.inputs(now), C, r), r);
    A.fwd_limit_steps = (int32_t)(A.phys_steps + mmToSteps(50.0f, C));   // switch earlier than expected
    MotionEvents e = runFor(5000);
    TEST_ASSERT_TRUE(e.motor_fault);
    TEST_ASSERT_FALSE(e.homing_failed);
    TEST_ASSERT_FALSE(S.homed);
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::ERROR);
    TEST_ASSERT_TRUE(A.force_stops >= 1);
}
void test_bwd_limit_during_traverse_is_motor_fault_and_ends_acq() {
    home();
    DrvRequest r;
    start(ApiError::OK, motionStartTraverse(S, 60, 25, 3000, A.inputs(now), C, r), r);
    runFor(3000);                         // approach done, running towards 25 mm
    TEST_ASSERT_TRUE(S.phase == SeqPhase::TRAV_RUN || S.phase == SeqPhase::IDLE);
    S.phase = SeqPhase::TRAV_RUN; S.mode = OpMode::TRAVERSE;   // force acquiring state
    A.running = true; A.phys_steps = -100;  // slipped onto the backward switch
    MotionEvents e = runFor(1);
    TEST_ASSERT_TRUE(e.motor_fault);
    TEST_ASSERT_TRUE(e.acq_ended);
    TEST_ASSERT_TRUE(S.mode == OpMode::NONE);
}
// V1.2: a move to the full axis length must NOT touch the forward switch (at 250 mm)
void test_move_to_axis_max_does_not_hit_forward_switch() {
    home();
    DrvRequest r;
    start(ApiError::OK, motionStartMove(S, C.axis_max_mm, 3000.0f, A.inputs(now), C, r), r);
    MotionEvents e = runFor(8000);
    TEST_ASSERT_FALSE(e.motor_fault);
    TEST_ASSERT_TRUE(S.phase == SeqPhase::IDLE);
    TEST_ASSERT_FALSE(A.limitFwd());
    TEST_ASSERT_EQUAL_INT32(mmToSteps(248.0f, C), A.drvPos());
}
void test_static_limit_without_motion_is_no_fault() {
    home();
    A.phys_steps = A.fwd_limit_steps + 5;  // switch active, but nothing runs
    MotionEvents e = runFor(10);
    TEST_ASSERT_FALSE(e.motor_fault);
}
void test_successful_start_clears_error_latched() {
    A.block = true;
    DrvRequest r;
    start(ApiError::OK, motionStartHoming(S, A.inputs(now), C, r), r);
    runFor(C.homing_timeout_ms + 10);
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::ERROR);
    A.block = false;
    home();
    TEST_ASSERT_TRUE(motionStateOf(S) == MotionState::IDLE);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_conversions);
    RUN_TEST(test_homing_start_requests_run_backward);
    RUN_TEST(test_homing_full_sequence_sets_zero_2mm_off_switch);
    RUN_TEST(test_homing_when_already_on_switch);
    RUN_TEST(test_homing_timeout);
    RUN_TEST(test_homing_fwd_limit_fails);
    RUN_TEST(test_homing_busy_when_not_idle);
    RUN_TEST(test_move_requires_homing);
    RUN_TEST(test_move_range_and_speed_checks);
    RUN_TEST(test_move_executes_and_returns_idle);
    RUN_TEST(test_move_settles_even_if_driver_stopped_elsewhere);
    RUN_TEST(test_traverse_param_checks);
    RUN_TEST(test_traverse_full_run);
    RUN_TEST(test_traverse_backwards_allowed);
    RUN_TEST(test_measure_holds_until_stop);
    RUN_TEST(test_stop_when_idle_is_noop);
    RUN_TEST(test_stop_during_homing_leaves_unhomed);
    RUN_TEST(test_emergency_while_moving_loses_reference);
    RUN_TEST(test_emergency_while_idle_keeps_reference);
    RUN_TEST(test_fwd_limit_during_move_is_motor_fault);
    RUN_TEST(test_bwd_limit_during_traverse_is_motor_fault_and_ends_acq);
    RUN_TEST(test_move_to_axis_max_does_not_hit_forward_switch);
    RUN_TEST(test_static_limit_without_motion_is_no_fault);
    RUN_TEST(test_successful_start_clears_error_latched);
    return UNITY_END();
}
