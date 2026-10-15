/**
 * @file hal_io.cpp
 * @brief Implementation of src/hal/hal_io.h (Sprint H1).
 */

#include <Arduino.h>
#include <FastAccelStepper.h>
#include "config.h"
#include "hal/motion_drv.h"
#include "hal/hal_io.h"

namespace {

// ---------------------------------------------------------------------------
// Debounce helper – the spec (V1.5.18 header) defines this precisely:
//   sample == state -> pending = false
//   sample != state: set t0 at the first differing sample; once
//     (now - t0) >= INPUT_DEBOUNCE_MS, flip state and clear pending
// ---------------------------------------------------------------------------
struct Deb {
    bool state  = true;   // fail-safe: active until first real sample
    bool pending = false;
    uint32_t t0      = 0;
};

static void debounce(Deb& d, bool sample, uint32_t now) {
    if (sample == d.state) {
        d.pending = false;
    } else {
        if (!d.pending) {
            d.t0  = now;
            d.pending = true;
        }
        if ((now - d.t0) >= INPUT_DEBOUNCE_MS) {
            d.state = sample;
            d.pending = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Output helper – write only when the value differs from cache (cache -1 = unknown)
// ---------------------------------------------------------------------------
static void out(uint8_t pin, bool on, int8_t& cache) {
    uint8_t level = on ? HIGH : LOW;
    if ((int8_t)level != cache) {
        digitalWrite(pin, level);
        cache = (int8_t)level;
    }
}

// ---------------------------------------------------------------------------
// File-static state
// ---------------------------------------------------------------------------
Deb for_fwd, bwd, estop;
int8_t  shutter_cache   = -1;
int8_t  hv_cache        = -1;
int8_t  heartbeat_cache = -1;
int8_t  buzzer_cache    = -1;

FastAccelStepperEngine* engine = nullptr;
bool sampled = false;

bool active_high   = false;   // default: LOW = active (estop_nc = false)
bool used_high     = false;   // polarity of the previous halUpdateInputs decode

}  // namespace

// ---------------------------------------------------------------------------
// halInit – V1.5.18 step-by-step (in exact order)
// ---------------------------------------------------------------------------
void halInit() {
    // Step 1: outputs LOW (PIN_SHUTTER, PIN_HV_LED, PIN_HEARTBEAT_LED in this order)
    pinMode(PIN_SHUTTER, OUTPUT);
    digitalWrite(PIN_SHUTTER, LOW);
    pinMode(PIN_HV_LED, OUTPUT);
    digitalWrite(PIN_HV_LED, LOW);
    pinMode(PIN_HEARTBEAT_LED, OUTPUT);
    digitalWrite(PIN_HEARTBEAT_LED, LOW);

    // Step 2: inputs (external pull-ups)
    pinMode(PIN_LIMIT_FWD, INPUT);
    pinMode(PIN_LIMIT_BWD, INPUT);
    pinMode(PIN_ESTOP, INPUT);

    // Step 3: ADC read then attenuation
    (void)analogReadMilliVolts(PIN_SENSOR_ADC);
    analogSetPinAttenuation(PIN_SENSOR_ADC, ADC_11db);

    // Step 4: engine + buzzer off
    engine = motionDrvEngine();
    if (engine) {
        engine->i2sMuxSetBit(I2SO_TO_SLOT(I2SO_BUZZER), false);
    }

    // Step 5: caches unknown, inputs fail-safe (TRUE until first update)
    shutter_cache   = -1;
    hv_cache        = -1;
    heartbeat_cache = -1;
    buzzer_cache    = -1;

    for_fwd.state  = true; for_fwd.pending = false; for_fwd.t0  = 0;
    bwd.state      = true; bwd.pending = false; bwd.t0      = 0;
    estop.state    = true; estop.pending = false; estop.t0   = 0;

    sampled    = false;
}

// ---------------------------------------------------------------------------
// halSetEstopActiveHigh – store polarity only (setter does not decode)
// ---------------------------------------------------------------------------
void halSetEstopActiveHigh(bool on) {
    active_high = on;
}

// ---------------------------------------------------------------------------
// halUpdateInputs – V1.5.18: read once, decode, debounce limits, estop immediate activation
// ---------------------------------------------------------------------------
void halUpdateInputs(uint32_t now_ms) {
    // Read each input exactly once
    int raw_fwd  = digitalRead(PIN_LIMIT_FWD);
    int raw_bwd  = digitalRead(PIN_LIMIT_BWD);
    int raw_estop = digitalRead(PIN_ESTOP);

    // Decode: limit active when level == LIMIT_ACTIVE_LEVEL
    bool dec_fwd  = (raw_fwd  == LIMIT_ACTIVE_LEVEL);
    bool dec_bwd  = (raw_bwd  == LIMIT_ACTIVE_LEVEL);

    // E-stop active depends on polarity
    bool dec_estop = active_high ? (raw_estop == HIGH) : (raw_estop == LOW);

    // First call after halInit: take samples directly (no debounce)
    if (!sampled) {
        for_fwd.state  = dec_fwd;
        bwd.state      = dec_bwd;
        estop.state    = dec_estop;
        sampled = true;
        used_high = active_high;
        return;
    }

    // Limit switches: debounced both directions
    debounce(for_fwd, dec_fwd, now_ms);
    debounce(bwd,     dec_bwd, now_ms);

    // E-stop: active sample -> immediate activation + cancel pending release
    //         inactive sample -> debounce release
    if (dec_estop) {
        estop.state = true;
        estop.pending = false;
    } else {
        // Inactive sample under current polarity
        bool polarity_changed = (active_high != used_high);
        if (polarity_changed) {
            // Cancel pending release and restart from now
            estop.pending = false;
        }
        debounce(estop, dec_estop, now_ms);
        used_high = active_high;
    }
}

bool halEstop()      { return estop.state; }
bool halLimitFwd()   { return for_fwd.state; }
bool halLimitBwd()   { return bwd.state; }

// ---------------------------------------------------------------------------
// halReadSensorSamples
// ---------------------------------------------------------------------------
void halReadSensorSamples(uint16_t* buf, uint8_t n) {
    if (!buf || n == 0) return;
    for (uint8_t i = 0; i < n; ++i) {
        uint32_t mv = analogReadMilliVolts(PIN_SENSOR_ADC);
        buf[i] = (mv > 65535u) ? 65535 : (uint16_t)mv;
    }
}

// ---------------------------------------------------------------------------
// Output setters – write only on change
// ---------------------------------------------------------------------------
void halSetShutter(bool open) {
    out(PIN_SHUTTER, open, shutter_cache);
}

void halSetHvLed(bool on) {
    out(PIN_HV_LED, on, hv_cache);
}

void halSetBuzzer(bool on) {
    if (!engine) return;  // no engine -> nothing written, cache stays unknown
    uint8_t slot = I2SO_TO_SLOT(I2SO_BUZZER);
    if ((int8_t)on != buzzer_cache) {
        engine->i2sMuxSetBit(slot, on);
        buzzer_cache = (int8_t)on;
    }
}

void halSetHeartbeat(bool on) {
    out(PIN_HEARTBEAT_LED, on, heartbeat_cache);
}
