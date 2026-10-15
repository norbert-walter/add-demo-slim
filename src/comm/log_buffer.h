/**
 * @file log_buffer.h
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
 *  - logDump copies the ring TEXT (all lines, not only indices or lengths) inside the same
 *    critical section into a local/static copy and reads only that copy afterwards (V1.5.15),
 *    then writes the lines oldest
 *    first, each followed by '\n', plus a terminating NUL. If not all lines fit into cap-1
 *    chars, the OLDEST lines are left out (only complete lines; the newest that fit are kept).
 *    cap == 0 -> writes nothing, returns 0. Otherwise returns strlen(buf).
 */
#pragma once
#include <stddef.h>

void   logLine(const char* fmt, ...);            ///< printf-style; also echoes to Serial
size_t logDump(char* buf, size_t cap);           ///< oldest first, '\n' separated; returns length
