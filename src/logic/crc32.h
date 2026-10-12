/**
 * @file crc32.h
 * @brief CRC32 (IEEE 802.3 / zlib) and the JSON CRC frame (FROZEN CONTRACT V1.5.2 §4).
 *
 * Pure C++, no ESP-IDF dependency. The firmware uses this implementation on the
 * target as well, so host tests and target behave identically.
 * Implementation: src/logic/crc32.cpp (Sprint S2.1).
 *
 * CRC frame rule (identical for requests, responses and /proc):
 *   1. The sender builds the JSON object WITHOUT the crc field, e.g.
 *        {"cmd":"HV_ON","params":{}}
 *   2. CRC32 is computed over exactly these bytes.
 *   3. The sender inserts  ,"crc":"XXXXXXXX"  (8 uppercase hex digits) directly
 *      before the final '}':
 *        {"cmd":"HV_ON","params":{},"crc":"1A2B3C4D"}
 *   4. The receiver takes the text before the LAST occurrence of ,"crc":"
 *      appends '}', computes CRC32 and compares (hex compare case-insensitive).
 *   No re-serialisation of JSON is ever involved.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * zlib-compatible CRC32. Chaining: crc32Ieee(b, lb, crc32Ieee(a, la)) equals the
 * CRC of the concatenation a+b. Start value for a new CRC: prev = 0.
 * Reference: crc32Ieee("123456789", 9) == 0xCBF43926, crc32Ieee("", 0) == 0.
 */
uint32_t crc32Ieee(const uint8_t* data, size_t len, uint32_t prev = 0);

/// Formats crc as 8 uppercase hex digits plus NUL into out[9].
void crc32Hex(uint32_t crc, char out[9]);

/**
 * Appends the CRC frame to a JSON object held in buf.
 * @param buf  NUL-terminated JSON text of length len, must end with '}'
 *             (no trailing whitespace) and start with '{'.
 * @param len  strlen(buf)
 * @param cap  total size of buf in bytes
 * @return new length (without NUL) or -1 if buf does not end with '}',
 *         the object is empty ("{}"), or cap is too small (needs cap >= len + 17 + 1;
 *         the frame adds exactly 17 characters).
 *         On -1, buf is unchanged.
 */
int crcFrameAppend(char* buf, size_t len, size_t cap);

/**
 * Verifies a framed JSON text.
 * Accepted: optional whitespace (space, \t, \r, \n) after the final '}'.
 * Required: the last occurrence of ,"crc":" is followed by exactly 8 hex digits,
 *           a closing quote and '}' (then only whitespace until len).
 * @return true if the frame is well-formed and the CRC matches.
 */
bool crcFrameVerify(const char* body, size_t len);

/*
 * V1.5.2 implementation constraints (crc32.cpp):
 *   - No internal size limit: crcFrameVerify must work for any len (frames of
 *     several kB, e.g. /proc responses). No fixed-size copy buffer.
 *   - Any len is valid input, including len < 8 (e.g. "x", "{}"): return false,
 *     no out-of-bounds access, no pointer arithmetic before body.
 *   - No static mutable state (no lazily initialised tables, no static flags);
 *     a const table or computing on the fly is fine. Functions are reentrant.
 */
