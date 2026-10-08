// FROZEN TESTS (Sprint 0b) – Sprint S2.4 must make these pass. Do not modify.
// Covers: src/logic/app_logic.cpp (CONTRACT §10) – complete scenarios with a
// simulated axis (fake_axis.h) and a simulated distance sensor.
#include <unity.h>
#include <math.h>
#include <string.h>
#include "logic/app_logic.h"
#include "../fake_axis.h"

static AppConfig C;
static AppState  S;
static FakeAxis  A;
static uint32_t  now;
static bool      estop;
static uint32_t  last_rx;
static bool      heartbeat;      // simulated API client sends a packet every second
static HwOutputs last_out;
static int       beeps, persists;

// ---- simulated sensor: base 200 mm, staircase material 40..200 mm ---------------
static float thickAt(float pos_mm) {
    if (pos_mm < 40.0f || pos_mm >= 200.0f) return 0.0f;
    return 5.0f * (1 + (int)((pos_mm - 40.0f) / 40.0f));   // 5,10,15,20 mm
}
static float mvForDist(float d) { return 1000.0f * powf(d / 120.8f, 1.0f / -1.058f); }
static float sensorMv() { return mvForDist(200.0f - thickAt((float)(A.phys_steps / C.motion.steps_per_mm))); }

static HwInputs hw() {
    HwInputs h;
    h.now_ms = now; h.estop = estop;
    h.limit_fwd = A.limitFwd(); h.limit_bwd = A.limitBwd();
    h.drv_running = A.running; h.drv_pos_steps = A.drvPos();
    h.drv_speed_mm_min = A.running ? (float)(A.speed_hz * 60.0 / C.motion.steps_per_mm) : 0.0f;
    h.last_api_rx_ms = last_rx;
    return h;
}

// One 1 ms control cycle; optionally executes `cmd` (returns its result via *res).
static bool cycle(const Command* cmd = nullptr, ApiError* res = nullptr) {
    now++;
    A.step(1);
    if (heartbeat && now % 1000 == 0) last_rx = now;
    if (now % 20 == 0) appMeasure(S, C, sensorMv(), now);
    HwOutputs out = appTick(S, hw(), C);
    bool executed = false;
    if (cmd && out.drv.op == DrvOp::NONE) {
        *res = appCommand(S, *cmd, hw(), C, out);
        if (cmd->source == CmdSource::API) last_rx = now;
        executed = true;
    }
    A.apply(out.drv);
    if (out.beep) beeps++;
    if (out.persist) persists++;
    last_out = out;
    return executed;
}
static void run(uint32_t ms) { for (uint32_t i = 0; i < ms; ++i) cycle(); }

static Command mk(CmdId id, CmdSource src = CmdSource::API) {
    Command c; c.id = id; c.source = src; c.want_reply = src == CmdSource::API; return c;
}
static ApiError send(const Command& c) {
    ApiError r = ApiError::UNKNOWN_CMD;
    for (int i = 0; i < 10; ++i) if (cycle(&c, &r)) return r;
    TEST_FAIL_MESSAGE("command not executed within 10 cycles");
    return r;
}
static ApiError send(CmdId id, CmdSource src = CmdSource::API) { return send(mk(id, src)); }
static ApiError setNum(const char* key, double v) {
    Command c = mk(CmdId::SET); strcpy(c.key, key); c.vtype = ValueType::NUMBER; c.num = v; return send(c);
}
static ApiError setBool(const char* key, bool v) {
    Command c = mk(CmdId::SET); strcpy(c.key, key); c.vtype = ValueType::BOOL; c.b = v; return send(c);
}
static ApiError calib(CalibAction a) { Command c = mk(CmdId::CALIB); c.calib = a; return send(c); }
static ApiError traverse(float a, float b, float v) {
    Command c = mk(CmdId::START_TRAVERSE);
    c.has_start = c.has_end = c.has_speed = true; c.start = a; c.end = b; c.speed = v; return send(c);
}
static ProcData proc() { ProcData p; appFillProc(S, C, hw(), p); return p; }

#define API(e, x)   TEST_ASSERT_EQUAL_INT((int)(e), (int)(x))
#define STATE(e)    TEST_ASSERT_EQUAL_STRING(safetyStateName(e), safetyStateName(S.safety.state))

void setUp() {
    C = AppConfig{}; appInit(S); A = FakeAxis{};
    A.phys_steps = 100.0 * C.motion.steps_per_mm;
    now = 10000; estop = false; last_rx = 0; heartbeat = true; beeps = 0; persists = 0;
    run(100);
}
void tearDown() {}

static void homeAndReady() {
    API(ApiError::OK, send(CmdId::INIT));
    run(20000);
    STATE(SafetyState::READY);
    TEST_ASSERT_TRUE(S.seq.homed);
}
static void running() {
    homeAndReady();
    API(ApiError::OK, setBool("interlock", true));
    API(ApiError::OK, setBool("cooling_flow", true));
    API(ApiError::OK, send(CmdId::HV_ON));
    STATE(SafetyState::RUNNING);
}

// ---- boot ---------------------------------------------------------------------------
void test_boot_state() {
    STATE(SafetyState::SAFE_START);
    ProcData p = proc();
    TEST_ASSERT_FALSE(p.safety.hv_on); TEST_ASSERT_FALSE(p.safety.shutter_open);
    TEST_ASSERT_FALSE(p.motion.homed);
    TEST_ASSERT_TRUE(p.motion.state == MotionState::IDLE);
    TEST_ASSERT_TRUE(p.measure.in_range);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 190.0f, p.measure.dist);   // axis at 100 mm: 10 mm step
    TEST_ASSERT_FALSE(p.measure.valid);
    TEST_ASSERT_FALSE(p.safety.comm_armed);
}
void test_motion_commands_rejected_before_init() {
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 50;
    API(ApiError::STATE, send(m));
    API(ApiError::STATE, send(CmdId::START_MEASURE));
    API(ApiError::STATE, send(CmdId::HV_ON));
}

// ---- homing -------------------------------------------------------------------------
void test_init_homes_and_goes_ready_with_beep() {
    API(ApiError::OK, send(CmdId::INIT));
    TEST_ASSERT_TRUE(proc().motion.state == MotionState::HOMING);
    TEST_ASSERT_TRUE(S.comm_armed);              // remote operation running
    run(20000);
    STATE(SafetyState::READY);
    TEST_ASSERT_TRUE(proc().motion.homed);
    TEST_ASSERT_EQUAL_INT(1, beeps);
    TEST_ASSERT_FALSE(S.comm_armed);             // idle and HV off -> disarmed
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, proc().motion.pos);
}
void test_local_init_does_not_arm_watchdog() {
    heartbeat = false;
    API(ApiError::OK, send(CmdId::INIT, CmdSource::LOCAL));
    TEST_ASSERT_FALSE(S.comm_armed);
    run(20000);                                  // > 5 s without packets
    STATE(SafetyState::READY);
}

// ---- HV / flags -----------------------------------------------------------------------
void test_hv_on_needs_flags() {
    homeAndReady();
    API(ApiError::PRECONDITION, send(CmdId::HV_ON));
    API(ApiError::OK, setBool("interlock", true));
    API(ApiError::PRECONDITION, send(CmdId::HV_ON));
    API(ApiError::OK, setBool("cooling_flow", true));
    TEST_ASSERT_TRUE(proc().safety.hv_ok);
    API(ApiError::OK, send(CmdId::HV_ON));
    STATE(SafetyState::RUNNING);
    TEST_ASSERT_TRUE(last_out.hv_on);
    TEST_ASSERT_FALSE(last_out.shutter_open);
}
void test_flags_not_persisted_params_persisted() {
    API(ApiError::OK, setBool("interlock", true));
    TEST_ASSERT_EQUAL_INT(0, persists);
    API(ApiError::OK, setNum("measure_pos", 100));
    TEST_ASSERT_EQUAL_INT(1, persists);
    API(ApiError::BAD_PARAM, setNum("measure_pos", 2));
    TEST_ASSERT_EQUAL_INT(1, persists);
}
void test_shutter_manual_open_close() {
    running();
    API(ApiError::PRECONDITION, send(CmdId::SHUTTER_OPEN));   // park position 0 mm
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 50; m.has_speed = true; m.speed = 3000;
    API(ApiError::OK, send(m));
    run(2000);
    API(ApiError::OK, send(CmdId::SHUTTER_OPEN));
    TEST_ASSERT_TRUE(last_out.shutter_open);
    TEST_ASSERT_TRUE(proc().measure.valid);
    API(ApiError::OK, send(CmdId::SHUTTER_CLOSE));
    TEST_ASSERT_FALSE(last_out.shutter_open);
}

// ---- traverse scenario --------------------------------------------------------------------
void test_traverse_auto_shutter_and_statistics() {
    running();
    API(ApiError::OK, traverse(10, 230, 3000));
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::TRAVERSE);
    bool sawOpen = false, sawOpenInPark = false;
    for (int i = 0; i < 8000; ++i) {
        cycle();
        if (last_out.shutter_open) {
            sawOpen = true;
            if (proc().motion.pos <= C.safety.park_pos_mm) sawOpenInPark = true;
        }
        if (S.seq.mode == OpMode::NONE && i > 100) break;
    }
    run(5);
    TEST_ASSERT_TRUE(sawOpen);
    TEST_ASSERT_FALSE(sawOpenInPark);
    TEST_ASSERT_FALSE(last_out.shutter_open);    // closed after the run
    TEST_ASSERT_TRUE(last_out.hv_on);            // HV stays on
    ProcData p = proc();
    TEST_ASSERT_TRUE(p.motion.mode == OpMode::NONE);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 230.0f, p.motion.pos);
    TEST_ASSERT_TRUE(p.measure.n > 50);          // samples only on material
    TEST_ASSERT_FLOAT_WITHIN(1.5f, 12.5f, p.measure.avg);   // mean of 5/10/15/20 mm steps
    STATE(SafetyState::RUNNING);
}
void test_traverse_uses_param_defaults() {
    running();
    API(ApiError::OK, setNum("traverse_start", 20));
    API(ApiError::OK, setNum("traverse_end", 30));
    API(ApiError::OK, setNum("traverse_speed", 3000));
    API(ApiError::OK, send(CmdId::START_TRAVERSE));
    run(3000);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 30.0f, proc().motion.pos);
}
void test_traverse_rejected_in_ready() {
    homeAndReady();
    API(ApiError::STATE, traverse(10, 230, 200));
}
void test_stop_during_traverse_closes_shutter() {
    running();
    API(ApiError::OK, traverse(10, 230, 600));
    run(2000);
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, send(CmdId::STOP));
    run(2);
    TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_TRUE(S.seq.homed);               // controlled stop keeps reference
}
void test_measure_mode_holds_and_averages() {
    running();
    Command m = mk(CmdId::START_MEASURE); m.has_pos = true; m.pos = 130;   // 15 mm step
    API(ApiError::OK, send(m));
    run(10000);
    ProcData p = proc();
    TEST_ASSERT_TRUE(p.motion.mode == OpMode::MEASURE);
    TEST_ASSERT_TRUE(p.measure.valid);
    TEST_ASSERT_TRUE(p.measure.on_material);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 15.0f, p.measure.thick);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 15.0f, p.measure.avg);
}

// ---- safety scenarios ------------------------------------------------------------------------
void test_estop_during_traverse() {
    running();
    API(ApiError::OK, traverse(10, 230, 600));
    run(2000);
    int fs = A.force_stops;
    estop = true;
    run(1);
    STATE(SafetyState::FAULT);
    TEST_ASSERT_FALSE(last_out.hv_on); TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_TRUE(A.force_stops > fs);
    TEST_ASSERT_FALSE(A.running);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::ESTOP, (int)proc().err);
    run(1);
    STATE(SafetyState::WAIT_ACK);
    API(ApiError::PRECONDITION, send(CmdId::ACK));   // still pressed
    estop = false;
    run(10);
    STATE(SafetyState::WAIT_ACK);                    // no automatic restart
    API(ApiError::OK, send(CmdId::ACK));
    run(2);
    STATE(SafetyState::SAFE_START);                  // reference lost -> INIT required
    TEST_ASSERT_FALSE(last_out.hv_on);
}
void test_temp_fault_while_idle_keeps_reference() {
    running();
    API(ApiError::OK, setNum("temp", 35.0));
    run(2);
    STATE(SafetyState::WAIT_ACK);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::TEMP_HIGH, (int)proc().err);
    API(ApiError::PRECONDITION, send(CmdId::ACK));
    API(ApiError::OK, setNum("temp", 18.0));
    API(ApiError::OK, send(CmdId::ACK));
    run(2);
    STATE(SafetyState::READY);                       // idle at fault -> homed kept
    TEST_ASSERT_FALSE(last_out.hv_on);               // HV never comes back by itself
}
void test_temp_warning_band_drops_hv_and_stops_traverse() {
    running();
    API(ApiError::OK, traverse(10, 230, 600));
    run(2000);
    API(ApiError::OK, setNum("temp", 31.0));
    run(3);
    STATE(SafetyState::READY);
    TEST_ASSERT_EQUAL_INT(0, (int)proc().err);       // not a fault
    TEST_ASSERT_FALSE(last_out.hv_on); TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_TRUE(S.seq.homed);
}
void test_hv_off_during_traverse_stops_motion() {
    running();
    API(ApiError::OK, traverse(10, 230, 600));
    run(2000);
    API(ApiError::OK, send(CmdId::HV_OFF));
    run(3);
    STATE(SafetyState::READY);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_FALSE(last_out.shutter_open);
}
void test_comm_timeout_without_heartbeat() {
    running();
    API(ApiError::OK, traverse(10, 230, 300));
    heartbeat = false;
    run(5100);
    TEST_ASSERT_TRUE(S.safety.state == SafetyState::FAULT || S.safety.state == SafetyState::WAIT_ACK);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::COMM_TIMEOUT, (int)proc().err);
    TEST_ASSERT_FALSE(A.running);
    TEST_ASSERT_FALSE(S.comm_armed);                 // disarmed once idle and HV off
    API(ApiError::OK, send(CmdId::ACK));             // ACK possible without new timeout
    run(2);
    STATE(SafetyState::SAFE_START);
}
void test_heartbeat_keeps_traverse_alive() {
    running();
    API(ApiError::OK, traverse(10, 230, 600));
    run(15000);                                      // heartbeat every second
    STATE(SafetyState::RUNNING);
    TEST_ASSERT_EQUAL_INT(0, (int)proc().err);
}
void test_comm_watchdog_armed_by_hv_on_from_api() {
    homeAndReady();
    API(ApiError::OK, setBool("interlock", true));
    API(ApiError::OK, setBool("cooling_flow", true));
    API(ApiError::OK, send(CmdId::HV_ON));
    TEST_ASSERT_TRUE(proc().safety.comm_armed);
    heartbeat = false;
    run(5100);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::COMM_TIMEOUT, (int)proc().err);
}
void test_local_hv_on_disarms() {
    homeAndReady();
    API(ApiError::OK, setBool("interlock", true));
    API(ApiError::OK, setBool("cooling_flow", true));
    API(ApiError::OK, send(CmdId::HV_ON, CmdSource::LOCAL));
    TEST_ASSERT_FALSE(proc().safety.comm_armed);
    heartbeat = false;
    run(6000);
    STATE(SafetyState::RUNNING);
}
void test_fault_beeps_once() {
    running();
    int b = beeps;
    estop = true;
    run(50);
    TEST_ASSERT_EQUAL_INT(b + 1, beeps);
}

// ---- calibration ------------------------------------------------------------------------------
void test_calib_capture_requires_stability() {
    homeAndReady();
    S.stab = StabilityTracker{};            // fresh tracker: not stable yet
    API(ApiError::NOT_STABLE, calib(CalibAction::NEAR));
    run(700);
    API(ApiError::OK, calib(CalibAction::NEAR));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 200.0f, S.params.calib.near_raw);
}
void test_calib_save_validation_and_reset() {
    homeAndReady();
    run(700);
    API(ApiError::OK, calib(CalibAction::NEAR));
    API(ApiError::OK, calib(CalibAction::FAR));       // same distance -> invalid pair
    int p = persists;
    API(ApiError::CALIB_INVALID, calib(CalibAction::SAVE));
    TEST_ASSERT_EQUAL_INT(p, persists);
    API(ApiError::OK, calib(CalibAction::RESET));
    API(ApiError::OK, calib(CalibAction::SAVE));
    TEST_ASSERT_EQUAL_INT(p + 1, persists);
}
void test_calib_base_sets_base_dist() {
    homeAndReady();
    run(700);
    API(ApiError::OK, setNum("base_dist", 250));
    API(ApiError::OK, calib(CalibAction::BASE));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 200.0f, S.params.base_dist);
}
void test_calib_busy_during_motion() {
    homeAndReady();
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 200; m.has_speed = true; m.speed = 100;
    API(ApiError::OK, send(m));
    run(700);
    API(ApiError::BUSY, calib(CalibAction::NEAR));
}
void test_calib_base_needs_range() {
    homeAndReady();
    A.block = true;
    // sensor without signal: force out of range
    for (int i = 0; i < 40; ++i) { now++; if (now % 20 == 0) appMeasure(S, C, 0.0f, now); appTick(S, hw(), C); }
    TEST_ASSERT_FALSE(proc().measure.in_range);
    Command c = mk(CmdId::CALIB); c.calib = CalibAction::BASE;
    HwOutputs o; API(ApiError::PRECONDITION, appCommand(S, c, hw(), C, o));
}

// ---- misc -------------------------------------------------------------------------------------
void test_reboot_rules() {
    running();
    API(ApiError::STATE, send(CmdId::REBOOT));
    API(ApiError::OK, send(CmdId::HV_OFF));
    Command c = mk(CmdId::REBOOT);
    HwOutputs o;
    API(ApiError::OK, appCommand(S, c, hw(), C, o));
    TEST_ASSERT_TRUE(o.reboot);
}
void test_get_commands_always_ok_without_effect() {
    API(ApiError::OK, send(CmdId::GET_STATE));
    API(ApiError::OK, send(CmdId::GET_CONFIG));
    STATE(SafetyState::SAFE_START);
}
void test_proc_reflects_flags() {
    API(ApiError::OK, setNum("temp", 30.5));
    ProcData p = proc();
    TEST_ASSERT_FALSE(p.safety.temp_ok);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.5f, p.safety.temp);
    TEST_ASSERT_FALSE(p.safety.hv_ok);
    API(ApiError::OK, setBool("interlock", true));
    TEST_ASSERT_TRUE(proc().safety.interlock);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_boot_state);
    RUN_TEST(test_motion_commands_rejected_before_init);
    RUN_TEST(test_init_homes_and_goes_ready_with_beep);
    RUN_TEST(test_local_init_does_not_arm_watchdog);
    RUN_TEST(test_hv_on_needs_flags);
    RUN_TEST(test_flags_not_persisted_params_persisted);
    RUN_TEST(test_shutter_manual_open_close);
    RUN_TEST(test_traverse_auto_shutter_and_statistics);
    RUN_TEST(test_traverse_uses_param_defaults);
    RUN_TEST(test_traverse_rejected_in_ready);
    RUN_TEST(test_stop_during_traverse_closes_shutter);
    RUN_TEST(test_measure_mode_holds_and_averages);
    RUN_TEST(test_estop_during_traverse);
    RUN_TEST(test_temp_fault_while_idle_keeps_reference);
    RUN_TEST(test_temp_warning_band_drops_hv_and_stops_traverse);
    RUN_TEST(test_hv_off_during_traverse_stops_motion);
    RUN_TEST(test_comm_timeout_without_heartbeat);
    RUN_TEST(test_heartbeat_keeps_traverse_alive);
    RUN_TEST(test_comm_watchdog_armed_by_hv_on_from_api);
    RUN_TEST(test_local_hv_on_disarms);
    RUN_TEST(test_fault_beeps_once);
    RUN_TEST(test_calib_capture_requires_stability);
    RUN_TEST(test_calib_save_validation_and_reset);
    RUN_TEST(test_calib_base_sets_base_dist);
    RUN_TEST(test_calib_busy_during_motion);
    RUN_TEST(test_calib_base_needs_range);
    RUN_TEST(test_reboot_rules);
    RUN_TEST(test_get_commands_always_ok_without_effect);
    RUN_TEST(test_proc_reflects_flags);
    return UNITY_END();
}
