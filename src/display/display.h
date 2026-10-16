/**
 * @file display.h
 * @brief Local status display + touch operation (CONTRACT V1.4 §13, Sprint I1).
 *
 * Runs in its own low-priority task on Core 1 (DISPLAY_PERIOD_MS), reads only
 * snapshotGet(), snapshotGetConfig() (estop_nc) and the wifi_manager getters for
 * the IP line, sends commands only via
 * cmdQueueSend() with source = LOCAL. It never calls HAL, app or safety logic;
 * the only logic it uses is the pure src/logic/display_logic.h.
 *
 * All decisions come from src/logic/display_logic.h (pure, natively tested):
 * screenModeOf(), ackCauseActive(), ackPressUpdate(). Touch is polled every
 * DISPLAY_TOUCH_POLL_MS, the screen is redrawn every DISPLAY_PERIOD_MS.
 *
 * Screen modes (screenModeOf(snapshot.state)):
 *  NORMAL  (SAFE_START, READY, RUNNING, RECOVER):
 *          status area + 7 buttons at y = 260, h = 60 (V1.2):
 *          INIT · HV · MEAS · TRAV · PARK · SERV  (64 px each, text size 2)
 *          and STOP (x 384..479, 96 px, text size 3 – the largest target).
 *          PARK -> Command PARK, SERV -> Command SERVICE (API names).
 *          V1.3: SPEED in mm/s. IP line: "AP 192.168.4.1  STA 192.168.1.57" or
 *          "STA connecting 2/3" / "STA retry 87 s" / "STA off".
 *          If estop_nc == false the ESTOP indicator is labelled "ESTOP(NO)" in the
 *          warning colour: simple switch, no wire-break detection.
 *          Acoustic feedback for accepted buttons comes from the control task
 *          (CLICK pattern), not from the display.
 *          A "LOCAL" badge is shown while snapshot.local is true.
 *          Flags interlock / cooling_flow / temp are labelled "SIM" (simulated).
 *  FAULT   (state FAULT, one cycle): button row hidden, fault banner only.
 *  ACK     (state WAIT_ACK): ALL normal buttons hidden. Shows
 *          - fault banner: "FAULT: <errorCodeName>" (large, red) + one-line remedy (English,
 *            texts in Pflichtenheft §5.8)
 *          - ONE ACK area across the full width (x 0..479, y 260..319):
 *            cause still active (estop, temp > 32 °C) -> grey, text
 *              "CAUSE ACTIVE: <name> - RELEASE FIRST", touch ignored;
 *            cause gone -> yellow, "ACK - HOLD 1 s"; a progress bar fills while
 *              pressed; ACK is sent ONCE after 1000 ms continuous press; release
 *              before that cancels. After sending, the area is locked until the
 *              state has left WAIT_ACK (no repeated ACKs).
 *  "Cause still active" = ackCauseActive(snapshot, SafetyConfig{}).
 *  The press must START on the ACK screen (touch-down edge): a finger that was
 *  already on STOP when the fault happened never acknowledges.
 *  A touch on a hidden button never sends a command.
 *
 * V1.5.20 – exact behaviour (tested natively with the LovyanGFX shim: test/native/test_display).
 *  File-static: LGFX lcd (display/lgfx_config.h), AckPress, previous touch state, current screen
 *  mode, time of the last drawing, a cache of what each screen zone currently shows.
 *  Text: built-in font, setTextSize(n) = 6n x 8n px per character, colours below (RGB888).
 *  displayStart(): if (!lcd.init()) return false; setRotation(1); setBrightness(255);
 *    fillScreen(COL_BG); then xTaskCreatePinnedToCore(<task>, "disp", DISPLAY_TASK_STACK, nullptr,
 *    DISPLAY_TASK_PRIO, nullptr, DISPLAY_TASK_CORE) == pdPASS. The task loops forever:
 *    displayTick(millis()); vTaskDelay(pdMS_TO_TICKS(DISPLAY_TOUCH_POLL_MS)).
 *  displayTick(now), in this order:
 *    1. snapshotGet(d); mode = screenModeOf(d.state); touch = lcd.getTouch(&x, &y) > 0
 *       (exactly one getTouch per tick); in_row = touch && 260 <= y < 320 && 0 <= x < 480.
 *    2. Draw if: first call, OR mode differs from the drawn mode (at once – the picture must match
 *       the mode BEFORE the touch is evaluated), OR (uint32_t)(now - last drawing) >= DISPLAY_PERIOD_MS.
 *       Drawing redraws only zones whose content changed (cache); a mode change clears the whole
 *       screen first. With an unchanged snapshot a periodic drawing issues NO draw call.
 *    3. Buttons: mode NORMAL && in_row && touch-down edge (!previous touch):
 *       x < 384: button x / 64 = INIT, HV, MEAS, TRAV, PARK, SERV -> CmdId INIT, (d.safety.hv_on ?
 *       HV_OFF : HV_ON), START_MEASURE, START_TRAVERSE, PARK, SERVICE; x >= 384: STOP.
 *       Exactly one cmdQueueSend per edge: Command with id, source = LOCAL, want_reply = false,
 *       all parameters default (the control task uses the stored parameters).
 *    4. ackPressUpdate(ack, mode, ackCauseActive(d, SafetyConfig{}), in_row, now, ACK_HOLD_MS,
 *       &progress) on EVERY tick; true -> cmdQueueSend(ACK, source LOCAL, want_reply false).
 *    5. previous touch = touch.
 *  Screen (480 x 320; "text(x, y, size)" = top-left datum unless "centred" = middle_center):
 *    Header y 0..31, background COL_PANEL, in all modes:
 *      "ADD DEMO" (8, 8, 2) COL_TEXT; "[ <safetyStateName> ]" (150, 8, 2), colour COL_OK for READY /
 *      RUNNING, COL_FAULT for FAULT / WAIT_ACK, else COL_WARN; "LOCAL" (288, 8, 2) COL_WARN only
 *      while d.local; "HV" (360, 8, 2) and "SHUTTER" (396, 8, 2): COL_WARN if on / open, else COL_LABEL.
 *    NORMAL:
 *      values y 34..107, four fields of 120 px (x = 0, 120, 240, 360): label (x+6, 38, 2) COL_LABEL
 *        "POS" "SPEED" "DIST" "THICK"; value (x+6, 60, 3) "%.1f" of motion.pos, motion.vel,
 *        measure.dist, measure.thick ("---" for DIST and THICK while !measure.valid), colours
 *        COL_TEXT, COL_TEXT, COL_DIST, COL_THICK; unit (x+6, 92, 1) COL_LABEL "mm" "mm/s" "mm" "mm".
 *      position bar y 112..131, x 4..475, V1.5.21 MIRRORED (as seen from the front of the gauge:
 *        axis 0 = home switch on the RIGHT): bx(mm) = 472 - (int)(clamp(mm, 0, 242) / 242 * 468);
 *        park zone fillRect(bx(20) + 4, 112, bx(0) - bx(20), 20, COL_PARK), maintenance zone
 *        fillRect(bx(60) + 4, 112, bx(20) - bx(60), 20, COL_WARN), frame COL_FRAME,
 *        marker fillRect(bx(pos), 112, 4, 20, COL_TEXT).
 *      (6, 144, 2) "ESTOP" if estop_nc else "ESTOP(NO)"; colour COL_FAULT while safety.estop, else
 *        COL_OK if estop_nc, else COL_WARN (simple switch, no wire-break detection).
 *      (180, 144, 2) "TEMP(SIM) %.1fC" COL_OK if temp_ok else COL_WARN.
 *      (6, 170, 2) "INTERLOCK(SIM)", (220, 170, 2) "COOLING(SIM)": COL_OK if true, else COL_FAULT.
 *      (6, 196, 2) "MODE %s  AVG %.1f (%lu)" opModeName(motion.mode), measure.avg, measure.n; COL_TEXT.
 *      (6, 222, 2) IP line, COL_LABEL: "AP <wifiGetApIp>  " followed by, from wifiStaStatus():
 *        CONNECTED "STA <wifiGetStaIp>", CONNECTING "STA connecting <attempt>/<StaConfig{}.attempts>",
 *        BACKOFF "STA retry <s> s" with s = ceil(staBackoffRemainingMs(st, now, StaConfig{}) / 1000),
 *        OFF "STA off".
 *      buttons y 260..319: six fillRect(i*64 + 1, 261, 62, 58, bg) with bg COL_BTN (HV: COL_WARN while
 *        hv_on), label centred (i*64 + 32, 290, 2) COL_TEXT; STOP fillRect(385, 261, 94, 58, COL_FAULT),
 *        "STOP" centred (432, 290, 3) COL_TEXT.
 *    FAULT and ACK: y 34..257 COL_BG with, centred: "FAULT: <errorCodeName>" (240, 90, 3) COL_FAULT;
 *      remedy (240, 140, 2) COL_TEXT: ESTOP "Release E-stop, then acknowledge", TEMP_HIGH
 *      "Lower temp <= 32 C", MOTOR_FAULT "Check axis and end switches", COMM_TIMEOUT
 *      "Remote client lost", HOMING_FAILED "Check home switch, then INIT", NONE "";
 *      "POS %.1f mm" (240, 190, 2) COL_TEXT; "ESTOP ACTIVE  TEMP(SIM) %.1fC" or
 *      "ESTOP OK  TEMP(SIM) %.1fC" (240, 214, 2), COL_FAULT while estop else COL_TEXT.
 *      FAULT: y 260..319 filled COL_BG (no buttons, no text).
 *      ACK: y 260..319 is ONE area: cause active -> fill COL_GREY, centred (240, 284, 2) COL_TEXT
 *        "CAUSE ACTIVE: <errorCodeName> - RELEASE FIRST"; else fill COL_WARN, centred (240, 284, 2)
 *        COL_BLACK "ACK - HOLD 1 s", progress bar fillRect(10, 306, (int)(progress * 460), 8, COL_TEXT)
 *        if that width > 0 (progress from the previous tick).
 *  Only lcd calls that exist in test/native/freertos_shim/LovyanGFX.hpp; no Serial, no delay().
 */
#pragma once
#include <stdint.h>

// V1.5.20: colours (RGB888, style guide dark; Pflichtenheft §5.8)
constexpr uint32_t COL_BG    = 0x202020;
constexpr uint32_t COL_PANEL = 0x1E1E1E;
constexpr uint32_t COL_FRAME = 0x2E2E2E;
constexpr uint32_t COL_BTN   = 0x2E2E2E;
constexpr uint32_t COL_TEXT  = 0xFFFFFF;
constexpr uint32_t COL_LABEL = 0x8AB2D0;
constexpr uint32_t COL_DIST  = 0x00C8A0;
constexpr uint32_t COL_THICK = 0x0084FF;
constexpr uint32_t COL_OK    = 0x00C8A0;
constexpr uint32_t COL_WARN  = 0xF5A623;
constexpr uint32_t COL_FAULT = 0xE84040;
constexpr uint32_t COL_GREY  = 0x606060;
constexpr uint32_t COL_PARK  = 0x3A3A3A;
constexpr uint32_t COL_BLACK = 0x000000;

bool displayStart();               ///< init LGFX, start display task; false if panel init failed
void displayTick(uint32_t now_ms); ///< V1.5.20: one poll of the display task (every DISPLAY_TOUCH_POLL_MS)
