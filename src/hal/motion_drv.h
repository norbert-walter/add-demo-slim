/**
 * @file motion_drv.h
 * @brief Adapter between DrvRequest (logic/motion_seq.h) and FastAccelStepper
 *        in I2S_MUX mode (FROZEN CONTRACT V1.5 §11).
 *
 * Implementation: src/hal/motion_drv.cpp (Sprint H1, from the smoketest).
 * Only the control task calls these functions.
 *
 * FastAccelStepper (master / 2.x, ESP-IDF >= 5.3):
 *   engine.init(STEPPER_TASK_CORE);
 *   engine.initI2sMux(PIN_I2S_DATA, PIN_I2S_BCK, PIN_I2S_WS);
 *   stepper = engine.stepperConnectToPin(I2SO_TO_SLOT(I2SO_X_STEP) | PIN_I2S_FLAG, DRIVER_I2S_MUX);
 *   stepper->setDirectionPin(I2SO_TO_SLOT(I2SO_X_DIR) | PIN_I2S_FLAG, DIR_POSITIVE_HIGH);
 *   stepper->setEnablePin(I2SO_TO_SLOT(I2SO_SHARED_EN) | PIN_I2S_FLAG, true);  // active low
 *   stepper->setAutoEnable(true);
 *   stepper->setAcceleration(ACCEL_MM_S2 * STEPS_PER_MM);
 */
#pragma once

#include <stdint.h>
#include "logic/motion_seq.h"

class FastAccelStepperEngine;

/// Initialises engine, I2S mux and stepper. Returns false if any step fails.
bool motionDrvInit();

/// Engine pointer for hal_io (i2sMuxSetBit). NULL before motionDrvInit().
FastAccelStepperEngine* motionDrvEngine();

/**
 * Executes one request:
 *   MOVE_TO      -> setSpeedInHz(speed_hz); moveTo(target_steps)
 *   RUN_BACKWARD -> setSpeedInHz(speed_hz); runBackward()
 *   STOP         -> stopMove()                         (decelerated)
 *   FORCE_STOP   -> forceStopAndNewPosition(getCurrentPosition())   (V1.4, see below)
 *   ZERO_HERE    -> forceStopAndNewPosition(0)
 *   NONE         -> nothing
 *
 * V1.4 (smoketest 0a, 2026-10-11): forceStop() is NOT used. If it arrives before the
 * stepper task has taken over a pending move (e.g. MOVE_TO in one cycle, FORCE_STOP
 * in the next), FastAccelStepper drops the immediate stop in favour of the newer
 * move (RampGenerator::getNextCommand). forceStopAndNewPosition() stops the ramp
 * generator and empties the queue synchronously. app_logic additionally repeats
 * FORCE_STOP in every cycle while the driver still runs (step 5) – do not turn this
 * into a one-shot request.
 *
 * V1.5.18 – exact behaviour (tested natively with the FastAccelStepper shim: test/native/test_motion_drv):
 *  One file-static FastAccelStepperEngine and a FastAccelStepper* (NULL until connected).
 *  motionDrvInit(), in this order, stopping with false at the first failure:
 *    1. engine.init(STEPPER_TASK_CORE)
 *    2. engine.initI2sMux(PIN_I2S_DATA, PIN_I2S_BCK, PIN_I2S_WS)            false -> return false
 *    3. engine.i2sMuxSetBit(slot, false) for slot 0..31 in ascending order, then
 *       engine.i2sMuxSetBit(I2SO_TO_SLOT(I2SO_SHARED_EN), true)  (driver off; enable is active LOW)
 *       From here on motionDrvEngine() returns &engine.
 *    4. stepper = engine.stepperConnectToPin(I2SO_TO_SLOT(I2SO_X_STEP) | PIN_I2S_FLAG, DRIVER_I2S_MUX)
 *                                                                           NULL  -> return false
 *    5. setDirectionPin(I2SO_TO_SLOT(I2SO_X_DIR) | PIN_I2S_FLAG, DIR_POSITIVE_HIGH)  (2 arguments)
 *       setEnablePin(I2SO_TO_SLOT(I2SO_SHARED_EN) | PIN_I2S_FLAG, true)
 *       setAutoEnable(true)
 *       setAcceleration((int32_t)(ACCEL_MM_S2 * STEPS_PER_MM))                 < 0  -> return false
 *    6. return true. On the device it is called once (control task start); every call runs the
 *       steps above again from step 1 (the host tests call it several times).
 *  motionDrvApply: stepper NULL -> nothing. Otherwise exactly the calls of the table above, no
 *    others (no isRunning() checks, no extra stops). MOVE_TO / RUN_BACKWARD: setSpeedInHz first,
 *    then moveTo(target_steps) / runBackward() (one argument / none: non-blocking defaults).
 *    FORCE_STOP: getCurrentPosition() is read in the same call, immediately before
 *    forceStopAndNewPosition(). Every call of motionDrvApply executes its request again
 *    (no memory of the previous request).
 *  motionDrvRunning(): stepper ? stepper->isRunning() : false
 *  motionDrvPos():     stepper ? stepper->getCurrentPosition() : 0
 *  motionDrvSpeedMmS(): stepper ? fabsf((float)getCurrentSpeedInMilliHz()) / 1000.0f / STEPS_PER_MM : 0
 *    (getCurrentSpeedInMilliHz without argument = realtime; the sign is the direction)
 *  No delay(), no Serial, no heap after init: only the control task calls these functions.
 */
void motionDrvApply(const DrvRequest& r);

bool    motionDrvRunning();          ///< isRunning()
int32_t motionDrvPos();              ///< getCurrentPosition() in steps
float   motionDrvSpeedMmS();         ///< V1.3: |getCurrentSpeedInMilliHz()| / 1000 / STEPS_PER_MM (mm/s)
