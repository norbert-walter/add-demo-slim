/**
 * @file api_parse.cpp
 * @brief Parsing of POST /api bodies into a Command (FROZEN CONTRACT V1.5 §4, §5.2).
 */

#include "api_parse.h"
#include "crc32.h"
#include <ArduinoJson.hpp>
#include <cmath>
#include <cstring>

using namespace ArduinoJson;

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/// Extract a numeric parameter from a JsonObject.
/// @param p    JsonObjectConst (the "params" object)
/// @param key  Parameter name (e.g. "pos", "speed")
/// @param v    Output: parsed float value
/// @param has  Output set to true when the value is present and valid
/// @param required  true if missing → return false; false if optional → return true on missing
/// @return false on error (not a number, bool, NaN/Inf after float conversion).
static bool getNum(JsonObjectConst p, const char* key, float& v, bool& has, bool required) {
    JsonVariantConst val = p[key];
    if (val.isNull()) {
        has = false;
        return !required;
    }
    // Must be a number (not a bool): is<float>() returns true for any JSON number
    if (!val.is<float>()) {
        return false;
    }
    float f = val.as<float>();
    if (!std::isfinite(f)) {
        return false;
    }
    v = f;
    has = true;
    return true;
}

/// Copy a null-terminated string into dst (size n), truncating if needed.
static void strCopy(char* dst, const char* src, size_t n) {
    if (n == 0) return;
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}
// ---------------------------------------------------------------------------
// apiParseRequest
// ---------------------------------------------------------------------------

ApiError apiParseRequest(const char* body, size_t len, Command& out) {
    // Reset output completely before any work.
    out = Command{};

    // Check 1: CRC frame verification.
    if (!crcFrameVerify(body, len)) {
        return ApiError::BAD_CRC;
    }

    // Check 2: Parse JSON body as an object.
    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, body, len);
    if (jsonErr || !doc.is<JsonObjectConst>()) {
        return ApiError::BAD_JSON;
    }
    JsonObjectConst root = doc.as<JsonObjectConst>();

    // Check 3: "cmd" must be a known string.
    JsonVariantConst cmdVal = root["cmd"];
    if (cmdVal.isNull() || !cmdVal.is<const char*>()) {
        return ApiError::UNKNOWN_CMD;
    }
    CmdId cmd = cmdFromName(cmdVal.as<const char*>());
    if (cmd == CmdId::NONE) {
        return ApiError::UNKNOWN_CMD;
    }
    out.id = cmd;

    // Check 4: "params" must be absent or an object.
    JsonVariantConst paramsVal = root["params"];
    JsonObjectConst params;
    if (!paramsVal.isNull()) {
        if (!paramsVal.is<JsonObjectConst>()) {
            return ApiError::BAD_PARAM;
        }
        params = paramsVal.as<JsonObjectConst>();
    }

    // Check 5: command-specific parameter validation.
    switch (cmd) {
        case CmdId::START_MEASURE: {
            float v; bool has;
            if (!getNum(params, "pos", v, has, false)) return ApiError::BAD_PARAM;
            out.has_pos = has;
            if (has) out.pos = v;
            break;
        }
        case CmdId::START_TRAVERSE: {
            float v; bool has;
            if (!getNum(params, "start", v, has, false)) return ApiError::BAD_PARAM;
            out.has_start = has;
            if (has) out.start = v;
            if (!getNum(params, "end", v, has, false)) return ApiError::BAD_PARAM;
            out.has_end = has;
            if (has) out.end = v;
            if (!getNum(params, "speed", v, has, false)) return ApiError::BAD_PARAM;
            out.has_speed = has;
            if (has) out.speed = v;
            out.has_speed = has;
            break;
        }
        case CmdId::MOVE: {
            float v; bool has;
            // pos is REQUIRED
            if (!getNum(params, "pos", v, has, true)) return ApiError::BAD_PARAM;
            out.pos = v;
            out.has_pos = has;
            // speed is optional
            if (!getNum(params, "speed", v, has, false)) return ApiError::BAD_PARAM;
            out.has_speed = has;
            if (has) out.speed = v;
            break;
        }
        case CmdId::SET: {
            // key: string, REQUIRED, max 23 chars (sizeof(key) - 1)
            JsonVariantConst keyVal = params["key"];
            if (keyVal.isNull() || !keyVal.is<const char*>()) return ApiError::BAD_PARAM;
            const char* keyStr = keyVal.as<const char*>();
            if (strlen(keyStr) >= sizeof(out.key)) return ApiError::BAD_PARAM; // max 23 chars
            strCopy(out.key, keyStr, sizeof(out.key));

            // value: number | bool | string, REQUIRED
            JsonVariantConst val = params["value"];
            if (val.isNull()) return ApiError::BAD_PARAM;

            if (val.is<bool>()) {
                out.vtype = ValueType::BOOL;
                out.b = val.as<bool>();
            } else if (val.is<float>()) {
                double d = val.as<double>();
                if (!std::isfinite(static_cast<float>(d))) return ApiError::BAD_PARAM;
                out.vtype = ValueType::NUMBER;
                out.num = d;
            } else if (val.is<const char*>()) {
                size_t slen = strlen(val.as<const char*>());
                if (slen >= sizeof(out.str)) return ApiError::BAD_PARAM; // max 63 chars
                strCopy(out.str, val.as<const char*>(), sizeof(out.str));
                out.vtype = ValueType::STRING;
            } else {
                return ApiError::BAD_PARAM;
            }
            break;
        }
        case CmdId::CALIB: {
            // action: string, REQUIRED, one of near|far|base|save|reset
            JsonVariantConst actVal = params["action"];
            if (actVal.isNull() || !actVal.is<const char*>()) return ApiError::BAD_PARAM;
            CalibAction ca = calibActionFromName(actVal.as<const char*>());
            if (ca == CalibAction::NONE) return ApiError::BAD_PARAM;
            out.calib = ca;
            break;
        }
        default:
            // All other commands: params ignored.
            break;
    }

    // Success: set meta fields.
    out.source = CmdSource::API;
    out.want_reply = true;
    out.seq = 0;

    return ApiError::OK;
}
