/**
 * @file config.h
 * @brief Board, pin and timing constants for the ADD Demo System (FROZEN – Contract V1.3).
 *
 * Board: Makerbase MKS DLC32 V2.1 (ESP32-WROOM-32U), display MKS TS35R.
 * Toolchain: pioarduino (Arduino Core 3.x / ESP-IDF 5.x).
 *
 * Values marked  VERIFY(0a)  are confirmed or corrected with the smoketest
 * (Sprint 0a, docs/Smoketest_Protokoll.md) BEFORE Sprint 1 starts. After that
 * this file is frozen. Logic parameters (thresholds, speeds) live in
 * the config structs of the src/logic headers, not here.
 */
#pragma once

// =========================================================================
// Inputs (input-only GPIOs, external pull-ups on the board)
// =========================================================================
#define PIN_LIMIT_FWD        36   // X- connector: forward end switch
#define PIN_LIMIT_BWD        35   // Y- connector: backward end switch (home)
#define PIN_SENSOR_ADC       34   // Z- connector: Sharp GP2Y0A41SK0F analog out (ADC1_CH6)
#define PIN_ESTOP            22   // PROBE connector: E-stop

// Active levels. Verified 0a (2026-10-11): both end switches LOW when pressed.
#define LIMIT_ACTIVE_LEVEL   LOW  // end switch pressed -> LOW
// E-stop active level is a PARAMETER since V1.3 (params.estop_nc, REST API SET):
//   estop_nc = false (default): normally-open switch to GND, board pull-up,
//                               E-stop active LOW, NO wire-break detection.
//   estop_nc = true:            normally-closed contact to GND, active HIGH,
//                               a broken wire is detected as E-stop.
// The smoketest shows the raw level; Smoketest 2.3-2.5 documents the wiring
// (0a: simple normally-open switch, estop_nc = false).

#define INPUT_DEBOUNCE_MS    5    // limit switches and E-stop

// =========================================================================
// Direct GPIO outputs
// =========================================================================
#define PIN_SHUTTER          32   // SPINDLE/TTL connector: shutter LED red (HIGH = open), 20 mA
#define PIN_HV_LED            4   // V1.2: HV LED green (HIGH = on), 20 mA – moved from i2so.3 (verified)
#define PIN_HEARTBEAT_LED     2   // V1.3: blue board LED, 0.5 Hz heartbeat from the control task (HIGH = on)

// =========================================================================
// I2S shift register (74HC595 chain) – driven by FastAccelStepper I2S_MUX
// Pins confirmed by FluidNC config "MKS_DLC32_21_XYZ.yaml".
// =========================================================================
#define PIN_I2S_DATA         21
#define PIN_I2S_BCK          16
#define PIN_I2S_WS           17

// FluidNC numbering "i2so.N" of the board signals:
#define I2SO_SHARED_EN        0   // stepper enable, active LOW
#define I2SO_X_STEP           1
#define I2SO_X_DIR            2
#define I2SO_BUZZER           7   // BEEPER

// Mapping FluidNC i2so.N -> FastAccelStepper mux slot (0..31).
// Verified 0a (2026-10-11) with "scan": enable slot 0, step 1, dir 2, buzzer 7 -> slot = n. Candidates were:
//   (n)  (31 - (n))  ((n) + 8)  ((n) + 16)  ((n) + 24)
#define I2SO_TO_SLOT(n)      (n)

#define DIR_POSITIVE_HIGH    false  // verified 0a (2026-10-11): DIR level for movement towards +mm

// =========================================================================
// Stepper drive (TMC2209, microstep jumper 1/16, rack 40.77 mm per revolution, measured 0a)
// Axis 0..242 mm (0 = 2 mm in front of the backward switch; forward switch measured at
// 246.3 mm from the backward switch = axis 244.3 mm, smoketest 0a 2026-10-11)
// =========================================================================
#define MOTOR_FULL_STEPS     200
#define MICROSTEPS           16
#define MM_PER_REV           40.77f // verified 0a: 126.2 mm measured for 130 mm with 42.0 (caliper, +-0.1 mm)
#define STEPS_PER_MM         ((MOTOR_FULL_STEPS * MICROSTEPS) / MM_PER_REV)   // 78.49
// Motor Nanotec ST4118L0804-A (0.8 A), TMC2209 standalone, current set to 0.8 A (verified)
#define ACCEL_MM_S2          500.0f // verified 0a: smooth, no step loss at 1..20 mm/s

// =========================================================================
// Display MKS TS35R (ST7796 480x320 landscape, touch ADS7843E), VSPI
// Free GPIOs according to FluidNC DLC32 notes: 5,18,19,22,23,25,26,27,32,33
// Verified 0a section 6 (display and touch work with these pins).
// =========================================================================
#define PIN_DISP_SCK         18
#define PIN_DISP_MOSI        23
#define PIN_DISP_MISO        19
#define PIN_DISP_DC          33
#define PIN_DISP_CS          25
#define PIN_DISP_RST         27
#define PIN_DISP_BL           5
#define PIN_TOUCH_CS         26
// Touch calibration: raw ADS7843 values extrapolated to the panel edges
// (smoketest 0a 6.2, four corner targets). Touch offset_rotation = 2 in lgfx_config.h.
#define TOUCH_X_MIN          130
#define TOUCH_X_MAX          3775
#define TOUCH_Y_MIN          200
#define TOUCH_Y_MAX          3900

// =========================================================================
// Timing
// =========================================================================
#define CONTROL_PERIOD_MS    1      // control task cycle (Core 1)
#define MEASURE_PERIOD_MS    20     // 50 Hz measurement (Sharp sensor updates ~every 16.5 ms)
#define ADC_SAMPLES          16     // per measurement, median-mean discards 4 + 4
#define ADC_DISCARD          4
#define DISPLAY_PERIOD_MS    200
#define DISPLAY_TOUCH_POLL_MS 20    // V1.1: touch polling (ACK long press, edge detection)
#define ACK_HOLD_MS          1000   // V1.1: ACK area must be held this long
#define WDT_TIMEOUT_S        10
#define COMM_TIMEOUT_MS      5000
#define REBOOT_DELAY_MS      700    // V1.3: >= soundDurationMs(REBOOT) (500 ms) + HTTP reply

// =========================================================================
// Tasks
// =========================================================================
#define CONTROL_TASK_CORE    1
#define CONTROL_TASK_PRIO    (configMAX_PRIORITIES - 2)
#define CONTROL_TASK_STACK   6144
#define NET_TASK_CORE        0
#define NET_TASK_PRIO        3
#define NET_TASK_STACK       8192
#define NET_TASK_PERIOD_MS   5      // V1.5.20: wifiLoop + handleClient + persist/reboot every 5 ms
#define LOCAL_HOLD_MS        0      // V1.5.24: LOCAL priority hold after the last display command;
                                    //   0 = LOCAL priority off (display commands never set "local"),
                                    //   1 ... 60000 ms = hold time (CONTRACT §6.6)
#define COMM_GAP_LOG_MS      2000   // V1.5.24: log an /api gap longer than this while the watchdog is armed
#define CONSOLE_LINE_MAX     120    // V1.5.23: serial console, longest accepted line (without '\n')
#define CONSOLE_READ_MAX     64     // V1.5.23: serial bytes read per netTaskStep at most
#define DISPLAY_TASK_CORE    1      // V1.5.20
#define DISPLAY_TASK_PRIO    1      // V1.5.20: below the control task
#define DISPLAY_TASK_STACK   6144   // V1.5.20
#define STEPPER_TASK_CORE    1      // FastAccelStepper engine.init(core)
#define CMD_QUEUE_LEN        8
#define CMD_PRIO_QUEUE_LEN   4      // V1.1: STOP / HV_OFF / SHUTTER_CLOSE

// =========================================================================
// Communication / storage
// =========================================================================
#define HTTP_PORT            80
#define HTTP_IDLE_DROP_MS    500    // V1.5.25: close a connection that sent no request for this long
#define HTTP_IDLE_LOG_MS     10000  // V1.5.25: at most one "http: idle connection closed" line per 10 s
#define NET_SLOW_LOG_MS      200    // V1.5.25: /log line when a net task part or the gap between steps takes longer
#define API_BODY_MAX         512
#define API_REPLY_TIMEOUT_MS 100    // HTTP waits this long for the control task
#define NVS_NAMESPACE        "add_demo"
#define WIFI_AP_SSID         "ADD-Demo"
#define WIFI_AP_PASS         "adddemo1"
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define LOG_LINES            20
#define LOG_LINE_LEN         128
#define FW_VERSION           "1.0.0"
