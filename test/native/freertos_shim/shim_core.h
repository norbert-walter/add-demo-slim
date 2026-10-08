// Host shim of the few Arduino / FreeRTOS / ESP-IDF calls used by src/app/command_queue.cpp.
// ONLY for test/native/test_command_queue. Threads = tasks, 1 tick = 1 ms.
// The guard makes sure src/logic can never compile against these headers by accident.
#pragma once
#ifndef ADD_HOST_SHIM
#error "Arduino/FreeRTOS headers are not available natively (src/logic must stay pure C++)"
#endif
#include <stdint.h>
#include <string.h>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>

typedef uint32_t TickType_t;
typedef int      BaseType_t;
#define pdTRUE  1
#define pdFALSE 0
#define pdPASS  1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define configMAX_PRIORITIES 25
#define LOW  0
#define HIGH 1

inline std::chrono::steady_clock::time_point shimT0() {
    static const auto t0 = std::chrono::steady_clock::now();
    return t0;
}
inline TickType_t xTaskGetTickCount() {
    return (TickType_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - shimT0()).count();
}
inline uint32_t millis() { return xTaskGetTickCount(); }
inline int64_t esp_timer_get_time() {
    return (int64_t)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - shimT0()).count();
}

struct portMUX_TYPE { std::mutex m; };
#define portMUX_INITIALIZER_UNLOCKED {}
inline void taskENTER_CRITICAL(portMUX_TYPE* p) { p->m.lock(); }
inline void taskEXIT_CRITICAL(portMUX_TYPE* p) { p->m.unlock(); }

struct ShimQueue {
    std::mutex m;
    std::condition_variable cv;
    std::deque<std::vector<uint8_t>> q;
    size_t cap, item;
    ShimQueue(size_t c, size_t i) : cap(c), item(i) {}
};
typedef ShimQueue* QueueHandle_t;
inline QueueHandle_t xQueueCreate(size_t len, size_t item) { return new ShimQueue(len, item); }
inline BaseType_t xQueueSend(QueueHandle_t h, const void* p, TickType_t ticks) {
    std::unique_lock<std::mutex> l(h->m);
    if (!h->cv.wait_for(l, std::chrono::milliseconds(ticks), [&] { return h->q.size() < h->cap; }))
        return pdFALSE;
    const uint8_t* b = (const uint8_t*)p;
    h->q.emplace_back(b, b + h->item);
    h->cv.notify_all();
    return pdTRUE;
}
inline BaseType_t xQueueReceive(QueueHandle_t h, void* p, TickType_t ticks) {
    std::unique_lock<std::mutex> l(h->m);
    if (!h->cv.wait_for(l, std::chrono::milliseconds(ticks), [&] { return !h->q.empty(); }))
        return pdFALSE;
    memcpy(p, h->q.front().data(), h->item);
    h->q.pop_front();
    h->cv.notify_all();
    return pdTRUE;
}

// FreeRTOS mutex semantics: only the owner may give it back (checked, abort otherwise).
struct ShimMutex {
    std::mutex m;
    std::condition_variable cv;
    bool locked = false;
    std::thread::id owner;
};
typedef ShimMutex* SemaphoreHandle_t;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return new ShimMutex(); }
inline BaseType_t xSemaphoreTake(SemaphoreHandle_t h, TickType_t ticks) {
    std::unique_lock<std::mutex> l(h->m);
    if (!h->cv.wait_for(l, std::chrono::milliseconds(ticks), [&] { return !h->locked; }))
        return pdFALSE;
    h->locked = true;
    h->owner = std::this_thread::get_id();
    return pdTRUE;
}
inline BaseType_t xSemaphoreGive(SemaphoreHandle_t h) {
    std::lock_guard<std::mutex> l(h->m);
    if (!h->locked || h->owner != std::this_thread::get_id()) {
        fprintf(stderr, "shim: xSemaphoreGive by non-owner\n");
        abort();
    }
    h->locked = false;
    h->cv.notify_one();
    return pdTRUE;
}
