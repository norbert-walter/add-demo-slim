/**
 * @file control_task.cpp
 * @brief Control task – glue between hardware and pure logic (Contract V1.3, complete).
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
static volatile bool s_reset_stats = false;   // V1.5.25: set by setup() at "boot: ready"

void controlResetCycleStats() { s_reset_stats = true; }

// V1.5.25: breakdown of the last overrun / late start (written here, read by the net task)
static portMUX_TYPE s_diag_mux = portMUX_INITIALIZER_UNLOCKED;
static CycleDiag    s_diag;

void controlGetCycleDiag(CycleDiag& d) {
    taskENTER_CRITICAL(&s_diag_mux);
    d = s_diag;
    taskEXIT_CRITICAL(&s_diag_mux);
}

static AppConfig makeConfig() {
    AppConfig c;
    c.motion.steps_per_mm = STEPS_PER_MM;
    c.comm_timeout_ms = COMM_TIMEOUT_MS;
    c.local_hold_ms = LOCAL_HOLD_MS;                // V1.5.24
    static_assert(LOCAL_HOLD_MS <= 60000, "LOCAL_HOLD_MS: 0 ... 60000 ms");
    return c;   // all other values: defaults from logic/*.h (CONTRACT)
}

static HwInputs readInputs(uint32_t now) {
    halSetEstopActiveHigh(s_app.params.estop_nc);   // V1.3: wiring from the parameter
    halUpdateInputs(now);
    HwInputs hw;
    hw.now_ms = now;
    hw.estop = halEstop();
    hw.limit_fwd = halLimitFwd();
    hw.limit_bwd = halLimitBwd();
    hw.drv_running = motionDrvRunning();
    hw.drv_pos_steps = motionDrvPos();
    hw.drv_speed_mm_s = motionDrvSpeedMmS();
    hw.last_api_rx_ms = snapshotLastApiRx();
    return hw;
}

static void applyOutputs(const HwOutputs& out) {
    halSetShutter(out.shutter_open);          // 1. close shutter first
    motionDrvApply(out.drv);                  // 2. motor
    halSetHvLed(out.hv_on);                   // 3. HV
    halSetBuzzer(out.buzzer);                 // V1.3: pattern computed by sound_logic
    halSetHeartbeat(out.heartbeat);           // V1.3: 0.5 Hz – proves the control loop runs
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
    uint8_t n_samples = 0;                    // V1.5.25: samples collected for the next measurement
    CycleStats cs;
    uint32_t prev_start_us = 0;
    bool have_prev = false;
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
        const uint32_t t_start = (uint32_t)esp_timer_get_time();   // wrap-safe differences
        if (s_reset_stats) {                  // V1.5.25: statistics count from "boot: ready" on
            s_reset_stats = false;
            cs = CycleStats();
            have_prev = false;
        }
        if (have_prev) {
            const uint32_t period = t_start - prev_start_us;
            if (period > cs.period_max_us) cs.period_max_us = period;
        }
        const uint32_t period_us = have_prev ? t_start - prev_start_us : 0;
        prev_start_us = t_start;
        have_prev = true;
        const uint32_t now = millis();

        // 1. measurement at 50 Hz. V1.5.25: ONE ADC sample per cycle (about 40 us) instead of
        //    16 in one cycle every 20 ms (about 0.7 ms, with WLAN load over 1 ms -> overruns).
        //    The samples spread over 16 ms, about one update period of the Sharp sensor.
        if (n_samples < ADC_SAMPLES) halReadSensorSamples(&samples[n_samples++], 1);
        if ((int32_t)(now - next_measure) >= 0) {
            next_measure = now + MEASURE_PERIOD_MS;
            if (n_samples > 2 * ADC_DISCARD)  // after a stall fewer samples are fine, none is not
                appMeasure(s_app, s_cfg, medianMean(samples, n_samples, ADC_DISCARD), now);
            n_samples = 0;
        }
        const uint32_t t_adc = (uint32_t)esp_timer_get_time();

        // 2. cyclic logic
        HwInputs hw = readInputs(now);
        HwOutputs out = appTick(s_app, hw, s_cfg);
        const uint32_t t_tick = (uint32_t)esp_timer_get_time();

        // 3a. ALL pending priority commands (STOP, HV_OFF, SHUTTER_CLOSE) in this
        //     cycle – idempotent, safe direction, driver requests merged by rank.
        Command  pcmd[CMD_PRIO_MAX_PER_CYCLE];
        ApiError perr[CMD_PRIO_MAX_PER_CYCLE];
        const int np = cmdQueueReceivePriority(pcmd, CMD_PRIO_MAX_PER_CYCLE);
        for (int i = 0; i < np; ++i) perr[i] = appCommand(s_app, pcmd[i], hw, s_cfg, out);

        // 3b. at most one further command, only if the cycle has no driver request
        Command cmd;
        bool have_cmd = out.drv.op == DrvOp::NONE && cmdQueueReceive(cmd, true);
        ApiError cmd_err = ApiError::OK;
        if (have_cmd) {
            cmd_err = appCommand(s_app, cmd, hw, s_cfg, out);
            if (cmd.id == CmdId::SET && cmd_err == ApiError::OK)
                snapshotPublishConfig(s_app.params, s_app.flags);   // GET_CONFIG sees flags too
        }

        const uint32_t t_cmd = (uint32_t)esp_timer_get_time();

        // 4. outputs
        applyOutputs(out);

        // 5. replies only now: "OK" means accepted AND outputs set
        const uint32_t t_out = (uint32_t)esp_timer_get_time();
        for (int i = 0; i < np; ++i) {
            const uint32_t lat = t_out - pcmd[i].t_enq_us;
            if (lat > cs.prio_lat_max_us) cs.prio_lat_max_us = lat;
            cmdQueueReply(pcmd[i], perr[i]);
        }
        if (have_cmd) {
            if (appCmdIsPriority(cmd.id)) {          // arrived during step 3a
                const uint32_t lat = t_out - cmd.t_enq_us;
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
        // V1.5.25: keep the breakdown of a cycle near the 3 ms limit for the /log (net task)
        if (dt_us > CYCLE_DIAG_CYCLE_US || period_us > CYCLE_DIAG_PERIOD_US) {
            taskENTER_CRITICAL(&s_diag_mux);
            s_diag.seq++;
            s_diag.total_us  = dt_us;
            s_diag.period_us = period_us;
            s_diag.adc_us    = t_adc - t_start;
            s_diag.tick_us   = t_tick - t_adc;
            s_diag.cmd_us    = t_cmd - t_tick;
            s_diag.out_us    = t_out - t_cmd;
            s_diag.pub_us    = (t_start + dt_us) - t_out;
            taskEXIT_CRITICAL(&s_diag_mux);
        }

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
