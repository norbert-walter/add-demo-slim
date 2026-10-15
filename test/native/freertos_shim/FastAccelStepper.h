#pragma once
// V1.5.18 host shim of FastAccelStepper (master 96459ed, the version used on the device) for the
// calls of src/hal/motion_drv.cpp and hal_io.cpp. Same signatures as the library. Every call is
// appended to shimHwLog() in call order ("fas init 1", "fas mux 21 16 17", "fas bit 7 1",
// "fas connect 65 4", "st dir 66 0", "st moveTo 1234", "st fsnp 77", ...). The test sets the
// results (failures, position, running, speed) in shimFas().
#include "Arduino.h"
#define PIN_I2S_FLAG 0x40
enum class FasDriver : uint8_t { MCPWM_PCNT = 0, RMT = 1, I2S_DIRECT = 2, I2S_MUX = 3, DONT_CARE = 255 };
#define DRIVER_MCPWM_PCNT FasDriver::MCPWM_PCNT
#define DRIVER_RMT FasDriver::RMT
#define DRIVER_I2S_DIRECT FasDriver::I2S_DIRECT
#define DRIVER_I2S_MUX FasDriver::I2S_MUX
#define DRIVER_DONT_CARE FasDriver::DONT_CARE
enum class MoveResultCode : int8_t { OK = 0, ErrorNoDirectionPin = -1, ErrorSpeedIsUndefined = -2, ErrorAccelerationIsUndefined = -3 };
#define MOVE_OK MoveResultCode::OK

struct ShimFas {
    // results the test can set
    bool    mux_fails = false;        ///< initI2sMux returns false
    bool    connect_fails = false;    ///< stepperConnectToPin returns nullptr
    int8_t  accel_result = 0;         ///< setAcceleration return value (< 0 = error)
    bool    running = false;          ///< isRunning()
    int32_t pos = 0;                  ///< getCurrentPosition()
    int32_t speed_mhz = 0;            ///< getCurrentSpeedInMilliHz()
    bool    stop_clears_running = true;   ///< forceStopAndNewPosition sets running = false
    // state recorded by the shim
    bool     engine_init = false, mux_ok = false;
    int      init_core = -1;
    uint32_t bits = 0;                ///< mux bits written with i2sMuxSetBit (bit n = slot n)
    uint32_t bits_written = 0;        ///< slots written at least once
    int      bit_writes = 0;
    int      connects = 0;
    void reset() { *this = ShimFas(); }
};
inline ShimFas& shimFas() { static ShimFas f; return f; }

class FastAccelStepperEngine;
class FastAccelStepper {
public:
    void setDirectionPin(uint8_t dirPin, bool dirHighCountsUp = true, uint16_t dir_change_delay_us = 0) {
        shimHw("st dir %u %d %u", dirPin, (int)dirHighCountsUp, dir_change_delay_us);
    }
    void setEnablePin(uint8_t enablePin, bool low_active_enables_stepper = true) {
        shimHw("st en %u %d", enablePin, (int)low_active_enables_stepper);
    }
    bool enableOutputs() { shimHw("st enableOutputs"); return true; }
    bool disableOutputs() { shimHw("st disableOutputs"); return true; }
    void setAutoEnable(bool auto_enable) { shimHw("st autoEnable %d", (int)auto_enable); }
    int32_t getCurrentPosition() const { shimHw("st getPos"); return shimFas().pos; }
    void setCurrentPosition(int32_t new_pos) { shimHw("st setPos %ld", (long)new_pos); shimFas().pos = new_pos; }
    bool isRunning() const { shimHw("st isRunning"); return shimFas().running; }
    int8_t setSpeedInHz(uint32_t speed_hz) { shimHw("st speed %lu", (unsigned long)speed_hz); return speed_hz ? 0 : -1; }
    int32_t getCurrentSpeedInMilliHz(bool realtime = true) const { shimHw("st getSpeed %d", (int)realtime); return shimFas().speed_mhz; }
    int8_t setAcceleration(int32_t step_s_s) { shimHw("st accel %ld", (long)step_s_s); return shimFas().accel_result; }
    MoveResultCode move(int32_t move, bool blocking = false) { shimHw("st move %ld %d", (long)move, (int)blocking); shimFas().running = true; return MOVE_OK; }
    MoveResultCode moveTo(int32_t position, bool blocking = false) { shimHw("st moveTo %ld %d", (long)position, (int)blocking); shimFas().running = true; return MOVE_OK; }
    MoveResultCode runForward() { shimHw("st runForward"); shimFas().running = true; return MOVE_OK; }
    MoveResultCode runBackward() { shimHw("st runBackward"); shimFas().running = true; return MOVE_OK; }
    void stopMove() { shimHw("st stopMove"); }
    void forceStop() { shimHw("st forceStop"); }
    void forceStopAndNewPosition(int32_t new_pos) {
        shimHw("st fsnp %ld", (long)new_pos); shimFas().pos = new_pos;
        if (shimFas().stop_clears_running) shimFas().running = false;
    }
};

class FastAccelStepperEngine {
public:
    void init(uint8_t cpu_core = 255) { shimHw("fas init %u", cpu_core); shimFas().engine_init = true; shimFas().init_core = cpu_core; }
    bool initI2sMux(uint8_t data_pin, uint8_t bclk_pin, uint8_t ws_pin) {
        shimHw("fas mux %u %u %u", data_pin, bclk_pin, ws_pin);
        shimFas().mux_ok = shimFas().engine_init && !shimFas().mux_fails;
        return shimFas().mux_ok;
    }
    void i2sMuxSetBit(uint8_t slot, bool value) {
        shimHw("fas bit %u %d", slot, (int)value);
        ShimFas& f = shimFas(); ++f.bit_writes;
        if (slot < 32) { f.bits_written |= (1u << slot); if (value) f.bits |= (1u << slot); else f.bits &= ~(1u << slot); }
    }
    FastAccelStepper* stepperConnectToPin(uint8_t step_pin, FasDriver driver_type = DRIVER_DONT_CARE) {
        shimHw("fas connect %u %d", step_pin, (int)driver_type);
        ShimFas& f = shimFas(); ++f.connects;
        if (f.connect_fails || !f.mux_ok) return nullptr;
        static FastAccelStepper s; return &s;
    }
};
