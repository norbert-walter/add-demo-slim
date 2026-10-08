// FROZEN TESTS (Sprint 0b) – Sprint S2.1 must make these pass. Do not modify.
// Covers: src/logic/safety_logic.cpp (CONTRACT §6). Rules R1..R7 refer to safety_logic.h.
#include <unity.h>
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
static SafetyStatus st(SafetyState s, bool hv = false, bool sh = false, ErrorCode e = ErrorCode::NONE) {
    SafetyStatus x; x.state = s; x.hv_on = hv; x.shutter_open = sh; x.err = e; return x;
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
void test_r7_shutter_open_outside_park() {
    SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, good());
    TEST_ASSERT_TRUE(r.next.shutter_open); ASSERT_API(ApiError::OK, r.api);
}
void test_r7_shutter_open_in_park_rejected() {
    SafetyInputs in = good(); in.pos_mm = 5.0f;   // == park -> not allowed
    SafetyResult r = step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in);
    TEST_ASSERT_FALSE(r.next.shutter_open); ASSERT_API(ApiError::PRECONDITION, r.api);
    in.pos_mm = 5.1f;
    TEST_ASSERT_TRUE(step(st(SafetyState::RUNNING, true), SafetyEvent::SHUTTER_OPEN, in).next.shutter_open);
}
void test_r7_shutter_auto_closes_entering_park() {
    SafetyInputs in = good(); in.pos_mm = 4.0f;
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
    s = step(s, SafetyEvent::SHUTTER_OPEN, in).next; TEST_ASSERT_TRUE(s.shutter_open);
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

// ---- shutter automation ----------------------------------------------------------------------
void test_auto_open_when_acquiring_outside_park() {
    ASSERT_STATE(SafetyEvent::SHUTTER_OPEN,
                 safetyShutterAutomation(true, false, st(SafetyState::RUNNING, true), 50.0f, CFG));
}
void test_auto_no_open_in_park_or_not_running() {
    ASSERT_STATE(SafetyEvent::TICK, safetyShutterAutomation(true, false, st(SafetyState::RUNNING, true), 5.0f, CFG));
    ASSERT_STATE(SafetyEvent::TICK, safetyShutterAutomation(true, false, st(SafetyState::READY), 50.0f, CFG));
    ASSERT_STATE(SafetyEvent::TICK, safetyShutterAutomation(false, false, st(SafetyState::RUNNING, true), 50.0f, CFG));
    ASSERT_STATE(SafetyEvent::TICK, safetyShutterAutomation(true, false, st(SafetyState::RUNNING, true, true), 50.0f, CFG));
}
void test_auto_close_on_acquisition_end() {
    ASSERT_STATE(SafetyEvent::SHUTTER_CLOSE,
                 safetyShutterAutomation(false, true, st(SafetyState::RUNNING, true, true), 50.0f, CFG));
    ASSERT_STATE(SafetyEvent::TICK,
                 safetyShutterAutomation(false, true, st(SafetyState::RUNNING, true, false), 50.0f, CFG));
    ASSERT_STATE(SafetyEvent::SHUTTER_CLOSE,   // close has priority
                 safetyShutterAutomation(true, true, st(SafetyState::RUNNING, true, true), 50.0f, CFG));
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
            else if (r < 25) in.pos_mm = (float)(rnd() % 240);
            else if (r < 27) in.homed = !in.homed;
            in.motor_fault = (rnd() % 100) < 2;
            in.comm_timeout = (rnd() % 100) < 1;
            SafetyEvent ev = (SafetyEvent)(rnd() % 8);
            SafetyResult res = safetyStep(s, ev, in, CFG);
            s = res.next;
            checkInvariants(s, in);
        }
    }
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
    RUN_TEST(test_r7_shutter_open_outside_park);
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
    RUN_TEST(test_auto_open_when_acquiring_outside_park);
    RUN_TEST(test_auto_no_open_in_park_or_not_running);
    RUN_TEST(test_auto_close_on_acquisition_end);
    RUN_TEST(test_comm_timeout);
    RUN_TEST(test_comm_timeout_wraparound);
    RUN_TEST(test_invariants_random_sequences);
    return UNITY_END();
}
