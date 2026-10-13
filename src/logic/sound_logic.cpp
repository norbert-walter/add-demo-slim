#include "logic/sound_logic.h"

/* ---- pattern data (static const, no mutable state) ---- */

static const uint16_t s_click_seg[]  = {50};
static const uint16_t s_homed_seg[]  = {150};
static const uint16_t s_boot_seg[]   = {100, 100, 300};
static const uint16_t s_reboot_seg[] = {300, 100, 100};
static const uint16_t s_fault_seg[]  = {150, 150, 150, 150, 150};
static const uint16_t s_estop_seg[]  = {4000};

struct Pattern {
    const uint16_t* seg;
    uint8_t         count;
};

/* Index == Sound enum value (NONE=0 → empty). */
static const Pattern s_pat[] = {
    {},           /* NONE   = 0 */
    {s_click_seg,  1},  /* CLICK  = 1 */
    {s_homed_seg,  1},  /* HOMED  = 2 */
    {s_boot_seg,   3},  /* BOOT   = 3 */
    {s_reboot_seg, 3},  /* REBOOT = 4 */
    {s_fault_seg,  5},  /* FAULT  = 5 */
    {s_estop_seg,  1},  /* ESTOP  = 6 */
};

/* Priority: higher enum value → higher priority. */
static uint8_t prio(Sound s) { return static_cast<uint8_t>(s); }

/* ---- public API ---- */

uint32_t soundDurationMs(Sound s) {
    const auto& p = s_pat[static_cast<uint8_t>(s)];
    uint32_t d = 0;
    for (uint8_t i = 0; i < p.count; ++i) {
        d += p.seg[i];
    }
    return d;
}

bool soundStart(SoundPlayer& p, Sound s, uint32_t now_ms) {
    if (s == Sound::NONE) {
        return false;
    }

    /* NONE → always start. */
    if (p.cur == Sound::NONE) {
        p.cur   = s;
        p.t0_ms = now_ms;
        return true;
    }

    /* Replace only if same or higher priority. */
    if (prio(s) >= prio(p.cur)) {
        p.cur   = s;
        p.t0_ms = now_ms;
        return true;
    }

    return false;
}

bool soundOutput(SoundPlayer& p, uint32_t now_ms) {
    if (p.cur == Sound::NONE) {
        return false;
    }

    const auto& pat = s_pat[static_cast<uint8_t>(p.cur)];
    uint32_t dt     = now_ms - p.t0_ms;

    for (uint8_t i = 0; i < pat.count; ++i) {
        if (dt < pat.seg[i]) {
            /* Even index → ON, odd → OFF. */
            return (i % 2) == 0;
        }
        dt -= pat.seg[i];
    }

    /* Pattern finished. */
    p.cur = Sound::NONE;
    return false;
}

const char* soundName(Sound s) {
    switch (s) {
        case Sound::NONE:   return "none";
        case Sound::CLICK:  return "click";
        case Sound::HOMED:  return "homed";
        case Sound::BOOT:   return "boot";
        case Sound::REBOOT: return "reboot";
        case Sound::FAULT:  return "fault";
        case Sound::ESTOP:  return "estop";
    }
    return "none";
}
