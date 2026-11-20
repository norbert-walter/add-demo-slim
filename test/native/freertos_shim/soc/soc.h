#pragma once
// V1.6.6 host shim of the ESP32 register access macros (soc/soc.h). Registers are an array of
// 32-bit words in host memory, addressed by small fake addresses (see soc/rtc_cntl_reg.h).
#include <stdint.h>
inline uint32_t* shimRegs() { static uint32_t r[16] = {0}; return r; }
inline uint32_t& shimRegWrites() { static uint32_t n = 0; return n; }
/// bits set here are read-only (a write keeps their old value) - to test read-back
inline uint32_t& shimRegStuck() { static uint32_t m = 0; return m; }
#define REG_READ(_r)            (shimRegs()[(_r)])
#define REG_WRITE(_r, _v)       do { shimRegs()[(_r)] = ((uint32_t)(_v) & ~shimRegStuck()) | \
                                     (shimRegs()[(_r)] & shimRegStuck()); ++shimRegWrites(); } while (0)
#define REG_GET_FIELD(_r, _f)   ((REG_READ(_r) >> (_f##_S)) & (_f##_V))
#define REG_SET_FIELD(_r, _f, _v) \
    REG_WRITE((_r), ((REG_READ(_r) & ~((_f##_V) << (_f##_S))) | (((_v) & (_f##_V)) << (_f##_S))))
