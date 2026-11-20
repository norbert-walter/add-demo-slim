#pragma once
// V1.6.6 host shim: the brownout register of the ESP32 (same field layout as ESP-IDF:
// RTC_CNTL_DBROWN_OUT_THRES bits 29:27; other bits of the register are kept by REG_SET_FIELD).
#define RTC_CNTL_BROWN_OUT_REG       3   /* fake address = index into shimRegs() */
#define RTC_CNTL_DBROWN_OUT_THRES    0x00000007
#define RTC_CNTL_DBROWN_OUT_THRES_V  0x7
#define RTC_CNTL_DBROWN_OUT_THRES_S  27
#define RTC_CNTL_BROWN_OUT_ENA       (1u << 30)
