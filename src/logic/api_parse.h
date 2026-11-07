/**
 * @file api_parse.h
 * @brief Parsing of POST /api bodies into a Command (FROZEN CONTRACT V1.5 §4, §5.2).
 *
 * Implementation: src/logic/api_parse.cpp (Sprint L5). Uses ArduinoJson 7
 * (header-only, also builds natively). No range checks here – those live in
 * params.cpp / motion_seq.cpp. Only syntax and types are checked.
 *
 * Order of checks (first failure wins):
 *   1. crcFrameVerify(body)                       else BAD_CRC
 *   2. JSON object                                 else BAD_JSON
 *   3. "cmd" string known by cmdFromName()         else UNKNOWN_CMD
 *   4. "params": absent or object                  else BAD_PARAM
 *   5. command-specific params:
 *      START_MEASURE  pos   (number, optional)
 *      START_TRAVERSE start, end, speed (numbers, each optional)
 *      MOVE           pos   (number, REQUIRED), speed (number, optional)
 *      SET            key   (string, REQUIRED, max 23 chars),
 *                     value (number | bool | string max 63 chars, REQUIRED)
 *      CALIB          action (string, REQUIRED, point|base|save|reset)  (V1.6.0, near/far -> BAD_PARAM)
 *                     index  (number, REQUIRED for action point: integer value 1..6, 3 and 3.0 are
 *                             accepted; 3.5, 0, 7, a string or a missing index -> BAD_PARAM;
 *                             valid -> out.calib_index = index). Ignored for the other actions
 *                             (calib_index stays 0).
 *      all others     params ignored (incl. PARK, SERVICE – V1.2)
 *      Wrong type, missing required, unknown action, too long -> BAD_PARAM.
 *      V1.4.1: numbers must be finite after conversion to float (1e39 -> +inf)
 *      -> BAD_PARAM. NaN / Infinity literals are not JSON -> BAD_JSON.
 * On success `out` is fully reset first, then filled (id, has_*, values,
 * vtype/num/b/str, calib, calib_index). V1.5.12: a value whose has_* is false stays 0. source = API, want_reply = true, seq = 0 (caller sets).
 * On error `out` may be partially filled; only out.id may be used (it is set as
 * soon as check 3 succeeds, so the error response can name the command).
 */
#pragma once

#include "types.h"

ApiError apiParseRequest(const char* body, size_t len, Command& out);
