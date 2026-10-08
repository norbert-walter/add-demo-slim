// FROZEN – Contract V1.1 (complete implementation). See snapshot.h.
#include "snapshot.h"
#include <Arduino.h>

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static ProcData     s_proc;
static Params       s_params;
static SafetyFlags  s_flags;
static volatile uint32_t s_last_rx = 0;
static volatile bool s_persist = false;
static volatile bool s_reboot = false;   // 32-bit aligned store/load is atomic on ESP32

void snapshotPublish(const ProcData& d) {
    taskENTER_CRITICAL(&s_mux);
    s_proc = d;
    taskEXIT_CRITICAL(&s_mux);
}
void snapshotGet(ProcData& out) {
    taskENTER_CRITICAL(&s_mux);
    out = s_proc;
    taskEXIT_CRITICAL(&s_mux);
}
void snapshotPublishConfig(const Params& p, const SafetyFlags& f) {
    taskENTER_CRITICAL(&s_mux);
    s_params = p;
    s_flags = f;
    taskEXIT_CRITICAL(&s_mux);
}
void snapshotGetConfig(Params& p, SafetyFlags& f) {
    taskENTER_CRITICAL(&s_mux);
    p = s_params;
    f = s_flags;
    taskEXIT_CRITICAL(&s_mux);
}
void snapshotSetLastApiRx(uint32_t now_ms) { s_last_rx = now_ms; }
uint32_t snapshotLastApiRx() { return s_last_rx; }

void snapshotRequestPersist() { s_persist = true; }
bool snapshotTakePersistRequest() {
    taskENTER_CRITICAL(&s_mux);
    bool r = s_persist; s_persist = false;
    taskEXIT_CRITICAL(&s_mux);
    return r;
}
void snapshotRequestReboot() { s_reboot = true; }
bool snapshotTakeRebootRequest() {
    taskENTER_CRITICAL(&s_mux);
    bool r = s_reboot; s_reboot = false;
    taskEXIT_CRITICAL(&s_mux);
    return r;
}

static uint32_t s_cycle_max_us = 0;
static uint32_t s_overruns = 0;
void snapshotSetCycleStats(uint32_t max_us, uint32_t overruns) {
    taskENTER_CRITICAL(&s_mux);
    s_cycle_max_us = max_us; s_overruns = overruns;
    taskEXIT_CRITICAL(&s_mux);
}
void snapshotGetCycleStats(uint32_t& max_us, uint32_t& overruns) {
    taskENTER_CRITICAL(&s_mux);
    max_us = s_cycle_max_us; overruns = s_overruns;
    taskEXIT_CRITICAL(&s_mux);
}
