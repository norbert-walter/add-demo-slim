/**
 * @file display.cpp
 * @brief Local status display + touch operation (CONTRACT V1.4, Sprint I1).
 */

#include "display.h"
#include "../config.h"
#include "../logic/display_logic.h"
#include "../logic/types.h"
#include "../logic/safety_logic.h"
#include "../app/snapshot.h"
#include "../app/command_queue.h"
#include "../comm/wifi_manager.h"

#include <LovyanGFX.hpp>
#include <Arduino.h>
#include <algorithm>

namespace {

LGFX lcd;

static ScreenMode   current_mode = ScreenMode::NORMAL;
static uint32_t     last_draw  = 0;
static bool         first_draw  = true;
static AckPress     ack_press{};

struct ZoneCache {
    uint8_t header   : 1;
    uint8_t values   : 1;
    uint8_t bar      : 1;
    uint8_t flags    : 1;
    uint8_t buttons  : 1;
} zone_cache{};

static inline int clamp(int v, int lo, int hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

static int barX(float mm) {
    int m = clamp((int)mm, 0, 242);
    return 4 + (m * 468) / 242;
}

static const char* safetyStateName(SafetyState s) {
    switch (s) {
        case SafetyState::SAFE_START:     return "START";
        case SafetyState::READY:          return "READY";
        case SafetyState::RUNNING:        return "RUNNING";
        case SafetyState::RECOVER:        return "RECOVER";
        case SafetyState::FAULT:          return "FAULT";
        case SafetyState::WAIT_ACK:       return "WAIT_ACK";
        default:                          return "UNKNOWN";
    }
}

static const char* opModeName(MotionMode m) {
    switch (m) {
        case MotionMode::TRAVERSE:  return "traverse";
        case MotionMode::MEASURE:   return "measure";
        case MotionMode::PARK:      return "park";
        case MotionMode::SERVICE:   return "service";
        default:                    return "unknown";
    }
}

static const char* errorCodeName(FaultCode f) {
    switch (f) {
        case FaultCode::ESTOP:            return "E-STOP";
        case FaultCode::TEMP_HIGH:        return "TEMP HIGH";
        case FaultCode::MOTOR_FAULT:      return "MOTOR FAULT";
        case FaultCode::COMM_TIMEOUT:     return "COMM TIMEOUT";
        case FaultCode::HOMING_FAILED:    return "HOMING FAILED";
        default:                          return "NONE";
    }
}

static void drawText(const char* s, int x, int y, int size, uint32_t fg) {
    lcd.setTextSize(size);
    lcd.setTextColor(fg);
    lcd.setTextDatum(top_left);
    lcd.drawString(s, x, y);
}

static void drawTextCentered(const char* s, int x, int y, int size, uint32_t fg) {
    lcd.setTextSize(size);
    lcd.setTextColor(fg);
    lcd.setTextDatum(middle_center);
    lcd.drawString(s, x, y);
}

static void drawHeader(const Snapshot& d, ScreenMode mode) {
    lcd.fillRect(0, 0, 480, 32, COL_PANEL);
    drawText("ADD DEMO", 8, 8, 2, COL_TEXT);
    const char* state = safetyStateName(d.state);
    char buf[32];
    snprintf(buf, sizeof buf, "[ %s ]", state);
    uint32_t stateColor = COL_WARN;
    if (mode == ScreenMode::NORMAL) {
        if (d.state == SafetyState::READY || d.state == SafetyState::RUNNING)
            stateColor = COL_OK;
        else if (d.state == SafetyState::SAFE_START || d.state == SafetyState::RECOVER)
            stateColor = COL_WARN;
    } else {
        stateColor = COL_FAULT;
    }
    drawText(buf, 150, 8, 2, stateColor);
    if (d.local) {
        drawText("LOCAL", 288, 8, 2, COL_WARN);
    }
    if (d.hv_on) {
        drawText("HV", 360, 8, 2, COL_WARN);
    } else {
        drawText("HV", 360, 8, 2, COL_LABEL);
    }
    if (d.shutter_open) {
        drawText("SHUTTER", 396, 8, 2, COL_WARN);
    } else {
        drawText("SHUTTER", 396, 8, 2, COL_LABEL);
    }
}

static void drawValues(const Snapshot& d) {
    struct Field {
        int x;
        const char* label;
        const char* unit;
        uint32_t valColor;
        bool hasValue;
    };
    float values[4] = { d.motion.pos, d.motion.vel, d.measure.dist, d.measure.thick };
    Field fields[4] = {
        { 0,   "POS",   "mm",     COL_TEXT,  true },
        { 120, "SPEED", "mm/s",   COL_TEXT,  true },
        { 240, "DIST",  "mm",     COL_DIST,  d.measure.valid },
        { 360, "THICK", "mm",     COL_THICK, d.measure.valid }
    };
    for (int i = 0; i < 4; ++i) {
        int x = fields[i].x;
        drawText(fields[i].label, x + 6, 38, 2, COL_LABEL);
        char vbuf[16];
        if (!fields[i].hasValue) {
            strncpy(vbuf, "---", sizeof vbuf);
            vbuf[3] = 0;
        } else {
            snprintf(vbuf, sizeof vbuf, "%.1f", values[i]);
        }
        lcd.setTextSize(3);
        lcd.setTextColor(fields[i].valColor);
        lcd.setTextDatum(top_left);
        lcd.drawString(vbuf, x + 6, 60);
        drawText(fields[i].unit, x + 6, 92, 1, COL_LABEL);
    }
}

static void drawBar(float pos_mm) {
    int bx0  = barX(0.0f);
    int bx20 = barX(20.0f);
    int bx60 = barX(60.0f);
    lcd.fillRect(bx0, 112, bx20 - bx0, 20, COL_PARK);
    lcd.fillRect(bx20, 112, bx60 - bx20, 20, COL_WARN);
    lcd.drawRect(4, 112, 468, 20, COL_FRAME);
    int px = barX(pos_mm);
    lcd.fillRect(px, 112, 4, 20, COL_TEXT);
}

static void drawFlags(const Snapshot& d) {
    bool estop_nc = snapshotGetConfig().estop_nc;
    if (d.safety.estop) {
        drawText("ESTOP", 6, 144, 2, COL_FAULT);
    } else {
        if (estop_nc)
            drawText("ESTOP", 6, 144, 2, COL_OK);
        else
            drawText("ESTOP(NO)", 6, 144, 2, COL_WARN);
    }
    float temp = d.safety.temp;
    bool temp_ok = (temp <= 32.0f);
    char tbuf[32];
    snprintf(tbuf, sizeof tbuf, "TEMP(SIM) %.1fC", temp);
    drawText(tbuf, 180, 144, 2, temp_ok ? COL_OK : COL_WARN);
    if (d.safety.interlock)
        drawText("INTERLOCK(SIM)", 6, 170, 2, COL_OK);
    else
        drawText("INTERLOCK(SIM)", 6, 170, 2, COL_FAULT);
    if (d.safety.cooling_flow)
        drawText("COOLING(SIM)", 220, 170, 2, COL_OK);
    else
        drawText("COOLING(SIM)", 220, 170, 2, COL_FAULT);
    char mbuf[128];
    snprintf(mbuf, sizeof mbuf, "MODE %s  AVG %.1f (%lu)",
             opModeName(d.motion.mode), d.measure.avg, (unsigned long)d.measure.n);
    drawText(mbuf, 6, 196, 2, COL_TEXT);
    const char* apIp = "192.168.4.1";
    char ipLine[128];
    snprintf(ipLine, sizeof ipLine, "AP %s  STA off", apIp);
    drawText(ipLine, 6, 222, 2, COL_LABEL);
}

static void drawButtons(const Snapshot& d) {
    struct Btn {
        int idx;
        const char* label;
        bool isStop;
        uint32_t bgCol;
    };
    Btn btns[6] = {
        { 0, "INIT", false, COL_BTN },
        { 1, "HV",   false, COL_BTN },
        { 2, "MEAS", false, COL_BTN },
        { 3, "TRAV", false, COL_BTN },
        { 4, "PARK", false, COL_BTN },
        { 5, "SERV", false, COL_WARN }
    };
    for (int i = 0; i < 6; ++i) {
        uint32_t bg = btns[i].bgCol;
        if (!btns[i].isStop && btns[i].idx == 1 && d.hv_on) {
            bg = COL_WARN;
        }
        lcd.fillRect(btns[i].idx * 64 + 1, 261, 62, 58, bg);
        drawTextCentered(btns[i].label, btns[i].idx * 64 + 32, 290, 2, COL_TEXT);
    }
    lcd.fillRect(385, 261, 94, 58, COL_FAULT);
    drawTextCentered("STOP", 432, 290, 3, COL_TEXT);
}

static void drawFaultBanner(const Snapshot& d) {
    lcd.fillRect(0, 34, 480, 224, COL_BG);
    char fbBuf[64];
    snprintf(fbBuf, sizeof fbBuf, "FAULT: %s", errorCodeName(d.fault.code));
    drawTextCentered(fbBuf, 240, 90, 3, COL_FAULT);
    const char* remedy = "";
    switch (d.fault.code) {
        case FaultCode::ESTOP:            remedy = "Release E-stop, then acknowledge"; break;
        case FaultCode::TEMP_HIGH:        remedy = "Lower temp <= 32 C"; break;
        case FaultCode::MOTOR_FAULT:      remedy = "Check axis and end switches"; break;
        case FaultCode::COMM_TIMEOUT:     remedy = "Remote client lost"; break;
        case FaultCode::HOMING_FAILED:    remedy = "Check home switch, then INIT"; break;
        default:                          remedy = ""; break;
    }
    if (remedy[0]) {
        drawTextCentered(remedy, 240, 140, 2, COL_TEXT);
    }
    char posBuf[32];
    snprintf(posBuf, sizeof posBuf, "POS %.1f mm", d.motion.pos);
    drawTextCentered(posBuf, 240, 190, 2, COL_TEXT);
    char etBuf[64];
    if (d.safety.estop) {
        snprintf(etBuf, sizeof etBuf, "ESTOP ACTIVE  TEMP(SIM) %.1fC", d.safety.temp);
        drawTextCentered(etBuf, 240, 214, 2, COL_FAULT);
    } else {
        snprintf(etBuf, sizeof etBuf, "ESTOP OK  TEMP(SIM) %.1fC", d.safety.temp);
        drawTextCentered(etBuf, 240, 214, 2, COL_TEXT);
    }
}

static void draw(Snapshot& d, ScreenMode mode, uint32_t now) {
    bool modeChanged = (mode != current_mode);
    if (first_draw || modeChanged) {
        lcd.fillRect(0, 0, 480, 320, COL_BG);
        zone_cache = {0, 0, 0, 0, 0};
    }
    drawHeader(d, mode);
    zone_cache.header = 1;
    if (mode == ScreenMode::NORMAL) {
        drawValues(d);
        zone_cache.values = 1;
        drawBar(d.motion.pos);
        zone_cache.bar = 1;
        drawFlags(d);
        zone_cache.flags = 1;
        drawButtons(d);
        zone_cache.buttons = 1;
    } else if (mode == ScreenMode::FAULT) {
        drawFaultBanner(d);
        lcd.fillRect(0, 260, 480, 60, COL_BG);
    } else if (mode == ScreenMode::ACK) {
        drawFaultBanner(d);
        lcd.fillRect(0, 260, 480, 60, COL_BG);
    }
    last_draw = now;
    current_mode = mode;
    first_draw = false;
}

static void displayTask(void* param) {
    (void)param;
    while (true) {
        displayTick(millis());
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_TOUCH_POLL_MS));
    }
}

} // anonymous namespace

bool displayStart() {
    if (!lcd.init()) {
        return false;
    }
    lcd.setRotation(1);
    lcd.setBrightness(0);
    lcd.fillScreen(COL_BG);
    TaskHandle_t handle = nullptr;
    if (xTaskCreatePinnedToCore(displayTask, "disp", DISPLAY_TASK_STACK,
                                nullptr, DISPLAY_TASK_PRIO,
                                &handle, DISPLAY_TASK_CORE) != pdPASS) {
        return false;
    }
    return true;
}

void displayTick(uint32_t now_ms) {
    Snapshot d;
    snapshotGet(d);
    ScreenMode mode = screenModeOf(d.state);
    int tx = 0, ty = 0;
    bool touch = lcd.getTouch(&tx, &ty) > 0;
    (void)touch; (void)tx; (void)ty;
    if (mode != ScreenMode::ACK) {
        ack_press.pressing = false;
        ack_press.sent     = false;
    }
    ack_press.prev_touch = touch;
    bool modeChanged = (mode != current_mode);
    bool elapsed = (now_ms - last_draw) >= DISPLAY_PERIOD_MS;
    if (first_draw || modeChanged || elapsed) {
        snapshotGet(d);
        draw(d, mode, now_ms);
    }
}
