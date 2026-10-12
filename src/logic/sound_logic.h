/**
 * @file sound_logic.h
 * @brief Buzzer patterns for the ACTIVE buzzer on i2so.7 (FROZEN CONTRACT V1.5 §11.1).
 *
 * Implementation: src/logic/sound_logic.cpp (Sprint S2.4). Pure, no hardware.
 * The buzzer can only be switched on/off (fixed tone), so signals differ by
 * rhythm. app_logic owns one SoundPlayer and writes soundOutput() to
 * HwOutputs.buzzer every cycle; the HAL only sets the bit.
 *
 * Patterns (ms, alternating ON/OFF, starting with ON):
 *   CLICK   50                      successful command from the display
 *   HOMED   150                     homing finished
 *   BOOT    100, 100, 300           firmware ready ("short-long", rising)
 *   REBOOT  300, 100, 100           before a restart ("long-short", falling)
 *   FAULT   150, 150, 150, 150, 150 fault entry other than ESTOP (3 beeps)
 *   ESTOP   4000                    fault entry with ESTOP (one 4 s tone)
 *
 * Priority (higher wins): ESTOP > FAULT > REBOOT > BOOT > HOMED > CLICK > NONE.
 * soundStart() replaces the playing sound only if the new one has the same or a
 * higher priority (a click never cuts the E-stop tone; a second click restarts).
 */
#pragma once

#include <stdint.h>

enum class Sound : uint8_t { NONE = 0, CLICK, HOMED, BOOT, REBOOT, FAULT, ESTOP };

struct SoundPlayer {
    Sound    cur   = Sound::NONE;
    uint32_t t0_ms = 0;
};

/// Total duration of a pattern in ms (NONE -> 0). REBOOT: the network task
/// restarts the device only after this time (REBOOT_DELAY_MS >= duration).
uint32_t soundDurationMs(Sound s);

/// Starts s at now_ms if s != NONE and prio(s) >= prio(p.cur). Returns true if started.
bool soundStart(SoundPlayer& p, Sound s, uint32_t now_ms);

/// Buzzer level for now_ms (wrap-around safe). When the pattern has ended,
/// p.cur becomes NONE and the result is false.
bool soundOutput(SoundPlayer& p, uint32_t now_ms);

const char* soundName(Sound s);   ///< "none", "click", "homed", "boot", "reboot", "fault", "estop"
