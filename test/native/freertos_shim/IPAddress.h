#pragma once
#include <stdint.h>
#include "WString.h"
class IPAddress {
    uint8_t b_[4] = {0, 0, 0, 0};
public:
    IPAddress() {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) { b_[0] = a; b_[1] = b; b_[2] = c; b_[3] = d; }
    uint8_t operator[](int i) const { return b_[i]; }
    String toString() const { char t[16]; snprintf(t, sizeof t, "%u.%u.%u.%u", b_[0], b_[1], b_[2], b_[3]); return String(t); }
};
