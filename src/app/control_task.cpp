/**
 * @file control_task.cpp
 * @brief Control task – glue between hardware and pure logic (Contract V1.1, complete).
 *
 * RULES (do not break):
 *  - This task is the ONLY writer of AppState. Other tasks talk to it through
 *    command_queue (in) and snapshot (out).
 *  - No blocking calls, no Serial output, no NVS access, no heap allocation
 *    inside the loop. Persist/reboot are delegated to the network task.
 *  - Output order every cycle: shutter -> motor request -> HV LED.
 *  - V1.1: priority commands (STOP, HV_OFF, SHUTTER_CLOSE) are executed even
 *    if the cycle already has a driver request. The reply is sent AFTER the
 *    outputs were applied ("OK" = accepted and outputs set; the end of a
 *    motion is only visible in /proc).
 *  - V1.1: timing is measured on the target (CycleStats, CONTRACT §10):
 *    work time per cycle, distance between cycle starts (jitter), overruns and
 *    the latency of priority commands from cmdQueueSend() to applied outputs.
 *    The publication of the statistics itself (< 2 µs) is not included.
 */
#include "control_task.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include "config.h"
#include "logic/app_logic.h"
#include "hal/hal_io.h"
#include "hal/motion_drv.h"
#include "app/command_queue.h"
#include "app/snapshot.h"

static AppState  s_app;
static AppConfig s_cfg;

static AppConfig makeConfig() {
    AppConfig c;
    c.motion.steps_per_mm = STEPS_PER_MM;
    c.comm_timeout_ms = COMM_TIMEOUT_MS;
    return c;   // all other values: defaults from logic/*.h (CONTRACT)
}

static HwInputs readInputs(uint32_t now) {
    halUpdateInputs(now);
    HwInputs hw;
    hw.now_ms = now;
    hw.estop = halEstop();
    hw.limit_fwd = halLimitFwd();
    hw.limit_bwd = halLimitBwd();
    hw.drv_running = motionDrvRunning();
    hw.drv_pos_steps = motionDrvPos();
    hw.drv_speed_mm_min = motionDrvSpeedMmMin();
    hw.last_api_rx_ms = snapshotLastApiRx();
    return hw;
}

static void applyOutputs(const HwOutputs& out, uint32_t now) {
    halSetShutter(out.shutter_open);          // 1. close shutter first
    motionDrvApply(out.drv);                  // 2. motor
    halSetHvLed(out.hv_on);                   // 3. HV
    if (out.beep) halBeep(now, BEEP_MS);
    halUpdateBuzzer(now);
    if (out.persist) {
        snapshotPublishConfig(s_app.params, s_app.flags);
        snapshotRequestPersist();
    }
    if (out.reboot) snapshotRequestReboot();
}

static void controlLoop(void*) {
    esp_task_wdt_add(nullptr);
    TickType_t last = xTaskGetTickCount();
    uint32_t next_measure = 0;
    uint16_t samples[ADC_SAMPLES];
    CycleStats cs;
    uint32_t prev_start_us = 0;
    bool have_prev = false;
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
        const uint32_t t_start = (uint32_t)esp_timer_get_time();   // wrap-safe differences
        if (have_prev) {
            const uint32_t period = t_start - prev_start_us;
            if (period > cs.period_max_us) cs.period_max_us = period;
        }
        prev_start_us = t_start;
        have_prev = true;
        const uint32_t now = millis();

        // 1. measurement at 50 Hz
        if ((int32_t)(now - next_measure) >= 0) {
            next_measure = now + MEASURE_PERIOD_MS;
            halReadSensorSamples(samples, ADC_SAMPLES);
            appMeasure(s_app, s_cfg, medianMean(samples, ADC_SAMPLES, ADC_DISCARD), now);
        }

        // 2. cyclic logic
        HwInputs hw = readInputs(now);
        HwOutputs out = appTick(s_app, hw, s_cfg);

        // 3. at most one command per cycle. Priority commands always; normal
        //    commands only if the cycle has no driver request yet.
        Command cmd;
        bool have_cmd = cmdQueueReceive(cmd, out.drv.op == DrvOp::NONE);
        ApiError cmd_err = ApiError::OK;
        if (have_cmd) {
            cmd_err = appCommand(s_app, cmd, hw, s_cfg, out);
            if (cmd.id == CmdId::SET && cmd_err == ApiError::OK)
                snapshotPublishConfig(s_app.params, s_app.flags);   // GET_CONFIG sees flags too
        }

        // 4. outputs
        applyOutputs(out, now);

        // 5. reply only now: "OK" means accepted AND outputs set
        if (have_cmd) {
            if (appCmdIsPriority(cmd.id)) {
                const uint32_t lat = (uint32_t)esp_timer_get_time() - cmd.t_enq_us;
                if (lat > cs.prio_lat_max_us) cs.prio_lat_max_us = lat;
            }
            cmdQueueReply(cmd, cmd_err);
        }

        // 6. publish process image
        ProcData pd;
        appFillProc(s_app, s_cfg, hw, pd);
        snapshotPublish(pd);

        // 7. cycle time monitoring
        const uint32_t dt_us = (uint32_t)esp_timer_get_time() - t_start;
        if (dt_us > cs.cycle_max_us) cs.cycle_max_us = dt_us;
        if (dt_us > CONTROL_PERIOD_MS * 1000u) cs.overruns++;
        snapshotSetCycleStats(cs);

        esp_task_wdt_reset();
    }
}

bool controlTaskStart(const Params& params) {
    s_cfg = makeConfig();
    appInit(s_app);
    s_app.params = params;               // safety flags stay at fail-safe defaults
    if (!motionDrvInit()) return false;
    halInit();
    snapshotPublishConfig(s_app.params, s_app.flags);
    BaseType_t ok = xTaskCreatePinnedToCore(controlLoop, "control", CONTROL_TASK_STACK, nullptr,
                                            CONTROL_TASK_PRIO, nullptr, CONTROL_TASK_CORE);
    return ok == pdPASS;
}
