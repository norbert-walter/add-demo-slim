/**
 * @file crc32.cpp
 * @brief CRC32 (IEEE 802.3 / zlib) and JSON CRC framing (Sprint S2.1).
 */
#include "crc32.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Table-driven CRC32 (polynomial 0xEDB88320, reflected form)
// Init = 0, xorout = 0 — chaining works: crc32Ieee(b,len,crc32Ieee(a,la)) ==
// crc32Ieee(concat(a,b), la+lb).
// ---------------------------------------------------------------------------
static const uint32_t* get_crc_table() {
    static bool initialized = false;
    static uint32_t table[256];
    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        initialized = true;
    }
    return table;
}

uint32_t crc32Ieee(const uint8_t* data, size_t len, uint32_t prev) {
    const uint32_t* table = get_crc_table();
    uint32_t crc = prev ^ 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

void crc32Hex(uint32_t crc, char out[9]) {
    // Format as 8 uppercase hex digits
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) {
        out[i] = hex[crc & 0xFu];
        crc >>= 4;
    }
    out[8] = '\0';
}

// ---------------------------------------------------------------------------
// CRC frame: append ,"crc":"XXXXXXXX" before the final '}'
// ---------------------------------------------------------------------------
int crcFrameAppend(char* buf, size_t len, size_t cap) {
    if (!buf || len == 0) return -1;
    // Must start with '{' and end with '}'
    if (buf[0] != '{' || buf[len - 1] != '}') return -1;
    // Reject empty object "{}"
    if (len == 2 && buf[0] == '{' && buf[1] == '}') return -1;

    // Need cap >= len + 18 (17 new chars + NUL terminator)
    if (cap < len + 18) return -1;

    // Compute CRC over the full JSON object (including final '}')
    uint32_t crc = crc32Ieee((const uint8_t*)buf, len);
    char hex[9];
    crc32Hex(crc, hex);

    // Insert ,"crc":"XXXXXXXX" before the final '}'
    const char* frame = ",\"crc\":\"";
    size_t frame_len = strlen(frame);

    // Save the final '}' and NUL, then shift everything after position len-1
    char final_brace = buf[len - 1];
    memmove(buf + len + frame_len + 9, buf + len, 1); // move NUL to new end
    // Overwrite the original '}' with the frame prefix
    memcpy(buf + len - 1, frame, frame_len);
    // Write the hex digits after the prefix
    memcpy(buf + len - 1 + frame_len, hex, 8);
    // Closing quote for the crc string value
    buf[len - 1 + frame_len + 8] = '"';
    // The original '}' now closes the main object at the new end
    buf[len - 1 + frame_len + 9] = final_brace;

    return (int)(len + frame_len + 9); // new length without NUL
}

// ---------------------------------------------------------------------------
// CRC frame verify: find last ,"crc":" and check
// ---------------------------------------------------------------------------
static bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool crcFrameVerify(const char* body, size_t len) {
    if (!body || len == 0) return false;

    // Find the LAST occurrence of ,"crc":"
    const char* marker = ",\"crc\":\"";
    size_t marker_len = strlen(marker); // 8
    const char* last = nullptr;
    for (const char* p = body; p <= body + len - marker_len; ++p) {
        if (memcmp(p, marker, marker_len) == 0) {
            last = p;
        }
    }
    if (!last) return false;

    // After the marker we need: exactly 8 hex digits + " + } + optional whitespace
    const char* after_marker = last + marker_len;
    size_t remaining = len - (after_marker - body);
    if (remaining < 10) return false; // 8 hex + " + }

    // Parse 8 hex digits
    for (int i = 0; i < 8; ++i) {
        if (!isHexDigit(after_marker[i])) return false;
    }

    // Closing quote
    if (after_marker[8] != '"') return false;
    // Closing brace
    if (after_marker[9] != '}') return false;

    // After '}' only whitespace allowed
    for (size_t i = 10; i < remaining; ++i) {
        char c = after_marker[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return false;
    }

    // The character at 'last' was originally '}' (the JSON closing brace).
    // Temporarily restore it in a copy so CRC matches what append computed.
    size_t overwrite_pos = (size_t)(last - body);
    uint32_t expected_crc = 0;
    {
        char tmp[512];
        if (overwrite_pos + 1 <= sizeof(tmp)) {
            memcpy(tmp, body, overwrite_pos);
            tmp[overwrite_pos] = '}';
            expected_crc = crc32Ieee((const uint8_t*)tmp, overwrite_pos + 1);
        }
    }

    // Parse stored CRC (case-insensitive)
    uint32_t stored_crc = 0;
    for (int i = 0; i < 8; ++i) {
        char c = after_marker[i];
        stored_crc <<= 4;
        if (c >= '0' && c <= '9') stored_crc |= (c - '0');
        else if (c >= 'a' && c <= 'f') stored_crc |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') stored_crc |= (c - 'A' + 10);
    }

    return expected_crc == stored_crc;
}
