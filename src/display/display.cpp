/**
 * @file display.cpp
 * @brief Local status display + touch operation (CONTRACT V1.5.20 §13, Sprint I1).
 *
 * SKELETON (Claude, 2026-10-16): structure, includes, state and the small helpers are given.
 * Fill in the functions marked "TODO(I1.x)" exactly as described in src/display/display.h
 * (V1.5.20 block). Do not add functions, types, includes or file-static variables, and do not
 * change the given helpers. Types and names come from the project headers: ProcData
 * (logic/types.h), safetyStateName / errorCodeName / opModeName (logic/types.h), ScreenMode,
 * AckPress, screenModeOf, ackCauseActive, ackPressUpdate (logic/display_logic.h), StaStatus,
 * StaState, StaConfig, staBackoffRemainingMs (logic/wifi_logic.h).
 */
#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "logic/types.h"
#include "logic/params.h"
#include "logic/display_logic.h"
#include "logic/wifi_logic.h"
#include "app/snapshot.h"
#include "app/command_queue.h"
#include "comm/wifi_manager.h"
#include "display/lgfx_config.h"   // class LGFX (panel, touch, pins)
#include "display/display.h"

namespace {

// ---- state (given) ----------------------------------------------------------------------------
LGFX       lcd;
bool       s_drawn = false;              // false until the first drawing
ScreenMode s_mode = ScreenMode::NORMAL;  // mode of the current picture
uint32_t   s_last_draw = 0;              // time of the last drawing
bool       s_prev_touch = false;         // touch state of the previous tick (edge detection)
AckPress   s_ack;                        // long press of the ACK area
float      s_progress = 0.0f;            // ACK progress 0..1 (from ackPressUpdate)

enum Zone { Z_STATE, Z_LOCAL, Z_HV, Z_SHUTTER, Z_V0, Z_V1, Z_V2, Z_V3, Z_BAR, Z_ESTOP, Z_TEMP,
            Z_FLAGS, Z_MODE, Z_IP, Z_BOTTOM, Z_BANNER, Z_COUNT };
char s_key[Z_COUNT][96];                 // what each zone shows now; "" = must be drawn

// ---- helpers (given, do not change) -----------------------------------------------------------
/// false if `key` equals what zone z shows; otherwise remembers key and returns true (= draw it).
bool changed(Zone z, const char* key) {
    if (strcmp(s_key[z], key) == 0) return false;
    snprintf(s_key[z], sizeof s_key[z], "%s", key);
    return true;
}
/// One text: datum, size, colours (fg on bg), drawString.
void text(const char* s, int x, int y, uint8_t size, uint32_t fg, uint32_t bg,
          lgfx::textdatum_t datum = lgfx::top_left) {
    lcd.setTextDatum(datum);
    lcd.setTextSize(size);
    lcd.setTextColor(fg, bg);
    lcd.drawString(s, x, y);
}
/// x pixel of a position in mm on the position bar (bx() of the header).
int barX(float mm) {
    if (!(mm > 0.0f)) mm = 0.0f;
    if (mm > 242.0f) mm = 242.0f;
    return 4 + (int)(mm / 242.0f * 468.0f);
}
/// Colour of "[ <state> ]" in the header.
uint32_t stateColour(SafetyState s) {
    switch (s) {
        case SafetyState::READY: case SafetyState::RUNNING: return COL_OK;
        case SafetyState::FAULT: case SafetyState::WAIT_ACK: return COL_FAULT;
        default: return COL_WARN;
    }
}

// ---- I1.2a ------------------------------------------------------------------------------------
/// TODO(I1.2a) header zones Z_STATE, Z_LOCAL, Z_HV, Z_SHUTTER (each: build key, if changed():
/// fillRect of the zone in COL_PANEL where needed, then text()).
void drawHeader(const ProcData& d) {
    // Z_STATE: "[ <safetyStateName> ]" at (150, 8, 2)
    char key[32];
    snprintf(key, sizeof key, "[ %s ]", safetyStateName(d.state));
    if (changed(Z_STATE, key)) {
        lcd.fillRect(150, 0, 168, 32, COL_PANEL);
        text(key, 150, 8, 2, stateColour(d.state), COL_PANEL);
    }

    // Z_LOCAL: "LOCAL" at (288, 8, 2) only while d.local; colour COL_WARN
    if (d.local) {
        if (changed(Z_LOCAL, "LOCAL")) {
            lcd.fillRect(288, 0, 60, 32, COL_PANEL);
            text("LOCAL", 288, 8, 2, COL_WARN, COL_PANEL);
        }
    } else {
        if (changed(Z_LOCAL, "")) {
            lcd.fillRect(288, 0, 60, 32, COL_PANEL);
        }
    }

    // Z_HV: "HV" at (360, 8, 2) — COL_WARN while hv_on, else COL_LABEL
    char hv_key[8];
    snprintf(hv_key, sizeof hv_key, "%s", d.safety.hv_on ? "HV_ON" : "HV");
    if (changed(Z_HV, hv_key)) {
        lcd.fillRect(360, 0, 24, 32, COL_PANEL);
        text("HV", 360, 8, 2, d.safety.hv_on ? COL_WARN : COL_LABEL, COL_PANEL);
    }

    // Z_SHUTTER: "SHUTTER" at (396, 8, 2) — COL_WARN while open, else COL_LABEL
    char sh_key[16];
    snprintf(sh_key, sizeof sh_key, "%s", d.safety.shutter_open ? "SHUTTER_OPEN" : "SHUTTER");
    if (changed(Z_SHUTTER, sh_key)) {
        lcd.fillRect(396, 0, 84, 32, COL_PANEL);
        text("SHUTTER", 396, 8, 2, d.safety.shutter_open ? COL_WARN : COL_LABEL, COL_PANEL);
    }
}
/// TODO(I1.2a) the four value fields, zones Z_V0..Z_V3; key = the value text ("%.1f" or "---").
void drawValues(const ProcData& d) {
    char val[16];

    // Z_V0: POS at x=0; label(6,38), value(6,60), unit "mm"(6,92); colour COL_TEXT
    snprintf(val, sizeof val, "%.1f", d.motion.pos);
    if (changed(Z_V0, val)) {
        lcd.fillRect(0, 34, 120, 74, COL_PANEL);
        text("POS", 6, 38, 2, COL_LABEL, COL_PANEL);
        text(val, 6, 60, 3, COL_TEXT, COL_PANEL);
        text("mm", 6, 92, 1, COL_LABEL, COL_PANEL);
    }

    // Z_V1: SPEED at x=120; label(126,38), value(126,60), unit "mm/s"(126,92)
    snprintf(val, sizeof val, "%.1f", d.motion.vel);
    if (changed(Z_V1, val)) {
        lcd.fillRect(120, 34, 120, 74, COL_PANEL);
        text("SPEED", 126, 38, 2, COL_LABEL, COL_PANEL);
        text(val, 126, 60, 3, COL_TEXT, COL_PANEL);
        text("mm/s", 126, 92, 1, COL_LABEL, COL_PANEL);
    }

    // Z_V2: DIST at x=240; "---" when !valid; colour COL_DIST / COL_TEXT
    if (d.measure.valid) {
        snprintf(val, sizeof val, "%.1f", d.measure.dist);
    } else {
        strncpy(val, "---", sizeof val - 1);
        val[sizeof val - 1] = '\0';
    }
    if (changed(Z_V2, val)) {
        lcd.fillRect(240, 34, 120, 74, COL_PANEL);
        text("DIST", 246, 38, 2, COL_LABEL, COL_PANEL);
        text(val, 246, 60, 3, d.measure.valid ? COL_DIST : COL_TEXT, COL_PANEL);
        text("mm", 246, 92, 1, COL_LABEL, COL_PANEL);
    }

    // Z_V3: THICK at x=360; "---" when !valid; colour COL_THICK / COL_TEXT
    if (d.measure.valid) {
        snprintf(val, sizeof val, "%.1f", d.measure.thick);
    } else {
        strncpy(val, "---", sizeof val - 1);
        val[sizeof val - 1] = '\0';
    }
    if (changed(Z_V3, val)) {
        lcd.fillRect(360, 34, 120, 74, COL_PANEL);
        text("THICK", 366, 38, 2, COL_LABEL, COL_PANEL);
        text(val, 366, 60, 3, d.measure.valid ? COL_THICK : COL_TEXT, COL_PANEL);
        text("mm", 366, 92, 1, COL_LABEL, COL_PANEL);
    }
}

// ---- I1.2b ------------------------------------------------------------------------------------
/// TODO(I1.2b) position bar, zone Z_BAR; key = barX(pos) as text.
void drawBar(const ProcData& d) {
    char key[16];
    snprintf(key, sizeof key, "%d", barX(d.motion.pos));
    if (changed(Z_BAR, key)) {
        lcd.fillRect(4, 112, 472, 20, COL_FRAME);
        int bx0 = barX(0.0f), bx20 = barX(20.0f), bx60 = barX(60.0f);
        lcd.fillRect(bx0, 112, bx20 - bx0, 20, COL_PARK);
        lcd.fillRect(bx20, 112, bx60 - bx20, 20, COL_WARN);
        lcd.fillRect(barX(d.motion.pos), 112, 4, 20, COL_TEXT);
    }
}
/// TODO(I1.2b) zones Z_ESTOP, Z_TEMP, Z_FLAGS, Z_MODE, Z_IP (IP line from the wifi getters).
void drawFlags(const ProcData& d, bool estop_nc, uint32_t now) {
    // Z_ESTOP
    char estop_key[64];
    const char* estop_text = estop_nc ? "ESTOP" : "ESTOP(NO)";
    uint32_t estop_color = d.safety.estop ? COL_FAULT : (estop_nc ? COL_OK : COL_WARN);
    snprintf(estop_key, sizeof estop_key, "%s|%x", estop_text, (unsigned)estop_color);
    if (changed(Z_ESTOP, estop_key)) {
        lcd.fillRect(0, 144, 168, 20, COL_PANEL);
        text(estop_text, 6, 144, 2, estop_color, COL_PANEL);
    }

    // Z_TEMP
    char temp_key[64];
    char temp_text[32];
    snprintf(temp_text, sizeof temp_text, "TEMP(SIM) %.1fC", d.safety.temp);
    uint32_t temp_color = d.safety.temp_ok ? COL_OK : COL_WARN;
    snprintf(temp_key, sizeof temp_key, "%s|%x", temp_text, (unsigned)temp_color);
    if (changed(Z_TEMP, temp_key)) {
        lcd.fillRect(170, 144, 310, 20, COL_PANEL);
        text(temp_text, 180, 144, 2, temp_color, COL_PANEL);
    }

    // Z_FLAGS: INTERLOCK(SIM) and COOLING(SIM)
    uint32_t il_color = d.safety.interlock ? COL_OK : COL_FAULT;
    uint32_t cf_color = d.safety.cooling_flow ? COL_OK : COL_FAULT;
    char flags_key[64];
    snprintf(flags_key, sizeof flags_key, "%x|%x", (unsigned)il_color, (unsigned)cf_color);
    if (changed(Z_FLAGS, flags_key)) {
        lcd.fillRect(0, 170, 480, 20, COL_PANEL);
        text("INTERLOCK(SIM)", 6, 170, 2, il_color, COL_PANEL);
        text("COOLING(SIM)", 220, 170, 2, cf_color, COL_PANEL);
    }

    // Z_MODE: "MODE %s  AVG %.1f (%u)"
    char mode_text[128];
    snprintf(mode_text, sizeof mode_text, "MODE %s  AVG %.1f (%u)",
             opModeName(d.motion.mode), d.measure.avg, (unsigned)d.measure.n);
    if (changed(Z_MODE, mode_text)) {
        lcd.fillRect(0, 196, 480, 20, COL_PANEL);
        text(mode_text, 6, 196, 2, COL_TEXT, COL_PANEL);
    }

    // Z_IP: "AP <ap_ip>  STA <part>"
    char ap_ip[32];
    wifiGetApIp(ap_ip, sizeof ap_ip);
    StaStatus st = wifiStaStatus();
    char ip_text[128];
    switch (st.state) {
        case StaState::CONNECTED: {
            char sta_ip[32];
            wifiGetStaIp(sta_ip, sizeof sta_ip);
            snprintf(ip_text, sizeof ip_text, "AP %s  STA %s", ap_ip, sta_ip);
            break;
        }
        case StaState::CONNECTING: {
            snprintf(ip_text, sizeof ip_text, "AP %s  STA connecting %d/%d",
                     ap_ip, st.attempt, (int)StaConfig{}.attempts);
            break;
        }
        case StaState::BACKOFF: {
            uint32_t remaining_s = (uint32_t)ceil(staBackoffRemainingMs(st, now, StaConfig{}) / 1000.0);
            snprintf(ip_text, sizeof ip_text, "AP %s  STA retry %lu s", ap_ip, (unsigned long)remaining_s);
            break;
        }
        default:
            snprintf(ip_text, sizeof ip_text, "AP %s  STA off", ap_ip);
            break;
    }
    if (changed(Z_IP, ip_text)) {
        lcd.fillRect(0, 222, 480, 20, COL_PANEL);
        text(ip_text, 6, 222, 2, COL_LABEL, COL_PANEL);
    }
}
/// TODO(I1.2b) button row, zone Z_BOTTOM; key "B1" while hv_on, else "B0".
void drawButtons(const ProcData& d) {
    const char* key = d.safety.hv_on ? "B1" : "B0";
    if (changed(Z_BOTTOM, key)) {
        lcd.fillRect(0, 260, 480, 60, COL_BG);

        const char* labels[6] = {"INIT", "HV", "MEAS", "TRAV", "PARK", "SERV"};
        for (int i = 0; i < 6; i++) {
            uint32_t bg = COL_BTN;
            if (i == 1) { // HV button
                bg = d.safety.hv_on ? COL_WARN : COL_BTN;
            }
            lcd.fillRect(i * 64 + 1, 261, 62, 58, bg);
            lcd.setTextSize(2);
            lcd.setTextColor(COL_TEXT);
            lcd.setTextDatum(lgfx::middle_center);
            lcd.drawString(labels[i], i * 64 + 32, 290);
        }

        // STOP button
        lcd.fillRect(385, 261, 94, 58, COL_FAULT);
        lcd.setTextSize(3);
        lcd.setTextColor(COL_TEXT);
        lcd.setTextDatum(lgfx::middle_center);
        lcd.drawString("STOP", 432, 290);
    }
}

// ---- I1.3 -------------------------------------------------------------------------------------
/// TODO(I1.3) Command with id, source LOCAL, want_reply false; cmdQueueSend.
void send(CmdId id) {
    (void)id;
}
/// TODO(I1.3) remedy texts of the header.
const char* remedy(ErrorCode e) {
    (void)e;
    return "";
}
/// TODO(I1.3) FAULT / ACK picture: zone Z_BANNER (key from the banner texts), then FAULT: clear
/// the button row (zone Z_BOTTOM, key "F"); ACK: the ACK area (zone Z_BOTTOM, key from cause
/// active, error name and progress width).
void drawFault(const ProcData& d, ScreenMode mode) {
    (void)d; (void)mode;
}

// ---- I1.2a ------------------------------------------------------------------------------------
/// TODO(I1.2a) one drawing: Params p; SafetyFlags f; snapshotGetConfig(p, f); lcd.startWrite();
/// on the first drawing or a mode change: fillScreen(COL_BG), fillRect(0, 0, 480, 32, COL_PANEL),
/// text("ADD DEMO", 8, 8, 2, COL_TEXT, COL_PANEL) and clear every s_key entry (s_key[i][0] = 0);
/// drawHeader(d); NORMAL: drawValues, drawBar, drawFlags(d, p.estop_nc, now), drawButtons;
/// else drawFault(d, mode); lcd.endWrite(); s_drawn = true; s_mode = mode; s_last_draw = now.
void draw(const ProcData& d, ScreenMode mode, uint32_t now) {
    Params p; SafetyFlags f; snapshotGetConfig(p, f);
    lcd.startWrite();
    if (!s_drawn || s_mode != mode) {
        lcd.fillScreen(COL_BG);
        lcd.fillRect(0, 0, 480, 32, COL_PANEL);
        text("ADD DEMO", 8, 8, 2, COL_TEXT, COL_PANEL);
        for (int i = 0; i < Z_COUNT; i++) s_key[i][0] = '\0';
    }
    drawHeader(d);
    if (mode == ScreenMode::NORMAL) {
        drawValues(d);
        drawBar(d);
        drawFlags(d, p.estop_nc, now);
        drawButtons(d);
    } else {
        drawFault(d, mode);
    }
    lcd.endWrite();
    s_drawn = true;
    s_mode = mode;
    s_last_draw = now;
}

/// TODO(I1.2a) the task function: for (;;) { displayTick(millis());
/// vTaskDelay(pdMS_TO_TICKS(DISPLAY_TOUCH_POLL_MS)); }
void displayTask(void*) {
    for (;;) {
        displayTick(millis());
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_TOUCH_POLL_MS));
    }
}

}  // namespace

/// TODO(I1.2a) steps 1, 2 and 5 of the header (I1.3 adds steps 3 and 4 between 2 and 5).
void displayTick(uint32_t now_ms) {
    ProcData d; snapshotGet(d);
    ScreenMode mode = screenModeOf(d.state);
    int x = 0, y = 0;
    bool touch = lcd.getTouch(&x, &y) > 0;

    if (!s_drawn || s_mode != mode || (uint32_t)(now_ms - s_last_draw) >= DISPLAY_PERIOD_MS) {
        draw(d, mode, now_ms);
    }

    s_prev_touch = touch;
}

/// TODO(I1.2a) as in the header.
bool displayStart() {
    if (!lcd.init()) return false;
    lcd.setRotation(1);
    lcd.setBrightness(255);
    lcd.fillScreen(COL_BG);
    return xTaskCreatePinnedToCore(displayTask, "disp", DISPLAY_TASK_STACK, nullptr,
                                   DISPLAY_TASK_PRIO, nullptr, DISPLAY_TASK_CORE) == pdPASS;
}
