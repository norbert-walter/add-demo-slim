/**
 * @file display_logic.h
 * @brief Pure decision logic of the touch display (FROZEN CONTRACT V1.5 §13).
 *
 * Implementation: src/logic/display_logic.cpp (Sprint L5). The display task
 * (src/display/display.cpp) only draws and reads the touch panel; every
 * decision about WHAT is shown and WHEN a command is sent comes from here.
 */
#pragma once

#include "types.h"
#include "safety_logic.h"

enum class ScreenMode : uint8_t {
    NORMAL = 0,  ///< SAFE_START, READY, RUNNING, RECOVER: status + 7 buttons (V1.2)
    FAULT  = 1,  ///< FAULT (one cycle): fault banner, NO buttons
    ACK    = 2   ///< WAIT_ACK: fault banner + ONE full-width ACK area, no other buttons
};

ScreenMode screenModeOf(SafetyState s);

/**
 * True if the latched fault cause is still present, so an ACK would be rejected
 * (PRECONDITION). Derived from the snapshot only:
 *   ESTOP     -> p.safety.estop
 *   TEMP_HIGH -> p.safety.temp > cfg.temp_fault_c
 *   others    -> false
 */
bool ackCauseActive(const ProcData& p, const SafetyConfig& cfg);

/// Long-press state of the ACK area. Zero-initialise.
struct AckPress {
    bool     prev_touch = false;  ///< touch state of the previous poll (edge detection)
    bool     pressing   = false;  ///< a valid press is running
    uint32_t t0_ms      = 0;      ///< start of the valid press
    bool     sent       = false;  ///< ACK sent in this WAIT_ACK episode
};

/**
 * Called on every touch poll (DISPLAY_TOUCH_POLL_MS). Returns true exactly once
 * when the ACK command must be sent (source LOCAL).
 *   - mode != ACK: reset everything (sent = false, pressing = false), return false.
 *   - A press only STARTS on a touch-down edge inside the area
 *     (touch_in_area && !prev_touch) while !cause_active && !sent.
 *     A finger that was already on the screen when the ACK screen appeared
 *     therefore never acknowledges.
 *   - Release, leaving the area or cause_active becoming true cancels the press.
 *   - (now - t0) >= hold_ms -> sent = true, return true.
 *   - progress (0..1) is written if not NULL (0 when not pressing, 1 after sent).
 *     V1.5.13: "1 after sent" holds on every call while sent (not computed from time).
 * prev_touch is updated at the end of every call (also in mode != ACK).
 */
bool ackPressUpdate(AckPress& a, ScreenMode mode, bool cause_active, bool touch_in_area,
                    uint32_t now_ms, uint32_t hold_ms, float* progress);

// ---------------------------------------------------------------------------------------------
// V1.6.8 (Sprint B2, 21.11.2026): touch debounce and button lock.
// Acceptance M10: a light press bounced (contact - short gap - contact); the HV button saw two
// touch-down edges and switched HV on and at once off again.
// ---------------------------------------------------------------------------------------------

/// Debounced touch state. Zero-initialise.
struct TouchFilter {
    bool     down    = false;  ///< debounced touch state
    uint32_t last_ms = 0;      ///< time of the last poll with raw contact
    int      x = 0, y = 0;     ///< coordinates of the last poll with raw contact
};

/**
 * Called on every touch poll with the raw result of the panel (raw, x, y). Returns f.down.
 *  - raw: down = true, x = x, y = y, last_ms = now_ms (contact is taken at once – no delay).
 *  - !raw && down && (uint32_t)(now_ms - last_ms) >= release_ms: down = false (x, y kept).
 *  - otherwise nothing changes (x, y are NOT overwritten without contact).
 * A gap in the contact shorter than release_ms (a bouncing finger) is therefore no release and
 * gives no new touch-down edge; while down, x / y are the last touched coordinates.
 */
bool touchFilterUpdate(TouchFilter& f, bool raw, int x, int y, uint32_t now_ms, uint32_t release_ms);

/// Lock between two button commands. Zero-initialise.
struct ButtonLock {
    bool     used    = false;  ///< a button command was sent before
    uint32_t last_ms = 0;      ///< time of the last button command
};

/**
 * Called once for every button touch-down edge, before the command is sent. Returns true if the
 * command may be sent.
 *  - is_stop: always true (STOP is never locked).
 *  - otherwise false if used && (uint32_t)(now_ms - last_ms) < lock_ms (nothing changes then).
 * On true: used = true, last_ms = now_ms (also for STOP).
 */
bool buttonLockAllow(ButtonLock& b, bool is_stop, uint32_t now_ms, uint32_t lock_ms);
