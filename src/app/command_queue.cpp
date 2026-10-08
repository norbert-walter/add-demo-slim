// FROZEN – Contract V1.1 (complete implementation). See command_queue.h.
#include "command_queue.h"
#include "config.h"
#include "logic/app_logic.h"   // appCmdIsPriority
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_timer.h>

static QueueHandle_t s_cmd = nullptr;
static QueueHandle_t s_prio = nullptr;
static QueueHandle_t s_reply = nullptr;
static SemaphoreHandle_t s_wait_mutex = nullptr;   // serialises cmdQueueSendAndWait
static portMUX_TYPE  s_latch_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t      s_latch_bits = 0;              // priority overflow latch (appPrioLatch*)
static uint32_t      s_latch_t_us = 0;              // enqueue time of the oldest latched request
static portMUX_TYPE  s_seq_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t      s_seq = 0;

void cmdQueueInit() {
    s_cmd = xQueueCreate(CMD_QUEUE_LEN, sizeof(Command));
    s_prio = xQueueCreate(CMD_PRIO_QUEUE_LEN, sizeof(Command));
    s_reply = xQueueCreate(2, sizeof(CommandResult));
    s_wait_mutex = xSemaphoreCreateMutex();
}

static uint32_t nextSeq() {
    taskENTER_CRITICAL(&s_seq_mux);
    uint32_t s = ++s_seq;
    taskEXIT_CRITICAL(&s_seq_mux);
    return s;
}

bool cmdQueueSend(Command& cmd) {
    cmd.seq = nextSeq();
    cmd.t_enq_us = (uint32_t)esp_timer_get_time();
    if (!appCmdIsPriority(cmd.id)) return xQueueSend(s_cmd, &cmd, 0) == pdTRUE;
    if (xQueueSend(s_prio, &cmd, 0) == pdTRUE) return true;
    // Priority queue full: latch the request. It may be coalesced with an
    // identical pending one, but it is never dropped. No reply will come.
    taskENTER_CRITICAL(&s_latch_mux);
    if (s_latch_bits == 0) s_latch_t_us = cmd.t_enq_us;
    appPrioLatchSet(s_latch_bits, cmd);
    taskEXIT_CRITICAL(&s_latch_mux);
    return true;
}

static ApiError sendAndWaitLocked(Command& cmd, uint32_t timeout_ms) {
    CommandResult stale;
    while (xQueueReceive(s_reply, &stale, 0) == pdTRUE) {}   // late replies of OUR earlier timeouts
    cmd.want_reply = true;
    if (!cmdQueueSend(cmd)) return ApiError::BUSY;
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    CommandResult r;
    for (;;) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) return ApiError::BUSY;
        if (xQueueReceive(s_reply, &r, deadline - now) != pdTRUE) return ApiError::BUSY;
        if (r.seq == cmd.seq) return r.err;   // ignore replies of timed-out commands
    }
}

ApiError cmdQueueSendAndWait(Command& cmd, uint32_t timeout_ms) {
    if (xSemaphoreTake(s_wait_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        if (appCmdIsPriority(cmd.id)) {          // queue or latch: never dropped
            cmd.want_reply = false;
            cmdQueueSend(cmd);
        }
        return ApiError::BUSY;
    }
    ApiError e = sendAndWaitLocked(cmd, timeout_ms);
    xSemaphoreGive(s_wait_mutex);
    return e;
}

int cmdQueueReceivePriority(Command* out, int max) {
    int n = 0;
    for (int i = 0; i < CMD_PRIO_QUEUE_LEN && n < max; ++i) {   // only what was there
        if (xQueueReceive(s_prio, &out[n], 0) != pdTRUE) break;
        n++;
    }
    taskENTER_CRITICAL(&s_latch_mux);
    while (n < max && appPrioLatchTake(s_latch_bits, out[n])) {
        out[n].t_enq_us = s_latch_t_us;
        n++;
    }
    taskEXIT_CRITICAL(&s_latch_mux);
    return n;
}

bool cmdQueueReceive(Command& out, bool allow_normal) {
    if (xQueueReceive(s_prio, &out, 0) == pdTRUE) return true;   // arrived after step 3a
    return allow_normal && xQueueReceive(s_cmd, &out, 0) == pdTRUE;
}

void cmdQueueReply(const Command& cmd, ApiError err) {
    if (!cmd.want_reply) return;
    CommandResult r;
    r.seq = cmd.seq; r.id = cmd.id; r.err = err;
    xQueueSend(s_reply, &r, 0);
}
