#pragma once
// V1.6.6 host shim: on the host a plain static variable keeps its value between calls, which is
// what RTC memory does over a reset on the target.
#define RTC_NOINIT_ATTR
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif
