// FROZEN TESTS (Contract V1.5.18) – Sprint H1 must make these pass. Do not modify.
// Host test of the REAL src/hal/motion_drv.cpp with the FastAccelStepper shim
// (test/native/freertos_shim/FastAccelStepper.h, signatures of master 96459ed). Every library call
// is recorded in shimHwLog() in call order; the tests compare the exact call sequences.
// The tests run in this order (motion_drv keeps its engine and stepper in file-static variables).
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <math.h>
#include <string>
#include <vector>
#include "hal/motion_drv.cpp"   // implementation under test

static std::vector<std::string>& L() { return shimHwLog(); }
static void expectLog(const std::vector<std::string>& exp) {
    char m[160];
    snprintf(m, sizeof m, "call count: expected %u, got %u", (unsigned)exp.size(), (unsigned)L().size());
    for (size_t i = 0; i < exp.size() && i < L().size(); ++i) {
        char mm[200]; snprintf(mm, sizeof mm, "call #%u", (unsigned)i);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(exp[i].c_str(), L()[i].c_str(), mm);
    }
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(exp.size(), L().size(), m);
}
static std::vector<std::string> initPrefix() {
    std::vector<std::string> v = {"fas init 1", "fas mux 21 16 17"};
    for (int s = 0; s < 32; ++s) v.push_back("fas bit " + std::to_string(s) + " 0");
    v.push_back("fas bit 0 1");
    return v;
}
static std::vector<std::string> fullInit() {
    std::vector<std::string> v = initPrefix();
    v.push_back("fas connect 65 3");                                    // step slot 1 | PIN_I2S_FLAG, I2S_MUX
    v.push_back("st dir 66 0 0");                                       // dir slot 2 | flag, DIR_POSITIVE_HIGH
    v.push_back("st en 64 1");                                          // enable slot 0 | flag, active low
    v.push_back("st autoEnable 1");
    v.push_back("st accel " + std::to_string((long)(int32_t)(ACCEL_MM_S2 * STEPS_PER_MM)));
    return v;
}
static DrvRequest req(DrvOp op, int32_t t = 0, uint32_t hz = 0) { DrvRequest r; r.op = op; r.target_steps = t; r.speed_hz = hz; return r; }

void setUp() {}
void tearDown() {}

void test_before_init_nothing_happens() {
    L().clear();
    TEST_ASSERT_NULL(motionDrvEngine());
    TEST_ASSERT_FALSE(motionDrvRunning());
    TEST_ASSERT_EQUAL_INT32(0, motionDrvPos());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, motionDrvSpeedMmS());
    for (int op = 0; op <= (int)DrvOp::ZERO_HERE; ++op) motionDrvApply(req((DrvOp)op, 100, 500));
    expectLog({});
}
void test_init_fails_when_mux_fails() {
    shimFas().reset(); shimFas().mux_fails = true; L().clear();
    TEST_ASSERT_FALSE(motionDrvInit());
    expectLog({"fas init 1", "fas mux 21 16 17"});
    TEST_ASSERT_NULL(motionDrvEngine());                              // no mux -> no engine for hal_io
}
void test_init_fails_when_connect_fails() {
    shimFas().reset(); shimFas().connect_fails = true; L().clear();
    TEST_ASSERT_FALSE(motionDrvInit());
    std::vector<std::string> exp = initPrefix(); exp.push_back("fas connect 65 3");
    expectLog(exp);
    TEST_ASSERT_NOT_NULL(motionDrvEngine());                          // mux works: buzzer usable
    TEST_ASSERT_EQUAL_HEX32(1u, shimFas().bits);                      // only the enable bit set (driver off)
    L().clear();
    motionDrvApply(req(DrvOp::MOVE_TO, 100, 500));                    // no stepper: nothing
    TEST_ASSERT_FALSE(motionDrvRunning());
    expectLog({});
}
void test_init_fails_when_acceleration_rejected() {
    shimFas().reset(); shimFas().accel_result = -1; L().clear();
    TEST_ASSERT_FALSE(motionDrvInit());
    expectLog(fullInit());
}
void test_init_exact_sequence() {
    shimFas().reset(); L().clear();
    TEST_ASSERT_TRUE(motionDrvInit());
    expectLog(fullInit());
    TEST_ASSERT_EQUAL_INT(STEPPER_TASK_CORE, shimFas().init_core);
    TEST_ASSERT_TRUE(motionDrvEngine() != nullptr);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, shimFas().bits_written);     // every slot defined
    TEST_ASSERT_EQUAL_HEX32(1u << I2SO_SHARED_EN, shimFas().bits);    // driver disabled, buzzer off
}
void test_apply_exact_calls_per_request() {
    L().clear(); motionDrvApply(req(DrvOp::MOVE_TO, 1234, 785));
    expectLog({"st speed 785", "st moveTo 1234 0"});
    L().clear(); motionDrvApply(req(DrvOp::MOVE_TO, -20, 100));
    expectLog({"st speed 100", "st moveTo -20 0"});
    L().clear(); motionDrvApply(req(DrvOp::RUN_BACKWARD, 999, 392));
    expectLog({"st speed 392", "st runBackward"});
    L().clear(); motionDrvApply(req(DrvOp::STOP, 5, 6));
    expectLog({"st stopMove"});
    shimFas().pos = 777;
    L().clear(); motionDrvApply(req(DrvOp::FORCE_STOP, 5, 6));
    expectLog({"st getPos", "st fsnp 777"});
    shimFas().pos = 4321; shimFas().running = true;
    L().clear(); motionDrvApply(req(DrvOp::ZERO_HERE, 5, 6));
    expectLog({"st fsnp 0"});
    TEST_ASSERT_EQUAL_INT32(0, shimFas().pos);
    L().clear(); motionDrvApply(req(DrvOp::NONE, 5, 6));
    expectLog({});
}
void test_force_stop_after_pending_move_and_repeated() {
    // V1.4: MOVE_TO in one cycle, FORCE_STOP in the next while the stepper task has not taken the
    // move yet. forceStop() would be dropped by the library; forceStopAndNewPosition is required.
    shimFas().pos = 100; shimFas().stop_clears_running = false;
    L().clear();
    motionDrvApply(req(DrvOp::MOVE_TO, 5000, 785));
    shimFas().pos = 103;
    motionDrvApply(req(DrvOp::FORCE_STOP));
    shimFas().pos = 104;
    motionDrvApply(req(DrvOp::FORCE_STOP));                           // app_logic repeats it each cycle
    motionDrvApply(req(DrvOp::FORCE_STOP));
    expectLog({"st speed 785", "st moveTo 5000 0", "st getPos", "st fsnp 103", "st getPos", "st fsnp 104",
               "st getPos", "st fsnp 104"});
    shimFas().stop_clears_running = true; shimFas().running = false;
}
void test_same_request_is_executed_every_call() {
    L().clear();
    for (int i = 0; i < 3; ++i) motionDrvApply(req(DrvOp::STOP));
    for (int i = 0; i < 2; ++i) motionDrvApply(req(DrvOp::MOVE_TO, 50, 200));
    expectLog({"st stopMove", "st stopMove", "st stopMove", "st speed 200", "st moveTo 50 0", "st speed 200", "st moveTo 50 0"});
}
void test_queries() {
    shimFas().running = true; shimFas().pos = -1234; shimFas().speed_mhz = -785000;
    L().clear();
    TEST_ASSERT_TRUE(motionDrvRunning());
    TEST_ASSERT_EQUAL_INT32(-1234, motionDrvPos());
    float v = motionDrvSpeedMmS();
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, fabsf(-785000.0f) / 1000.0f / STEPS_PER_MM, v);   // ~10.0 mm/s, sign dropped
    expectLog({"st isRunning", "st getPos", "st getSpeed 1"});
    shimFas().speed_mhz = 392500;
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 392500.0f / 1000.0f / STEPS_PER_MM, motionDrvSpeedMmS());
    shimFas().running = false;
    TEST_ASSERT_FALSE(motionDrvRunning());
}
void test_no_serial_output() {
    TEST_ASSERT_EQUAL_STRING("", Serial.out.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_before_init_nothing_happens);
    RUN_TEST(test_init_fails_when_mux_fails);
    RUN_TEST(test_init_fails_when_connect_fails);
    RUN_TEST(test_init_fails_when_acceleration_rejected);
    RUN_TEST(test_init_exact_sequence);
    RUN_TEST(test_apply_exact_calls_per_request);
    RUN_TEST(test_force_stop_after_pending_move_and_repeated);
    RUN_TEST(test_same_request_is_executed_every_call);
    RUN_TEST(test_queries);
    RUN_TEST(test_no_serial_output);
    return UNITY_END();
}
