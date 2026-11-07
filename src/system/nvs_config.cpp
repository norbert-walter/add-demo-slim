#include <Preferences.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "system/nvs_config.h"

static void loadNum(Params& p, Preferences& pr, const char* nvsKey, const char* setKey, float def) {
    SafetyFlags flags;
    bool persist;
    float v = pr.getFloat(nvsKey, def);
    paramSet(p, flags, setKey, ValueType::NUMBER, v, false, "", &persist);
}

void nvsLoadParams(Params& p) {
    paramsDefaults(p);

    Preferences pr;
    if (!pr.begin(NVS_NAMESPACE, true)) return;
    uint8_t ver = pr.getUChar("ver", 0);
    if (ver != 4 && ver != 5) { pr.end(); return; }

    // Float fields via loadNum (same for ver 4 and ver 5)
    loadNum(p, pr, "mpos",     "measure_pos",     p.measure_pos);
    loadNum(p, pr, "maint",    "maint_pos",       p.maint_pos);
    loadNum(p, pr, "tstart",   "traverse_start",  p.traverse_start);
    loadNum(p, pr, "tend",     "traverse_end",    p.traverse_end);
    loadNum(p, pr, "tspeed",   "traverse_speed",  p.traverse_speed);
    loadNum(p, pr, "shold",    "shutter_hold",    p.shutter_hold);
    loadNum(p, pr, "edge",     "edge_threshold",  p.edge_threshold);
    loadNum(p, pr, "base",     "base_dist",       p.base_dist);

    // Calibration: ver 5 uses craw1..craw6 + cref1..cref6; ver 4 keys are ignored.
    if (ver == 5) {
        // Craw keys (no SET key) – take if finite
        char key[8];
        for (int i = 0; i < 6; ++i) {
            snprintf(key, sizeof(key), "craw%d", i + 1);
            float v = pr.getFloat(key, NAN);
            if (isfinite(v)) p.calib.raw[i] = v;
        }
        // Cref keys via paramSet
        for (int i = 0; i < 6; ++i) {
            snprintf(key, sizeof(key), "cref%d", i + 1);
            SafetyFlags flags;
            bool persist;
            float v = pr.getFloat(key, NAN);
            char refKey[16];
            snprintf(refKey, sizeof(refKey), "calib_ref_%d", i + 1);
            paramSet(p, flags, refKey, ValueType::NUMBER, (double)v, false, "", &persist);
        }
    }

    // estnc
    p.estop_nc = pr.getBool("estnc", false);

    // ssid and pass – only if getString returned > 0
    char buf[64];
    memset(buf, 0, sizeof(buf));
    if (pr.getString("ssid", buf, sizeof(p.wifi_ssid)) > 0) {
        SafetyFlags flags;
        bool persist;
        paramSet(p, flags, "wifi_ssid", ValueType::STRING, 0, false, buf, &persist);
    }
    memset(buf, 0, sizeof(buf));
    if (pr.getString("pass", buf, sizeof(p.wifi_pass)) > 0) {
        SafetyFlags flags;
        bool persist;
        paramSet(p, flags, "wifi_pass", ValueType::STRING, 0, false, buf, &persist);
    }

    // Invalid calibration -> reset whole calib
    if (!calibValid(p.calib)) p.calib = CalibTable{};

    pr.end();
}

bool nvsSaveParams(const Params& p) {
    Preferences pr;
    if (!pr.begin(NVS_NAMESPACE, false)) return false;

    bool ok = true;

    // "ver" = 5
    if (pr.putUChar("ver", 5) == 0) ok = false;

    // Float fields
    if (ok && pr.putFloat("mpos",     p.measure_pos)     == 0) ok = false;
    if (ok && pr.putFloat("maint",    p.maint_pos)       == 0) ok = false;
    if (ok && pr.putFloat("tstart",   p.traverse_start)  == 0) ok = false;
    if (ok && pr.putFloat("tend",     p.traverse_end)    == 0) ok = false;
    if (ok && pr.putFloat("tspeed",   p.traverse_speed)  == 0) ok = false;
    if (ok && pr.putFloat("shold",    p.shutter_hold)    == 0) ok = false;
    if (ok && pr.putFloat("edge",     p.edge_threshold)  == 0) ok = false;
    if (ok && pr.putFloat("base",     p.base_dist)       == 0) ok = false;

    // Calib raw fields (no SET key) – craw1..craw6
    char key[8];
    for (int i = 0; i < 6; ++i) {
        snprintf(key, sizeof(key), "craw%d", i + 1);
        if (ok && pr.putFloat(key, p.calib.raw[i]) == 0) ok = false;
    }

    // Calib ref fields – cref1..cref6
    for (int i = 0; i < 6; ++i) {
        snprintf(key, sizeof(key), "cref%d", i + 1);
        if (ok && pr.putFloat(key, p.calib.ref[i]) == 0) ok = false;
    }

    // estnc
    if (ok && pr.putBool("estnc", p.estop_nc) == 0) ok = false;

    // ssid – empty string exception: putString("") returns 0 but is OK
    if (ok) {
        size_t s = pr.putString("ssid", p.wifi_ssid);
        if (s == 0 && strlen(p.wifi_ssid) > 0) ok = false;
    }

    // pass – same empty-string exception
    if (ok) {
        size_t s = pr.putString("pass", p.wifi_pass);
        if (s == 0 && strlen(p.wifi_pass) > 0) ok = false;
    }

    pr.end();
    return ok;
}

void nvsErase() {
    Preferences pr;
    if (pr.begin(NVS_NAMESPACE, false)) {
        pr.clear();
        pr.end();
    }
}