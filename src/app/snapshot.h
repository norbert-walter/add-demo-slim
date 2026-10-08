/**
 * @file snapshot.h
 * @brief Single-writer process image shared between cores (FROZEN – Sprint 0b, complete).
 *
 * Writer: control task (Core 1) once per cycle with snapshotPublish().
 * Readers: HTTP task (Core 0) and display (Core 1) with snapshotGet().
 * Both copy the whole ProcData under one spinlock (~150 bytes, < 2 us).
 * Nobody else may hold pointers into the shared copy.
 */
#pragma once
#include "logic/types.h"
#include "logic/params.h"

void snapshotPublish(const ProcData& d);
void snapshotGet(ProcData& out);

/// Configuration copy for GET_CONFIG (written by control task after each change).
void snapshotPublishConfig(const Params& p, const SafetyFlags& f);
void snapshotGetConfig(Params& p, SafetyFlags& f);

/// Requests from the control task that must NOT run in the 1 ms cycle.
/// The network task (Core 0) takes them: persist -> nvsSaveParams(config copy),
/// reboot -> ESP.restart() after the HTTP reply has been sent.
void snapshotRequestPersist();
bool snapshotTakePersistRequest();
void snapshotRequestReboot();
bool snapshotTakeRebootRequest();

/// Time of the last valid /api packet (written by the HTTP task, read by control).
void     snapshotSetLastApiRx(uint32_t now_ms);
uint32_t snapshotLastApiRx();
