#pragma once
#include "shim_core.h"
#include <stdarg.h>
#include <string>
#include "WString.h"   // V1.5.16

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
    void clear() { std::lock_guard<std::mutex> l(m); out.clear(); in.clear(); reads = 0; writes_in_critical = 0; baud = 0; }
    // V1.5.23: input for the serial console (the test fills "in"); read() counts every call
    std::string in;
    int reads = 0;
    int available() { std::lock_guard<std::mutex> l(m); return (int)in.size(); }
    int read() { std::lock_guard<std::mutex> l(m); ++reads; if (in.empty()) return -1; int c = (unsigned char)in[0]; in.erase(0, 1); return c; }
};
inline ShimSerial Serial;

struct ShimEsp {
    int restarts = 0;
    void restart() { ++restarts; }   // the real one does not return
    uint32_t getFreeHeap() { return 200000; }
};
inline ShimEsp ESP;

// ---- V1.5.18: GPIO and ADC (Arduino-ESP32 3.x signatures) for the H1 host tests ------------
// Every call is appended to shimHwLog() ("pinMode 32 3", "dw 32 0", "dr 22", "adc 34",
// "atten 34 3", ...), so tests can check the ORDER of hardware accesses across GPIO and stepper.
#include <vector>
#define INPUT          0x01
#define OUTPUT         0x03
#define INPUT_PULLUP   0x05
typedef enum { ADC_0db = 0, ADC_2_5db = 1, ADC_6db = 2, ADC_11db = 3 } adc_attenuation_t;
inline std::vector<std::string>& shimHwLog() { static std::vector<std::string> v; return v; }
inline void shimHw(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
inline void shimHw(const char* fmt, ...) {
    char b[96]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    shimHwLog().push_back(b);
}
struct ShimGpio {
    int mode[40];         ///< -1 = never configured
    int out[40];          ///< last written level, -1 = never written
    int in[40];           ///< level returned by digitalRead (set by the test)
    uint32_t mv[40];      ///< value returned by analogReadMilliVolts (set by the test)
    int atten[40];        ///< -1 = never set
    int writes[40];       ///< number of digitalWrite calls per pin
    int reads[40];        ///< number of digitalRead calls per pin
    int adc_reads[40];
    ShimGpio() { reset(); }
    void reset() {
        for (int i = 0; i < 40; ++i) { mode[i] = -1; out[i] = -1; in[i] = HIGH; mv[i] = 0; atten[i] = -1; writes[i] = 0; reads[i] = 0; adc_reads[i] = 0; }
    }
};
inline ShimGpio& shimGpio() { static ShimGpio g; return g; }
inline void pinMode(uint8_t pin, uint8_t mode) { shimHw("pinMode %u %u", pin, mode); if (pin < 40) shimGpio().mode[pin] = mode; }
inline void digitalWrite(uint8_t pin, uint8_t val) {
    shimHw("dw %u %u", pin, val ? 1u : 0u);
    if (pin < 40) { shimGpio().out[pin] = val ? HIGH : LOW; ++shimGpio().writes[pin]; }
}
inline int digitalRead(uint8_t pin) { shimHw("dr %u", pin); if (pin >= 40) return LOW; ++shimGpio().reads[pin]; return shimGpio().in[pin]; }
inline uint32_t analogReadMilliVolts(uint8_t pin) { shimHw("adc %u", pin); if (pin >= 40) return 0; ++shimGpio().adc_reads[pin]; return shimGpio().mv[pin]; }
inline uint16_t analogRead(uint8_t pin) { shimHw("adcraw %u", pin); return pin < 40 ? (uint16_t)(shimGpio().mv[pin] * 4095u / 3100u) : 0; }
inline void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t a) { shimHw("atten %u %d", pin, (int)a); if (pin < 40) shimGpio().atten[pin] = (int)a; }
