/**
 * @file motion_drv.cpp
 * @brief Implementation of the FastAccelStepper I2S-MUX adapter.
 *
 * V1.5.18 contract — every call is recorded in shimHwLog() and tested natively.
 */
#include <Arduino.h>
#include <FastAccelStepper.h>
#include <math.h>
#include "config.h"
#include "hal/motion_drv.h"

static FastAccelStepperEngine engine;
static FastAccelStepper* stepper = nullptr;
static bool mux_ok = false;

bool motionDrvInit() {
    // Step 1
    engine.init(STEPPER_TASK_CORE);

    // Step 2
    if (!engine.initI2sMux(PIN_I2S_DATA, PIN_I2S_BCK, PIN_I2S_WS))
        return false;

    // Step 3
    for (int s = 0; s < 32; ++s)
        engine.i2sMuxSetBit(s, false);
    engine.i2sMuxSetBit(I2SO_TO_SLOT(I2SO_SHARED_EN), true);
    mux_ok = true;

    // Step 4
    stepper = engine.stepperConnectToPin(
        I2SO_TO_SLOT(I2SO_X_STEP) | PIN_I2S_FLAG, DRIVER_I2S_MUX);
    if (!stepper)
        return false;

    // Step 5
    stepper->setDirectionPin(
        I2SO_TO_SLOT(I2SO_X_DIR) | PIN_I2S_FLAG, DIR_POSITIVE_HIGH);
    stepper->setEnablePin(
        I2SO_TO_SLOT(I2SO_SHARED_EN) | PIN_I2S_FLAG, true);
    stepper->setAutoEnable(true);
    int32_t accel = (int32_t)(ACCEL_MM_S2 * STEPS_PER_MM);
    if (stepper->setAcceleration(accel) < 0)
        return false;

    // Step 6
    return true;
}

FastAccelStepperEngine* motionDrvEngine() {
    return mux_ok ? &engine : nullptr;
}

void motionDrvApply(const DrvRequest& r) {
    if (!stepper)
        return;

    switch (r.op) {
        case DrvOp::MOVE_TO:
            stepper->setSpeedInHz(r.speed_hz);
            stepper->moveTo(r.target_steps);
            break;
        case DrvOp::RUN_BACKWARD:
            stepper->setSpeedInHz(r.speed_hz);
            stepper->runBackward();
            break;
        case DrvOp::STOP:
            stepper->stopMove();
            break;
        case DrvOp::FORCE_STOP: {
            int32_t pos = stepper->getCurrentPosition();
            stepper->forceStopAndNewPosition(pos);
            break;
        }
        case DrvOp::ZERO_HERE:
            stepper->forceStopAndNewPosition(0);
            break;
        case DrvOp::NONE:
        default:
            break;
    }
}

bool motionDrvRunning() {
    return stepper ? stepper->isRunning() : false;
}

int32_t motionDrvPos() {
    return stepper ? stepper->getCurrentPosition() : 0;
}

float motionDrvSpeedMmS() {
    return stepper ? fabsf((float)stepper->getCurrentSpeedInMilliHz()) / 1000.0f / STEPS_PER_MM : 0.0f;
}