/**
 * @file wifi_logic.cpp
 * @brief Station (router) connection state machine implementation.
 *
 * Rules evaluated in documented order:
 *   1. !configured → OFF; if was not OFF → DISCONNECT
 *   2. OFF + configured → CONNECTING, attempt=1, t0=now, BEGIN
 *   3. CONNECTING: link_up → CONNECTED (attempt=0); else timeout check:
 *        attempt < attempts → attempt+1, t0=now, BEGIN
 *        else → BACKOFF, t0=now, DISCONNECT
 *   4. CONNECTED: !link_up → CONNECTING, attempt=1, t0=now, BEGIN
 *   5. BACKOFF: link_up → CONNECTED; else timeout → CONNECTING, attempt=1, t0=now, BEGIN
 */

#include "wifi_logic.h"

StaAction staStep(StaStatus& s, bool configured, bool link_up, uint32_t now_ms, const StaConfig& c) {
    StaAction action = StaAction::NONE;

    if (!configured) {
        if (s.state != StaState::OFF)
            action = StaAction::DISCONNECT;
        s.state   = StaState::OFF;
        s.attempt = 0;
        return action;
    }

    switch (s.state) {
        case StaState::OFF:
            s.state     = StaState::CONNECTING;
            s.attempt   = 1;
            s.t0_ms     = now_ms;
            action      = StaAction::BEGIN;
            break;

        case StaState::CONNECTING:
            if (link_up) {
                s.state   = StaState::CONNECTED;
                s.attempt = 0;
            } else {
                uint32_t elapsed = (uint32_t)(now_ms - s.t0_ms);
                if (elapsed >= c.attempt_timeout_ms) {
                    if (s.attempt < c.attempts) {
                        ++s.attempt;
                        s.t0_ms = now_ms;
                        action  = StaAction::BEGIN;
                    } else {
                        s.state     = StaState::BACKOFF;
                        s.t0_ms     = now_ms;
                        s.attempt   = 0;
                        action      = StaAction::DISCONNECT;
                    }
                }
            }
            break;

        case StaState::CONNECTED:
            if (!link_up) {
                s.state     = StaState::CONNECTING;
                s.attempt   = 1;
                s.t0_ms     = now_ms;
                action      = StaAction::BEGIN;
            }
            break;

        case StaState::BACKOFF:
            if (link_up) {
                s.state = StaState::CONNECTED;
            } else {
                uint32_t elapsed = (uint32_t)(now_ms - s.t0_ms);
                if (elapsed >= c.backoff_ms) {
                    s.state     = StaState::CONNECTING;
                    s.attempt   = 1;
                    s.t0_ms     = now_ms;
                    action      = StaAction::BEGIN;
                }
            }
            break;
    }

    return action;
}

const char* staStateName(StaState s) {
    switch (s) {
        case StaState::OFF:         return "off";
        case StaState::CONNECTING:  return "connecting";
        case StaState::CONNECTED:   return "connected";
        case StaState::BACKOFF:     return "backoff";
    }
    return "unknown";
}

uint32_t staBackoffRemainingMs(const StaStatus& s, uint32_t now_ms, const StaConfig& c) {
    if (s.state != StaState::BACKOFF)
        return 0;
    uint32_t elapsed = (uint32_t)(now_ms - s.t0_ms);
    if (elapsed >= c.backoff_ms)
        return 0;
    return c.backoff_ms - elapsed;
}