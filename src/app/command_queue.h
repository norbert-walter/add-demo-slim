/**
 * @file command_queue.h
 * @brief FreeRTOS queues between HTTP/display and the control task
 *        (FROZEN – Contract V1.1, complete).
 *
 * Producers: HTTP task (Core 0), display touch (Core 1).
 * Consumer:  control task (Core 1), max. one command per cycle.
 * Results:   only for commands with want_reply = true (HTTP), via a reply queue.
 *
 * V1.1 – two queues:
 *   priority queue (CMD_PRIO_QUEUE_LEN): STOP, HV_OFF, SHUTTER_CLOSE (appCmdIsPriority).
 *            Always served first and even in a cycle that already has a driver
 *            request. A full normal queue can therefore never delay a STOP.
 *   normal queue (CMD_QUEUE_LEN): all other commands, served only when the
 *            cycle has no driver request yet.
 *   overflow latch: if the priority queue is full, the request is stored as a
 *            bit (appPrioLatchSet). Identical pending requests coalesce, nothing
 *            is dropped. Served by cmdQueueReceivePriority() after the queue.
 *            A latched request gets no reply (a waiting caller sees BUSY).
 *   => every priority request pending when the control task reaches step 3a is
 *      executed in that cycle (cmdQueueReceivePriority), independent of how many
 *      are pending. Bound: enqueue -> outputs <= period_max_us + cycle_max_us
 *      (acceptance: <= 2000 µs, measured as prio_lat_max_us).
 *
 * Concurrent callers (V1.1): cmdQueueSendAndWait() is SERIALISED by a mutex,
 *   so only one caller waits for a reply at any time and nobody can consume or
 *   drop a foreign reply. Today there is only one caller (the WebServer of the
 *   network task handles one request at a time); the mutex keeps this true if
 *   that ever changes (e.g. AsyncWebServer).
 *   If the mutex is not free within timeout_ms the call returns BUSY. A priority
 *   command (STOP, HV_OFF, SHUTTER_CLOSE) is then STILL queued (or latched)
 *   without a reply.
 * The display uses cmdQueueSend() (no reply).
 */
#pragma once
#include "logic/types.h"
#include "config.h"

void cmdQueueInit();

/// Producer side. Sets cmd.seq and cmd.t_enq_us, routes priority commands to the
/// priority queue (or the overflow latch). Returns false only for a NORMAL command
/// whose queue is full (-> ApiError::BUSY); priority commands always return true.
bool cmdQueueSend(Command& cmd);

/// Sends cmd and waits up to timeout_ms for its result (serialised, see above).
/// Returns BUSY if the queue is full or no reply arrives in time. After a timeout
/// the command may still be executed later – clients must check /proc before
/// retrying (CONTRACT §5.2 retry rule).
ApiError cmdQueueSendAndWait(Command& cmd, uint32_t timeout_ms);

/// Maximum number of priority commands returned by one cmdQueueReceivePriority().
constexpr int CMD_PRIO_MAX_PER_CYCLE = CMD_PRIO_QUEUE_LEN + 3;

/// Consumer side (control task, step 3a): non-blocking. Returns ALL priority
/// commands that are pending at the moment of the call: up to CMD_PRIO_QUEUE_LEN
/// from the priority queue (FIFO), then every latched request. out must hold
/// CMD_PRIO_MAX_PER_CYCLE entries. Requests arriving during the call are taken
/// by the next call.
int cmdQueueReceivePriority(Command* out, int max);

/// Consumer side (control task, step 3b): non-blocking. A priority command that
/// arrived after step 3a first, else a normal command if allow_normal. The
/// overflow latch is served ONLY by cmdQueueReceivePriority().
bool cmdQueueReceive(Command& out, bool allow_normal);

/// Consumer side: deliver the result of a command with want_reply = true.
/// Called AFTER the outputs of the cycle were applied.
void cmdQueueReply(const Command& cmd, ApiError err);
