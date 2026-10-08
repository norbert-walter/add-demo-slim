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
 *   => STOP latency from enqueue to driver request <= 2 control cycles (2 ms),
 *      as long as fewer than CMD_PRIO_QUEUE_LEN priority commands are waiting.
 *
 * Concurrent callers (V1.1): cmdQueueSendAndWait() is SERIALISED by a mutex,
 *   so only one caller waits for a reply at any time and nobody can consume or
 *   drop a foreign reply. Today there is only one caller (the WebServer of the
 *   network task handles one request at a time); the mutex keeps this true if
 *   that ever changes (e.g. AsyncWebServer).
 *   If the mutex is not free within timeout_ms the call returns BUSY. A priority
 *   command (STOP, HV_OFF, SHUTTER_CLOSE) is then STILL queued without a reply,
 *   so a safety command is never lost because another request is waiting.
 * The display uses cmdQueueSend() (no reply).
 */
#pragma once
#include "logic/types.h"

void cmdQueueInit();

/// Producer side. Sets cmd.seq, routes priority commands to the priority queue.
/// Returns false if the target queue is full (-> ApiError::BUSY).
bool cmdQueueSend(Command& cmd);

/// Sends cmd and waits up to timeout_ms for its result (serialised, see above).
/// Returns BUSY if the queue is full or no reply arrives in time. After a timeout
/// the command may still be executed later – clients must check /proc before
/// retrying (CONTRACT §5.2 retry rule).
ApiError cmdQueueSendAndWait(Command& cmd, uint32_t timeout_ms);

/// Consumer side (control task): non-blocking. Takes a priority command if one
/// is waiting; otherwise a normal command, but only if allow_normal is true.
bool cmdQueueReceive(Command& out, bool allow_normal);

/// Consumer side: deliver the result of a command with want_reply = true.
/// Called AFTER the outputs of the cycle were applied.
void cmdQueueReply(const Command& cmd, ApiError err);
