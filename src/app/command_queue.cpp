// FROZEN – Sprint 0b (complete implementation). See command_queue.h.
#include "command_queue.h"
#include "config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static QueueHandle_t s_cmd = nullptr;
static QueueHandle_t s_reply = nullptr;
static portMUX_TYPE  s_seq_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t      s_seq = 0;

void cmdQueueInit() {
    s_cmd = xQueueCreate(CMD_QUEUE_LEN, sizeof(Command));
    s_reply = xQueueCreate(2, sizeof(CommandResult));
}

static uint32_t nextSeq() {
    taskENTER_CRITICAL(&s_seq_mux);
    uint32_t s = ++s_seq;
    taskEXIT_CRITICAL(&s_seq_mux);
    return s;
}

bool cmdQueueSend(Command& cmd) {
    cmd.seq = nextSeq();
    return xQueueSend(s_cmd, &cmd, 0) == pdTRUE;
}

ApiError cmdQueueSendAndWait(Command& cmd, uint32_t timeout_ms) {
    CommandResult stale;
    while (xQueueReceive(s_reply, &stale, 0) == pdTRUE) {}   // drop late replies
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

bool cmdQueueReceive(Command& out) { return xQueueReceive(s_cmd, &out, 0) == pdTRUE; }

void cmdQueueReply(const Command& cmd, ApiError err) {
    if (!cmd.want_reply) return;
    CommandResult r;
    r.seq = cmd.seq; r.id = cmd.id; r.err = err;
    xQueueSend(s_reply, &r, 0);
}
