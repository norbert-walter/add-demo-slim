/**
 * @file wifi_logic.h
 * @brief Station (router) connection state machine (FROZEN CONTRACT V1.5 §12.1).
 *
 * Implementation: src/logic/wifi_logic.cpp (Sprint S1). Pure, no hardware.
 * wifi_manager.cpp calls staStep() every 100 ms from the network task and
 * executes the returned action (WiFi.begin / WiFi.disconnect). The access
 * point "ADD-Demo" is ALWAYS on (WIFI_AP_STA) and not handled here.
 *
 * Rules (evaluated in this order):
 *  - !configured (ssid empty): state OFF; if the state was not OFF -> DISCONNECT.
 *  - OFF and configured        -> CONNECTING, attempt 1, t0 = now, BEGIN.
 *  - CONNECTING:
 *      link_up                              -> CONNECTED, attempt = 0.
 *      (now - t0) >= attempt_timeout_ms:
 *          attempt < attempts               -> attempt + 1, t0 = now, BEGIN.
 *          else                             -> BACKOFF, t0 = now, DISCONNECT.
 *  - CONNECTED: !link_up (connection lost)  -> CONNECTING, attempt 1, t0 = now, BEGIN.
 *  - BACKOFF:   link_up                     -> CONNECTED.
 *               (now - t0) >= backoff_ms    -> CONNECTING, attempt 1, t0 = now, BEGIN.
 * All time comparisons are wrap-around safe. This cycle repeats forever:
 * 3 attempts of 10 s, then 2 min pause, then 3 attempts again, ...
 * Boot is the same as "OFF and configured" (automatic login after reboot).
 */
#pragma once

#include <stdint.h>

enum class StaState  : uint8_t { OFF = 0, CONNECTING, CONNECTED, BACKOFF };
enum class StaAction : uint8_t { NONE = 0, BEGIN, DISCONNECT };

struct StaConfig {
    uint8_t  attempts           = 3;
    uint32_t attempt_timeout_ms = 10000;
    uint32_t backoff_ms         = 120000;
};

struct StaStatus {
    StaState state   = StaState::OFF;
    uint8_t  attempt = 0;   ///< 1..attempts while CONNECTING, else 0
    uint32_t t0_ms   = 0;   ///< start of the current attempt or of the backoff
};

StaAction   staStep(StaStatus& s, bool configured, bool link_up, uint32_t now_ms, const StaConfig& c);
const char* staStateName(StaState s);   ///< "off", "connecting", "connected", "backoff"

/// Remaining backoff in ms (0 if not in BACKOFF), for the display / log.
uint32_t staBackoffRemainingMs(const StaStatus& s, uint32_t now_ms, const StaConfig& c);
