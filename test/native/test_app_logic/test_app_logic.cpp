// FROZEN TESTS (Contract V1.5.10) – speeds in mm/s – Sprint L4 must make these pass. Do not modify.
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
static int       beeps, persists;   // beeps = HOMED/FAULT/ESTOP patterns started
static int       snd[8];           // V1.3: started patterns per Sound

// ---- simulated sensor = demo geometry (V1.4.2, smoketest 0a 2026-10-11, axis coordinates) --
// Stair body 105..225 mm, six 20 mm steps centred at 115, 135 ... 215 mm with sensor
// distances 65, 60 ... 40 mm. Beside the material the sensor sees the lower boom at 135 mm.
// Thickness with base_dist 70: 5, 10 ... 30 mm.
static float distAt(float axis_mm) {
    if (axis_mm < 105.0f || axis_mm >= 225.0f) return 135.0f;
    return 65.0f - 5.0f * (float)(int)((axis_mm - 105.0f) / 20.0f);
}
static float mvForDist(float d) { return 1000.0f * powf(d / C.sensor.k_mm, 1.0f / C.sensor.exponent); }
// axis 0 = phys 2 mm (home pull-off); the sensor moves with the carriage
static float sensorMv() { return mvForDist(distAt((float)(A.phys_steps / C.motion.steps_per_mm) - 2.0f)); }

static HwInputs hw() {
    HwInputs h;
    h.now_ms = now; h.estop = estop;
    h.limit_fwd = A.limitFwd(); h.limit_bwd = A.limitBwd();
    h.drv_running = A.running; h.drv_pos_steps = A.drvPos();
    h.drv_speed_mm_s = A.running ? (float)(A.speed_hz / C.motion.steps_per_mm) : 0.0f;
    h.last_api_rx_ms = last_rx;
    return h;
}

// Control task rule (CONTRACT §10 step 3): priority commands always, others only
// if appTick produced no driver request in this cycle.
static bool mayExecute(const HwOutputs& out, CmdId id) {
    return out.drv.op == DrvOp::NONE || appCmdIsPriority(id);
}

// One 1 ms control cycle; optionally executes `cmd` (returns its result via *res).
static bool cycle(const Command* cmd = nullptr, ApiError* res = nullptr) {
    now++;
    A.step(1);
    if (heartbeat && now % 1000 == 0) last_rx = now;
    if (now % 20 == 0) appMeasure(S, C, sensorMv(), now);
    HwOutputs out = appTick(S, hw(), C);
    bool executed = false;
    if (cmd && mayExecute(out, cmd->id)) {
        *res = appCommand(S, *cmd, hw(), C, out);
        if (cmd->source == CmdSource::API) last_rx = now;
        executed = true;
    }
    A.apply(out.drv);
    if (out.sound != Sound::NONE) {
        snd[(int)out.sound]++;
        if (out.sound == Sound::HOMED || out.sound == Sound::FAULT || out.sound == Sound::ESTOP) beeps++;
    }
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
    A.phys_steps = 112.0 * C.motion.steps_per_mm;   // V1.4.2: axis 110 = first step
    now = 10000; estop = false; last_rx = 0; heartbeat = true; beeps = 0; persists = 0;
    for (int& x : snd) x = 0;
    run(100);
}
void tearDown() {}

static void homeAndReady() {
    API(ApiError::OK, send(CmdId::INIT));
    run(25000);                                  // ~110 mm at 5 mm/s
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
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 65.0f, p.measure.dist);    // axis at ~110 mm: first step (65 mm)
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 5.0f, p.measure.thick);
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
    run(25000);
    STATE(SafetyState::READY);
    TEST_ASSERT_TRUE(proc().motion.homed);
    TEST_ASSERT_EQUAL_INT(1, beeps);
    TEST_ASSERT_EQUAL_INT(1, snd[(int)Sound::HOMED]);
    TEST_ASSERT_EQUAL_INT(1, snd[(int)Sound::BOOT]);         // V1.3: boot pattern once after start
    TEST_ASSERT_EQUAL_INT(0, snd[(int)Sound::CLICK]);        // API commands do not click
    TEST_ASSERT_FALSE(S.comm_armed);             // idle and HV off -> disarmed
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, proc().motion.pos);
}
void test_local_init_does_not_arm_watchdog() {
    heartbeat = false;
    API(ApiError::OK, send(CmdId::INIT, CmdSource::LOCAL));
    TEST_ASSERT_FALSE(S.comm_armed);
    run(25000);                                  // > 5 s without packets
    STATE(SafetyState::READY);
}

// ---- HV / flags -----------------------------------------------------------------------
void test_hv_on_needs_flags() {
    homeAndReady();
    TEST_ASSERT_TRUE(proc().safety.hv_ok);                              // V1.5.23: flags true after boot
    API(ApiError::OK, setBool("interlock", false)); API(ApiError::OK, setBool("cooling_flow", false));
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
    API(ApiError::PRECONDITION, send(CmdId::SHUTTER_OPEN));   // park zone (0 mm)
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 145; m.has_speed = true; m.speed = 20;
    API(ApiError::OK, send(m));
    run(9000);                                                // 145 mm at 20 mm/s
    API(ApiError::PRECONDITION, send(CmdId::SHUTTER_OPEN));   // V1.2: material zone, manual open not allowed
    m.pos = 50;
    API(ApiError::OK, send(m));
    run(6000);
    API(ApiError::OK, send(CmdId::SHUTTER_OPEN));             // service zone
    TEST_ASSERT_TRUE(last_out.shutter_open);
    TEST_ASSERT_TRUE(proc().measure.valid);
    API(ApiError::OK, send(CmdId::SHUTTER_CLOSE));
    TEST_ASSERT_FALSE(last_out.shutter_open);
}

// ---- V1.2: PARK and SERVICE -----------------------------------------------------------------
void test_service_moves_to_maint_pos_and_allows_manual_shutter() {
    running();
    API(ApiError::OK, send(CmdId::SERVICE));
    TEST_ASSERT_TRUE(S.comm_armed);                           // motion started from the API
    run(5000);                                                // 40 mm at 10 mm/s
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 40.0f, proc().motion.pos);
    API(ApiError::OK, send(CmdId::SHUTTER_OPEN));
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, setNum("maint_pos", 55));               // customer-defined position
    API(ApiError::OK, send(CmdId::SERVICE));
    run(2000);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 55.0f, proc().motion.pos);
    API(ApiError::BAD_PARAM, setNum("maint_pos", 61));        // limited to the service zone
    API(ApiError::BAD_PARAM, setNum("maint_pos", 20));
}
void test_park_moves_to_2mm_and_closes_shutter() {
    running();
    API(ApiError::OK, send(CmdId::SERVICE));
    run(5000);
    API(ApiError::OK, send(CmdId::SHUTTER_OPEN));
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, send(CmdId::PARK));
    bool closed_before_park_zone_left = true;
    for (int i = 0; i < 5000; ++i) {                          // 38 mm at 10 mm/s
        cycle();
        if (last_out.shutter_open && proc().motion.pos <= C.safety.park_pos_mm) closed_before_park_zone_left = false;
    }
    TEST_ASSERT_TRUE(closed_before_park_zone_left);           // never open inside the park zone
    TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.0f, proc().motion.pos);
    TEST_ASSERT_TRUE(last_out.hv_on);                         // parking alone does not switch HV off
    STATE(SafetyState::RUNNING);
}
void test_park_and_service_gate_and_local() {
    API(ApiError::STATE, send(CmdId::PARK));                  // SAFE_START: not homed
    API(ApiError::STATE, send(CmdId::SERVICE));
    homeAndReady();
    API(ApiError::OK, send(CmdId::SERVICE, CmdSource::LOCAL));
    TEST_ASSERT_FALSE(S.comm_armed);
    API(ApiError::LOCAL_CONTROL, send(CmdId::PARK));          // remote motion blocked while LOCAL
    API(ApiError::LOCAL_CONTROL, send(CmdId::SERVICE));
    TEST_ASSERT_TRUE(appLocalBlocksApi(CmdId::PARK));
    TEST_ASSERT_TRUE(appLocalBlocksApi(CmdId::SERVICE));
}

// ---- traverse scenario --------------------------------------------------------------------
void test_traverse_auto_shutter_and_statistics() {
    running();
    API(ApiError::OK, traverse(80, 240, 20));                 // default profile (V1.5), max speed
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::TRAVERSE);
    bool sawOpen = false, sawOpenOutsideMeasZone = false;
    for (int i = 0; i < 20000; ++i) {                         // 8 s approach + 8 s run
        cycle();
        if (last_out.shutter_open) {
            sawOpen = true;
            if (proc().motion.pos <= C.safety.maint_max_mm) sawOpenOutsideMeasZone = true;
        }
        if (S.seq.mode == OpMode::NONE && i > 100) break;
    }
    run(5);
    TEST_ASSERT_TRUE(sawOpen);
    TEST_ASSERT_FALSE(sawOpenOutsideMeasZone);   // V1.5: automatic shutter only in the measuring zone
    TEST_ASSERT_TRUE(last_out.shutter_open);     // V1.5: still open during the hold time (3 s)
    run(2900);
    TEST_ASSERT_TRUE(last_out.shutter_open);
    run(200);
    TEST_ASSERT_FALSE(last_out.shutter_open);    // closed after the hold time
    TEST_ASSERT_TRUE(last_out.hv_on);            // HV stays on
    ProcData p = proc();
    TEST_ASSERT_TRUE(p.motion.mode == OpMode::NONE);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 240.0f, p.motion.pos);
    TEST_ASSERT_TRUE(p.measure.n > 50);          // samples only on material
    TEST_ASSERT_FLOAT_WITHIN(1.5f, 17.5f, p.measure.avg);   // mean of the 5 ... 30 mm steps
    STATE(SafetyState::RUNNING);
}
void test_traverse_uses_param_defaults() {
    running();
    API(ApiError::BAD_PARAM, setNum("traverse_start", 60));   // V1.5: service zone not allowed
    API(ApiError::OK, setNum("traverse_start", 70));
    API(ApiError::OK, setNum("traverse_end", 80));
    API(ApiError::OK, setNum("traverse_speed", 20));
    API(ApiError::OK, send(CmdId::START_TRAVERSE));
    run(9000);                                                // approach 7 s + 0.5 s
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 80.0f, proc().motion.pos);
}
void test_traverse_rejected_in_ready() {
    homeAndReady();
    API(ApiError::STATE, traverse(70, 240, 200));
}
void test_traverse_and_measure_outside_measuring_zone_rejected() {   // V1.5
    running();
    API(ApiError::BAD_PARAM, traverse(60, 240, 10));
    API(ApiError::BAD_PARAM, traverse(240, 40, 10));
    Command m = mk(CmdId::START_MEASURE); m.has_pos = true; m.pos = 60;
    API(ApiError::BAD_PARAM, send(m));
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
}
// V1.5: continuous back-and-forth profile - the shutter stays open over all runs
static bool runUntilIdle(uint32_t max_ms, int& closings) {
    bool was_open = last_out.shutter_open;
    for (uint32_t i = 0; i < max_ms; ++i) {
        cycle();
        if (was_open && !last_out.shutter_open) closings++;
        was_open = last_out.shutter_open;
        if (S.seq.mode == OpMode::NONE && i > 5) return true;
    }
    return false;
}
void test_continuous_profile_keeps_shutter_open() {
    running();
    int closings = 0;
    API(ApiError::OK, traverse(80, 240, 20));
    TEST_ASSERT_TRUE(runUntilIdle(20000, closings));
    TEST_ASSERT_TRUE(last_out.shutter_open);
    for (int pass = 0; pass < 4; ++pass) {                    // client turns around within ~0.3 s
        run(300);
        const bool fwd = (pass % 2) == 1;
        API(ApiError::OK, traverse(fwd ? 80.0f : 240.0f, fwd ? 240.0f : 80.0f, 20));
        TEST_ASSERT_TRUE(runUntilIdle(12000, closings));
        TEST_ASSERT_TRUE(proc().measure.n > 50);              // every run has its own statistics
    }
    TEST_ASSERT_EQUAL_INT(0, closings);                       // never closed between the runs
    TEST_ASSERT_TRUE(last_out.shutter_open);
    run(3100);
    TEST_ASSERT_FALSE(last_out.shutter_open);                 // programme ended -> hold time -> closed
}
void test_shutter_hold_parameter() {
    running();
    API(ApiError::OK, setNum("shutter_hold", 0));
    API(ApiError::OK, traverse(80, 100, 20));
    int closings = 0;
    TEST_ASSERT_TRUE(runUntilIdle(12000, closings));
    run(2);
    TEST_ASSERT_FALSE(last_out.shutter_open);                 // hold 0: closes at the end of the run
    API(ApiError::OK, setNum("shutter_hold", 1.0));
    API(ApiError::OK, traverse(100, 80, 20));
    TEST_ASSERT_TRUE(runUntilIdle(12000, closings));
    run(950);
    TEST_ASSERT_TRUE(last_out.shutter_open);
    run(100);
    TEST_ASSERT_FALSE(last_out.shutter_open);
    API(ApiError::BAD_PARAM, setNum("shutter_hold", 10.1));
}
// V1.5: the approach of the next run belongs to the programme - no closing even if it
// takes longer than the hold time (axis stays in the measuring zone)
void test_shutter_stays_open_during_long_approach() {
    running();
    API(ApiError::OK, traverse(200, 240, 20));
    int closings = 0;
    TEST_ASSERT_TRUE(runUntilIdle(30000, closings));
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, traverse(80, 240, 20));                 // approach 160 mm at 10 mm/s = 16 s
    for (int i = 0; i < 10000; ++i) { cycle(); if (!last_out.shutter_open) closings++; }
    TEST_ASSERT_TRUE(S.seq.phase == SeqPhase::TRAV_APPROACH);
    TEST_ASSERT_EQUAL_INT(0, closings);
}
// V1.5: the automatic shutter closes when the axis enters the service zone (<= 60 mm)
void test_auto_shutter_closes_entering_service_zone() {
    running();
    API(ApiError::OK, setNum("shutter_hold", 10));
    API(ApiError::OK, traverse(100, 80, 20));
    int closings = 0;
    TEST_ASSERT_TRUE(runUntilIdle(15000, closings));
    TEST_ASSERT_TRUE(last_out.shutter_open);                  // hold time running
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 30; m.has_speed = true; m.speed = 20;
    API(ApiError::OK, send(m));
    bool openInServiceZone = false;
    for (int i = 0; i < 3000; ++i) {
        cycle();
        if (last_out.shutter_open && proc().motion.pos <= C.safety.maint_max_mm) openInServiceZone = true;
    }
    TEST_ASSERT_FALSE(openInServiceZone);
    TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_TRUE(last_out.hv_on);
}
// V1.5: STOP ends the measuring programme - the shutter closes at once, also during the hold time
void test_stop_closes_shutter_during_hold_time() {
    running();
    API(ApiError::OK, traverse(80, 100, 20));
    int closings = 0;
    TEST_ASSERT_TRUE(runUntilIdle(12000, closings));
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, send(CmdId::STOP));
    TEST_ASSERT_FALSE(last_out.shutter_open);                 // in the same cycle ("OK" = outputs set)
}
// V1.5: a manually opened shutter (service zone) is not closed by the hold time
void test_manual_shutter_not_closed_by_hold_time() {
    running();
    API(ApiError::OK, send(CmdId::SERVICE));
    run(5000);
    API(ApiError::OK, send(CmdId::SHUTTER_OPEN));
    run(5000);
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, send(CmdId::STOP));                     // STOP without a programme: manual shutter stays
    TEST_ASSERT_TRUE(last_out.shutter_open);
}
void test_stop_during_traverse_closes_shutter() {
    running();
    API(ApiError::OK, traverse(70, 240, 10));
    run(8000);                                       // approach 70 mm at 10 mm/s = 7 s
    TEST_ASSERT_TRUE(last_out.shutter_open);
    API(ApiError::OK, send(CmdId::STOP));
    TEST_ASSERT_FALSE(last_out.shutter_open);        // V1.5: closed in the STOP cycle itself
    run(2);
    TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_TRUE(S.seq.homed);               // controlled stop keeps reference
}
void test_measure_mode_holds_and_averages() {
    running();
    Command m = mk(CmdId::START_MEASURE); m.has_pos = true; m.pos = 155;   // V1.4.2 normal position: 55 mm -> 15 mm
    API(ApiError::OK, send(m));
    run(20000);                                               // ~45 mm at 10 mm/s + settling
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
    API(ApiError::OK, traverse(70, 240, 10));
    run(8000);                                       // approach 70 mm at 10 mm/s = 7 s
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
    API(ApiError::OK, traverse(70, 240, 10));
    run(8000);                                       // approach 70 mm at 10 mm/s = 7 s
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
    API(ApiError::OK, traverse(70, 240, 10));
    run(8000);                                       // approach 70 mm at 10 mm/s = 7 s
    API(ApiError::OK, send(CmdId::HV_OFF));
    run(3);
    STATE(SafetyState::READY);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_FALSE(last_out.shutter_open);
}
void test_comm_timeout_without_heartbeat() {
    running();
    API(ApiError::OK, traverse(70, 240, 5));
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
    API(ApiError::OK, traverse(70, 240, 10));
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
    int e = snd[(int)Sound::ESTOP];
    estop = true;
    run(50);
    TEST_ASSERT_EQUAL_INT(b + 1, beeps);
    TEST_ASSERT_EQUAL_INT(e + 1, snd[(int)Sound::ESTOP]);    // V1.3: E-stop -> one 4 s tone
}

// ---- calibration ------------------------------------------------------------------------------
void test_calib_capture_requires_stability() {
    homeAndReady();
    S.stab = StabilityTracker{};            // fresh tracker: not stable yet
    API(ApiError::NOT_STABLE, calib(CalibAction::NEAR));
    run(700);
    API(ApiError::OK, calib(CalibAction::NEAR));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 135.0f, S.params.calib.near_raw);   // at 0 mm: lower boom
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
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 135.0f, S.params.base_dist);       // at 0 mm: lower boom
}
void test_calib_busy_during_motion() {
    homeAndReady();
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 200; m.has_speed = true; m.speed = 2;
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


// ---- V1.1: LOCAL operator priority (CONTRACT §6.6) ----------------------------------------
static Command moveCmd(float pos, float speed, CmdSource src) {
    Command c = mk(CmdId::MOVE, src); c.has_pos = c.has_speed = true; c.pos = pos; c.speed = speed; return c;
}
void test_v11_priority_and_block_tables() {
    TEST_ASSERT_TRUE(appCmdIsPriority(CmdId::STOP));
    TEST_ASSERT_TRUE(appCmdIsPriority(CmdId::HV_OFF));
    TEST_ASSERT_TRUE(appCmdIsPriority(CmdId::SHUTTER_CLOSE));
    TEST_ASSERT_FALSE(appCmdIsPriority(CmdId::ACK));
    TEST_ASSERT_FALSE(appCmdIsPriority(CmdId::INIT));
    const CmdId blocked[] = {CmdId::INIT, CmdId::MOVE, CmdId::START_MEASURE, CmdId::START_TRAVERSE,
                             CmdId::HV_ON, CmdId::SHUTTER_OPEN, CmdId::CALIB, CmdId::ACK, CmdId::REBOOT};
    for (CmdId c : blocked) TEST_ASSERT_TRUE_MESSAGE(appLocalBlocksApi(c), cmdName(c));
    const CmdId free_[] = {CmdId::STOP, CmdId::HV_OFF, CmdId::SHUTTER_CLOSE, CmdId::SET,
                           CmdId::GET_STATE, CmdId::GET_CONFIG};
    for (CmdId c : free_) TEST_ASSERT_FALSE_MESSAGE(appLocalBlocksApi(c), cmdName(c));
}
void test_local_command_blocks_api_start_commands() {
    homeAndReady();
    API(ApiError::OK, setBool("interlock", true));
    API(ApiError::OK, setBool("cooling_flow", true));
    TEST_ASSERT_FALSE(proc().local);
    API(ApiError::OK, send(CmdId::HV_ON, CmdSource::LOCAL));
    TEST_ASSERT_TRUE(proc().local);
    API(ApiError::LOCAL_CONTROL, send(CmdId::HV_ON));
    API(ApiError::LOCAL_CONTROL, send(CmdId::INIT));
    API(ApiError::LOCAL_CONTROL, send(moveCmd(50, 10, CmdSource::API)));
    API(ApiError::LOCAL_CONTROL, send(CmdId::START_MEASURE));
    API(ApiError::LOCAL_CONTROL, traverse(70, 240, 10));
    API(ApiError::LOCAL_CONTROL, send(CmdId::SHUTTER_OPEN));
    API(ApiError::LOCAL_CONTROL, calib(CalibAction::RESET));
    API(ApiError::LOCAL_CONTROL, send(CmdId::REBOOT));
    API(ApiError::LOCAL_CONTROL, send(CmdId::ACK));          // LOCAL check comes before the state gate
    STATE(SafetyState::RUNNING);
    TEST_ASSERT_FALSE(S.comm_armed);
    API(ApiError::OK, setNum("temp", 20.0));                 // SET stays allowed (fault simulation)
    API(ApiError::OK, send(CmdId::GET_STATE));
    API(ApiError::OK, send(CmdId::SHUTTER_CLOSE));
    API(ApiError::OK, send(CmdId::HV_OFF));                  // remote safe-direction always allowed
    STATE(SafetyState::READY);
    TEST_ASSERT_FALSE(last_out.hv_on);
}
void test_local_released_30s_after_last_touch_when_idle() {
    API(ApiError::OK, send(CmdId::INIT, CmdSource::LOCAL));
    uint32_t t0 = now;
    API(ApiError::LOCAL_CONTROL, send(CmdId::INIT));
    run(25000);                                              // homing done, idle
    STATE(SafetyState::READY);
    while (now < t0 + C.local_hold_ms - 20) cycle();
    TEST_ASSERT_TRUE(S.local);
    run(40);
    TEST_ASSERT_FALSE(S.local);
    TEST_ASSERT_FALSE(proc().local);
    API(ApiError::OK, send(CmdId::INIT));                    // remote control possible again
}
void test_local_rejected_command_still_takes_priority() {
    homeAndReady();
    API(ApiError::STATE, send(CmdId::ACK, CmdSource::LOCAL)); // gate rejects, but operator is present
    TEST_ASSERT_TRUE(S.local);
    API(ApiError::LOCAL_CONTROL, send(CmdId::INIT));
}
void test_local_held_while_hv_on() {
    homeAndReady();
    API(ApiError::OK, setBool("interlock", true));
    API(ApiError::OK, setBool("cooling_flow", true));
    API(ApiError::OK, send(CmdId::HV_ON, CmdSource::LOCAL));
    run(C.local_hold_ms + 10000);
    TEST_ASSERT_TRUE(S.local);                               // HV still on -> operator keeps priority
    STATE(SafetyState::RUNNING);                             // and no comm watchdog fault
    API(ApiError::OK, send(CmdId::HV_OFF));
    run(2);
    TEST_ASSERT_FALSE(S.local);                              // idle, HV off, hold time long over
}
void test_local_held_until_local_motion_ends() {
    homeAndReady();
    API(ApiError::OK, send(moveCmd(230, 5, CmdSource::LOCAL)));     // 230 mm at 5 mm/s = 46 s
    run(C.local_hold_ms + 5000);
    TEST_ASSERT_TRUE(proc().motion.state == MotionState::MOVING);
    TEST_ASSERT_TRUE(S.local);
    run(20000);
    TEST_ASSERT_TRUE(proc().motion.state == MotionState::IDLE);
    TEST_ASSERT_FALSE(S.local);
}
void test_local_takes_over_remote_operation() {
    running();
    API(ApiError::OK, traverse(70, 240, 5));
    TEST_ASSERT_TRUE(S.comm_armed);
    run(1000);
    API(ApiError::OK, send(CmdId::STOP, CmdSource::LOCAL));
    TEST_ASSERT_TRUE(S.local);
    TEST_ASSERT_FALSE(S.comm_armed);                         // operator present: remote watchdog off
    heartbeat = false;
    run(6000);
    TEST_ASSERT_EQUAL_INT(0, (int)proc().err);
    API(ApiError::LOCAL_CONTROL, traverse(70, 240, 5));
}
void test_api_stop_allowed_during_local_operation() {
    running();
    Command t = mk(CmdId::START_TRAVERSE, CmdSource::LOCAL);
    API(ApiError::OK, send(t));
    run(2000);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::TRAVERSE);
    API(ApiError::OK, send(CmdId::STOP));
    run(2);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_FALSE(last_out.shutter_open);
}
void test_v1524_local_hold_zero_turns_local_priority_off() {
    homeAndReady();
    uint32_t keep = C.local_hold_ms; C.local_hold_ms = 0;
    API(ApiError::OK, send(CmdId::HV_ON));                                // remote HV on arms the watchdog
    TEST_ASSERT_TRUE(S.comm_armed);
    API(ApiError::OK, send(CmdId::STOP, CmdSource::LOCAL));              // display command
    TEST_ASSERT_FALSE(S.local);                                           // no LOCAL priority
    TEST_ASSERT_FALSE(proc().local);
    TEST_ASSERT_FALSE(S.comm_armed);                                      // but the operator is present
    API(ApiError::OK, send(CmdId::HV_OFF));
    run(100);
    API(ApiError::OK, send(CmdId::INIT));                                 // API not blocked
    C.local_hold_ms = keep;
}
void test_local_ack_in_fault() {
    running();
    API(ApiError::OK, send(CmdId::HV_OFF, CmdSource::LOCAL));
    estop = true; run(2); estop = false; run(5);
    STATE(SafetyState::WAIT_ACK);
    API(ApiError::LOCAL_CONTROL, send(CmdId::ACK));
    API(ApiError::OK, send(CmdId::ACK, CmdSource::LOCAL));
    run(2);
    STATE(SafetyState::READY);
}

// ---- V1.1: STOP priority (CONTRACT §6.7) ----------------------------------------------
void test_stop_executes_in_first_cycle_during_traverse() {
    running();
    for (int k = 0; k < 25; ++k) {                           // STOP at many different sequencer phases
        API(ApiError::OK, traverse(70, 240, 20));
        run((uint32_t)(k * 211));
        Command c = mk(CmdId::STOP); ApiError r = ApiError::UNKNOWN_CMD;
        TEST_ASSERT_TRUE_MESSAGE(cycle(&c, &r), "STOP deferred");   // never waits for a free cycle
        API(ApiError::OK, r);
        TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
        run(300);
        TEST_ASSERT_FALSE(A.running);
        STATE(SafetyState::RUNNING);
    }
}
// Cycle in which STOP arrives exactly when appTick emits a driver request.
// Returns true if that situation occurred (and the STOP was handled per the rule).
static bool cycleStopOnRequest(bool& executed, ApiError& res, DrvOp& tick_op, DrvOp& final_op) {
    now++;
    A.step(1);
    if (heartbeat && now % 1000 == 0) last_rx = now;
    if (now % 20 == 0) appMeasure(S, C, sensorMv(), now);
    HwOutputs out = appTick(S, hw(), C);
    bool hit = out.drv.op != DrvOp::NONE;
    tick_op = out.drv.op;
    executed = false;
    if (hit) {
        Command c = mk(CmdId::STOP);
        if (mayExecute(out, c.id)) { res = appCommand(S, c, hw(), C, out); executed = true; }
    }
    final_op = out.drv.op;
    A.apply(out.drv);
    last_out = out;
    return hit;
}
void test_stop_executed_in_cycle_with_driver_request() {
    running();
    API(ApiError::OK, traverse(70, 240, 20));
    bool executed = false; ApiError r = ApiError::UNKNOWN_CMD; DrvOp t = DrvOp::NONE, f = DrvOp::NONE;
    bool hit = false;
    for (int i = 0; i < 20000 && !hit; ++i) hit = cycleStopOnRequest(executed, r, t, f);
    TEST_ASSERT_TRUE_MESSAGE(hit, "traverse never produced a driver request");
    TEST_ASSERT_TRUE(t == DrvOp::MOVE_TO);                   // approach -> run segment
    TEST_ASSERT_TRUE_MESSAGE(executed, "STOP deferred although a driver request was pending");
    API(ApiError::OK, r);
    TEST_ASSERT_TRUE(f == DrvOp::STOP);                      // STOP replaced the MOVE_TO
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    run(500);
    TEST_ASSERT_FALSE(A.running);
    TEST_ASSERT_FALSE(last_out.shutter_open);
}
void test_stop_overrides_pending_move_request() {
    running();
    API(ApiError::OK, traverse(70, 240, 10));
    run(500);
    HwOutputs out; out.drv.op = DrvOp::MOVE_TO; out.drv.target_steps = 12345;
    API(ApiError::OK, appCommand(S, mk(CmdId::STOP), hw(), C, out));
    TEST_ASSERT_TRUE(out.drv.op == DrvOp::STOP);
}
void test_stop_never_weakens_immediate_stop() {
    running();
    Command t = mk(CmdId::START_TRAVERSE);
    API(ApiError::OK, send(t));
    run(500);
    HwOutputs out; out.drv.op = DrvOp::FORCE_STOP;
    appCommand(S, mk(CmdId::STOP), hw(), C, out);
    TEST_ASSERT_TRUE(out.drv.op == DrvOp::FORCE_STOP);
    API(ApiError::OK, send(CmdId::HV_OFF));
    run(500);
    API(ApiError::OK, send(CmdId::INIT));                    // homing seek (axis not at switch)
    run(10);
    HwOutputs out2; out2.drv.op = DrvOp::ZERO_HERE;
    appCommand(S, mk(CmdId::STOP), hw(), C, out2);
    TEST_ASSERT_TRUE(out2.drv.op == DrvOp::ZERO_HERE);
}

// ---- V1.1: priority overflow latch ------------------------------------------------------
void test_prio_latch_order_coalesce_and_source() {
    uint32_t bits = 0; Command out;
    TEST_ASSERT_FALSE(appPrioLatchTake(bits, out));
    appPrioLatchSet(bits, mk(CmdId::INIT));                  // ignored
    appPrioLatchSet(bits, mk(CmdId::ACK));
    TEST_ASSERT_EQUAL_UINT32(0, bits);
    appPrioLatchSet(bits, mk(CmdId::SHUTTER_CLOSE));
    appPrioLatchSet(bits, mk(CmdId::HV_OFF, CmdSource::LOCAL));
    appPrioLatchSet(bits, mk(CmdId::STOP));
    appPrioLatchSet(bits, mk(CmdId::STOP));                  // coalesced
    TEST_ASSERT_TRUE(appPrioLatchTake(bits, out));
    TEST_ASSERT_TRUE(out.id == CmdId::STOP);
    TEST_ASSERT_TRUE(out.source == CmdSource::API);          // origin per command: LOCAL HV_OFF does not leak
    TEST_ASSERT_FALSE(out.want_reply);
    TEST_ASSERT_TRUE(appPrioLatchTake(bits, out));
    TEST_ASSERT_TRUE(out.id == CmdId::HV_OFF);
    TEST_ASSERT_TRUE(out.source == CmdSource::LOCAL);
    TEST_ASSERT_TRUE(appPrioLatchTake(bits, out));
    TEST_ASSERT_TRUE(out.id == CmdId::SHUTTER_CLOSE);
    TEST_ASSERT_TRUE(out.source == CmdSource::API);
    TEST_ASSERT_FALSE(appPrioLatchTake(bits, out));
    TEST_ASSERT_EQUAL_UINT32(0, bits);
    appPrioLatchSet(bits, mk(CmdId::STOP));                  // same command from API and LOCAL
    appPrioLatchSet(bits, mk(CmdId::STOP, CmdSource::LOCAL));
    TEST_ASSERT_TRUE(appPrioLatchTake(bits, out));
    TEST_ASSERT_TRUE(out.source == CmdSource::LOCAL);        // -> one LOCAL STOP
    TEST_ASSERT_FALSE(appPrioLatchTake(bits, out));
}

// All pending priority commands in ONE cycle (control task step 3a).
void test_all_priority_commands_in_one_cycle() {
    running();
    API(ApiError::OK, traverse(70, 240, 10));
    run(8000);                                       // approach 70 mm at 10 mm/s = 7 s
    TEST_ASSERT_TRUE(last_out.shutter_open);
    now++; A.step(1);
    HwOutputs out = appTick(S, hw(), C);
    const CmdId ids[] = {CmdId::SHUTTER_CLOSE, CmdId::HV_OFF, CmdId::STOP, CmdId::STOP};
    for (CmdId id : ids) API(ApiError::OK, appCommand(S, mk(id), hw(), C, out));
    A.apply(out.drv);
    TEST_ASSERT_TRUE(out.drv.op == DrvOp::STOP);
    TEST_ASSERT_FALSE(out.hv_on);
    TEST_ASSERT_FALSE(out.shutter_open);
    STATE(SafetyState::READY);
    run(500);
    TEST_ASSERT_FALSE(A.running);
    TEST_ASSERT_TRUE(proc().motion.mode == OpMode::NONE);
    TEST_ASSERT_TRUE(S.seq.homed);                           // controlled stop keeps reference
}

// ---- V1.1: concurrent faults -------------------------------------------------------------
void test_estop_and_cooling_loss_simultaneous() {
    running();
    API(ApiError::OK, traverse(70, 240, 10));
    run(8000);                                       // approach 70 mm at 10 mm/s = 7 s
    estop = true;
    S.flags.cooling_flow = false;                            // same cycle (only the test may do this)
    run(1);
    STATE(SafetyState::FAULT);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::ESTOP, (int)proc().err);   // fault wins over HV drop
    TEST_ASSERT_FALSE(last_out.hv_on); TEST_ASSERT_FALSE(last_out.shutter_open);
    TEST_ASSERT_FALSE(A.running);
    TEST_ASSERT_FALSE(S.seq.homed);
}


// ---- V1.3: sounds, heartbeat, E-stop wiring parameter, speeds in mm/s ------------------------
void test_click_only_for_accepted_local_commands() {
    homeAndReady();
    int c0 = snd[(int)Sound::CLICK];
    API(ApiError::OK, send(CmdId::STOP, CmdSource::LOCAL));
    TEST_ASSERT_EQUAL_INT(c0 + 1, snd[(int)Sound::CLICK]);
    API(ApiError::OK, setBool("interlock", false));                       // V1.5.23: flags are true after boot
    TEST_ASSERT_EQUAL_INT(c0 + 1, snd[(int)Sound::CLICK]);
    API(ApiError::PRECONDITION, send(CmdId::HV_ON, CmdSource::LOCAL));   // flags missing -> no click
    TEST_ASSERT_EQUAL_INT(c0 + 1, snd[(int)Sound::CLICK]);
    API(ApiError::OK, send(CmdId::STOP));                                 // API -> no click
    TEST_ASSERT_EQUAL_INT(c0 + 1, snd[(int)Sound::CLICK]);
}
void test_estop_tone_4s_not_cut_by_click() {
    running();
    estop = true; run(2);
    TEST_ASSERT_EQUAL_INT(1, snd[(int)Sound::ESTOP]);
    int on = 0;
    for (int i = 0; i < 4500; ++i) {
        if (i == 100) { Command c = mk(CmdId::HV_OFF, CmdSource::LOCAL); ApiError r; cycle(&c, &r); }
        else cycle();
        if (last_out.buzzer) on++;
    }
    TEST_ASSERT_INT_WITHIN(5, 4000, on);                    // one 4 s tone, the click did not cut it
}
void test_other_fault_three_beeps() {
    running();
    API(ApiError::OK, setNum("temp", 35.0));
    run(2);
    TEST_ASSERT_EQUAL_INT(1, snd[(int)Sound::FAULT]);
    TEST_ASSERT_EQUAL_INT(0, snd[(int)Sound::ESTOP]);
    int edges = 0; bool prev = last_out.buzzer;
    for (int i = 0; i < 1000; ++i) { cycle(); if (last_out.buzzer && !prev) edges++; prev = last_out.buzzer; }
    TEST_ASSERT_TRUE(edges >= 2 && edges <= 3);             // 3 beeps (the first one started before the loop)
}
void test_reboot_plays_pattern_and_requests_restart() {
    Command c = mk(CmdId::REBOOT);
    HwOutputs o;
    API(ApiError::OK, appCommand(S, c, hw(), C, o));
    TEST_ASSERT_TRUE(o.reboot);
    TEST_ASSERT_TRUE(o.sound == Sound::REBOOT);
    TEST_ASSERT_TRUE(o.buzzer);
    TEST_ASSERT_TRUE(soundDurationMs(Sound::REBOOT) <= 700);   // REBOOT_DELAY_MS
}
void test_heartbeat_half_hertz() {
    int on = 0, edges = 0; bool prev = last_out.heartbeat;
    for (int i = 0; i < 4000; ++i) {
        cycle();
        if (last_out.heartbeat) on++;
        if (last_out.heartbeat != prev) edges++;
        prev = last_out.heartbeat;
    }
    TEST_ASSERT_INT_WITHIN(2, 2000, on);                    // 50 % duty
    TEST_ASSERT_TRUE(edges >= 3 && edges <= 4);              // toggles every second -> 0.5 Hz
}
void test_estop_nc_only_while_idle_and_hv_off() {
    running();
    Command c = mk(CmdId::SET); strcpy(c.key, "estop_nc"); c.vtype = ValueType::BOOL; c.b = true;
    API(ApiError::PRECONDITION, send(c));                    // HV on
    API(ApiError::OK, send(CmdId::HV_OFF));
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 150;
    API(ApiError::OK, send(m));
    API(ApiError::PRECONDITION, send(c));                    // axis moving
    run(20000);
    int p = persists;
    API(ApiError::OK, send(c));
    TEST_ASSERT_TRUE(S.params.estop_nc);
    TEST_ASSERT_EQUAL_INT(p + 1, persists);                  // wiring setting is persisted
}
void test_speeds_mm_per_s() {
    homeAndReady();
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 150;   // no speed -> normal 10 mm/s
    API(ApiError::OK, send(m));
    run(2000);
    TEST_ASSERT_EQUAL_DOUBLE((double)mmPerSecToHz(10.0f, C.motion), A.speed_hz);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 10.0f, proc().motion.vel);     // /proc vel in mm/s
    m.speed = 25; m.has_speed = true;
    run(15000);
    API(ApiError::BAD_PARAM, send(m));                            // > 20 mm/s
    m.speed = 20;
    API(ApiError::OK, send(m));
}

// V1.4.1: nonsense values from the API never move the axis (end to end).
void test_non_finite_command_values_rejected() {
    running();
    int32_t pos0 = A.drvPos();
    const float bad[] = {NAN, INFINITY, -INFINITY};
    for (float b : bad) {
        Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = b;
        API(ApiError::BAD_PARAM, send(m));
        m.pos = 100; m.has_speed = true; m.speed = b;
        API(ApiError::BAD_PARAM, send(m));
        API(ApiError::BAD_PARAM, traverse(b, 100, 10));
        API(ApiError::BAD_PARAM, traverse(70, b, 10));
        API(ApiError::BAD_PARAM, traverse(70, 100, b));
        Command s = mk(CmdId::START_MEASURE); s.has_pos = true; s.pos = b;
        API(ApiError::BAD_PARAM, send(s));
        API(ApiError::BAD_PARAM, setNum("measure_pos", b));
    }
    run(500);
    TEST_ASSERT_FALSE(A.running);
    TEST_ASSERT_EQUAL_INT32(pos0, A.drvPos());
    STATE(SafetyState::RUNNING);                                   // no fault, nothing changed
}

// ---- V1.5.9: mutation test before Sprint L4 (direct calls, independent of the scenario) ----
static HwInputs hwAt(uint32_t t, int32_t pos_steps = 0, bool running_ = false, bool estop_ = false) {
    HwInputs h; h.now_ms = t; h.drv_pos_steps = pos_steps; h.drv_running = running_; h.estop = estop_;
    h.last_api_rx_ms = t; return h;
}
static void forceRunning(AppState& s) {          // RUNNING with HV on and all conditions met
    s.flags.interlock = true; s.flags.cooling_flow = true;
    s.seq.homed = true; s.safety.state = SafetyState::RUNNING; s.safety.hv_on = true;
}
void test_v159_measure_chain_uses_calibrated_distance() {
    appInit(S);
    S.params.calib.near_raw = 40; S.params.calib.near_ref = 45;     // offset +5 mm
    S.params.calib.far_raw = 65;  S.params.calib.far_ref = 70;
    appMeasure(S, C, mvForDist(50.0f), 1000);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 50.0f, S.raw_mm);                // uncalibrated
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 55.0f, S.dist_mm);               // calibrated
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 15.0f, S.thick_mm);              // 70 - 55
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 55.0f, S.stab.ref);              // stability on dist
}
void test_v159_measure_out_of_range_resets_filter_and_stability() {
    appInit(S);
    for (int i = 0; i < 5; ++i) appMeasure(S, C, mvForDist(60.0f), 1000 + 20 * i);
    appMeasure(S, C, 0.0f, 1200);                                   // no signal
    TEST_ASSERT_FALSE(S.in_range);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, S.thick_mm);
    TEST_ASSERT_FALSE(S.on_material);
    TEST_ASSERT_FALSE(S.stab.initialized);
    appMeasure(S, C, mvForDist(45.0f), 1220);                       // filter starts again
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 45.0f, S.raw_mm);
}
void test_v159_stats_only_while_acquiring_with_open_shutter() {
    appInit(S);
    S.safety.shutter_open = true;                                   // e.g. manual shutter, idle
    appMeasure(S, C, mvForDist(50.0f), 1000);                       // idle: no statistics
    TEST_ASSERT_EQUAL_UINT32(0, S.stats.n);
    S.safety.shutter_open = false;
    S.seq.phase = SeqPhase::TRAV_RUN;                               // acquiring, shutter closed
    appMeasure(S, C, mvForDist(50.0f), 1020);
    TEST_ASSERT_EQUAL_UINT32(0, S.stats.n);
    S.safety.shutter_open = true;
    appMeasure(S, C, mvForDist(50.0f), 1040);
    TEST_ASSERT_EQUAL_UINT32(1, S.stats.n);
}
void test_v159_homing_failure_faults() {
    C.motion.homing_timeout_ms = 500;                               // axis is ~110 mm away
    API(ApiError::OK, send(CmdId::INIT));
    run(1000);
    TEST_ASSERT_TRUE(S.safety.state == SafetyState::FAULT || S.safety.state == SafetyState::WAIT_ACK);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::HOMING_FAILED, (int)S.safety.err);
    TEST_ASSERT_TRUE(snd[(int)Sound::FAULT] >= 1);
}
void test_v159_fault_stops_sequencer_before_driver_runs() {
    appInit(S); S.seq.homed = true; S.safety.state = SafetyState::READY;
    HwOutputs o;
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 100;
    API(ApiError::OK, appCommand(S, m, hwAt(20000), C, o));        // sequencer MOVE, driver not yet running
    HwOutputs t = appTick(S, hwAt(20001, 0, false, true), C);       // E-stop
    TEST_ASSERT_EQUAL_INT((int)DrvOp::FORCE_STOP, (int)t.drv.op);
    TEST_ASSERT_EQUAL_INT((int)SeqPhase::IDLE, (int)S.seq.phase);
    TEST_ASSERT_FALSE(S.seq.homed);
}
void test_v159_stats_reset_on_new_acquisition() {
    running();
    API(ApiError::OK, traverse(110, 130, 10));
    int closings = 0;
    TEST_ASSERT_TRUE(runUntilIdle(30000, closings));
    S.stats.sum = 1.0e6; S.stats.n = 1;                              // garbage from an old run
    API(ApiError::OK, traverse(130, 110, 10));
    TEST_ASSERT_TRUE(runUntilIdle(30000, closings));
    TEST_ASSERT_TRUE(statsMean(S.stats) < 100.0f);
}
void test_v159_shutter_closed_during_approach_in_measuring_zone() {
    running();
    API(ApiError::OK, traverse(150, 200, 10));                       // approach 0 -> 150 mm
    int opened_early = 0;
    for (int i = 0; i < 30000 && S.seq.phase == SeqPhase::TRAV_APPROACH; ++i) {
        cycle();
        if (S.seq.phase == SeqPhase::TRAV_APPROACH && last_out.shutter_open) opened_early++;
    }
    TEST_ASSERT_EQUAL_INT((int)SeqPhase::TRAV_RUN, (int)S.seq.phase);
    TEST_ASSERT_EQUAL_INT(0, opened_early);
}
void test_v159_local_release_uses_config_and_wraps() {
    appInit(S); C.local_hold_ms = 1000;
    S.local = true; S.local_last_ms = 50000;
    appTick(S, hwAt(50999), C); TEST_ASSERT_TRUE(S.local);
    appTick(S, hwAt(51000), C); TEST_ASSERT_FALSE(S.local);
    S.local = true; S.local_last_ms = 0xFFFFFF00u;                   // timer wraps
    appTick(S, hwAt(0xFFFFFF10u), C); TEST_ASSERT_TRUE(S.local);
    appTick(S, hwAt(0xFFFFFF00u + 1000u), C); TEST_ASSERT_FALSE(S.local);
}
void test_v159_heartbeat_phase() {
    appInit(S);
    TEST_ASSERT_TRUE(appTick(S, hwAt(12500), C).heartbeat);         // 12 s: even -> on
    TEST_ASSERT_FALSE(appTick(S, hwAt(13500), C).heartbeat);        // 13 s: odd -> off
}
void test_v159_park_speed_and_traverse_speed_default() {
    appInit(S); S.seq.homed = true; S.safety.state = SafetyState::READY;
    HwOutputs o;
    API(ApiError::OK, appCommand(S, mk(CmdId::PARK), hwAt(20000, 8000), C, o));
    TEST_ASSERT_EQUAL_UINT32(mmPerSecToHz(C.motion.approach_speed, C.motion), o.drv.speed_hz);
    appInit(S); forceRunning(S); S.params.traverse_speed = 3.0f;
    HwOutputs o2;
    API(ApiError::OK, appCommand(S, mk(CmdId::START_TRAVERSE), hwAt(20000, 8000), C, o2));
    TEST_ASSERT_EQUAL_UINT32(mmPerSecToHz(3.0f, C.motion), S.seq.trav_speed_hz);
}
void test_v159_driver_request_merge_equal_rank_replaces() {
    appInit(S);                                                      // SAFE_START, idle
    HwOutputs o; o.drv.op = DrvOp::FORCE_STOP;
    HwInputs h = hwAt(20000); h.limit_bwd = true;                    // already on the home switch
    API(ApiError::OK, appCommand(S, mk(CmdId::INIT), h, C, o));
    TEST_ASSERT_EQUAL_INT((int)DrvOp::ZERO_HERE, (int)o.drv.op);     // rank 3 >= rank 3
    appInit(S); S.seq.homed = true; S.safety.state = SafetyState::READY;
    HwOutputs o2; o2.drv.op = DrvOp::STOP;
    Command m = mk(CmdId::MOVE); m.has_pos = true; m.pos = 100;
    API(ApiError::OK, appCommand(S, m, hwAt(20000), C, o2));
    TEST_ASSERT_EQUAL_INT((int)DrvOp::STOP, (int)o2.drv.op);         // MOVE_TO never weakens STOP
}
void test_v159_calib_base_uses_dist_near_uses_raw() {
    homeAndReady();
    S.params.calib.near_raw = 40; S.params.calib.near_ref = 45;     // offset +5 mm
    S.params.calib.far_raw = 65;  S.params.calib.far_ref = 70;
    run(800);
    API(ApiError::OK, calib(CalibAction::BASE));
    TEST_ASSERT_FLOAT_WITHIN(0.6f, S.raw_mm + 5.0f, S.params.base_dist);
    API(ApiError::OK, calib(CalibAction::NEAR));
    TEST_ASSERT_FLOAT_WITHIN(0.6f, 135.0f, S.params.calib.near_raw); // raw, not calibrated
}
void test_v159_rejected_hv_on_does_not_arm_watchdog() {
    homeAndReady();
    run(100);
    TEST_ASSERT_FALSE(S.comm_armed);
    API(ApiError::OK, setBool("interlock", false));                   // V1.5.23: flags are true after boot
    TEST_ASSERT_FALSE(S.comm_armed);
    API(ApiError::PRECONDITION, send(CmdId::HV_ON));                 // flags missing
    TEST_ASSERT_FALSE(S.comm_armed);
}
void test_v159_fault_entered_by_command_starts_sound() {
    appInit(S); S.seq.homed = true; S.safety.state = SafetyState::READY;
    S.flags.interlock = true; S.flags.cooling_flow = true;
    HwOutputs o;
    appCommand(S, mk(CmdId::HV_ON), hwAt(20000, 0, false, true), C, o);   // E-stop pressed
    TEST_ASSERT_EQUAL_INT((int)SafetyState::FAULT, (int)S.safety.state);
    TEST_ASSERT_EQUAL_INT((int)Sound::ESTOP, (int)o.sound);
}
void test_v159_proc_fields() {
    appInit(S);
    S.flags.interlock = true; S.flags.cooling_flow = true; S.flags.temp_c = C.safety.temp_warn_c;
    HwInputs h = hwAt(23456); h.limit_fwd = true;
    ProcData p; appFillProc(S, C, h, p);
    TEST_ASSERT_EQUAL_UINT32(23456, p.ts_ms);
    TEST_ASSERT_TRUE(p.safety.temp_ok);                              // temp == warn is ok
    TEST_ASSERT_TRUE(p.safety.hv_ok);
    TEST_ASSERT_TRUE(p.motion.limit_fwd);
    TEST_ASSERT_FALSE(p.motion.limit_bwd);
    h.estop = true; appFillProc(S, C, h, p);
    TEST_ASSERT_FALSE(p.safety.hv_ok);                               // E-stop blocks HV
    TEST_ASSERT_TRUE(p.safety.estop);
}
void test_v159_safety_position_uses_motion_config() {
    appInit(S); C.motion.steps_per_mm = 100.0f; forceRunning(S);
    HwOutputs o;                                                     // 5500 steps = 55 mm: service zone
    API(ApiError::OK, appCommand(S, mk(CmdId::SHUTTER_OPEN), hwAt(20000, 5500), C, o));
    TEST_ASSERT_TRUE(o.shutter_open);
}
void test_v159_comm_timeout_uses_config() {
    C.comm_timeout_ms = 1000;
    running();                                                       // HV_ON from API arms
    TEST_ASSERT_TRUE(S.comm_armed);
    heartbeat = false; last_rx = now;
    run(1200);
    TEST_ASSERT_EQUAL_INT((int)ErrorCode::COMM_TIMEOUT, (int)S.safety.err);
}

// ---- V1.5.10: review of L4.1b (appMeasure exactly as in app_logic.h) ----
void test_v1510_in_range_uses_unfiltered_mm_not_calibrated_dist() {
    appInit(S);
    S.params.calib.near_raw = 40; S.params.calib.near_ref = 45;     // offset +5 mm
    S.params.calib.far_raw = 65;  S.params.calib.far_ref = 70;
    appMeasure(S, C, mvForDist(297.0f), 1000);                      // mm 297 in range (max 300)
    TEST_ASSERT_TRUE(S.in_range);                                   // although dist 302 > max
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 302.0f, S.dist_mm);
    TEST_ASSERT_TRUE(S.stab.initialized);
}
void test_v1510_out_of_range_raw_and_dist_are_mm() {
    appInit(S);
    for (int i = 0; i < 5; ++i) appMeasure(S, C, mvForDist(60.0f), 1000 + 20 * i);
    appMeasure(S, C, 0.0f, 1200);                                   // no signal
    TEST_ASSERT_EQUAL_FLOAT(SENSOR_NO_SIGNAL_MM, S.raw_mm);
    TEST_ASSERT_EQUAL_FLOAT(SENSOR_NO_SIGNAL_MM, S.dist_mm);
    TEST_ASSERT_EQUAL_FLOAT(SENSOR_NO_SIGNAL_MM, proc().measure.dist);
    appMeasure(S, C, mvForDist(400.0f), 1220);                      // beyond max_mm
    TEST_ASSERT_FALSE(S.in_range);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 400.0f, S.raw_mm);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 400.0f, S.dist_mm);
}
void test_v1510_out_of_range_keeps_stability_settings() {
    appInit(S);
    S.stab.window_ms = 1234; S.stab.tol_mm = 0.9f;
    appMeasure(S, C, mvForDist(60.0f), 1000);
    appMeasure(S, C, 0.0f, 1020);
    TEST_ASSERT_FALSE(S.stab.initialized);                          // only this flag is reset
    TEST_ASSERT_EQUAL_UINT32(1234, S.stab.window_ms);
    TEST_ASSERT_EQUAL_FLOAT(0.9f, S.stab.tol_mm);
}
void test_v1510_stats_only_in_trav_run_or_meas_hold() {
    static const SeqPhase no[] = { SeqPhase::HOME_SEEK, SeqPhase::HOME_LATCH, SeqPhase::HOME_PULLOFF,
                                   SeqPhase::MOVE, SeqPhase::TRAV_APPROACH, SeqPhase::MEAS_APPROACH };
    appInit(S);
    S.safety.shutter_open = true;
    uint32_t t = 1000;
    for (SeqPhase ph : no) {
        S.seq.phase = ph;
        appMeasure(S, C, mvForDist(50.0f), t += 20);
        TEST_ASSERT_EQUAL_UINT32(0, S.stats.n);
    }
    S.seq.phase = SeqPhase::MEAS_HOLD;
    appMeasure(S, C, mvForDist(50.0f), t += 20);
    TEST_ASSERT_EQUAL_UINT32(1, S.stats.n);
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
    RUN_TEST(test_service_moves_to_maint_pos_and_allows_manual_shutter);
    RUN_TEST(test_park_moves_to_2mm_and_closes_shutter);
    RUN_TEST(test_park_and_service_gate_and_local);
    RUN_TEST(test_traverse_auto_shutter_and_statistics);
    RUN_TEST(test_traverse_uses_param_defaults);
    RUN_TEST(test_traverse_rejected_in_ready);
    RUN_TEST(test_stop_during_traverse_closes_shutter);
    RUN_TEST(test_traverse_and_measure_outside_measuring_zone_rejected);
    RUN_TEST(test_continuous_profile_keeps_shutter_open);
    RUN_TEST(test_shutter_hold_parameter);
    RUN_TEST(test_shutter_stays_open_during_long_approach);
    RUN_TEST(test_auto_shutter_closes_entering_service_zone);
    RUN_TEST(test_stop_closes_shutter_during_hold_time);
    RUN_TEST(test_manual_shutter_not_closed_by_hold_time);
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
    RUN_TEST(test_v11_priority_and_block_tables);
    RUN_TEST(test_local_command_blocks_api_start_commands);
    RUN_TEST(test_local_released_30s_after_last_touch_when_idle);
    RUN_TEST(test_local_rejected_command_still_takes_priority);
    RUN_TEST(test_local_held_while_hv_on);
    RUN_TEST(test_local_held_until_local_motion_ends);
    RUN_TEST(test_local_takes_over_remote_operation);
    RUN_TEST(test_api_stop_allowed_during_local_operation);
    RUN_TEST(test_v1524_local_hold_zero_turns_local_priority_off);
    RUN_TEST(test_local_ack_in_fault);
    RUN_TEST(test_stop_executes_in_first_cycle_during_traverse);
    RUN_TEST(test_stop_executed_in_cycle_with_driver_request);
    RUN_TEST(test_stop_overrides_pending_move_request);
    RUN_TEST(test_stop_never_weakens_immediate_stop);
    RUN_TEST(test_estop_and_cooling_loss_simultaneous);
    RUN_TEST(test_prio_latch_order_coalesce_and_source);
    RUN_TEST(test_all_priority_commands_in_one_cycle);
    RUN_TEST(test_click_only_for_accepted_local_commands);
    RUN_TEST(test_estop_tone_4s_not_cut_by_click);
    RUN_TEST(test_other_fault_three_beeps);
    RUN_TEST(test_reboot_plays_pattern_and_requests_restart);
    RUN_TEST(test_heartbeat_half_hertz);
    RUN_TEST(test_estop_nc_only_while_idle_and_hv_off);
    RUN_TEST(test_speeds_mm_per_s);
    RUN_TEST(test_non_finite_command_values_rejected);
    RUN_TEST(test_v159_measure_chain_uses_calibrated_distance);
    RUN_TEST(test_v159_measure_out_of_range_resets_filter_and_stability);
    RUN_TEST(test_v159_stats_only_while_acquiring_with_open_shutter);
    RUN_TEST(test_v159_homing_failure_faults);
    RUN_TEST(test_v159_fault_stops_sequencer_before_driver_runs);
    RUN_TEST(test_v159_stats_reset_on_new_acquisition);
    RUN_TEST(test_v159_shutter_closed_during_approach_in_measuring_zone);
    RUN_TEST(test_v159_local_release_uses_config_and_wraps);
    RUN_TEST(test_v159_heartbeat_phase);
    RUN_TEST(test_v159_park_speed_and_traverse_speed_default);
    RUN_TEST(test_v159_driver_request_merge_equal_rank_replaces);
    RUN_TEST(test_v159_calib_base_uses_dist_near_uses_raw);
    RUN_TEST(test_v159_rejected_hv_on_does_not_arm_watchdog);
    RUN_TEST(test_v159_fault_entered_by_command_starts_sound);
    RUN_TEST(test_v159_proc_fields);
    RUN_TEST(test_v159_safety_position_uses_motion_config);
    RUN_TEST(test_v159_comm_timeout_uses_config);
    RUN_TEST(test_v1510_in_range_uses_unfiltered_mm_not_calibrated_dist);
    RUN_TEST(test_v1510_out_of_range_raw_and_dist_are_mm);
    RUN_TEST(test_v1510_out_of_range_keeps_stability_settings);
    RUN_TEST(test_v1510_stats_only_in_trav_run_or_meas_hold);
    return UNITY_END();
}
