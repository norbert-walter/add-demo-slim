/**
 * @file net_task.h
 * @brief Network task: WLAN, HTTP server, NVS persistence, delayed reboot (V1.5.20, Sprint I1),
 *        serial console (V1.5.23).
 *
 * Implementation: src/app/net_task.cpp. Runs on NET_TASK_CORE (Core 0) every NET_TASK_PERIOD_MS.
 * It is the ONLY task that calls wifiLoop(), httpServerLoop() (and through it
 * cmdQueueSendAndWait()), nvsSaveParams() and systemRestart().
 *
 * V1.5.20 – exact behaviour (tested natively: test/native/test_net_task):
 *  netTaskStep(now), in this order:
 *    0. V1.5.23 serial console (see below) – before step 1, so that a SET/REBOOT it sends is
 *       persisted / scheduled by steps 3 and 4 of the same call
 *    1. wifiLoop(now)
 *    2. httpServerLoop()
 *    2a. V1.5.24 comm diagnostics (log only, no effect on the device): ProcData d; snapshotGet(d);
 *       rx = snapshotLastApiRx(). When rx differs from the value of the previous step (a new valid
 *       /api packet) and the watchdog was armed at the previous packet and in every step since
 *       (d.safety.comm_armed): gap = rx - previous rx; the longest such gap is kept;
 *       gap > COMM_GAP_LOG_MS -> logLine("comm: /api gap %u ms", gap). The first step only stores rx.
 *       d.err == COMM_TIMEOUT for the first step (edge) -> logLine("comm: timeout, last /api packet
 *       %u ms ago, longest gap while armed %u ms", now - rx, longest gap). The longest gap is reset
 *       while the watchdog is disarmed and no COMM_TIMEOUT is shown.
 *    3. if snapshotTakePersistRequest(): Params p; SafetyFlags f; snapshotGetConfig(p, f);
 *       logLine("nvs: saved") if nvsSaveParams(p) returned true, else logLine("nvs: save FAILED")
 *       V1.5.25: a taken request stays PENDING and is saved only in a step where the snapshot of
 *       step 2a shows motion.state IDLE and safety.hv_on false (a flash write stops both cores for
 *       tens of ms). While pending and not idle: logLine("nvs: save deferred until axis idle and
 *       HV off") once per request. The values are active in RAM from the SET on. Step 5 saves a
 *       pending request right before systemRestart().
 *    4. if snapshotTakeRebootRequest() and no reboot is pending: reboot pending, due at
 *       now + REBOOT_DELAY_MS; logLine("reboot: in %u ms", REBOOT_DELAY_MS). Further requests while
 *       pending are taken (cleared) but do not move the due time.
 *    5. if a reboot is pending and (int32_t)(now - due) >= 0 (wrap-around safe): pending = false,
 *       systemRestart(). (A pending flag, not due == 0, marks the state: now + 700 may be 0.)
 *    Step 3 comes before step 5: a SET (persist) and a REBOOT in the same step are saved first.
 *    V1.5.25 slow-step diagnostics (log only): millis() is read around step 0, 1, 2 and the NVS
 *    write in step 3; a part longer than NET_SLOW_LOG_MS -> logLine("net: <part> %u ms") with part
 *    console / wifi / http / nvs. At the start of every step but the first: time since the end of
 *    the previous step > NET_SLOW_LOG_MS -> logLine("net: gap %u ms") (logged before step 0).
 *
 *  V1.5.23 – serial console (115200 Bd, the USB port of the board; answers are ONE line of JSON each,
 *  written with Serial.print(text) and Serial.print("\n")):
 *    Reading: at most CONSOLE_READ_MAX bytes per step (Serial.available() / Serial.read()); '\r' is
 *    ignored; '\n' ends a line. A line longer than CONSOLE_LINE_MAX characters is dropped up to its
 *    '\n' and answered {"error":"line too long"}. Leading spaces are skipped; an empty line gets no
 *    answer. At most ONE command is executed per step (bytes after its '\n' stay for the next step).
 *    Commands are sent like API commands: Command with id, source = API, want_reply = true, SET as
 *    key + vtype STRING + str; err = cmdQueueSendAndWait(cmd, API_REPLY_TIMEOUT_MS).
 *    Error answer of a command: {"error":"<apiErrorMessage(err)>","err":<int>} (+ ,"key":"<key>" for a SET).
 *    help    -> {"help":"help | ver | status | reboot | wifi <ssid> <password> | wifi \"<ssid>\" <password> | wifi off"}
 *    ver     -> {"fw":"<FW_VERSION>"}
 *    status  -> snapshotGet(d), snapshotGetConfig(p, f), wifiStaStatus(), wifiGetApIp(), wifiGetStaIp():
 *               {"state":"<safetyStateName>","err":<int>,"local":<bool>,"ap":"<ap ip>","sta":"<staStateName>",
 *                "sta_ip":"<ip or empty>","ssid":"<p.wifi_ssid>","pass_set":<p.wifi_pass[0] != 0>}
 *    reboot  -> REBOOT; OK -> {"reboot":true}; else the error answer
 *    wifi <ssid> <password>    ssid = the first word, password = everything after the single space
 *    wifi "<ssid>" <password>  (ssid with spaces: up to the next '"', then one space, then the password)
 *            -> SET wifi_ssid, then SET wifi_pass, then REBOOT. A failing SET stops at once (error
 *               answer with its key, no REBOOT). Both SET ok: {"wifi":"saved","ssid":"<ssid>","reboot":true}
 *               or, if REBOOT fails, {"wifi":"saved","ssid":"<ssid>","reboot":false,"err":<int>}
 *               (effective after the next power-up). "wifi" alone or a missing password ->
 *               {"error":"usage: wifi <ssid> <password> | wifi off"}
 *    wifi off -> SET wifi_ssid "", SET wifi_pass "", REBOOT; answers as above with "wifi":"off" and no "ssid"
 *    anything else -> {"error":"unknown command, try help"}
 *    The password is never written to Serial or the log. Strings in answers are JSON-escaped
 *    ('"' and '\\' with a backslash, control characters as '?').
 *  netTaskStart(): xTaskCreatePinnedToCore(<task function>, "net", NET_TASK_STACK, nullptr,
 *    NET_TASK_PRIO, nullptr, NET_TASK_CORE); returns true if that returned pdPASS.
 *    The task function loops forever: netTaskStep(millis()); vTaskDelay(pdMS_TO_TICKS(NET_TASK_PERIOD_MS)).
 *
 * src/main.cpp (V1.5.20, tested natively in the same suite), setup() in this order:
 *    1. systemInit()
 *    2. SPIFFS.begin(true)  (format on failure); false -> logLine("spiffs: mount failed"), continue
 *    3. Params p; nvsLoadParams(p)
 *    4. cmdQueueInit()
 *    5. controlTaskStart(p); false -> logLine("FATAL: control task start failed"), continue
 *    6. displayStart();      false -> logLine("display: start failed"), continue
 *    7. wifiInit(p.wifi_ssid, p.wifi_pass)
 *    8. httpServerInit()
 *    9. netTaskStart();      false -> logLine("FATAL: net task start failed")
 *   10. logLine("boot: ready, FW %s", FW_VERSION)
 *  loop(): vTaskDelay(portMAX_DELAY) (the Arduino loop task is not used).
 *  The device keeps running after a failed step: the web server and /log show what failed.
 */
#pragma once
#include <stdint.h>

void netTaskStep(uint32_t now_ms);
bool netTaskStart();
