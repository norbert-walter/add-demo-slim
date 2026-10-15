#pragma once
#include "shim_core.h"
#include <stdarg.h>
#include <string>

// ---- V1.5.14: Serial, ESP and core version for the P1 host tests ----------------------------
#define ESP_ARDUINO_VERSION_MAJOR 3
#define ESP_ARDUINO_VERSION_MINOR 1
#define ESP_ARDUINO_VERSION_PATCH 3

struct ShimSerial {
    std::mutex m;
    std::string out;                 ///< everything written
    unsigned long baud = 0;
    int writes_in_critical = 0;      ///< calls while the caller held a portMUX critical section
    void note() { if (shimCritDepth() > 0) ++writes_in_critical; }
    void begin(unsigned long b) { std::lock_guard<std::mutex> l(m); baud = b; }
    void end() {}
    void flush() {}
    explicit operator bool() const { return true; }
    size_t print(const char* s) { note(); std::lock_guard<std::mutex> l(m); out += s; return strlen(s); }
    size_t print(const std::string& s) { return print(s.c_str()); }
    size_t print(int v) { char b[16]; snprintf(b, sizeof b, "%d", v); return print(b); }
    size_t print(unsigned v) { char b[16]; snprintf(b, sizeof b, "%u", v); return print(b); }
    size_t print(long v) { char b[24]; snprintf(b, sizeof b, "%ld", v); return print(b); }
    size_t print(unsigned long v) { char b[24]; snprintf(b, sizeof b, "%lu", v); return print(b); }
    size_t println() { return print("\r\n"); }
    template <typename T> size_t println(T v) { size_t n = print(v); return n + print("\r\n"); }
    size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        char b[512]; va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
        print(b); return n < 0 ? 0 : (size_t)n;
    }
    size_t write(const uint8_t* p, size_t n) { std::string s((const char*)p, n); return print(s.c_str()); }
    void clear() { std::lock_guard<std::mutex> l(m); out.clear(); writes_in_critical = 0; baud = 0; }
};
inline ShimSerial Serial;

struct ShimEsp {
    int restarts = 0;
    void restart() { ++restarts; }   // the real one does not return
    uint32_t getFreeHeap() { return 200000; }
};
inline ShimEsp ESP;
