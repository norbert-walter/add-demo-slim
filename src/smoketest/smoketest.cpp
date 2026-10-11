/**
 * @file smoketest.cpp
 * @brief Sprint 0a – environment and hardware smoketest for the ADD Demo System.
 *
 * Purpose
 * -------
 * A small, self-contained firmware that checks toolchain, flash layout, inputs,
 * I2S shift register, stepper drive, sensor, display and WiFi BEFORE the real
 * firmware exists. Its results fill docs/Smoketest_Protokoll.md and verify the
 * VERIFY(0a) values in src/config.h. It uses NO module of src/logic, src/app or
 * src/hal – only config.h, the board support package and the libraries.
 *
 * Build / flash / use
 * -------------------
 *   pio run -e smoketest                 build only
 *   pio run -e smoketest -t upload       flash (USER APPROVAL – can move the motor)
 *   pio device monitor -e smoketest      interactive terminal, type "help"
 *   python scripts/serial_cmd.py "ver"   one command, scriptable (Cline)
 *
 * Protocol
 * --------
 * One command per line (terminated by '\n'; '\r' is ignored, max. 95 chars).
 * Words are separated by blanks: <command> [arg1] [arg2] (max. 23 chars each).
 * Every result is ONE line starting with '{' (JSON). Lines starting with '#'
 * are information only. Long-running commands print progress lines and always
 * end with a JSON line containing "done".
 * Errors: {"error":"usage: <syntax>"} for missing, malformed, surplus or too
 * long arguments; a specific text for values out of range ("mm out of range
 * (0.01..250)") or a refused state ("estop active", "stepper running");
 * {"error":"unknown command '<x>' - type 'help'"} for unknown commands.
 * In every error case NOTHING is switched or moved. Only "stop" is always
 * executed, whatever follows it.
 *
 * Command overview                                         (M) = can move the axis
 * ----------------------------------------------------------------------------------
 *  help                     list of commands
 *
 *  -- toolchain / memory (protocol section 1) -------------------------------------
 *  ver                      firmware, Arduino core, ESP-IDF, chip revision, flash
 *                           size, free heap, I2S mux ok, steps per mm
 *  crc                      own CRC32 vs. esp_rom_crc32_le for "123456789"
 *                           (expected CBF43926, CONTRACT §4)
 *  part                     partition table (label, type, offset, size)
 *  nvs                      write + read back a float and a string in NVS
 *                           (namespace "smoketest", cleared afterwards)
 *  fs                       mount SPIFFS (formats it on the first call)
 *  uptime                   milliseconds since boot (added by Qwen, protocol 8.1)
 *
 *  -- inputs (section 2) ----------------------------------------------------------
 *  in                       limit switches and E-stop once: logical + raw level
 *  watch [s]                like "in", 5x per second for s seconds (1..120, default 10)
 *  estop <low|high>         active level of the E-stop input for this session:
 *                           low  = simple normally-open switch (default, estop_nc=false)
 *                           high = normally-closed switch with wire-break detection
 *
 *  -- I2S shift register and outputs (section 3) ----------------------------------
 *  scan [ms]           (M)  switches slot 0..31 one after another for ms
 *                           (100..5000, default 800) + ms/2 pause; buzzer, enable,
 *                           step can be observed. Afterwards all slots off and the
 *                           driver disabled again.
 *  slot <n> <0|1>      (M)  sets one shift register slot n (0..31) directly.
 *                           Slot 0 = driver enable, ACTIVE LOW: "slot 0 0" ENABLES
 *                           the driver, "slot 0 1" disables it.
 *  beep [ms]                buzzer on for ms (1..5000, default 200)
 *  hv <0|1>                 HV LED (green, GPIO 4)
 *  shutter <0|1>            shutter LED (red, GPIO 32)
 *
 *  -- drive (section 4) -----------------------------------------------------------
 *  stepper                  attaches FastAccelStepper (step/dir/enable via I2S mux,
 *                           auto-enable: the driver is only powered while moving)
 *  move <mm> <mm/s>    (M)  RELATIVE move. mm: -250..250 (not 0), mm/s: 1..20.
 *                           Both arguments are required. Positive = away from the
 *                           home (backward) switch.
 *  home                (M)  runs backward at 5 mm/s until the backward switch,
 *                           sets position 0 there (timeout 70 s)
 *  pos                      current position in steps and mm
 *  stop                     immediate stop (always allowed)
 *
 *  -- distance sensor (section 5) -------------------------------------------------
 *  adc [n]                  n readings (1..200, default 10) every 100 ms:
 *                           mV (median-mean of ADC_SAMPLES) and mm (curve 168.8*V^-1.398)
 *
 *  -- display / touch (section 6) -------------------------------------------------
 *  disp                     test pattern: colour bars, frame, four touch targets
 *  touch [s]                prints raw and mapped touch points for s seconds
 *                           (1..120, default 15)
 *
 *  -- WiFi (section 7) ------------------------------------------------------------
 *  wifi                     starts the access point + web server with GET /ping
 *  sta <ssid> [pass]        additionally connects to a router (max. 23 chars each,
 *                           no blanks), waits up to 10 s; the AP stays on
 *
 * SAFETY
 * ------
 * Commands marked (M) can move the axis. Keep the E-stop within reach.
 * Every motion stops on the E-stop or on the limit switch in the direction of
 * travel – inside waitMotion() AND in superviseMotion(), which loop() calls on
 * every pass while the stepper runs. NOTE: the E-stop only acts through this
 * software; it does not cut the motor power (see Pflichtenheft, open points).
 *
 * History
 * -------
 * 0a.1 (11.10.2026) – fixes after the incident "move 2010" (typo for "move 20 10"):
 *  - Standing on the home switch, the forward move aborted at once ("limit_bwd")
 *    because ALL limit switches were checked. The forceStop() came ~0 ms after
 *    move(); FastAccelStepper clears an immediate stop while a NEW move is still
 *    pending (RampGenerator::getNextCommand), so the move ran on – and nothing
 *    checked the E-stop or the forward switch any more.
 *  - Now: limits checked per direction, race-free stopNow(), supervisor in
 *    loop(), strict argument checks for all commands (move, slot, scan, ...),
 *    scan leaves the driver disabled, sensor curve per CONTRACT V1.4.2.
 * 0a.2 (11.10.2026) – "sta" failed with ESP_ERR_WIFI_STATE ("sta is connecting")
 *    because an earlier attempt was still retrying. Now: earlier attempt
 *    cancelled first, no auto-reconnect, no credentials in NVS, disconnect
 *    reason in the reply.
 * 0a.3 (11.10.2026) – "uptime" (written by Qwen, protocol 8.1); ADC attenuation
 *    set after the first read (boot log error in 0a.2).
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
#include <stdlib.h>
#include <math.h>

// =============================================================================
// Self-contained helpers. The smoketest must run BEFORE any src/logic module
// exists (Sprint 0a), so these mirror the contract instead of including it.
// =============================================================================

/// CRC-32/IEEE 802.3 (zlib), bitwise. Same algorithm as CONTRACT §4 / crc32.h.
static uint32_t crc32Ieee(const uint8_t* d, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= d[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}

/// Sorts n samples in place, discards d values at each end and returns the mean
/// of the rest (outlier-robust ADC filter, CONTRACT §7). Requires n > 2*d.
static float medianMean(uint16_t* s, size_t n, size_t d) {
    std::sort(s, s + n);
    double sum = 0; for (size_t i = d; i < n - d; ++i) sum += s[i];
    return (float)(sum / (n - 2 * d));
}

/// Sharp GP2Y0A41SK0F curve fitted to the demo (CONTRACT §7.1, V1.4.2): mm = 168.8 * V^-1.398.
/// Requires the 11 kOhm load Vo-GND at the sensor (without it the input hangs at full scale).
/// Returns 9999 for "no signal" (<= 1 mV).
static float sensorMvToMm(float mv) { return mv > 1.0f ? 168.8f * powf(mv / 1000.0f, -1.398f) : 9999.0f; }

/// Valid measuring range of the sensor.
static bool sensorInRange(float mm) { return mm >= 35.0f && mm <= 300.0f; }   // V1.4.2

// =============================================================================
// Global state (single task: everything runs in the Arduino loop task)
// =============================================================================
static FastAccelStepperEngine engine;          ///< owns the I2S mux (all 32 slots)
static FastAccelStepper* stepper = nullptr;    ///< created by the "stepper" command
static bool i2s_ok = false;                    ///< initI2sMux() succeeded in setup()
static LGFX lcd;                               ///< display, initialised on first "disp"
static bool lcd_ok = false;
static WebServer* web = nullptr;               ///< created by "wifi"

static char line[96];                          ///< serial input line buffer
static size_t line_len = 0;

static int8_t motion_dir = 0;                  ///< current motion: +1 forward, -1 backward, 0 none
static const float MOVE_MAX_MM = 250.0f;       ///< switch to switch 246.3 mm (measured 0a); longer = typo
static int estop_active_level = LOW;           ///< E-stop level that means "pressed" (cmd "estop")

// =============================================================================
// Basic I/O helpers
// =============================================================================

/// Forward (far end) limit switch pressed? Level from config.h (verified 0a: LOW).
static bool rawLimitFwd() { return digitalRead(PIN_LIMIT_FWD) == LIMIT_ACTIVE_LEVEL; }

/// Backward (home / reference) limit switch pressed?
static bool rawLimitBwd() { return digitalRead(PIN_LIMIT_BWD) == LIMIT_ACTIVE_LEVEL; }

/// E-stop pressed? Default LOW = simple normally-open switch (estop_nc = false).
static bool rawEstop() { return digitalRead(PIN_ESTOP) == estop_active_level; }

/// One filtered sensor reading in millivolt (ADC_SAMPLES samples, ADC_DISCARD
/// discarded at each end, see config.h).
static float readMv() {
    uint16_t s[ADC_SAMPLES];
    for (int i = 0; i < ADC_SAMPLES; ++i) s[i] = (uint16_t)analogReadMilliVolts(PIN_SENSOR_ADC);
    return medianMean(s, ADC_SAMPLES, ADC_DISCARD);
}

/// Sets one bit (slot 0..31) of the I2S shift register. No effect without I2S mux.
static void setSlot(int slot, bool v) { if (i2s_ok) engine.i2sMuxSetBit((uint8_t)slot, v); }

/// Driver enable is ACTIVE LOW: HIGH on the enable slot = driver off, motor free.
static void driverOff() { setSlot(I2SO_TO_SLOT(I2SO_SHARED_EN), true); }

/// "true"/"false" for JSON output.
static const char* tf(bool b) { return b ? "true" : "false"; }

/// Parses an integer argument. Empty -> def (if allowed); otherwise the whole
/// string must be a number within [lo, hi]. Returns false on any error.
static bool argInt(const char* s, long lo, long hi, long def, bool optional, long& out) {
    if (!s[0]) { if (!optional) return false; out = def; return true; }
    char* end = nullptr;
    long v = strtol(s, &end, 10);
    if (end == s || *end != 0 || v < lo || v > hi) return false;
    out = v;
    return true;
}

/// Parses a float argument (required). The whole string must be a number.
static bool argFloat(const char* s, float& out) {
    if (!s[0]) return false;
    char* end = nullptr;
    float v = strtof(s, &end);
    if (end == s || *end != 0 || !isfinite(v)) return false;
    out = v;
    return true;
}

/// Prints {"error":"<msg>"}.
static void error(const char* msg) { Serial.printf("{\"error\":\"%s\"}\n", msg); }

// =============================================================================
// Motion safety
// =============================================================================

/**
 * Race-free immediate stop. Returns true if the stepper stands.
 *
 * forceStop() alone is NOT enough: if it comes before the stepper task has
 * picked up a pending move, FastAccelStepper discards the stop in favour of
 * the newer move (incident 11.10.2026). forceStopAndNewPosition() stops the
 * ramp generator and empties the queue synchronously. It is repeated for up to
 * 50 ms until isRunning() is false; if the stepper still runs, the driver
 * outputs are disabled as last resort.
 */
static bool stopNow() {
    if (!stepper) return true;
    uint32_t t0 = millis();
    do {
        stepper->forceStopAndNewPosition(stepper->getCurrentPosition());
        if (!stepper->isRunning()) break;
        delay(1);
    } while (millis() - t0 < 50);
    bool stopped = !stepper->isRunning();
    if (!stopped) {
        stepper->disableOutputs();
        driverOff();
        error("stop failed - driver disabled");
    }
    motion_dir = 0;
    return stopped;
}

/**
 * Reason to abort the running motion, or nullptr if it may continue.
 *  - the E-stop always counts;
 *  - only the limit switch in the direction of travel counts, so a move AWAY
 *    from a pressed switch is possible (e.g. forward from the home switch);
 *  - homing: the backward switch is the goal ("home_switch"), not an error;
 *  - unknown direction (should not happen): any limit switch counts.
 */
static const char* abortReason(bool homing) {
    if (rawEstop()) return "estop";
    if (motion_dir > 0 && rawLimitFwd()) return "limit_fwd";
    if (motion_dir < 0 && rawLimitBwd()) return homing ? "home_switch" : "limit_bwd";
    if (motion_dir == 0 && (rawLimitFwd() || rawLimitBwd())) return "limit_unknown_dir";
    return nullptr;
}

/**
 * Blocks while the stepper runs and checks abortReason() every millisecond.
 * Ends with one JSON line: reason (arrived | estop | limit_fwd | limit_bwd |
 * home_switch | timeout), position in steps and mm, duration in ms.
 * Never returns with a running axis.
 * @param timeout_ms  stop with reason "timeout" after this time
 * @param homing      true: the backward switch ends the motion and sets position 0
 */
static void waitMotion(uint32_t timeout_ms, bool homing) {
    uint32_t t0 = millis();
    const char* reason = "arrived";
    while (stepper && stepper->isRunning()) {
        const char* r = abortReason(homing);
        if (r) {
            if (homing && r[0] == 'h') { stepper->forceStopAndNewPosition(0); motion_dir = 0; }   // home_switch
            else stopNow();
            reason = r;
            break;
        }
        if (millis() - t0 > timeout_ms) { stopNow(); reason = "timeout"; break; }
        delay(1);
    }
    if (stepper && stepper->isRunning()) stopNow();   // safety net
    motion_dir = 0;
    Serial.printf("{\"done\":true,\"reason\":\"%s\",\"pos_steps\":%ld,\"pos_mm\":%.2f,\"ms\":%lu}\n",
                  reason, (long)stepper->getCurrentPosition(),
                  stepper->getCurrentPosition() / STEPS_PER_MM, (unsigned long)(millis() - t0));
}

/**
 * Supervisor, called from loop() on every pass (every ~2 ms while idle).
 * A running stepper outside waitMotion() is an error in itself
 * ("unsupervised_motion"); E-stop and limit switches are checked as well.
 * Note: blocking commands (watch, adc, scan, beep, touch, sta) do not call it,
 * but no motion can run during them – waitMotion() never returns while moving.
 */
static void superviseMotion() {
    if (!stepper || !stepper->isRunning()) return;
    const char* r = abortReason(false);
    if (!r && motion_dir == 0) r = "unsupervised_motion";
    if (r) {
        stopNow();
        Serial.printf("{\"supervisor\":\"%s\",\"pos_steps\":%ld}\n", r, (long)stepper->getCurrentPosition());
    }
}

// =============================================================================
// Commands – section 1: toolchain, memory
// =============================================================================

/// "help": short list of all commands (details: comment at the top of this file).
static void cmdHelp() {
    Serial.println("# ver | crc | part | nvs | fs | uptime | in | watch [s] | estop <low|high> | adc [n]");
    Serial.println("# scan [ms] | slot <n> <0|1> | beep [ms] | hv <0|1> | shutter <0|1>");
    Serial.println("# stepper | move <mm> <mm/s> | home | pos | stop");
    Serial.println("# disp | touch [s] | wifi | sta <ssid> [pass] | help");
    Serial.println("{\"done\":true}");
}

/// "ver": versions and key figures (protocol 1.3). i2s_mux must be true.
static void cmdVer() {
    Serial.printf("{\"fw\":\"smoketest-0a.3\",\"arduino\":\"%d.%d.%d\",\"idf\":\"%s\",\"chip_rev\":%d,"
                  "\"cpu_mhz\":%lu,\"flash_mb\":%lu,\"heap\":%lu,\"i2s_mux\":%s,\"steps_per_mm\":%.3f}\n",
                  ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH,
                  esp_get_idf_version(), ESP.getChipRevision(), (unsigned long)ESP.getCpuFreqMHz(),
                  (unsigned long)(ESP.getFlashChipSize() / (1024 * 1024)), (unsigned long)ESP.getFreeHeap(),
                  tf(i2s_ok), STEPS_PER_MM);
}

/// "crc": own CRC32 vs. ROM function for the standard check string (protocol 1.4).
static void cmdCrc() {
    const char* v = "123456789";
    uint32_t own = crc32Ieee((const uint8_t*)v, 9);
    uint32_t rom = esp_rom_crc32_le(0, (const uint8_t*)v, 9);
    Serial.printf("{\"own\":\"%08lX\",\"esp_rom_crc32_le\":\"%08lX\",\"expected\":\"CBF43926\",\"match\":%s}\n",
                  (unsigned long)own, (unsigned long)rom, tf(own == 0xCBF43926u && rom == own));
}

/// "part": lists all partitions of partitions.csv as found in flash (protocol 1.5).
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

/// "nvs": writes a float and a string, reads them back, clears the namespace
/// (protocol 1.6). The real parameter namespace is not touched.
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

/// "fs": mounts SPIFFS; formats it if mounting fails (first call after flashing
/// takes 10..30 s and logs "mount failed, -10025" once – normal). Protocol 1.6.
static void cmdFs() {
    bool ok = SPIFFS.begin(true);
    Serial.printf("{\"spiffs\":%s,\"total\":%u,\"used\":%u}\n", tf(ok), ok ? SPIFFS.totalBytes() : 0,
                  ok ? SPIFFS.usedBytes() : 0);
}

/// "uptime": milliseconds since boot.
static void cmdUptime() {
    Serial.printf("{\"uptime_ms\":%lu}\n", (unsigned long)millis());
}

// =============================================================================
// Commands – section 2: inputs
// =============================================================================

/// "in": one line with logical state (true = pressed) and raw pin level of
/// both limit switches and the E-stop.
static void printInputs() {
    Serial.printf("{\"limit_fwd\":%s,\"limit_bwd\":%s,\"estop\":%s,\"lvl_fwd\":%d,\"lvl_bwd\":%d,\"lvl_estop\":%d}\n",
                  tf(rawLimitFwd()), tf(rawLimitBwd()), tf(rawEstop()), digitalRead(PIN_LIMIT_FWD),
                  digitalRead(PIN_LIMIT_BWD), digitalRead(PIN_ESTOP));
}

/// "watch [s]": printInputs() every 200 ms for s seconds (protocol 2.1..2.5).
static void cmdWatch(const char* a1) {
    long sec;
    if (!argInt(a1, 1, 120, 10, true, sec)) { error("usage: watch [1..120]"); return; }
    uint32_t end = millis() + (uint32_t)sec * 1000UL;
    while ((int32_t)(end - millis()) > 0) { printInputs(); delay(200); }
    Serial.println("{\"done\":true}");
}

/// "estop <low|high>": active level of the E-stop input for this session.
/// Persistently this is the REST parameter estop_nc of the real firmware.
static void cmdEstop(const char* a1) {
    String v(a1);
    if (v != "low" && v != "high") { error("usage: estop <low|high>"); return; }
    estop_active_level = (v == "high") ? HIGH : LOW;
    Serial.printf("{\"estop_active\":\"%s\"}\n", estop_active_level == HIGH ? "high" : "low");
}

// =============================================================================
// Commands – section 5: distance sensor
// =============================================================================

/// "adc [n]": n filtered readings every 100 ms with mV, mm and range flag
/// (protocol 5.1..5.4).
static void cmdAdc(const char* a1) {
    long n;
    if (!argInt(a1, 1, 200, 10, true, n)) { error("usage: adc [1..200]"); return; }
    for (long i = 0; i < n; ++i) {
        float mv = readMv();
        float mm = sensorMvToMm(mv);
        Serial.printf("{\"mv\":%.0f,\"mm\":%.1f,\"in_range\":%s}\n", mv, mm, tf(sensorInRange(mm)));
        delay(100);
    }
    Serial.println("{\"done\":true}");
}

// =============================================================================
// Commands – section 3: I2S shift register, outputs
// =============================================================================

/**
 * "scan [ms]": switches every slot 0..31 on for ms, then off for ms/2, and
 * prints the slot number before switching it (protocol 3.1). Observe:
 * buzzer (i2so.7), driver enable (i2so.0: active LOW, so the motor gets
 * holding torque when this slot goes OFF again), step (i2so.1, one microstep).
 * Afterwards all slots are off and the driver is disabled (as after setup()).
 * Refused while the stepper is attached and running.
 */
static void cmdScan(const char* a1) {
    if (!i2s_ok) { error("i2s mux not initialised"); return; }
    if (stepper && stepper->isRunning()) { error("stepper running"); return; }
    long ms;
    if (!argInt(a1, 100, 5000, 800, true, ms)) { error("usage: scan [100..5000]"); return; }
    Serial.println("# Watch buzzer (expected i2so.7), motor enable (i2so.0, holding torque), step (i2so.1), dir (i2so.2). HV LED is GPIO 4 (cmd hv).");
    for (int s = 0; s < 32; ++s) {
        Serial.printf("{\"slot\":%d,\"state\":1}\n", s);
        setSlot(s, true);
        delay(ms);
        setSlot(s, false);
        delay(ms / 2);
    }
    for (int s = 0; s < 32; ++s) setSlot(s, false);
    driverOff();
    Serial.println("{\"done\":true,\"driver\":\"off\"}");
}

/**
 * "slot <n> <0|1>": sets one shift register slot directly (protocol 3.x,
 * manual checks). Both arguments are required. Slot 0 is the driver enable,
 * ACTIVE LOW: "slot 0 0" powers the driver, "slot 0 1" switches it off.
 */
static void cmdSlot(const char* a1, const char* a2) {
    long n, v;
    if (!argInt(a1, 0, 31, 0, false, n) || !argInt(a2, 0, 1, 0, false, v)) { error("usage: slot <0..31> <0|1>"); return; }
    if (stepper && stepper->isRunning()) { error("stepper running"); return; }
    setSlot((int)n, v != 0);
    Serial.printf("{\"slot\":%ld,\"state\":%ld}\n", n, v);
}

/// "beep [ms]": buzzer (I2S slot i2so.7) on for ms (protocol 3.3).
static void cmdBeep(const char* a1) {
    long ms;
    if (!argInt(a1, 1, 5000, 200, true, ms)) { error("usage: beep [1..5000]"); return; }
    setSlot(I2SO_TO_SLOT(I2SO_BUZZER), true);
    delay(ms);
    setSlot(I2SO_TO_SLOT(I2SO_BUZZER), false);
    Serial.println("{\"done\":true}");
}

/// "hv <0|1>": green HV LED on GPIO 4 (protocol 3.4).
static void cmdHv(const char* a1) {
    long v;
    if (!argInt(a1, 0, 1, 0, false, v)) { error("usage: hv <0|1>"); return; }
    digitalWrite(PIN_HV_LED, v ? HIGH : LOW);
    Serial.printf("{\"hv\":%ld}\n", v);
}

/// "shutter <0|1>": red shutter LED on GPIO 32 (protocol 3.4).
static void cmdShutter(const char* a1) {
    long v;
    if (!argInt(a1, 0, 1, 0, false, v)) { error("usage: shutter <0|1>"); return; }
    digitalWrite(PIN_SHUTTER, v ? HIGH : LOW);
    Serial.printf("{\"shutter\":%ld}\n", v);
}

// =============================================================================
// Commands – section 4: drive
// =============================================================================

/**
 * "stepper": connects FastAccelStepper to the I2S mux slots from config.h
 * (step, dir, enable active LOW) with auto-enable (driver powered only while
 * moving) and the acceleration ACCEL_MM_S2. Idempotent. Protocol 4.1.
 */
static void cmdStepper() {
    if (!i2s_ok) { error("i2s mux not initialised"); return; }
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

/**
 * "move <mm> <mm/s>": RELATIVE move (protocol 4.2..4.7). Both arguments are
 * required; mm in -250..250 without 0, speed 1..20 mm/s (V1.3). Refused if the
 * E-stop is pressed or the limit switch in the direction of travel is already
 * active. Blocks until the end of the move (waitMotion, timeout 120 s).
 */
static void cmdMove(const char* a1, const char* a2) {
    if (!stepper) { error("run 'stepper' first"); return; }
    float mm, speed;
    if (!argFloat(a1, mm) || !argFloat(a2, speed)) { error("usage: move <mm> <mm/s>"); return; }
    if (!(fabsf(mm) >= 0.01f && fabsf(mm) <= MOVE_MAX_MM)) {
        Serial.printf("{\"error\":\"mm out of range (0.01..%.0f)\"}\n", MOVE_MAX_MM); return;
    }
    if (!(speed >= 1.0f && speed <= 20.0f)) { error("mm/s out of range (1..20)"); return; }
    if (rawEstop()) { error("estop active"); return; }
    motion_dir = mm > 0 ? 1 : -1;
    if ((motion_dir > 0 && rawLimitFwd()) || (motion_dir < 0 && rawLimitBwd())) {
        motion_dir = 0; error("limit switch active in this direction"); return;
    }
    stepper->setSpeedInHz((uint32_t)(speed * STEPS_PER_MM));
    stepper->move((int32_t)lroundf(mm * STEPS_PER_MM));
    Serial.printf("# move %.2f mm at %.1f mm/s - measure the real distance!\n", mm, speed);
    waitMotion(120000, false);
}

/**
 * "home": runs backward at 5 mm/s (V1.3 homing speed) until the backward
 * switch and sets position 0 THERE (the real firmware pulls off 2 mm and sets
 * 0 there). If the switch is already pressed: position 0 at once.
 * Timeout 70 s (= 350 mm at 5 mm/s). Protocol 4.6.
 */
static void cmdHome() {
    if (!stepper) { error("run 'stepper' first"); return; }
    if (rawEstop()) { error("estop active"); return; }
    if (rawLimitBwd()) { stepper->forceStopAndNewPosition(0); Serial.println("{\"done\":true,\"reason\":\"already_on_switch\"}"); return; }
    stepper->setSpeedInHz((uint32_t)(5.0f * STEPS_PER_MM));
    motion_dir = -1;
    stepper->runBackward();
    Serial.println("# seeking backward switch at 5 mm/s (timeout 70 s)");
    waitMotion(70000, true);
}

/// "pos": current position (0 = home switch after "home").
static void cmdPos() {
    long p = stepper ? (long)stepper->getCurrentPosition() : 0L;
    Serial.printf("{\"pos_steps\":%ld,\"pos_mm\":%.2f}\n", p, p / STEPS_PER_MM);
}

// =============================================================================
// Commands – section 6: display, touch
// =============================================================================

/// "disp": initialises the display (LovyanGFX, lgfx_config.h) and draws a test
/// pattern: red/green/blue bars, yellow frame, four cyan touch targets.
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
    for (int i = 0; i < 4; ++i) {   // corner targets for the touch check
        int x = (i % 2) ? 460 : 20, y = (i / 2) ? 300 : 60;
        lcd.drawCircle(x, y, 10, TFT_CYAN);
    }
    Serial.printf("{\"display\":true,\"w\":%d,\"h\":%d}\n", lcd.width(), lcd.height());
}

/// "touch [s]": for s seconds every 100 ms the raw and the mapped touch point;
/// a magenta dot marks the mapped point (calibration check).
static void cmdTouch(const char* a1) {
    long sec;
    if (!argInt(a1, 1, 120, 15, true, sec)) { error("usage: touch [1..120]"); return; }
    if (!lcd_ok) cmdDisp();
    if (!lcd_ok) return;
    Serial.println("# touch the four cyan circles; compare raw vs mapped coordinates");
    uint32_t end = millis() + (uint32_t)sec * 1000UL;
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

// =============================================================================
// Commands – section 7: WiFi
// =============================================================================

static volatile int sta_reason = 0;   ///< last STA disconnect reason (wifi_err_reason_t)

/// "wifi": starts AP+STA mode (V1.3: the AP is always on), the access point
/// from config.h and a web server with GET /ping (JSON: idf, uptime).
static void cmdWifi() {
    if (!web) {
        WiFi.persistent(false);     // never store router credentials in NVS
        WiFi.setAutoReconnect(false);   // a failed "sta" must not keep retrying in the background
        WiFi.onEvent([](arduino_event_id_t, arduino_event_info_t info) {
            sta_reason = info.wifi_sta_disconnected.reason;
        }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        WiFi.mode(WIFI_AP_STA);
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

/// "sta <ssid> [pass]": additionally connects to a router (starts "wifi" first
/// if needed) and waits up to 10 s. Max. 23 chars each, no blanks; empty
/// password = open network. The AP stays on. A running or failed earlier attempt
/// is cancelled first (otherwise ESP_ERR_WIFI_STATE "sta is connecting").
/// On failure "reason" is the last disconnect reason of the IDF, e.g.
/// 15 = 4-way handshake timeout (wrong password), 201 = no AP found
/// (wrong SSID or 5 GHz only), 202 = auth fail, 204 = handshake timeout.
/// The ESP32 only supports 2.4 GHz.
static void cmdSta(const char* ssid, const char* pass) {
    if (!ssid[0]) { error("usage: sta <ssid> [pass]"); return; }
    if (!web) cmdWifi();
    WiFi.disconnect(false, true);   // stop auto-reconnect of an earlier attempt, keep the AP
    delay(200);
    sta_reason = 0;
    WiFi.begin(ssid, pass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(100);
    bool ok = WiFi.status() == WL_CONNECTED;
    Serial.printf("{\"sta\":%s,\"ip\":\"%s\",\"rssi\":%d,\"ms\":%lu,\"ap_ip\":\"%s\",\"reason\":%d}\n",
                  tf(ok), ok ? WiFi.localIP().toString().c_str() : "", ok ? WiFi.RSSI() : 0,
                  (unsigned long)(millis() - t0), WiFi.softAPIP().toString().c_str(), ok ? 0 : (int)sta_reason);
    if (!ok) WiFi.disconnect(false, true);   // no endless retries in the background
}

// =============================================================================
// Command dispatcher, setup, loop
// =============================================================================

/// Command table: name, maximum number of arguments, usage text.
/// The dispatcher rejects surplus arguments with this usage text, so a typo
/// like "home 20 10" or "move 20 10 5" never runs silently.
struct CmdInfo { const char* name; uint8_t max_args; const char* usage; };
static const CmdInfo CMDS[] = {
    {"help", 0, "help"},          {"ver", 0, "ver"},             {"crc", 0, "crc"},
    {"part", 0, "part"},          {"nvs", 0, "nvs"},             {"fs", 0, "fs"},
    {"uptime", 0, "uptime"},
    {"in", 0, "in"},              {"watch", 1, "watch [1..120]"}, {"estop", 1, "estop <low|high>"},
    {"adc", 1, "adc [1..200]"},   {"scan", 1, "scan [100..5000]"}, {"slot", 2, "slot <0..31> <0|1>"},
    {"beep", 1, "beep [1..5000]"}, {"hv", 1, "hv <0|1>"},        {"shutter", 1, "shutter <0|1>"},
    {"stepper", 0, "stepper"},    {"move", 2, "move <mm> <mm/s>"}, {"home", 0, "home"},
    {"pos", 0, "pos"},            {"disp", 0, "disp"},           {"touch", 1, "touch [1..120]"},
    {"wifi", 0, "wifi"},          {"sta", 2, "sta <ssid> [pass]"},
};
static const size_t ARG_MAX_LEN = 23;   ///< longest accepted argument (e.g. SSID)

/**
 * Splits one input line into words (blank or tab separated) and calls the
 * command function.
 *  - empty line                    -> help
 *  - "stop" (with anything after)  -> ALWAYS executed: stopping is never refused
 *  - unknown command               -> error with a hint to "help"
 *  - more arguments than allowed   -> {"error":"usage: ..."}, nothing executed
 *  - argument longer than 23 chars -> {"error":"usage: ..."}, nothing executed
 * Missing or invalid VALUES are checked by each command function itself and
 * also answered with "usage: ...".
 */
static void dispatch(char* l) {
    char* tok[4] = {nullptr, nullptr, nullptr, nullptr};
    int n = 0;
    for (char* t = strtok(l, " \t"); t; t = strtok(nullptr, " \t")) {
        if (n < 4) tok[n] = t;
        ++n;
    }
    if (n == 0) { cmdHelp(); return; }
    const char* c = tok[0];

    if (strcmp(c, "stop") == 0) {   // safety: never rejected, arguments ignored
        bool ok = stopNow();
        Serial.printf("{\"done\":true,\"stopped\":%s}\n", tf(ok));
        return;
    }

    const CmdInfo* ci = nullptr;
    for (const CmdInfo& e : CMDS) if (strcmp(c, e.name) == 0) { ci = &e; break; }
    if (!ci) { Serial.printf("{\"error\":\"unknown command '%.15s' - type 'help'\"}\n", c); return; }

    int nargs = n - 1;
    bool too_long = false;
    for (int i = 1; i < n && i < 4; ++i) if (strlen(tok[i]) > ARG_MAX_LEN) too_long = true;
    if (nargs > ci->max_args || too_long) { Serial.printf("{\"error\":\"usage: %s\"}\n", ci->usage); return; }

    const char* a1 = nargs >= 1 ? tok[1] : "";
    const char* a2 = nargs >= 2 ? tok[2] : "";

    if      (!strcmp(c, "help"))    cmdHelp();
    else if (!strcmp(c, "ver"))     cmdVer();
    else if (!strcmp(c, "crc"))     cmdCrc();
    else if (!strcmp(c, "part"))    cmdPart();
    else if (!strcmp(c, "nvs"))     cmdNvs();
    else if (!strcmp(c, "fs"))      cmdFs();
    else if (!strcmp(c, "uptime"))   cmdUptime();
    else if (!strcmp(c, "in"))      printInputs();
    else if (!strcmp(c, "watch"))   cmdWatch(a1);
    else if (!strcmp(c, "estop"))   cmdEstop(a1);
    else if (!strcmp(c, "adc"))     cmdAdc(a1);
    else if (!strcmp(c, "scan"))    cmdScan(a1);
    else if (!strcmp(c, "slot"))    cmdSlot(a1, a2);
    else if (!strcmp(c, "beep"))    cmdBeep(a1);
    else if (!strcmp(c, "hv"))      cmdHv(a1);
    else if (!strcmp(c, "shutter")) cmdShutter(a1);
    else if (!strcmp(c, "stepper")) cmdStepper();
    else if (!strcmp(c, "move"))    cmdMove(a1, a2);
    else if (!strcmp(c, "home"))    cmdHome();
    else if (!strcmp(c, "pos"))     cmdPos();
    else if (!strcmp(c, "disp"))    cmdDisp();
    else if (!strcmp(c, "touch"))   cmdTouch(a1);
    else if (!strcmp(c, "wifi"))    cmdWifi();
    else if (!strcmp(c, "sta"))     cmdSta(a1, a2);
}

/**
 * Safe start state: inputs configured, shutter and HV LED off, heartbeat LED
 * off, ADC 0..3.1 V range. The I2S mux is started, all 32 slots cleared and
 * the driver disabled (enable HIGH). Between reset and this point the shift
 * register is undefined – the motor may twitch briefly (known, protocol 1.3).
 */
void setup() {
    Serial.begin(115200);
    delay(300);
    pinMode(PIN_LIMIT_FWD, INPUT);
    pinMode(PIN_LIMIT_BWD, INPUT);
    pinMode(PIN_ESTOP, INPUT);
    pinMode(PIN_SHUTTER, OUTPUT);
    digitalWrite(PIN_SHUTTER, LOW);
    pinMode(PIN_HV_LED, OUTPUT);
    digitalWrite(PIN_HV_LED, LOW);
    pinMode(PIN_HEARTBEAT_LED, OUTPUT);
    digitalWrite(PIN_HEARTBEAT_LED, LOW);
    // Arduino 3.x: the pin must be registered as an ADC channel (first read)
    // before its attenuation can be set, otherwise "__analogChannelConfig():
    // Pin is not configured as analog channel" (0a.2 boot log). 11 dB is also
    // the default, so the measurements of 0a.1/0a.2 are valid.
    (void)analogReadMilliVolts(PIN_SENSOR_ADC);
    analogSetPinAttenuation(PIN_SENSOR_ADC, ADC_11db);

    engine.init(STEPPER_TASK_CORE);
    i2s_ok = engine.initI2sMux(PIN_I2S_DATA, PIN_I2S_BCK, PIN_I2S_WS);
    if (i2s_ok) for (int s = 0; s < 32; ++s) engine.i2sMuxSetBit(s, false);
    if (i2s_ok) driverOff();

    Serial.println("# ADD Demo System - Sprint 0a smoketest. Type 'help'.");
    cmdVer();
}

/**
 * Main loop (Arduino loop task): collects serial characters into a line and
 * dispatches it on '\n', supervises a running motion, serves the web server
 * and blinks the heartbeat LED (GPIO 2, 1 s on / 1 s off). Period ~2 ms.
 */
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
    superviseMotion();
    if (web) web->handleClient();
    digitalWrite(PIN_HEARTBEAT_LED, ((millis() / 1000) % 2) == 0 ? HIGH : LOW);
    delay(2);
}
