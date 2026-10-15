/**
 * @file log_buffer.cpp
 * @brief Ring buffer of the last LOG_LINES log lines for GET /log (Sprint P1).
 * Thread-safe (spinlock). Lines are truncated to LOG_LINE_LEN-1 chars.
 * Never call from the control task loop (it does not log at all).
 *
 * V1.5.14 – exact behaviour (tested natively: test/native/test_log_buffer):
 *  - logLine formats with vsnprintf into a local char[LOG_LINE_LEN] (longer text is cut to
 *    LOG_LINE_LEN-1 chars). If the formatted text ends with '\n', that ONE '\n' is removed.
 *  - The line is copied into the ring inside ONE file-static portMUX_TYPE critical section
 *    (taskENTER_CRITICAL / taskEXIT_CRITICAL). Nothing else happens inside it.
 *  - Echo: Serial.println(line) AFTER taskEXIT_CRITICAL (Serial may block - never inside).
 *  - The ring keeps the newest LOG_LINES lines; the oldest is overwritten.
 *  - logDump copies the ring inside the same critical section, then writes the lines oldest
 *    first, each followed by '\n', plus a terminating NUL. If not all lines fit into cap-1
 *    chars, the OLDEST lines are left out (only complete lines; the newest that fit are kept).
 *    cap == 0 -> writes nothing, returns 0. Otherwise returns strlen(buf).
 */

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "comm/log_buffer.h"

// ---------------------------------------------------------------------------
// File-static state
// ---------------------------------------------------------------------------
static portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;

static char ring[LOG_LINES][LOG_LINE_LEN];  // ring buffer of lines
static uint32_t writeIdx = 0;               // next slot to overwrite
static uint32_t lineCount = 0;              // how many lines are stored (<= LOG_LINES)

// ---------------------------------------------------------------------------
// logLine – format, truncate, store, echo
// ---------------------------------------------------------------------------
void logLine(const char* fmt, ...) {
    char buf[LOG_LINE_LEN];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, LOG_LINE_LEN, fmt, ap);
    va_end(ap);

    // Remove ONE trailing '\n' if present.
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }

    // Copy into ring inside critical section. Nothing else happens inside.
    taskENTER_CRITICAL(&logMux);
    memcpy(ring[writeIdx], buf, LOG_LINE_LEN);  // always copy full slot (NUL-padded)
    writeIdx = (writeIdx + 1) % LOG_LINES;
    if (lineCount < LOG_LINES) {
        ++lineCount;
    }
    taskEXIT_CRITICAL(&logMux);

    // Echo AFTER critical section.
    Serial.println(buf);
}

// ---------------------------------------------------------------------------
// logDump – oldest first, '\n' separated, NUL-terminated
//           keeps the NEWEST complete lines that fit into cap-1 bytes.
// ---------------------------------------------------------------------------
size_t logDump(char* buf, size_t cap) {
    if (cap == 0) {
        return 0;
    }

    // Copy ring inside critical section. Nothing else happens inside.
    taskENTER_CRITICAL(&logMux);

    uint32_t start;   // index of the oldest line in the ring
    if (lineCount < LOG_LINES) {
        start = 0;
    } else {
        start = writeIdx;       // oldest slot was just overwritten by newest
    }

    struct LineInfo { uint32_t idx; size_t len; };
    LineInfo lines[LOG_LINES];
    for (uint32_t i = 0; i < lineCount; ++i) {
        uint32_t idx = (start + i) % LOG_LINES;
        lines[i].idx = idx;
        lines[i].len = strlen(ring[idx]);
    }

    taskEXIT_CRITICAL(&logMux);

    // First pass: total bytes needed for all lines.
    size_t totalLen = 0;
    for (uint32_t i = 0; i < lineCount; ++i) {
        totalLen += lines[i].len + 1;       // line + '\n'
    }

    // Trim oldest lines from the front until all remaining fit in cap-1.
    size_t outStart = 0;
    while (outStart < lineCount && totalLen > cap - 1) {
        totalLen -= lines[outStart].len + 1;
        ++outStart;
    }

    // Write remaining lines oldest-first.
    size_t outLen = 0;
    for (uint32_t i = outStart; i < lineCount; ++i) {
        memcpy(buf + outLen, ring[lines[i].idx], lines[i].len);
        outLen += lines[i].len;
        buf[outLen++] = '\n';
    }

    buf[outLen] = '\0';
    return (size_t)outLen;
}