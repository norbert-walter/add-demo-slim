#include <Preferences.h>
#include <math.h>
#include <string.h>
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
    if (pr.getUChar("ver", 0) != 4) { pr.end(); return; }

    // Float fields via loadNum
    loadNum(p, pr, "mpos",     "measure_pos",     p.measure_pos);
    loadNum(p, pr, "maint",    "maint_pos",       p.maint_pos);
    loadNum(p, pr, "tstart",   "traverse_start",  p.traverse_start);
    loadNum(p, pr, "tend",     "traverse_end",    p.traverse_end);
    loadNum(p, pr, "tspeed",   "traverse_speed",  p.traverse_speed);
    loadNum(p, pr, "shold",    "shutter_hold",    p.shutter_hold);
    loadNum(p, pr, "edge",     "edge_threshold",  p.edge_threshold);
    loadNum(p, pr, "base",     "base_dist",       p.base_dist);

    // Calib raw fields (no SET key) – take if finite
    float cnraw = pr.getFloat("cnraw", NAN);
    if (isfinite(cnraw)) p.calib.near_raw = cnraw;
    float cfraw = pr.getFloat("cfraw", NAN);
    if (isfinite(cfraw)) p.calib.far_raw = cfraw;

    // Calib ref fields via loadNum
    loadNum(p, pr, "cnref",  "calib_near_ref",  p.calib.near_ref);
    loadNum(p, pr, "cfref",  "calib_far_ref",   p.calib.far_ref);

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
    if (!calibValid(p.calib)) p.calib = CalibPoints{};

    pr.end();
}

bool nvsSaveParams(const Params& p) {
    Preferences pr;
    if (!pr.begin(NVS_NAMESPACE, false)) return false;

    bool ok = true;

    // "ver" = 4
    if (pr.putUChar("ver", 4) == 0) ok = false;

    // Float fields
    if (ok && pr.putFloat("mpos",     p.measure_pos)     == 0) ok = false;
    if (ok && pr.putFloat("maint",    p.maint_pos)       == 0) ok = false;
    if (ok && pr.putFloat("tstart",   p.traverse_start)  == 0) ok = false;
    if (ok && pr.putFloat("tend",     p.traverse_end)    == 0) ok = false;
    if (ok && pr.putFloat("tspeed",   p.traverse_speed)  == 0) ok = false;
    if (ok && pr.putFloat("shold",    p.shutter_hold)    == 0) ok = false;
    if (ok && pr.putFloat("edge",     p.edge_threshold)  == 0) ok = false;
    if (ok && pr.putFloat("base",     p.base_dist)       == 0) ok = false;

    // Calib fields
    if (ok && pr.putFloat("cnraw", p.calib.near_raw)  == 0) ok = false;
    if (ok && pr.putFloat("cnref", p.calib.near_ref)  == 0) ok = false;
    if (ok && pr.putFloat("cfraw", p.calib.far_raw)   == 0) ok = false;
    if (ok && pr.putFloat("cfref", p.calib.far_ref)   == 0) ok = false;

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