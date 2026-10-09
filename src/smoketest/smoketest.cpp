/**
 * @file smoketest.cpp
 * @brief Sprint 0a – environment and hardware smoketest for the ADD Demo System.
 *
 * Build/flash:  pio run -e smoketest -t upload
 * Use:          pio device monitor -e smoketest      (interactive, type "help")
 *               python scripts/serial_cmd.py "ver"     (scriptable, for Cline)
 *
 * Protocol: one command per line. Every result is ONE line starting with '{'
 * (JSON). Informational lines start with '#'. Long-running commands print
 * progress lines and always end with a final JSON line containing "done".
 *
 * SAFETY: commands "scan", "move", "home" can move the axis. Keep the E-stop
 * within reach. Every motion command stops on E-stop or on any limit switch.
 */
#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>
#include <esp_idf_version.h>
#include <FastAccelStepper.h>
#include "config.h"
#include "display/lgfx_config.h"
#include <algorithm>

// The smoketest is self-contained: it must run BEFORE any src/logic module
// exists (Sprint 0a). These helpers mirror CONTRACT §4 and §7.
static uint32_t crc32Ieee(const uint8_t* d, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= d[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
static float medianMean(uint16_t* s, size_t n, size_t d) {
    std::sort(s, s + n);
    double sum = 0; for (size_t i = d; i < n - d; ++i) sum += s[i];
    return (float)(sum / (n - 2 * d));
}
static float sensorMvToMm(float mv) { return mv > 1.0f ? 120.8f * powf(mv / 1000.0f, -1.058f) : 9999.0f; }
static bool sensorInRange(float mm) { return mm >= 40.0f && mm <= 300.0f; }

static FastAccelStepperEngine engine;
static FastAccelStepper* stepper = nullptr;
static bool i2s_ok = false;
static LGFX lcd;
static bool lcd_ok = false;
static WebServer* web = nullptr;

static char line[96];
static size_t line_len = 0;

// ------------------------------------------------------------------ helpers
static bool rawLimitFwd() { return digitalRead(PIN_LIMIT_FWD) == LIMIT_ACTIVE_LEVEL; }
static bool rawLimitBwd() { return digitalRead(PIN_LIMIT_BWD) == LIMIT_ACTIVE_LEVEL; }
static bool rawEstop()    { return digitalRead(PIN_ESTOP) == ESTOP_ACTIVE_LEVEL; }

static float readMv() {
    uint16_t s[ADC_SAMPLES];
    for (int i = 0; i < ADC_SAMPLES; ++i) s[i] = (uint16_t)analogReadMilliVolts(PIN_SENSOR_ADC);
    return medianMean(s, ADC_SAMPLES, ADC_DISCARD);
}

static void setSlot(int slot, bool v) { if (i2s_ok) engine.i2sMuxSetBit((uint8_t)slot, v); }

static const char* tf(bool b) { return b ? "true" : "false"; }

// Waits while the stepper runs; aborts on E-stop / unexpected limit / timeout.
static void waitMotion(uint32_t timeout_ms, bool allow_bwd_limit) {
    uint32_t t0 = millis();
    const char* reason = "arrived";
    while (stepper && stepper->isRunning()) {
        if (rawEstop()) { stepper->forceStop(); reason = "estop"; break; }
        if (rawLimitFwd()) { stepper->forceStop(); reason = "limit_fwd"; break; }
        if (rawLimitBwd() && !allow_bwd_limit) { stepper->forceStop(); reason = "limit_bwd"; break; }
        if (rawLimitBwd() && allow_bwd_limit) { stepper->forceStopAndNewPosition(0); reason = "home_switch"; break; }
        if (millis() - t0 > timeout_ms) { stepper->forceStop(); reason = "timeout"; break; }
        delay(1);
    }
    Serial.printf("{\"done\":true,\"reason\":\"%s\",\"pos_steps\":%ld,\"pos_mm\":%.2f,\"ms\":%lu}\n",
                  reason, (long)stepper->getCurrentPosition(),
                  stepper->getCurrentPosition() / STEPS_PER_MM, (unsigned long)(millis() - t0));
}

// ------------------------------------------------------------------ commands
static void cmdHelp() {
    Serial.println("# ver | crc | part | nvs | fs | in | watch <s> | adc [n]");
    Serial.println("# slot <n> <0|1> | scan [ms] | hv <0|1> | beep [ms] | shutter <0|1>");
    Serial.println("# stepper | move <mm> [mm/min] | home | pos | stop");
    Serial.println("# disp | touch <s> | wifi | help");
    Serial.println("{\"done\":true}");
}

static void cmdVer() {
    Serial.printf("{\"fw\":\"smoketest-0a\",\"arduino\":\"%d.%d.%d\",\"idf\":\"%s\",\"chip_rev\":%d,"
                  "\"cpu_mhz\":%lu,\"flash_mb\":%lu,\"heap\":%lu,\"i2s_mux\":%s,\"steps_per_mm\":%.3f}\n",
                  ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH,
                  esp_get_idf_version(), ESP.getChipRevision(), (unsigned long)ESP.getCpuFreqMHz(),
                  (unsigned long)(ESP.getFlashChipSize() / (1024 * 1024)), (unsigned long)ESP.getFreeHeap(),
                  tf(i2s_ok), STEPS_PER_MM);
}

static void cmdCrc() {
    const char* v = "123456789";
    uint32_t own = crc32Ieee((const uint8_t*)v, 9);
    uint32_t rom = esp_rom_crc32_le(0, (const uint8_t*)v, 9);
    Serial.printf("{\"own\":\"%08lX\",\"esp_rom_crc32_le\":\"%08lX\",\"expected\":\"CBF43926\",\"match\":%s}\n",
                  (unsigned long)own, (unsigned long)rom, tf(own == 0xCBF43926u && rom == own));
}

static void cmdPart() {
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    while (it) {
        const esp_partition_t* p = esp_partition_get(it);
        Serial.printf("# %-8s type=%d sub=%d off=0x%06lX size=0x%06lX (%lu KB)\n", p->label, p->type, p->subtype,
                      (unsigned long)p->address, (unsigned long)p->size, (unsigned long)(p->size / 1024));
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    Serial.println("{\"done\":true}");
}

static void cmdNvs() {
    Preferences p;
    bool ok = p.begin("smoketest", false);
    p.putFloat("f", 42.5f);
    p.putString("s", "add");
    float f = p.getFloat("f", 0);
    String s = p.getString("s", "");
    p.clear();
    p.end();
    Serial.printf("{\"nvs\":%s}\n", tf(ok && f == 42.5f && s == "add"));
}

static void cmdFs() {
    bool ok = SPIFFS.begin(true);
    Serial.printf("{\"spiffs\":%s,\"total\":%u,\"used\":%u}\n", tf(ok), ok ? SPIFFS.totalBytes() : 0,
                  ok ? SPIFFS.usedBytes() : 0);
}

static void printInputs() {
    Serial.printf("{\"limit_fwd\":%s,\"limit_bwd\":%s,\"estop\":%s,\"lvl_fwd\":%d,\"lvl_bwd\":%d,\"lvl_estop\":%d}\n",
                  tf(rawLimitFwd()), tf(rawLimitBwd()), tf(rawEstop()), digitalRead(PIN_LIMIT_FWD),
                  digitalRead(PIN_LIMIT_BWD), digitalRead(PIN_ESTOP));
}

static void cmdWatch(int sec) {
    if (sec <= 0) sec = 10;
    uint32_t end = millis() + sec * 1000UL;
    while ((int32_t)(end - millis()) > 0) { printInputs(); delay(200); }
    Serial.println("{\"done\":true}");
}

static void cmdAdc(int n) {
    if (n <= 0) n = 10;
    for (int i = 0; i < n; ++i) {
        float mv = readMv();
        float mm = sensorMvToMm(mv);
        Serial.printf("{\"mv\":%.0f,\"mm\":%.1f,\"in_range\":%s}\n", mv, mm, tf(sensorInRange(mm)));
        delay(100);
    }
    Serial.println("{\"done\":true}");
}

static void cmdScan(int ms) {
    if (!i2s_ok) { Serial.println("{\"error\":\"i2s mux not initialised\"}"); return; }
    if (ms <= 0) ms = 800;
    Serial.println("# Watch buzzer (expected i2so.7), motor enable (i2so.0, holding torque), step (i2so.1), dir (i2so.2). HV LED is GPIO 4 (cmd hv).");
    for (int s = 0; s < 32; ++s) {
        Serial.printf("{\"slot\":%d,\"state\":1}\n", s);
        setSlot(s, true);
        delay(ms);
        setSlot(s, false);
        delay(ms / 2);
    }
    Serial.println("{\"done\":true}");
}

static void cmdStepper() {
    if (!i2s_ok) { Serial.println("{\"error\":\"i2s mux not initialised\"}"); return; }
    if (!stepper) {
        stepper = engine.stepperConnectToPin(I2SO_TO_SLOT(I2SO_X_STEP) | PIN_I2S_FLAG, DRIVER_I2S_MUX);
        if (stepper) {
            stepper->setDirectionPin(I2SO_TO_SLOT(I2SO_X_DIR) | PIN_I2S_FLAG, DIR_POSITIVE_HIGH);
            stepper->setEnablePin(I2SO_TO_SLOT(I2SO_SHARED_EN) | PIN_I2S_FLAG, true);
            stepper->setAutoEnable(true);
            stepper->setAcceleration((int32_t)(ACCEL_MM_S2 * STEPS_PER_MM));
        }
    }
    Serial.printf("{\"stepper\":%s,\"step_slot\":%d,\"dir_slot\":%d,\"en_slot\":%d}\n", tf(stepper != nullptr),
                  I2SO_TO_SLOT(I2SO_X_STEP), I2SO_TO_SLOT(I2SO_X_DIR), I2SO_TO_SLOT(I2SO_SHARED_EN));
}

static void cmdMove(float mm, float speed) {
    if (!stepper) { Serial.println("{\"error\":\"run 'stepper' first\"}"); return; }
    if (speed <= 0) speed = 300;
    stepper->setSpeedInHz((uint32_t)(speed / 60.0f * STEPS_PER_MM));
    stepper->move((int32_t)lroundf(mm * STEPS_PER_MM));
    Serial.printf("# move %.2f mm at %.0f mm/min - measure the real distance!\n", mm, speed);
    waitMotion(120000, false);
}

static void cmdHome() {
    if (!stepper) { Serial.println("{\"error\":\"run 'stepper' first\"}"); return; }
    if (rawLimitBwd()) { stepper->forceStopAndNewPosition(0); Serial.println("{\"done\":true,\"reason\":\"already_on_switch\"}"); return; }
    stepper->setSpeedInHz((uint32_t)(600.0f / 60.0f * STEPS_PER_MM));
    stepper->runBackward();
    Serial.println("# seeking backward switch at 600 mm/min (timeout 40 s)");
    waitMotion(40000, true);
}

static void cmdDisp() {
    if (!lcd_ok) lcd_ok = lcd.init();
    if (!lcd_ok) { Serial.println("{\"display\":false}"); return; }
    lcd.setRotation(1);
    lcd.fillScreen(TFT_BLACK);
    lcd.fillRect(0, 0, 160, 40, TFT_RED);
    lcd.fillRect(160, 0, 160, 40, TFT_GREEN);
    lcd.fillRect(320, 0, 160, 40, TFT_BLUE);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(3);
    lcd.drawString("ADD SMOKETEST", 20, 70);
    lcd.setTextSize(2);
    lcd.drawString("RED  GREEN  BLUE (top bar)", 20, 120);
    lcd.drawString("480 x 320, rotation 1", 20, 150);
    lcd.drawRect(0, 0, 480, 320, TFT_YELLOW);
    for (int i = 0; i < 4; ++i) {   // corner targets for touch check
        int x = (i % 2) ? 460 : 20, y = (i / 2) ? 300 : 60;
        lcd.drawCircle(x, y, 10, TFT_CYAN);
    }
    Serial.printf("{\"display\":true,\"w\":%d,\"h\":%d}\n", lcd.width(), lcd.height());
}

static void cmdTouch(int sec) {
    if (!lcd_ok) cmdDisp();
    if (sec <= 0) sec = 15;
    Serial.println("# touch the four cyan circles; compare raw vs mapped coordinates");
    uint32_t end = millis() + sec * 1000UL;
    while ((int32_t)(end - millis()) > 0) {
        lgfx::touch_point_t raw, map;
        if (lcd.getTouchRaw(&raw, 1) && lcd.getTouch(&map, 1)) {
            Serial.printf("{\"raw_x\":%d,\"raw_y\":%d,\"x\":%d,\"y\":%d}\n", raw.x, raw.y, map.x, map.y);
            lcd.fillCircle(map.x, map.y, 3, TFT_MAGENTA);
        }
        delay(100);
    }
    Serial.println("{\"done\":true}");
}

static void cmdWifi() {
    if (!web) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
        web = new WebServer(HTTP_PORT);
        web->on("/ping", []() {
            char b[160];
            snprintf(b, sizeof(b), "{\"pong\":true,\"idf\":\"%s\",\"uptime_ms\":%lu}", esp_get_idf_version(),
                     (unsigned long)millis());
            web->send(200, "application/json", b);
        });
        web->begin();
    }
    Serial.printf("{\"ap\":\"%s\",\"ip\":\"%s\",\"url\":\"http://%s/ping\"}\n", WIFI_AP_SSID,
                  WiFi.softAPIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
}

static void dispatch(char* l) {
    char cmd[16] = {0};
    char a1[24] = {0}, a2[24] = {0};
    sscanf(l, "%15s %23s %23s", cmd, a1, a2);
    String c(cmd);
    if (c == "help" || c == "") cmdHelp();
    else if (c == "ver") cmdVer();
    else if (c == "crc") cmdCrc();
    else if (c == "part") cmdPart();
    else if (c == "nvs") cmdNvs();
    else if (c == "fs") cmdFs();
    else if (c == "in") printInputs();
    else if (c == "watch") cmdWatch(atoi(a1));
    else if (c == "adc") cmdAdc(atoi(a1));
    else if (c == "slot") { setSlot(atoi(a1), atoi(a2) != 0); Serial.printf("{\"slot\":%d,\"state\":%d}\n", atoi(a1), atoi(a2) != 0); }
    else if (c == "scan") cmdScan(atoi(a1));
    else if (c == "hv") { digitalWrite(PIN_HV_LED, atoi(a1) ? HIGH : LOW); Serial.printf("{\"hv\":%d}\n", atoi(a1) != 0); }
    else if (c == "beep") { int ms = atoi(a1) > 0 ? atoi(a1) : 200; setSlot(I2SO_TO_SLOT(I2SO_BUZZER), true); delay(ms);
                            setSlot(I2SO_TO_SLOT(I2SO_BUZZER), false); Serial.println("{\"done\":true}"); }
    else if (c == "shutter") { digitalWrite(PIN_SHUTTER, atoi(a1) ? HIGH : LOW); Serial.printf("{\"shutter\":%d}\n", atoi(a1) != 0); }
    else if (c == "stepper") cmdStepper();
    else if (c == "move") cmdMove((float)atof(a1), (float)atof(a2));
    else if (c == "home") cmdHome();
    else if (c == "pos") Serial.printf("{\"pos_steps\":%ld}\n", stepper ? (long)stepper->getCurrentPosition() : 0L);
    else if (c == "stop") { if (stepper) stepper->forceStop(); Serial.println("{\"done\":true}"); }
    else if (c == "disp") cmdDisp();
    else if (c == "touch") cmdTouch(atoi(a1));
    else if (c == "wifi") cmdWifi();
    else Serial.printf("{\"error\":\"unknown command '%s'\"}\n", cmd);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    pinMode(PIN_LIMIT_FWD, INPUT);
    pinMode(PIN_LIMIT_BWD, INPUT);
    pinMode(PIN_ESTOP, INPUT);
    pinMode(PIN_SHUTTER, OUTPUT);
    digitalWrite(PIN_SHUTTER, LOW);
    pinMode(PIN_HV_LED, OUTPUT);          // V1.2: HV LED on GPIO 4
    digitalWrite(PIN_HV_LED, LOW);
    analogSetPinAttenuation(PIN_SENSOR_ADC, ADC_11db);

    engine.init(STEPPER_TASK_CORE);
    i2s_ok = engine.initI2sMux(PIN_I2S_DATA, PIN_I2S_BCK, PIN_I2S_WS);
    if (i2s_ok) for (int s = 0; s < 32; ++s) engine.i2sMuxSetBit(s, false);
    if (i2s_ok) setSlot(I2SO_TO_SLOT(I2SO_SHARED_EN), true);   // enable is active LOW -> HIGH = disabled

    Serial.println("# ADD Demo System - Sprint 0a smoketest. Type 'help'.");
    cmdVer();
}

void loop() {
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\r') continue;
        if (ch == '\n') {
            line[line_len] = 0;
            dispatch(line);
            line_len = 0;
        } else if (line_len < sizeof(line) - 1) {
            line[line_len++] = ch;
        }
    }
    if (web) web->handleClient();
    delay(2);
}
