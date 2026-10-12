// FROZEN TESTS (Contract V1.5.3) – Sprint S2.1 must make these pass. Do not modify.
// Covers: src/logic/safety_logic.cpp (CONTRACT §6). Rules R1..R7 refer to safety_logic.h.
#include <unity.h>
#include <stdio.h>
#include "logic/safety_logic.h"

static SafetyConfig CFG;

void setUp() { CFG = SafetyConfig{}; }
void tearDown() {}

// ---- helpers -----------------------------------------------------------------
static SafetyInputs good() {
    SafetyInputs in;
    in.estop = false; in.interlock = true; in.cooling_flow = true; in.temp_c = 18.0f;
    in.motor_fault = false; in.comm_timeout = false; in.pos_mm = 100.0f; in.homed = true;
    return in;
}
static SafetyStatus st(SafetyState s, bool hv = false, bool sh = false, ErrorCode e = ErrorCode::NONE,
                       bool au = false) {
    SafetyStatus x; x.state = s; x.hv_on = hv; x.shutter_open = sh; x.err = e; x.shutter_auto = au; return x;
}
// V1.5: shutter opened by the automation (RUNNING, HV on, shutter open, shutter_auto)
static SafetyStatus stAuto() { return st(SafetyState::RUNNING, true, true, ErrorCode::NONE, true); }
// shorthand for the V1.5 automation call
static SafetyEvent automation(bool acquiring, bool active, uint32_t idle_ms, uint32_t hold_ms,
                              const SafetyStatus& s, float pos) {
    return safetyShutterAutomation(acquiring, active, idle_ms, hold_ms, s, pos, CFG);
}
static SafetyResult step(const SafetyStatus& s, SafetyEvent ev, const SafetyInputs& in) {
    return safetyStep(s, ev, in, CFG);
}
#define ASSERT_STATE(exp, act) TEST_ASSERT_EQUAL_INT((int)(exp), (int)(act))
#define ASSERT_API(exp, act)   TEST_ASSERT_EQUAL_INT((int)(exp), (int)(act))

static void checkInvariants(const SafetyStatus& s, const SafetyInputs& in) {
    if (s.shutter_open) TEST_ASSERT_TRUE_MESSAGE(s.hv_on, "I1 shutter_open => hv_on");
    if (s.hv_on) TEST_ASSERT_TRUE_MESSAGE(s.state == SafetyState::RUNNING, "I2 hv_on => RUNNING");
    if (s.state == SafetyState::FAULT || s.state == SafetyState::WAIT_ACK || s.state == SafetyState::RECOVER)
        TEST_ASSERT_TRUE_MESSAGE(!s.hv_on && !s.shutter_open, "I3 outputs off in fault states");
    bool errSet = s.err != ErrorCode::NONE;
    bool faultState = s.state == SafetyState::FAULT || s.state == SafetyState::WAIT_ACK;
    TEST_ASSERT_TRUE_MESSAGE(errSet == faultState, "I4 err <=> FAULT/WAIT_ACK");
    if (s.shutter_open) TEST_ASSERT_TRUE_MESSAGE(in.pos_mm > CFG.park_pos_mm, "I5 shutter only outside park");
    if (s.hv_on) TEST_ASSERT_TRUE_MESSAGE(safetyHvConditionsOk(in, CFG), "I6 hv_on => conditions ok");
    if (s.shutter_auto) TEST_ASSERT_TRUE_MESSAGE(s.shutter_open, "I7 shutter_auto => shutter_open");
    if (s.shutter_open && s.shutter_auto)
        TEST_ASSERT_TRUE_MESSAGE(in.pos_mm > CFG.maint_max_mm, "I8 automatic shutter only in the measuring zone");
}

// ---- basic functions -------------------------------------------------------------
void test_initial_status() {
    SafetyStatus s = safetyInitialStatus();
    ASSERT_STATE(SafetyState::SAFE_START, s.state);
    TEST_ASSERT_FALSE(s.hv_on); TEST_ASSERT_FALSE(s.shutter_open);
    ASSERT_STATE(ErrorCode::NONE, s.err);
}
void test_hv_conditions_and_logic() {
    SafetyInputs in = good();
    TEST_ASSERT_TRUE(safetyHvConditionsOk(in, CFG));
    in = good(); in.cooling_flow = false; TEST_ASSERT_FALSE(safetyHvConditionsOk(in, CFG));
    in = good(); in.interlock = false;    TEST_ASSERT_FALSE(safetyHvConditionsOk(in, CFG));
    in = good(); in.estop = true;         TEST_ASSERT_FALSE(safetyHvConditionsOk(in, CFG));
    in = good(); in.temp_c = 30.0f;       TEST_ASSERT_TRUE(safetyHvConditionsOk(in, CFG));   // boundary inclusive
    in = good(); in.temp_c = 30.1f;       TEST_ASSERT_FALSE(safetyHvConditionsOk(in, CFG));
}
void test_hv_conditions_do_not_depend_on_motion_or_comm() {
    SafetyInputs in = good(); in.pos_mm = 0.0f; in.homed = false; in.motor_fault = true; in.comm_timeout = true;
    TEST_ASSERT_TRUE(safetyHvConditionsOk(in, CFG));
}
void test_fault_cause_priority() {
    SafetyInputs in = good();
    ASSERT_STATE(ErrorCode::NONE, safetyFaultCause(in, CFG));
    in.comm_timeout = true;  ASSERT_STATE(ErrorCode::COMM_TIMEOUT, safetyFaultCause(in, CFG));
    in.motor_fault = true;   ASSERT_STATE(ErrorCode::MOTOR_FAULT, safetyFaultCause(in, CFG));
    in.temp_c = 32.1f;       ASSERT_STATE(ErrorCode::TEMP_HIGH, safetyFaultCause(in, CFG));
    in.estop = true;         ASSERT_STATE(ErrorCode::ESTOP, safetyFaultCause(in, CFG));
}
void test_temp_thresholds() {
    SafetyInputs in = good();
    in.temp_c = 32.0f; ASSERT_STATE(ErrorCode::NONE, safetyFaultCause(in, CFG));   // 32.0 is not > 32
    in.temp_c = 32.1f; ASSERT_STATE(ErrorCode::TEMP_HIGH, safetyFaultCause(in, CFG));
    in.temp_c = 31.0f; TEST_ASSERT_FALSE(safetyHvConditionsOk(in, CFG));            // warn band: no HV, no fault
}
void test_fault_cause_ignores_flags() {
    SafetyInputs in = good(); in.interlock = false; in.cooling_flow = false;
    ASSERT_STATE(ErrorCode::NONE, safetyFaultCause(in, CFG));   // missing flags are not faults
}

// ---- R5 SAFE_START -------------------------------------------------------------------
void test_r5_safe_start_init_done_to_ready() {
    SafetyResult r = step(st(SafetyState::SAFE_START), SafetyEvent::INIT_DONE, good());
    ASSERT_STATE(SafetyState::READY, r.next.state); ASSERT_API(ApiError::OK, r.api);
    TEST_ASSERT_FALSE(r.motor_stop);
}
void test_r5_safe_start_rejects_hv_shutter_ack() {
    ASSERT_API(ApiError::STATE, step(st(SafetyState::SAFE_START), SafetyEvent::HV_ON, good()).api);
    ASSERT_API(ApiError::STATE, step(st(SafetyState::SAFE_START), SafetyEvent::SHUTTER_OPEN, good()).api);
    ASSERT_API(ApiError::STATE, step(st(SafetyState::SAFE_START), SafetyEvent::ACK, good()).api);
    SafetyResult r = step(st(SafetyState::SAFE_START), SafetyEvent::HV_ON, good());
    ASSERT_STATE(SafetyState::SAFE_START, r.next.state); TEST_ASSERT_FALSE(r.next.hv_on);
}
void test_r5_safe_start_benign_events_ok() {
    ASSERT_API(ApiError::OK, step(st(SafetyState::SAFE_START), SafetyEvent::TICK, good()).api);
    ASSERT_API(ApiError::OK, step(st(SafetyState::SAFE_START), SafetyEvent::HV_OFF, good()).api);
    ASSERT_API(ApiError::OK, step(st(SafetyState::SAFE_START), SafetyEvent::SHUTTER_CLOSE, good()).api);
}

// ---- R6 READY ----------------------------------------------------------------------------
void test_r6_ready_hv_on_all_conditions() {
    SafetyResult r = step(st(SafetyState::READY), SafetyEvent::HV_ON, good());
    ASSERT_STATE(SafetyState::RUNNING, r.next.state); TEST_ASSERT_TRUE(r.next.hv_on);
    TEST_ASSERT_FALSE(r.next.shutter_open); ASSERT_API(ApiError::OK, r.api);
}
void test_r6_ready_hv_on_each_missing_condition() {
    SafetyInputs a = good(); a.interlock = false;
    SafetyInputs b = good(); b.cooling_flow = false;
    SafetyInputs c = good(); c.temp_c = 30.5f;
    SafetyInputs cases[] = {a, b, c};
    for (auto& in : cases) {
        SafetyResult r = step(st(SafetyState::READY), SafetyEvent::HV_ON, in);
        ASSERT_STATE(SafetyState::READY, r.next.state);
        TEST_ASSERT_FALSE(r.next.hv_on);
        ASSERT_API(ApiError::PRECONDITION, r.api);
    }
}
void test_r6_ready_shutter_open_and_ack_rejected() {
    ASSERT_API(ApiError::STATE, step(st(SafetyState::READY), SafetyEvent::SHUTTER_OPEN, good()).api);
    ASSERT_API(ApiError::STATE, step(st(SafetyState::READY), SafetyEvent::ACK, good()).api);
}
void test_r6_ready_init_done_stays_ready() {
    SafetyResult r = step(st(SafetyState::READY), SafetyEvent::INIT_DONE, good());
    ASSERT_STATE(SafetyState::READY, r.next.state); ASSERT_API(ApiError::OK, r.api);
}

// ---- R7 RUNNING --------------------------------------------------------------------------
void test_r7_running_condition_loss_drops_to_ready() {
    SafetyInputs a = good(); a.interlock = false;
    SafetyInputs b = good(); b.cooling_flow = false;
    SafetyInputs c = good(); c.temp_c = 30.1f;
    SafetyInputs cases[] = {a, b, c};
    for (auto& in : cases) {
        SafetyResult r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::TICK, in);
        ASSERT_STATE(SafetyState::READY, r.next.state);
        TEST_ASSERT_FALSE(r.next.hv_on); TEST_ASSERT_FALSE(r.next.shutter_open);
        ASSERT_STATE(ErrorCode::NONE, r.next.err);  // not a fault, no ACK needed
        ASSERT_API(ApiError::OK, r.api);
        TEST_ASSERT_FALSE(r.motor_stop);
    }
}
void test_r7_condition_loss_on_command_events() {
    SafetyInputs in = good(); in.cooling_flow = false;
    ASSERT_API(ApiError::PRECONDITION, step(st(SafetyState::RUNNING, true), SafetyEvent::HV_ON, in).api);
    ASSERT_API(ApiError::PRECONDITION, step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in).api);
    ASSERT_API(ApiError::OK, step(st(SafetyState::RUNNING, true), SafetyEvent::HV_OFF, in).api);
    ASSERT_API(ApiError::STATE, step(st(SafetyState::RUNNING, true), SafetyEvent::ACK, in).api);
    ASSERT_STATE(SafetyState::READY, step(st(SafetyState::RUNNING, true), SafetyEvent::HV_ON, in).next.state);
}
void test_r7_hv_off_to_ready_closes_shutter() {
    SafetyResult r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::HV_OFF, good());
    ASSERT_STATE(SafetyState::READY, r.next.state);
    TEST_ASSERT_FALSE(r.next.hv_on); TEST_ASSERT_FALSE(r.next.shutter_open);
}
void test_r7_hv_on_idempotent() {
    SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::HV_ON, good());
    ASSERT_STATE(SafetyState::RUNNING, r.next.state); ASSERT_API(ApiError::OK, r.api);
}
// V1.2: manual SHUTTER_OPEN only in the service zone (20, 60]
void test_r7_manual_shutter_open_only_in_service_zone() {
    SafetyInputs in = good();
    const float ok_pos[]  = {20.1f, 40.0f, 60.0f};
    const float bad_pos[] = {60.1f, 100.0f, 145.0f, 242.0f};
    for (float p : ok_pos) {
        in.pos_mm = p;
        SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in);
        TEST_ASSERT_TRUE(r.next.shutter_open); ASSERT_API(ApiError::OK, r.api);
    }
    for (float p : bad_pos) {
        in.pos_mm = p;
        SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in);
        TEST_ASSERT_FALSE(r.next.shutter_open); ASSERT_API(ApiError::PRECONDITION, r.api);
    }
}
void test_r7_shutter_open_in_park_rejected() {
    SafetyInputs in = good(); in.pos_mm = 20.0f;   // == park limit -> not allowed
    SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); ASSERT_API(ApiError::PRECONDITION, r.api);
    r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_AUTO_OPEN, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); ASSERT_API(ApiError::PRECONDITION, r.api);
    in.pos_mm = 20.1f;
    TEST_ASSERT_TRUE(step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in).next.shutter_open);
}
// V1.5: automation opens only in the measuring zone (> maint_max_mm = 60 mm)
void test_r7_auto_open_only_in_measuring_zone() {
    SafetyInputs in = good();
    const float ok_pos[]  = {60.1f, 105.0f, 145.0f, 242.0f};
    const float bad_pos[] = {20.1f, 40.0f, 60.0f};
    for (float p : ok_pos) {
        in.pos_mm = p;
        SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_AUTO_OPEN, in);
        TEST_ASSERT_TRUE(r.next.shutter_open); TEST_ASSERT_TRUE(r.next.shutter_auto);
        ASSERT_API(ApiError::OK, r.api);
    }
    for (float p : bad_pos) {
        in.pos_mm = p;
        SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_AUTO_OPEN, in);
        TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
        ASSERT_API(ApiError::PRECONDITION, r.api);
    }
    in.pos_mm = 145.0f;
    SafetyResult r;
    ASSERT_API(ApiError::STATE, step(st(SafetyState::READY), SafetyEvent::SHUTTER_AUTO_OPEN, in).api);
    in.cooling_flow = false;
    r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_AUTO_OPEN, in);
    ASSERT_API(ApiError::PRECONDITION, r.api); TEST_ASSERT_FALSE(r.next.shutter_open);
}
// V1.2: once opened in the service zone the shutter stays open when the axis leaves it
void test_r7_manual_open_stays_open_outside_service_zone() {
    SafetyInputs in = good(); in.pos_mm = 40.0f;
    SafetyStatus s = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in).next;
    TEST_ASSERT_TRUE(s.shutter_open); TEST_ASSERT_FALSE(s.shutter_auto);   // manual
    in.pos_mm = 145.0f;
    s = step(s, SafetyEvent::TICK, in).next;
    TEST_ASSERT_TRUE(s.shutter_open);
    in.pos_mm = 40.0f;                              // V1.5: manual shutter stays open in the service zone
    s = step(s, SafetyEvent::TICK, in).next;
    TEST_ASSERT_TRUE(s.shutter_open);
    in.pos_mm = 19.0f;                              // entering the park zone closes it
    s = step(s, SafetyEvent::TICK, in).next;
    TEST_ASSERT_FALSE(s.shutter_open);
}
// V1.5: the AUTOMATIC shutter closes when the axis enters the service zone (<= 60 mm)
void test_r7_auto_shutter_closes_entering_service_zone() {
    SafetyInputs in = good(); in.pos_mm = 60.1f;
    SafetyResult r = step(stAuto(), SafetyEvent::TICK, in);
    TEST_ASSERT_TRUE(r.next.shutter_open); TEST_ASSERT_TRUE(r.next.shutter_auto);
    in.pos_mm = 60.0f;
    r = step(stAuto(), SafetyEvent::TICK, in);
    ASSERT_STATE(SafetyState::RUNNING, r.next.state); TEST_ASSERT_TRUE(r.next.hv_on);
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
    in.pos_mm = 40.0f;                              // also with any command event
    r = step(stAuto(), SafetyEvent::HV_ON, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); ASSERT_API(ApiError::OK, r.api);
}
// V1.5: a manually opened shutter is taken over by the automation (SHUTTER_AUTO_OPEN
// while open): from then on the automatic closing rules apply
void test_r7_auto_takes_over_manual_shutter() {
    SafetyInputs in = good(); in.pos_mm = 40.0f;
    SafetyStatus s = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in).next;
    in.pos_mm = 145.0f;
    SafetyResult r = step(s, SafetyEvent::SHUTTER_AUTO_OPEN, in);
    ASSERT_API(ApiError::OK, r.api);
    TEST_ASSERT_TRUE(r.next.shutter_open); TEST_ASSERT_TRUE(r.next.shutter_auto);
    in.pos_mm = 50.0f;
    s = step(r.next, SafetyEvent::TICK, in).next;
    TEST_ASSERT_FALSE(s.shutter_open); TEST_ASSERT_FALSE(s.shutter_auto);
}
// V1.5: every way of closing clears shutter_auto
void test_r7_closing_clears_shutter_auto() {
    SafetyInputs in = good();
    SafetyResult r = step(stAuto(), SafetyEvent::SHUTTER_CLOSE, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
    r = step(stAuto(), SafetyEvent::HV_OFF, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
    in.cooling_flow = false;                         // HV conditions lost -> READY
    r = step(stAuto(), SafetyEvent::TICK, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
    in = good(); in.estop = true;                    // fault
    r = step(stAuto(), SafetyEvent::TICK, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
    in = good(); in.pos_mm = 19.0f;                  // park zone
    r = step(stAuto(), SafetyEvent::TICK, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
    in = good(); in.pos_mm = 40.0f;                  // manual open in the service zone is never "auto"
    r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in);
    TEST_ASSERT_TRUE(r.next.shutter_open); TEST_ASSERT_FALSE(r.next.shutter_auto);
}
void test_r7_shutter_auto_closes_entering_park() {
    SafetyInputs in = good(); in.pos_mm = 19.9f;
    SafetyResult r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::TICK, in);
    ASSERT_STATE(SafetyState::RUNNING, r.next.state);
    TEST_ASSERT_TRUE(r.next.hv_on); TEST_ASSERT_FALSE(r.next.shutter_open);
}
void test_r7_shutter_close() {
    SafetyResult r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::SHUTTER_CLOSE, good());
    TEST_ASSERT_FALSE(r.next.shutter_open); TEST_ASSERT_TRUE(r.next.hv_on);
}
void test_r7_running_ack_rejected() {
    ASSERT_API(ApiError::STATE, step(st(SafetyState::RUNNING, true), SafetyEvent::ACK, good()).api);
}

// ---- R1 fault entry ----------------------------------------------------------------------
void test_r1_estop_from_every_operational_state() {
    SafetyStatus from[] = {st(SafetyState::SAFE_START), st(SafetyState::READY),
                           st(SafetyState::RUNNING, true, true), st(SafetyState::RECOVER)};
    for (auto& s : from) {
        SafetyInputs in = good(); in.estop = true;
        SafetyResult r = step(s, SafetyEvent::TICK, in);
        ASSERT_STATE(SafetyState::FAULT, r.next.state);
        ASSERT_STATE(ErrorCode::ESTOP, r.next.err);
        TEST_ASSERT_FALSE(r.next.hv_on); TEST_ASSERT_FALSE(r.next.shutter_open);
        TEST_ASSERT_TRUE(r.motor_stop);
    }
}
void test_r1_each_cause() {
    SafetyInputs t = good(); t.temp_c = 35.0f;
    SafetyInputs m = good(); m.motor_fault = true;
    SafetyInputs c = good(); c.comm_timeout = true;
    ASSERT_STATE(ErrorCode::TEMP_HIGH, step(st(SafetyState::RUNNING, true), SafetyEvent::TICK, t).next.err);
    ASSERT_STATE(ErrorCode::MOTOR_FAULT, step(st(SafetyState::READY), SafetyEvent::TICK, m).next.err);
    ASSERT_STATE(ErrorCode::COMM_TIMEOUT, step(st(SafetyState::RUNNING, true), SafetyEvent::TICK, c).next.err);
}
void test_r1_init_failed() {
    SafetyResult r = step(st(SafetyState::SAFE_START), SafetyEvent::INIT_FAILED, good());
    ASSERT_STATE(SafetyState::FAULT, r.next.state); ASSERT_STATE(ErrorCode::HOMING_FAILED, r.next.err);
    r = step(st(SafetyState::READY), SafetyEvent::INIT_FAILED, good());
    ASSERT_STATE(ErrorCode::HOMING_FAILED, r.next.err);
}
void test_r1_api_codes_on_fault_entry() {
    SafetyInputs in = good(); in.estop = true;
    ASSERT_API(ApiError::OK, step(st(SafetyState::READY), SafetyEvent::TICK, in).api);
    ASSERT_API(ApiError::OK, step(st(SafetyState::READY), SafetyEvent::HV_OFF, in).api);
    ASSERT_API(ApiError::OK, step(st(SafetyState::READY), SafetyEvent::SHUTTER_CLOSE, in).api);
    ASSERT_API(ApiError::STATE, step(st(SafetyState::READY), SafetyEvent::HV_ON, in).api);
    ASSERT_STATE(SafetyState::FAULT, step(st(SafetyState::READY), SafetyEvent::HV_ON, in).next.state);
}

// ---- R2..R4 fault handling ---------------------------------------------------------------
void test_r2_fault_tick_to_wait_ack() {
    SafetyInputs in = good(); in.estop = true;   // cause still present
    SafetyResult r = step(st(SafetyState::FAULT, false, false, ErrorCode::ESTOP), SafetyEvent::TICK, in);
    ASSERT_STATE(SafetyState::WAIT_ACK, r.next.state);
    ASSERT_STATE(ErrorCode::ESTOP, r.next.err);   // first cause is latched
    TEST_ASSERT_TRUE(r.motor_stop);
}
void test_r2_fault_keeps_first_cause() {
    SafetyInputs in = good(); in.temp_c = 40.0f;  // different cause appears later
    SafetyResult r = step(st(SafetyState::FAULT, false, false, ErrorCode::COMM_TIMEOUT), SafetyEvent::TICK, in);
    ASSERT_STATE(ErrorCode::COMM_TIMEOUT, r.next.err);
}
void test_r2_fault_rejects_commands() {
    SafetyStatus f = st(SafetyState::FAULT, false, false, ErrorCode::ESTOP);
    ASSERT_API(ApiError::STATE, step(f, SafetyEvent::HV_ON, good()).api);
    ASSERT_API(ApiError::STATE, step(f, SafetyEvent::ACK, good()).api);
    ASSERT_API(ApiError::OK, step(f, SafetyEvent::HV_OFF, good()).api);
}
void test_r3_ack_rejected_while_cause_active() {
    SafetyInputs in = good(); in.estop = true;
    SafetyResult r = step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::ESTOP), SafetyEvent::ACK, in);
    ASSERT_STATE(SafetyState::WAIT_ACK, r.next.state); ASSERT_API(ApiError::PRECONDITION, r.api);
    ASSERT_STATE(ErrorCode::ESTOP, r.next.err);
}
void test_r3_ack_accepted_when_cleared() {
    SafetyResult r = step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::ESTOP), SafetyEvent::ACK, good());
    ASSERT_STATE(SafetyState::RECOVER, r.next.state); ASSERT_API(ApiError::OK, r.api);
    ASSERT_STATE(ErrorCode::NONE, r.next.err);
}
void test_r3_ack_not_blocked_by_missing_flags() {
    SafetyInputs in = good(); in.interlock = false; in.cooling_flow = false;  // flags are not faults
    ASSERT_STATE(SafetyState::RECOVER,
                 step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::TEMP_HIGH), SafetyEvent::ACK, in).next.state);
}
void test_r3_wait_ack_asserts_motor_stop_and_rejects_hv() {
    SafetyResult r = step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::ESTOP), SafetyEvent::TICK, good());
    TEST_ASSERT_TRUE(r.motor_stop); ASSERT_STATE(SafetyState::WAIT_ACK, r.next.state);
    ASSERT_API(ApiError::STATE,
               step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::ESTOP), SafetyEvent::HV_ON, good()).api);
}
void test_r4_recover_to_ready_if_homed() {
    SafetyResult r = step(st(SafetyState::RECOVER), SafetyEvent::TICK, good());
    ASSERT_STATE(SafetyState::READY, r.next.state);
}
void test_r4_recover_to_safe_start_if_not_homed() {
    SafetyInputs in = good(); in.homed = false;
    ASSERT_STATE(SafetyState::SAFE_START, step(st(SafetyState::RECOVER), SafetyEvent::TICK, in).next.state);
}
void test_r4_recover_refaults() {
    SafetyInputs in = good(); in.estop = true;
    ASSERT_STATE(SafetyState::FAULT, step(st(SafetyState::RECOVER), SafetyEvent::TICK, in).next.state);
}
void test_r4_recover_never_turns_hv_on() {
    SafetyResult r = step(st(SafetyState::RECOVER), SafetyEvent::HV_ON, good());
    TEST_ASSERT_FALSE(r.next.hv_on); ASSERT_API(ApiError::STATE, r.api);
}

// ---- full cycle ------------------------------------------------------------------------------
void test_full_cycle_estop_ack_recover() {
    SafetyInputs in = good();
    SafetyStatus s = safetyInitialStatus();
    s = step(s, SafetyEvent::INIT_DONE, in).next;  ASSERT_STATE(SafetyState::READY, s.state);
    s = step(s, SafetyEvent::HV_ON, in).next;      ASSERT_STATE(SafetyState::RUNNING, s.state);
    s = step(s, SafetyEvent::SHUTTER_AUTO_OPEN, in).next; TEST_ASSERT_TRUE(s.shutter_open);
    in.estop = true;
    s = step(s, SafetyEvent::TICK, in).next;       ASSERT_STATE(SafetyState::FAULT, s.state);
    s = step(s, SafetyEvent::TICK, in).next;       ASSERT_STATE(SafetyState::WAIT_ACK, s.state);
    in.estop = false;
    s = step(s, SafetyEvent::TICK, in).next;       ASSERT_STATE(SafetyState::WAIT_ACK, s.state);  // no auto-restart
    s = step(s, SafetyEvent::ACK, in).next;        ASSERT_STATE(SafetyState::RECOVER, s.state);
    in.homed = false;                               // reference lost during e-stop
    s = step(s, SafetyEvent::TICK, in).next;       ASSERT_STATE(SafetyState::SAFE_START, s.state);
    TEST_ASSERT_FALSE(s.hv_on); TEST_ASSERT_FALSE(s.shutter_open);
}

// ---- command gate ----------------------------------------------------------------------------
void test_cmd_gate_matrix() {
    const SafetyState S[] = {SafetyState::SAFE_START, SafetyState::READY, SafetyState::RUNNING,
                             SafetyState::FAULT, SafetyState::WAIT_ACK, SafetyState::RECOVER};
    struct Row { CmdId c; const char* allowed; };  // one char per state above: 1 = OK
    const Row rows[] = {
        {CmdId::INIT,           "110000"}, {CmdId::MOVE,          "011000"},
        {CmdId::PARK,           "011000"}, {CmdId::SERVICE,       "011000"},
        {CmdId::START_MEASURE,  "001000"}, {CmdId::START_TRAVERSE, "001000"},
        {CmdId::CALIB,          "111000"}, {CmdId::ACK,           "000010"},
        {CmdId::HV_ON,          "011000"}, {CmdId::SHUTTER_OPEN,  "001000"},
        {CmdId::REBOOT,         "110010"}, {CmdId::STOP,          "111111"},
        {CmdId::SET,            "111111"}, {CmdId::HV_OFF,        "111111"},
        {CmdId::SHUTTER_CLOSE,  "111111"}, {CmdId::GET_STATE,     "111111"},
        {CmdId::GET_CONFIG,     "111111"}, {CmdId::NONE,          "000000"},
    };
    for (auto& r : rows) {
        for (int i = 0; i < 6; ++i) {
            ApiError exp = r.allowed[i] == '1' ? ApiError::OK : ApiError::STATE;
            TEST_ASSERT_EQUAL_INT_MESSAGE((int)exp, (int)safetyCmdAllowed(S[i], r.c), cmdName(r.c));
        }
    }
}

// ---- shutter automation (V1.5) ----------------------------------------------------------------
void test_auto_open_when_acquiring_in_measuring_zone() {
    const SafetyStatus closed = st(SafetyState::RUNNING, true);
    ASSERT_STATE(SafetyEvent::SHUTTER_AUTO_OPEN, automation(true, true, 0, 3000, closed, 60.1f));
    ASSERT_STATE(SafetyEvent::SHUTTER_AUTO_OPEN, automation(true, true, 0, 3000, closed, 145.0f));
}
void test_auto_no_open_outside_measuring_zone_or_not_running() {
    const SafetyStatus closed = st(SafetyState::RUNNING, true);
    ASSERT_STATE(SafetyEvent::TICK, automation(true, true, 0, 3000, closed, 20.0f));
    ASSERT_STATE(SafetyEvent::TICK, automation(true, true, 0, 3000, closed, 50.0f));    // service zone
    ASSERT_STATE(SafetyEvent::TICK, automation(true, true, 0, 3000, closed, 60.0f));
    ASSERT_STATE(SafetyEvent::TICK, automation(true, true, 0, 3000, st(SafetyState::READY), 145.0f));
    ASSERT_STATE(SafetyEvent::TICK, automation(false, true, 0, 3000, closed, 145.0f));  // approach: no open
    ASSERT_STATE(SafetyEvent::TICK, automation(true, true, 0, 3000, stAuto(), 145.0f)); // already automatic
}
void test_auto_takes_over_manually_opened_shutter() {
    const SafetyStatus manual = st(SafetyState::RUNNING, true, true);   // open, shutter_auto false
    ASSERT_STATE(SafetyEvent::SHUTTER_AUTO_OPEN, automation(true, true, 0, 3000, manual, 145.0f));
    ASSERT_STATE(SafetyEvent::TICK, automation(false, false, 99999, 3000, manual, 145.0f)); // never auto-closed
}
// The shutter stays open over a whole measuring programme: the end of one traverse
// does NOT close it; it closes only after the hold time without a measuring mode.
void test_auto_close_only_after_hold_time() {
    ASSERT_STATE(SafetyEvent::TICK, automation(false, true, 0, 3000, stAuto(), 145.0f));      // approach of next run
    ASSERT_STATE(SafetyEvent::TICK, automation(false, false, 0, 3000, stAuto(), 145.0f));     // run just ended
    ASSERT_STATE(SafetyEvent::TICK, automation(false, false, 2999, 3000, stAuto(), 145.0f));
    ASSERT_STATE(SafetyEvent::SHUTTER_CLOSE, automation(false, false, 3000, 3000, stAuto(), 145.0f));
    ASSERT_STATE(SafetyEvent::SHUTTER_CLOSE, automation(false, false, 0, 0, stAuto(), 145.0f)); // hold 0
    ASSERT_STATE(SafetyEvent::TICK, automation(false, true, 99999, 0, stAuto(), 145.0f));     // active: never
    ASSERT_STATE(SafetyEvent::TICK, automation(false, false, 99999, 3000,
                                               st(SafetyState::RUNNING, true), 145.0f));     // already closed
}

// ---- comm watchdog ---------------------------------------------------------------------------
void test_comm_timeout() {
    TEST_ASSERT_FALSE(safetyCommTimeout(false, 100000, 0, 5000));
    TEST_ASSERT_FALSE(safetyCommTimeout(true, 5000, 0, 5000));     // exactly 5000 -> not yet
    TEST_ASSERT_TRUE(safetyCommTimeout(true, 5001, 0, 5000));
}
void test_comm_timeout_wraparound() {
    // last = 2^32 - 1000, now = 3000 -> elapsed 4000
    TEST_ASSERT_FALSE(safetyCommTimeout(true, 3000u, 0xFFFFFFFFu - 999u, 5000));
    TEST_ASSERT_TRUE(safetyCommTimeout(true, 4002u, 0xFFFFFFFFu - 999u, 5000));
}

// ---- property test: invariants hold for random event/input sequences ------------------------
static uint32_t rng = 12345;
static uint32_t rnd() { rng = rng * 1664525u + 1013904223u; return rng >> 8; }
void test_invariants_random_sequences() {
    for (int run = 0; run < 200; ++run) {
        SafetyStatus s = safetyInitialStatus();
        SafetyInputs in = good();
        for (int i = 0; i < 300; ++i) {
            // mutate inputs with low probability
            uint32_t r = rnd() % 100;
            if (r < 3) in.estop = !in.estop;
            else if (r < 8) in.interlock = !in.interlock;
            else if (r < 13) in.cooling_flow = !in.cooling_flow;
            else if (r < 18) in.temp_c = (float)(15 + rnd() % 22);         // 15..36
            else if (r < 25) in.pos_mm = (float)(rnd() % 249);
            else if (r < 27) in.homed = !in.homed;
            in.motor_fault = (rnd() % 100) < 2;
            in.comm_timeout = (rnd() % 100) < 1;
            SafetyEvent ev = (SafetyEvent)(rnd() % 9);   // incl. SHUTTER_AUTO_OPEN
            const bool was_open = s.shutter_open;
            SafetyResult res = safetyStep(s, ev, in, CFG);
            if (ev == SafetyEvent::SHUTTER_OPEN && !was_open && res.next.shutter_open)
                TEST_ASSERT_TRUE_MESSAGE(in.pos_mm > CFG.park_pos_mm && in.pos_mm <= CFG.maint_max_mm,
                                         "manual open only in the service zone");
            s = res.next;
            checkInvariants(s, in);
        }
    }
}

// V1.1: concurrent E-stop and HV-condition loss -> fault (R1) wins over the HV drop (R7)
void test_estop_and_cooling_loss_simultaneous() {
    SafetyInputs in = good(); in.estop = true; in.cooling_flow = false; in.interlock = false;
    SafetyResult r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::TICK, in);
    ASSERT_STATE(SafetyState::FAULT, r.next.state);
    ASSERT_STATE(ErrorCode::ESTOP, r.next.err);
    TEST_ASSERT_TRUE(r.motor_stop);
    TEST_ASSERT_FALSE(r.next.hv_on); TEST_ASSERT_FALSE(r.next.shutter_open);
    checkInvariants(r.next, in);
    in.temp_c = 40.0f;                                    // ESTOP also wins over TEMP_HIGH
    r = step(st(SafetyState::RUNNING, true, false), SafetyEvent::HV_ON, in);
    ASSERT_STATE(ErrorCode::ESTOP, r.next.err);
    ASSERT_API(ApiError::STATE, r.api);
}

// ---- V1.5.2: clarifications from the S2.1 review ------------------------------------
static const SafetyEvent ALL_EVENTS[] = {
    SafetyEvent::TICK, SafetyEvent::INIT_DONE, SafetyEvent::INIT_FAILED, SafetyEvent::HV_ON,
    SafetyEvent::HV_OFF, SafetyEvent::SHUTTER_OPEN, SafetyEvent::SHUTTER_CLOSE, SafetyEvent::ACK,
    SafetyEvent::SHUTTER_AUTO_OPEN};

// R2: motor_stop = true on EVERY event in FAULT (not only on TICK)
void test_r2_fault_motor_stop_on_every_event() {
    SafetyInputs in = good(); in.estop = true;
    for (SafetyEvent ev : ALL_EVENTS) {
        SafetyResult r = step(st(SafetyState::FAULT, false, false, ErrorCode::ESTOP), ev, in);
        TEST_ASSERT_TRUE_MESSAGE(r.motor_stop, "FAULT: motor_stop on every event (cause active)");
        r = step(st(SafetyState::FAULT, false, false, ErrorCode::ESTOP), ev, good());
        TEST_ASSERT_TRUE_MESSAGE(r.motor_stop, "FAULT: motor_stop on every event (cause gone)");
    }
}
// R3: motor_stop = true on EVERY event in WAIT_ACK, including a rejected ACK
void test_r3_motor_stop_on_every_event() {
    SafetyInputs in = good(); in.estop = true;
    for (SafetyEvent ev : ALL_EVENTS) {
        SafetyResult r = step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::ESTOP), ev, in);
        TEST_ASSERT_TRUE_MESSAGE(r.motor_stop, "WAIT_ACK: motor_stop on every event");
        if (ev != SafetyEvent::ACK) ASSERT_STATE(SafetyState::WAIT_ACK, r.next.state);
    }
    SafetyResult r = step(st(SafetyState::WAIT_ACK, false, false, ErrorCode::ESTOP), SafetyEvent::ACK, in);
    ASSERT_API(ApiError::PRECONDITION, r.api);
    TEST_ASSERT_TRUE_MESSAGE(r.motor_stop, "rejected ACK keeps motor_stop");
}
// R5: INIT_DONE -> READY unconditionally (in.homed is not checked)
void test_r5_init_done_ready_unconditional() {
    SafetyInputs in = good(); in.homed = false;
    SafetyResult r = step(st(SafetyState::SAFE_START), SafetyEvent::INIT_DONE, in);
    ASSERT_STATE(SafetyState::READY, r.next.state);
    ASSERT_API(ApiError::OK, r.api);
}
// R7: INIT_DONE and TICK in RUNNING -> OK, with conditions ok and after condition loss
void test_r7_init_done_ok() {
    SafetyResult r = step(st(SafetyState::RUNNING, true, false), SafetyEvent::INIT_DONE, good());
    ASSERT_API(ApiError::OK, r.api); ASSERT_STATE(SafetyState::RUNNING, r.next.state);
    SafetyInputs lost = good(); lost.interlock = false;
    r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::INIT_DONE, lost);
    ASSERT_API(ApiError::OK, r.api); ASSERT_STATE(SafetyState::READY, r.next.state);
    TEST_ASSERT_FALSE(r.next.hv_on); TEST_ASSERT_FALSE(r.next.shutter_open);
    r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::TICK, lost);
    ASSERT_API(ApiError::OK, r.api);
}
// Outside RUNNING, outputs are forced off even if cur is inconsistent
void test_outputs_forced_off_outside_running() {
    const SafetyState states[] = {SafetyState::SAFE_START, SafetyState::READY, SafetyState::RECOVER,
                                  SafetyState::FAULT, SafetyState::WAIT_ACK};
    for (SafetyState s : states) {
        ErrorCode e = (s == SafetyState::FAULT || s == SafetyState::WAIT_ACK) ? ErrorCode::ESTOP : ErrorCode::NONE;
        for (SafetyEvent ev : ALL_EVENTS) {
            SafetyResult r = step(st(s, true, true, e, true), ev, good());
            if (r.next.state == SafetyState::RUNNING) continue;   // e.g. READY + HV_ON
            TEST_ASSERT_FALSE_MESSAGE(r.next.hv_on, "hv_on forced off outside RUNNING");
            TEST_ASSERT_FALSE_MESSAGE(r.next.shutter_open, "shutter forced closed outside RUNNING");
            TEST_ASSERT_FALSE_MESSAGE(r.next.shutter_auto, "shutter_auto cleared outside RUNNING");
        }
    }
}

// ---- V1.5.3: R1 before every other rule, complete API table --------------------------
static bool benignEv(SafetyEvent ev) {
    return ev == SafetyEvent::TICK || ev == SafetyEvent::HV_OFF || ev == SafetyEvent::SHUTTER_CLOSE;
}
// consistent status for an operational state (RUNNING: HV on, automatic shutter open)
static SafetyStatus opStatus(SafetyState s) {
    if (s == SafetyState::RUNNING) return stAuto();
    return st(s);
}

// R1 matrix: every operational state x every cause x every event -> FAULT
void test_r1_matrix_every_event_every_cause() {
    const SafetyState states[] = {SafetyState::SAFE_START, SafetyState::READY, SafetyState::RUNNING,
                                  SafetyState::RECOVER};
    for (SafetyState s : states) {
        for (int c = 0; c < 4; ++c) {
            SafetyInputs in = good();
            ErrorCode exp = ErrorCode::NONE;
            switch (c) {
                case 0: in.estop = true;        exp = ErrorCode::ESTOP;        break;
                case 1: in.temp_c = 40.0f;      exp = ErrorCode::TEMP_HIGH;    break;
                case 2: in.motor_fault = true;  exp = ErrorCode::MOTOR_FAULT;  break;
                default: in.comm_timeout = true; exp = ErrorCode::COMM_TIMEOUT; break;
            }
            for (SafetyEvent ev : ALL_EVENTS) {
                char msg[96];
                snprintf(msg, sizeof(msg), "R1: state %d, cause %d, event %d", (int)s, (int)exp, (int)ev);
                SafetyResult r = step(opStatus(s), ev, in);
                TEST_ASSERT_EQUAL_INT_MESSAGE((int)SafetyState::FAULT, (int)r.next.state, msg);
                TEST_ASSERT_EQUAL_INT_MESSAGE((int)exp, (int)r.next.err, msg);   // cause wins over HOMING_FAILED
                TEST_ASSERT_TRUE_MESSAGE(r.motor_stop, msg);
                TEST_ASSERT_FALSE_MESSAGE(r.next.hv_on, msg);
                TEST_ASSERT_FALSE_MESSAGE(r.next.shutter_open, msg);
                TEST_ASSERT_FALSE_MESSAGE(r.next.shutter_auto, msg);
                TEST_ASSERT_EQUAL_INT_MESSAGE((int)(benignEv(ev) ? ApiError::OK : ApiError::STATE), (int)r.api, msg);
                checkInvariants(r.next, in);
            }
        }
    }
}
// a one-shot motor fault in the cycle in which homing finishes must not be lost
void test_r1_motor_fault_with_init_done() {
    SafetyInputs in = good(); in.motor_fault = true;
    SafetyResult r = step(st(SafetyState::SAFE_START), SafetyEvent::INIT_DONE, in);
    ASSERT_STATE(SafetyState::FAULT, r.next.state);
    ASSERT_STATE(ErrorCode::MOTOR_FAULT, r.next.err);
    TEST_ASSERT_TRUE(r.motor_stop);
}
// API code and next state for every (state, event) with all conditions good
void test_api_table_all_states_all_events() {
    const ApiError O = ApiError::OK, S = ApiError::STATE, P = ApiError::PRECONDITION;
    const SafetyState SS = SafetyState::SAFE_START, RD = SafetyState::READY, RU = SafetyState::RUNNING,
                      FA = SafetyState::FAULT, WA = SafetyState::WAIT_ACK, RC = SafetyState::RECOVER;
    // columns: TICK, INIT_DONE, INIT_FAILED, HV_ON, HV_OFF, SHUTTER_OPEN, SHUTTER_CLOSE, ACK, SHUTTER_AUTO_OPEN
    struct Row { SafetyState from; ApiError api[9]; SafetyState next[9]; };
    const Row rows[] = {
        {SS, {O, O, S, S, O, S, O, S, S},  {SS, RD, FA, SS, SS, SS, SS, SS, SS}},
        {RD, {O, O, S, O, O, S, O, S, S},  {RD, RD, FA, RU, RD, RD, RD, RD, RD}},
        // RUNNING at pos 100 (measuring zone), HV on, shutter closed
        {RU, {O, O, S, O, O, P, O, S, O},  {RU, RU, FA, RU, RD, RU, RU, RU, RU}},
        {FA, {O, S, S, S, O, S, O, S, S},  {WA, FA, FA, FA, FA, FA, FA, FA, FA}},
        {WA, {O, S, S, S, O, S, O, O, S},  {WA, WA, WA, WA, WA, WA, WA, RC, WA}},
        {RC, {O, S, S, S, O, S, O, S, S},  {RD, RC, FA, RC, RC, RC, RC, RC, RC}},
    };
    for (const Row& row : rows) {
        bool faultState = row.from == FA || row.from == WA;
        SafetyStatus cur = st(row.from, row.from == RU, false, faultState ? ErrorCode::ESTOP : ErrorCode::NONE);
        for (int e = 0; e < 9; ++e) {
            char msg[64];
            snprintf(msg, sizeof(msg), "state %d, event %d", (int)row.from, e);
            SafetyResult r = step(cur, ALL_EVENTS[e], good());
            TEST_ASSERT_EQUAL_INT_MESSAGE((int)row.api[e], (int)r.api, msg);
            TEST_ASSERT_EQUAL_INT_MESSAGE((int)row.next[e], (int)r.next.state, msg);
            checkInvariants(r.next, good());
        }
    }
}
// RECOVER with homed = false: INIT_DONE does not shortcut to READY; TICK -> SAFE_START
void test_r4_recover_init_done_rejected() {
    SafetyInputs in = good(); in.homed = false;
    SafetyResult r = step(st(SafetyState::RECOVER), SafetyEvent::INIT_DONE, in);
    ASSERT_STATE(SafetyState::RECOVER, r.next.state); ASSERT_API(ApiError::STATE, r.api);
    ASSERT_STATE(SafetyState::SAFE_START, step(st(SafetyState::RECOVER), SafetyEvent::TICK, in).next.state);
}
// RUNNING + INIT_DONE: the closing rules of R7 apply as for every other event
void test_r7_init_done_applies_closing_rules() {
    SafetyInputs in = good(); in.pos_mm = 40.0f;                 // service zone
    SafetyResult r = step(stAuto(), SafetyEvent::INIT_DONE, in);
    TEST_ASSERT_FALSE_MESSAGE(r.next.shutter_open, "automatic shutter closes in the service zone");
    TEST_ASSERT_FALSE(r.next.shutter_auto);
    in.pos_mm = 10.0f;                                           // park zone
    r = step(st(SafetyState::RUNNING, true, true), SafetyEvent::INIT_DONE, in);
    TEST_ASSERT_FALSE_MESSAGE(r.next.shutter_open, "shutter closes in the park zone");
    TEST_ASSERT_TRUE(r.next.hv_on);
    checkInvariants(r.next, in);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_status);
    RUN_TEST(test_hv_conditions_and_logic);
    RUN_TEST(test_hv_conditions_do_not_depend_on_motion_or_comm);
    RUN_TEST(test_fault_cause_priority);
    RUN_TEST(test_temp_thresholds);
    RUN_TEST(test_fault_cause_ignores_flags);
    RUN_TEST(test_r5_safe_start_init_done_to_ready);
    RUN_TEST(test_r5_safe_start_rejects_hv_shutter_ack);
    RUN_TEST(test_r5_safe_start_benign_events_ok);
    RUN_TEST(test_r6_ready_hv_on_all_conditions);
    RUN_TEST(test_r6_ready_hv_on_each_missing_condition);
    RUN_TEST(test_r6_ready_shutter_open_and_ack_rejected);
    RUN_TEST(test_r6_ready_init_done_stays_ready);
    RUN_TEST(test_r7_running_condition_loss_drops_to_ready);
    RUN_TEST(test_r7_condition_loss_on_command_events);
    RUN_TEST(test_r7_hv_off_to_ready_closes_shutter);
    RUN_TEST(test_r7_hv_on_idempotent);
    RUN_TEST(test_r7_manual_shutter_open_only_in_service_zone);
    RUN_TEST(test_r7_auto_open_only_in_measuring_zone);
    RUN_TEST(test_r7_auto_shutter_closes_entering_service_zone);
    RUN_TEST(test_r7_auto_takes_over_manual_shutter);
    RUN_TEST(test_r7_closing_clears_shutter_auto);
    RUN_TEST(test_r7_manual_open_stays_open_outside_service_zone);
    RUN_TEST(test_r7_shutter_open_in_park_rejected);
    RUN_TEST(test_r7_shutter_auto_closes_entering_park);
    RUN_TEST(test_r7_shutter_close);
    RUN_TEST(test_r7_running_ack_rejected);
    RUN_TEST(test_r1_estop_from_every_operational_state);
    RUN_TEST(test_r1_each_cause);
    RUN_TEST(test_r1_init_failed);
    RUN_TEST(test_r1_api_codes_on_fault_entry);
    RUN_TEST(test_r2_fault_tick_to_wait_ack);
    RUN_TEST(test_r2_fault_keeps_first_cause);
    RUN_TEST(test_r2_fault_rejects_commands);
    RUN_TEST(test_r3_ack_rejected_while_cause_active);
    RUN_TEST(test_r3_ack_accepted_when_cleared);
    RUN_TEST(test_r3_ack_not_blocked_by_missing_flags);
    RUN_TEST(test_r3_wait_ack_asserts_motor_stop_and_rejects_hv);
    RUN_TEST(test_r4_recover_to_ready_if_homed);
    RUN_TEST(test_r4_recover_to_safe_start_if_not_homed);
    RUN_TEST(test_r4_recover_refaults);
    RUN_TEST(test_r4_recover_never_turns_hv_on);
    RUN_TEST(test_full_cycle_estop_ack_recover);
    RUN_TEST(test_cmd_gate_matrix);
    RUN_TEST(test_auto_open_when_acquiring_in_measuring_zone);
    RUN_TEST(test_auto_no_open_outside_measuring_zone_or_not_running);
    RUN_TEST(test_auto_takes_over_manually_opened_shutter);
    RUN_TEST(test_auto_close_only_after_hold_time);
    RUN_TEST(test_comm_timeout);
    RUN_TEST(test_comm_timeout_wraparound);
    RUN_TEST(test_invariants_random_sequences);
    RUN_TEST(test_estop_and_cooling_loss_simultaneous);
    RUN_TEST(test_r2_fault_motor_stop_on_every_event);
    RUN_TEST(test_r3_motor_stop_on_every_event);
    RUN_TEST(test_r5_init_done_ready_unconditional);
    RUN_TEST(test_r7_init_done_ok);
    RUN_TEST(test_outputs_forced_off_outside_running);
    RUN_TEST(test_r1_matrix_every_event_every_cause);
    RUN_TEST(test_r1_motor_fault_with_init_done);
    RUN_TEST(test_api_table_all_states_all_events);
    RUN_TEST(test_r4_recover_init_done_rejected);
    RUN_TEST(test_r7_init_done_applies_closing_rules);
    return UNITY_END();
}
