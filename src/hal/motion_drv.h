/**
 * @file motion_drv.h
 * @brief Adapter between DrvRequest (logic/motion_seq.h) and FastAccelStepper
 *        in I2S_MUX mode (FROZEN CONTRACT V1.3 §11).
 *
 * Implementation: src/hal/motion_drv.cpp (Sprint S2.5, from the smoketest).
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
 *   FORCE_STOP   -> forceStop()
 *   ZERO_HERE    -> forceStopAndNewPosition(0)
 *   NONE         -> nothing
 */
void motionDrvApply(const DrvRequest& r);

bool    motionDrvRunning();          ///< isRunning()
int32_t motionDrvPos();              ///< getCurrentPosition() in steps
float   motionDrvSpeedMmS();         ///< V1.3: |getCurrentSpeedInMilliHz()| / 1000 / STEPS_PER_MM (mm/s)
