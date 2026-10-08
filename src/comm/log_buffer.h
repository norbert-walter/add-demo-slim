/**
 * @file log_buffer.h
 * @brief Ring buffer of the last LOG_LINES log lines for GET /log (Sprint S3).
 * Thread-safe (spinlock). Lines are truncated to LOG_LINE_LEN-1 chars.
 * Never call from the control task loop (it does not log at all).
 */
#pragma once
#include <stddef.h>

void   logLine(const char* fmt, ...);            ///< printf-style; also echoes to Serial
size_t logDump(char* buf, size_t cap);           ///< oldest first, '\n' separated; returns length
