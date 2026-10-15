/**
 * @file display_logic.cpp
 * @brief Pure decision logic of the touch display (FROZEN CONTRACT V1.5 §13).
 */
#include "display_logic.h"

ScreenMode screenModeOf(SafetyState s) {
    switch (s) {
        case SafetyState::FAULT: return ScreenMode::FAULT;
        case SafetyState::WAIT_ACK: return ScreenMode::ACK;
        default: return ScreenMode::NORMAL;  // SAFE_START, READY, RUNNING, RECOVER
    }
}

bool ackCauseActive(const ProcData& p, const SafetyConfig& cfg) {
    if (p.err == ErrorCode::ESTOP) {
        return p.safety.estop;
    }
    if (p.err == ErrorCode::TEMP_HIGH) {
        return p.safety.temp > cfg.temp_fault_c;
    }
    return false;
}

bool ackPressUpdate(AckPress& a, ScreenMode mode, bool cause_active, bool touch_in_area,
                    uint32_t now_ms, uint32_t hold_ms, float* progress) {
    if (mode != ScreenMode::ACK) {
        // Reset functional state when not in ACK mode, but preserve prev_touch
        // so that re-entering ACK with finger still down doesn't create a false edge.
        a.pressing     = false;
        a.t0_ms        = 0;
        a.sent         = false;
        if (progress) *progress = 0.0f;
        a.prev_touch   = touch_in_area;
        return false;
    }

    // In ACK mode: handle press logic
    if (!cause_active && !a.sent && touch_in_area && !a.prev_touch) {
        // Touch-down edge inside area, cause not active, not yet sent -> start press
        a.pressing     = true;
        a.t0_ms        = now_ms;
    }

    // Cancel if: release (no longer touching), leaving area, or cause_active became true
    if (!touch_in_area || cause_active) {
        a.pressing     = false;
        a.t0_ms        = 0;
    }

    // Check if hold time has elapsed while still pressing
    if (a.pressing && (uint32_t)(now_ms - a.t0_ms) >= hold_ms) {
        a.sent         = true;
        a.pressing     = false;
        if (progress) *progress = 1.0f;
        return true;
    }

    // Update progress
    if (progress) {
        if (a.sent) {
            *progress = 1.0f;
        } else if (a.pressing) {
            uint32_t elapsed = (uint32_t)(now_ms - a.t0_ms);
            if (hold_ms == 0) {
                *progress = 1.0f;
            } else {
                float p_val = static_cast<float>(elapsed) / static_cast<float>(hold_ms);
                if (p_val > 1.0f) p_val = 1.0f;
                *progress = p_val;
            }
        } else {
            *progress = 0.0f;
        }
    }

    a.prev_touch = touch_in_area;
    return false;
}