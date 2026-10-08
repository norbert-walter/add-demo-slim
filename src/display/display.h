/**
 * @file display.h
 * @brief Local status display + touch operation (CONTRACT V1.1 §13, Sprint S4).
 *
 * Runs in its own low-priority task on Core 1 (DISPLAY_PERIOD_MS), reads only
 * snapshotGet() (and wifiGetIp() for the IP line), sends commands only via
 * cmdQueueSend() with source = LOCAL. It never calls HAL, app or safety logic;
 * the only logic it uses is the pure src/logic/display_logic.h.
 *
 * All decisions come from src/logic/display_logic.h (pure, natively tested):
 * screenModeOf(), ackCauseActive(), ackPressUpdate(). Touch is polled every
 * DISPLAY_TOUCH_POLL_MS, the screen is redrawn every DISPLAY_PERIOD_MS.
 *
 * Screen modes (screenModeOf(snapshot.state)):
 *  NORMAL  (SAFE_START, READY, RUNNING, RECOVER):
 *          status area + 5 buttons INIT · HV · MEAS · TRAV · STOP (y = 260, 96x60 each).
 *          A "LOCAL" badge is shown while snapshot.local is true.
 *          Flags interlock / cooling_flow / temp are labelled "SIM" (simulated).
 *  FAULT   (state FAULT, one cycle): button row hidden, fault banner only.
 *  ACK     (state WAIT_ACK): ALL normal buttons hidden. Shows
 *          - fault banner: "FEHLER: <errorCodeName>" (large, red) + one-line remedy
 *          - ONE ACK area across the full width (x 0..479, y 260..319):
 *            cause still active (estop, temp > 32 °C) -> grey, text
 *              "Ursache aktiv: <name> – erst beheben", touch ignored;
 *            cause gone -> yellow, "ACK – 1 s halten"; a progress bar fills while
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
