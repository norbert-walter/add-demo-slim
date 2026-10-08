/**
 * @file command_queue.h
 * @brief FreeRTOS queues between HTTP/display and the control task
 *        (FROZEN – Sprint 0b, complete).
 *
 * Producers: HTTP task (Core 0), display touch (Core 1).
 * Consumer:  control task (Core 1), max. one command per cycle.
 * Results:   only for commands with want_reply = true (HTTP), via a reply queue.
 *            The HTTP server is single-threaded, so at most one reply is pending.
 */
#pragma once
#include "logic/types.h"

void cmdQueueInit();

/// Producer side. Sets cmd.seq. Returns false if the queue is full (-> ApiError::BUSY).
bool cmdQueueSend(Command& cmd);

/// HTTP side: sends cmd and waits up to timeout_ms for its result.
/// Returns BUSY if the queue is full or no reply arrives in time. After a timeout
/// the command may still be executed later – clients must check /proc.
ApiError cmdQueueSendAndWait(Command& cmd, uint32_t timeout_ms);

/// Consumer side (control task): non-blocking.
bool cmdQueueReceive(Command& out);

/// Consumer side: deliver the result of a command with want_reply = true.
void cmdQueueReply(const Command& cmd, ApiError err);
