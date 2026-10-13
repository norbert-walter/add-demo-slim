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
 */
#pragma once

bool displayStart();   ///< init LGFX, start display task; false if panel init failed
