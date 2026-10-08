/**
 * @file display_logic.h
 * @brief Pure decision logic of the touch display (FROZEN CONTRACT V1.1 §13).
 *
 * Implementation: src/logic/display_logic.cpp (Sprint S4a). The display task
 * (src/display/display.cpp) only draws and reads the touch panel; every
 * decision about WHAT is shown and WHEN a command is sent comes from here.
 */
#pragma once

#include "types.h"
#include "safety_logic.h"

enum class ScreenMode : uint8_t {
    NORMAL = 0,  ///< SAFE_START, READY, RUNNING, RECOVER: status + 5 buttons
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
 * prev_touch is updated at the end of every call (also in mode != ACK).
 */
bool ackPressUpdate(AckPress& a, ScreenMode mode, bool cause_active, bool touch_in_area,
                    uint32_t now_ms, uint32_t hold_ms, float* progress);
