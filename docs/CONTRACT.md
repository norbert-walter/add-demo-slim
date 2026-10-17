# ADD Demo System – Interface Contract

**Version:** 1.5.25 · **Status:** FROZEN after review by Norbert Walter · **Date:** 2026-10-17
**Basis:** Pflichtenheft Demoanlage ADD Slim V1.7.9

### Changes 1.4.2 → 1.5 (2026-10-12, decision Norbert Walter: shutter life in continuous profile scans)

| # | Change | Where |
|---|---|---|
| 1 | **Three separate zones** in machine coordinates (origin always on the park side = homing switch): park 0 … 20 (shutter always closed), service > 20 … 60 (shutter only manual), **measuring zone > 60 … 242** (shutter only by the automation). A mirrored installation is handled by `DIR_POSITIVE_HIGH` and the choice of the homing switch (new smoketest sections 2 and 4), never by mirroring the zones | §2, §6.1, `safety_logic.h` |
| 2 | **Shutter stays open over a measuring programme.** The end of a traverse run no longer closes the shutter. The automatic shutter closes only: after the **hold time** `shutter_hold` (default 3 s) without a measuring mode · on `STOP` (immediately, in the STOP cycle) · when the axis enters the service zone (pos ≤ 60) · on HV loss / `HV_OFF` · on any fault · on `SHUTTER_CLOSE`. A continuous back-and-forth scan (client sends the next `START_TRAVERSE` within the hold time) keeps it open | §5.2, §6.1, §6.4, `safety_logic.h`, `app_logic.h` |
| 3 | `SafetyStatus::shutter_auto` (automatic vs. manual shutter); invariants **I7** `shutter_auto ⇒ shutter_open`, **I8** automatic shutter ⇒ pos > 60. `SHUTTER_AUTO_OPEN` only at pos > 60 and also **takes over** a manually opened shutter during an acquisition; a manual shutter is never closed by the hold time | `safety_logic.h` |
| 4 | New signature `safetyShutterAutomation(acquiring, program_active, idle_ms, hold_ms, status, pos, cfg)`; `AppState::prog_active_ms` | `safety_logic.h`, `app_logic.h` |
| 5 | `START_MEASURE` / `START_TRAVERSE` positions **> 60 mm** (`MotionConfig::measure_min_mm`); parameter ranges `measure_pos`, `traverse_start`, `traverse_end` **61 … 242 mm**; default `traverse_start` **80 mm** (was 60 = service zone limit) | §5.2, §8, §9, `motion_seq.h`, `params.h` |
| 6 | New persisted parameter **`shutter_hold`** 0.0 … 10.0 s, default 3.0 (0 = close at the end of every run); in `GET_CONFIG` after `traverse_speed`; NVS key `shold`, schema version 4 | §5.3, §9, §12, `params.h`, `nvs_config.h` |
| 7 | Fix: the `GET_CONFIG` example showed `"traverse_speed":200.0` (pre-V1.3 unit) and lacked `estop_nc` | §5.3 |
| 8 | Tests: safety (zones, take-over, every way of closing clears `shutter_auto`, hold time), motion (measuring zone), params (ranges, `shutter_hold`, JSON), app_logic (continuous profile without closing, hold parameter, long approach, closing when entering the service zone, STOP during the hold time, manual shutter not closed by the hold time). Reference implementation: 13 suites, **274/274**; mutants without hold time, with the old zone limits, without take-over, without STOP closing or without the service-zone rule are caught | `test/native` |

**Clarifications 1.5.1 (consistency review 2026-10-12, no behaviour change):** display texts
are English as required by the Pflichtenheft (§13, `display.h`: `FAULT:`, `CAUSE ACTIVE`,
`ACK - HOLD 1 s`); calibration procedure uses the step centres 215/115 (§7.3); slot mapping
`I2SO_TO_SLOT(n) = n` (§11); lower-boom reading (§7.2).

**Clarifications 1.5.2 (review of Sprint S2.1, 2026-10-12, no behaviour change — the
reference implementation already behaved like this; the tests now enforce it):**

| # | Clarification | Where |
|---|---|---|
| 1 | FAULT: `motor_stop = true` on **every** event, not only on `TICK` (R2) | `safety_logic.h` |
| 2 | WAIT_ACK: `motor_stop = true` on every event, **including a rejected ACK** (R3) | `safety_logic.h` |
| 3 | SAFE_START: `INIT_DONE` → READY **unconditionally**; `in.homed` is not checked (R5) | `safety_logic.h` |
| 4 | RUNNING: `INIT_DONE` and `TICK` → OK, also after condition loss (R7) | `safety_logic.h` |
| 5 | Outside RUNNING the result always has `hv_on`, `shutter_open`, `shutter_auto` = false, even for an inconsistent `cur` | `safety_logic.h` |
| 6 | `crcFrameVerify`: no internal size limit (frames of several kB), any `len` incl. `len < 8` without out-of-bounds access; `crc32.cpp` without static mutable state (reentrant) | `crc32.h` |
| 7 | New tests: safety +5 (`test_r2_fault_motor_stop_on_every_event`, `test_r3_motor_stop_on_every_event`, `test_r5_init_done_ready_unconditional`, `test_r7_init_done_ok`, `test_outputs_forced_off_outside_running`), types_crc +3 (`test_frame_long_body_roundtrip`, `test_frame_long_body_fake_crc_rejected`, `test_frame_verify_short_inputs`). Reference implementation: 13 suites, **282/282** | `test/native` |

**Clarifications 1.5.3 (second review of Sprint S2.1, differential test against the reference
implementation, 2026-10-12, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | R1 (fault entry) is checked **first, for every event**, including `INIT_DONE`; no rule may short-cut it. A one-shot `motor_fault` in the cycle in which homing finishes leads to FAULT | `safety_logic.h` |
| 2 | Active fault cause together with `INIT_FAILED`: the cause wins (`err` = cause, not `HOMING_FAILED`) | `safety_logic.h` |
| 3 | New tests (safety +5): `test_r1_matrix_every_event_every_cause` (4 states × 4 causes × 9 events), `test_r1_motor_fault_with_init_done`, `test_api_table_all_states_all_events` (API code and next state for all 6 × 9 combinations), `test_r4_recover_init_done_rejected`, `test_r7_init_done_applies_closing_rules`. Reference implementation: 13 suites, **287/287** | `test/native` |

**Clarification 1.5.4 (sprint plan V3, 2026-10-13, no behaviour change):** the sprint names in the
header and test comments follow the new plan (L1–L5 pure logic, P1–P2 platform, H1 hardware,
I1 integration; mapping in `docs/Sprint_Prompts_Demoanlage_ADD_Slim_V2.md`). `log_buffer` belongs
to P1 (previously contradictory: S1 / S3).

**Clarifications 1.5.5 (test hardening before Sprint L2, mutation test against the reference,
2026-10-13, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | All measure functions use the fields of the structs passed in (`SensorCurve`, `Iir`, `StabilityTracker`); no hard-coded defaults. `AcqStats` sums in double | `measure_logic.h` |
| 2 | `calibValid`: refs 40..300 inclusive; `StabilityTracker`: the reference stays the first value of the stable period | `measure_logic.h` |
| 3 | `paramsToJsonMember`: needs `cap >= length + 1` | `params.h` |
| 4 | New tests: measure +10 (struct fields, median sizes and overflow, calib boundaries, stability tolerance and reference, double sum), params +4 (all JSON fields with distinct values, exact cap, string-rule boundaries, persist untouched on every error). 43 mutants of the reference: all caught except 2 equivalent ones. Reference: 13 suites, **301/301** | `test/native` |

**Clarifications 1.5.6 (review of Sprint L2: differential test and `test_app_logic` with the L2 code,
2026-10-13, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `stabilityUpdate`: a restart sets **both** `ref = value` and `since = now`; after a jump the tracker settles at the new level. (The L2 code kept the old `ref`, so calibration never became stable – found by `test_app_logic`, not by `test_measure_logic`.) | `measure_logic.h` |
| 2 | `paramSet`: the range check uses the double value as received; 60.999999999 is out of range for 61.0..242.0 | `params.h` |
| 3 | New tests: `test_stability_settles_at_new_level_after_jump`, `test_range_check_in_double`. Reference: 13 suites, **303/303** | `test/native` |

**Clarifications 1.5.7 (test hardening before Sprint L3, mutation test against the reference,
module suites and `test_app_logic`, 2026-10-13, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `motionTick`: timeout and settle time are uint32 wrap-around safe | `motion_seq.h` |
| 2 | New tests: motion_seq +10 (config used for conversions and speeds, check order, every start clears `error_latched`, emergency stop while the driver still runs, timeout boundary and wrap, arrival by position and by settle time, `motionAcquiring`, one-shot events), sound_logic +2 (all names, full priority matrix), wifi_logic +6 (names, config used, link in BACKOFF, unconfigured from every state, link before timeout, remaining time). 72 mutants: all caught except 2 equivalent ones. Reference: 13 suites, **321/321** | `test/native` |

**Clarifications 1.5.8 (review of Sprint L3, differential test, 2026-10-13, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | Every motion start checks BUSY first, then PRECONDITION, then BAD_PARAM (the L3 code checked `homed` first: a start during homing returned PRECONDITION) | `motion_seq.h` |
| 2 | `mmToSteps` / `mmPerSecToHz` are exactly `lroundf(x * steps_per_mm)`, also for negative values | `motion_seq.h` |
| 3 | New tests: `test_busy_before_precondition_during_homing`, `test_conversions_are_lroundf`. Reference: 13 suites, **323/323** | `test/native` |

**Clarifications 1.5.9 (test hardening before Sprint L4, mutation test against the reference,
2026-10-14, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `appTick` step 2: `pos_mm = stepsToMm(drv_pos_steps, cfg.motion)`; all time limits and conversions use the `AppConfig` fields | `app_logic.h` |
| 2 | New tests in `test_app_logic` (+17, direct calls independent of the scenario): measuring chain with calibration, out-of-range reset, statistics only while acquiring with open shutter, homing failure → FAULT, fault before the driver runs, statistics reset, shutter closed during the approach, LOCAL release with config and wrap-around, heartbeat phase, PARK/traverse speeds, driver request merge, CALIB BASE/NEAR, rejected HV_ON does not arm, fault sound from a command, process image fields, position and comm timeout from the config. 72 mutants: all caught except 6 equivalent / without practical effect (documented in `docs/sprints/L4.md`). Reference: 13 suites, **340/340** | `test/native` |

**Clarifications 1.5.10 (review of L4.1b, 2026-10-14, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `appMeasure`: `in_range` only from the unfiltered `mm`; out of range `raw_mm = dist_mm = mm` (not 0) and only `stab.initialized` is reset; statistics only in `TRAV_RUN`/`MEAS_HOLD` (`motionAcquiring`), not in every non-IDLE phase. Qwen's L4.1b deviated in all four points and still passed all 80 tests | `app_logic.h` |
| 2 | New tests in `test_app_logic` (+4): `test_v1510_in_range_uses_unfiltered_mm_not_calibrated_dist`, `test_v1510_out_of_range_raw_and_dist_are_mm`, `test_v1510_out_of_range_keeps_stability_settings`, `test_v1510_stats_only_in_trav_run_or_meas_hold`. Reference: 13 suites, **344/344** | `test/native` |

**Clarifications 1.5.11 (test hardening before Sprint L5, mutation test against the reference,
2026-10-14, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | New tests (+7): zero band and rounding of floats (`-0.045` → `0.0`, `0.07` → `0.1`), order of `limit_fwd`/`limit_bwd`, `temp` sanitised, `"status":"ok"` only for OK, error response with extra members, SET number keeps double precision, leaving the ACK screen cancels a running press. 77 mutants of proc_json/api_parse/display_logic: all caught except 9 equivalent / without practical effect (documented in `docs/sprints/L5.md`). Reference: 13 suites, **351/351** | `test/native` |

**Clarifications 1.5.12 (review of L5, 2026-10-14, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `apiParseRequest`: a number parameter that is absent leaves its value at 0 (`out` fully reset). Qwen's L5.3 copied an uninitialised local into `end`/`speed` etc. | `api_parse.h` |
| 2 | `ackPressUpdate`: progress stays 1 after the ACK was sent while the ACK screen is shown (header already said so; Qwen's L5.1 returned 0 from the next poll on) | `display_logic.h` |
| 3 | New tests (+2): `test_v1512_progress_stays_1_after_sent`, `test_v1512_absent_numbers_stay_zero`. Reference: 13 suites, **353/353** | `test/native` |

**Clarifications 1.5.13 (review of L5.4, 2026-10-15, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `ackPressUpdate`: "1 after sent" is a rule of its own, not the result of clamping elapsed/hold. Qwen's L5.4 computed the progress from `now - t0` also after sending; after a release `t0` was 0, so shortly after the uint32 wrap-around the progress dropped below 1. New test `test_v1513_progress_1_after_sent_across_wraparound`. Reference: 13 suites, **354/354** | `display_logic.h`, `test/native` |

**Clarifications 1.5.14 (preparation of Sprint P1, 2026-10-15, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | Exact behaviour of the log ring (truncation, one trailing `\n` removed, Serial echo outside the critical section, `logDump` keeps the newest complete lines) | `log_buffer.h` |
| 2 | Exact NVS behaviour: types per key, read-only load, values applied with `paramSet` (same ranges as SET), raw calibration values if finite, `calibValid` check, save error rules (incl. `putString("")` returning 0), erase | `nvs_config.h` |
| 3 | `systemInit`: banner content, watchdog `esp_task_wdt_reconfigure` first, `esp_task_wdt_init` only on `ESP_ERR_INVALID_STATE`, `idle_core_mask = 0`; `systemRestart` = `ESP.restart()` | `system.h` |
| 4 | Host shims (same signatures as Arduino-ESP32 / ESP-IDF 5.x): `Serial`, `ESP`, `Preferences`, `esp_task_wdt.h`, `esp_system.h`, `esp_idf_version.h` in `test/native/freertos_shim`. New suites `test_log_buffer` (11), `test_nvs_config` (14), `test_system` (4). Mutation test: 49 mutants, all caught except 2 (NVS save without begin check: equivalent; logDump reading the ring outside the lock: only ThreadSanitizer finds it – part of the review). Reference: 16 suites, **383/383** | `test/native` |

**Clarifications 1.5.15 (review of P1, 2026-10-15, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `logDump` copies the ring text under the lock and reads only the copy. Qwen's P1.1 copied only indices and lengths and read the text afterwards without the lock (ThreadSanitizer: data race; mixed lines possible). New test `test_v1515_dump_never_mixes_lines`. Reference: 16 suites, **384/384** | `log_buffer.h`, `test/native` |
| 2 | `firmware-partial`: weak empty `setup()`/`loop()` (`src/partial_stub.cpp`, only with `-DADD_PARTIAL_BUILD`) so that the compile check links until `main.cpp` exists (Sprint I1) | `platformio.ini` |

**Clarifications 1.5.16 (preparation of Sprint P2, 2026-10-15, one addition):**

| # | Clarification | Where |
|---|---|---|
| 1 | Exact `wifi_manager` behaviour: copies of SSID/password, `WIFI_AP_STA`, no auto-reconnect of the core (the state machine reconnects), mDNS `add-demo` + `_http._tcp`, first `wifiLoop` steps at once and then every ≥ 100 ms (wrap-around safe), `DISCONNECT` keeps the AP (`disconnect(false)`), exact log lines, `StaStatus` under a portMUX | `wifi_manager.h` |
| 2 | Exact `http_server` behaviour: one static `WebServer`, CORS header on every response, file routes via `SPIFFS` and `streamFile`, `/add.html` chunked with a fixed template (`</` → `<\/` in the script block, `& < >` escaped in the `pre` block, no-cache headers), `/llms.txt` exactly 4 lines, `/log`, `/proc`, `POST /api` (body limit, heartbeat on OK/BAD_PARAM, 400 only for BAD_CRC/BAD_JSON, GET_STATE/GET_CONFIG local, everything else through the command queue with `API_REPLY_TIMEOUT_MS`), 404 body | `http_server.h` |
| 3 | **Addition:** `OPTIONS /api` → 204 with `Access-Control-Allow-Methods: GET, POST, OPTIONS` and `Access-Control-Allow-Headers: Content-Type`. Without it, a browser on another host cannot POST JSON to `/api` (CORS preflight); the Pflichtenheft only required the `Allow-Origin` header | `http_server.h` |
| 4 | Host shims with the signatures of Arduino-ESP32 3.x: `WString.h` (minimal `String`), `IPAddress.h`, `WiFi.h`, `ESPmDNS.h`, `FS.h`, `SPIFFS.h`, `WebServer.h` (records requests and responses). New suites `test_wifi_manager` (9, one ordered scenario) and `test_http_server` (17, a thread plays the control task, parallel `/proc` test with self-checking data). Mutation test: 47 mutants, all caught except 1 (`wifiStaStatus` reading without the lock: only ThreadSanitizer finds it – part of the review); 3 survivors of the first run led to the tests `test_step_gap_near_wraparound`, `test_v1516_slow_reply_within_timeout_is_ok` and a first `wifiLoop` 40 ms after boot. Reference: 18 suites, **410/410**, `test_wifi_manager`/`test_http_server` also clean under ThreadSanitizer | `test/native` |

**Clarifications 1.5.17 (review of P2, 2026-10-15, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `wifiLoop`: the critical sections contain only the copy of the `StaStatus`; `WiFi.status()` is read before, `staStep` runs on the local copy, no `WiFi.*`/`logLine` inside a critical section. Qwen's P2.1 called `WiFi.status()` and `staStep` with interrupts masked (on the ESP32: FreeRTOS/lwIP calls inside a spinlock section). The WiFi shim now counts calls made inside a critical section; new test `test_v1517_no_wifi_call_inside_critical_section` | `wifi_manager.h`, `WiFi.h`, `test/native` |
| 2 | `/add.html`: the JSON is sent block-wise (at most 2 `sendContent` calls per started 64 bytes); a `<` as the last byte is sent too. Qwen's P2.2 sent every character as its own chunk (on the device: one TCP write per byte, ~8 000 writes for 4 kB) and lost a trailing `<`. The WebServer shim counts chunks; new test `test_v1517_add_html_sends_blocks_not_bytes` | `http_server.h`, `WebServer.h`, `test/native` |
| 3 | `POST /api`: `BAD_PARAM` is an error response also for `GET_STATE`/`GET_CONFIG` (Qwen's P2.3 answered them locally with OK, e.g. `"params":[1]`; found by the differential test). New test `test_v1517_bad_param_is_an_error_also_for_get_state_and_get_config` | `http_server.h`, `test/native` |
| 4 | Shim correction: `shimCritEnters()` is atomic (two tasks entering different critical sections raced on the counter; ThreadSanitizer reported it also for the reference – the statement "clean under ThreadSanitizer" in 1.5.16 was based on a truncated output). Reference: 18 suites, **413/413**, `test_wifi_manager`/`test_http_server` now really clean under ThreadSanitizer | `shim_core.h` |

**Clarifications 1.5.18 (preparation of Sprint H1, 2026-10-15, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | Exact `hal_io` behaviour: order in `halInit` (outputs LOW first, inputs, ADC read before 11 dB attenuation, buzzer bit off, engine stored), inputs fail-safe (active) until the first `halUpdateInputs`, first sample taken directly, debounce rule (continuous difference ≥ `INPUT_DEBOUNCE_MS`, wrap-safe), E-stop activation immediate, polarity change restarts a pending release, each input read once per update, output cache "unknown" after `halInit`, buzzer only with an engine | `hal_io.h` |
| 2 | Exact `motion_drv` behaviour: init sequence as verified in smoketest 0a (all 32 mux slots cleared, driver enable HIGH = off, then connect/dir/enable/auto-enable/acceleration), failure at each step, `motionDrvEngine()` only after a working mux, exact library calls per request (no extra checks, repeated FORCE_STOP executed every call), speed = abs(realtime mHz) / 1000 / `STEPS_PER_MM` | `motion_drv.h` |
| 3 | Host shims: GPIO/ADC functions in `Arduino.h` (`pinMode`, `digitalRead/Write`, `analogReadMilliVolts`, `analogSetPinAttenuation`) and `FastAccelStepper.h` (signatures of master 96459ed, the pinned version). Every hardware access is recorded in call order (`shimHwLog`). New suites `test_hal_io` (16) and `test_motion_drv` (10). Mutation test: 64 mutants, 3 survived the first run (output cache after a second `halInit`, first setter call with `true`, engine taken at call time) → `test_setters_write_again_after_halInit` and an extra check; afterwards all 64 caught. Reference: 20 suites, **439/439**, also under ThreadSanitizer | `test/native` |
| 4 | On-device test `test_nvs_roundtrip` restores the stored parameters (WLAN credentials, calibration, `estop_nc`) instead of writing the defaults | `test/embedded` |

**Clarifications 1.5.19 (first on-device test of H1, 2026-10-15, no behaviour change):**

| # | Clarification | Where |
|---|---|---|
| 1 | `test_hardware` crashed in `test_nvs_roundtrip` (Guru Meditation "Cache disabled but cached memory region accessed", PC in `StepperQueue::fill_i2s_buffer`). Cause in FastAccelStepper 96459ed: the I2S TX-done interrupt runs permanently with I2S_MUX and is IRAM-safe, but `fill_i2s_buffer()` and `I2sManager::i2sMuxSetBit()` were in flash; every flash operation (NVS write, SPIFFS read, OTA) crashed the board, also with the motor idle. Fix: both functions `IRAM_ATTR` (fork `norbert-walter/FastAccelStepper`, commit a64ceff on 96459ed; pull request to gin66 pending). Checked with nm/objdump: every function reachable from the interrupt is in IRAM, no literals into flash | `platformio.ini` |
| 2 | Post-build check `scripts/check_iram.py` (all ESP32 environments): the build fails if one of the I2S interrupt functions is linked into flash (protects against library updates). Verified: fails with 96459ed, passes with a64ceff | `platformio.ini`, `scripts/` |
| 3 | The nine native H1 tests could not see this (no flash cache on the host); the first five on-device tests were green (inputs, sensor, outputs, E-stop polarity change) | – |

**Changes 1.5.25 (I1 device test part 2c, results 7–14, 2026-10-17, decision Norbert Walter):**

| # | Change | Where |
|---|---|---|
| 1 | **Idle HTTP connections are closed after `HTTP_IDLE_DROP_MS` (500 ms).** The Arduino `WebServer` serves one connection at a time and waits up to 5 s (`HTTP_MAX_DATA_WAIT`, fixed in the core) for a connected client to send its request; a browser tab on the device opens spare connections and sends nothing on them, so the heartbeat of the REST client was blocked for 5 s and more and COMM_TIMEOUT fired (tests 9 and 11). `AddWebServer` (subclass, uses the protected members `_currentStatus`, `_statusChange`, `_currentClient`) drops such a connection in `httpServerLoop()` before `handleClient()`; `/log`: `http: idle connection closed after 500 ms (<n> so far)`, at most one line per `HTTP_IDLE_LOG_MS` (10 s) | `http_server.cpp`, `config.h`, WebServer shim, `test_http_server` (+1) |
| 2 | **Net task diagnostics:** `/log` line `net: <part> <ms> ms` when the console, `wifiLoop`, `httpServerLoop` or the NVS write takes longer than `NET_SLOW_LOG_MS` (200 ms), and `net: gap <ms> ms` when the time between two steps is longer (net task starved on core 0) | `net_task.cpp`, `test_net_task` (+1) |
| 3 | **ADC: one sample per control cycle** instead of 16 in one cycle every 20 ms (about 0.7 ms, over 1 ms with WLAN load → `overruns`; measured: `cycle_max_us` up to 3039, +600 overruns per 200 requests). The measurement is still evaluated every 20 ms with `medianMean` over the samples collected since the last evaluation (normally 16, at least 9) | `control_task.cpp` (§10 step 1) |
| 4 | **CycleStats count from `boot: ready` on** (`controlResetCycleStats()` in `setup()`); before, start-up effects (display init, WLAN start, flash) already gave 141 overruns | `control_task.h/.cpp`, `main.cpp`, `test_net_task` |
| 5 | `WiFi.persistent(false)`: the WiFi driver no longer writes the station config to flash on every `WiFi.begin()` (the credentials are in our own NVS keys); a flash write stops the other core for milliseconds | `wifi_manager.cpp`, WiFi shim, `test_wifi_manager` |
| 6 | `scripts/stress_api.py`: `parallel` 4 threads (listen backlog of the server is 4) and 10 s timeout; every test judges the overruns **during** the test; `cycle_max_us` < 800; `latency --prepare` (INIT, wait for READY, HV_ON, at the end HV_OFF). `add_client.py shell`: `hb` alone shows sent / answered / failed / longest time without an answer. `fake_device.py` answers GET_STATE with CycleStats | `scripts/` |
| 8 | **Control cycle diagnostics** (after the first device run of V1.5.25: overruns down from about 600 to 6 per 200 requests, `cycle_max_us` 1215, `period_max_us` 2357): the control task keeps the breakdown of the last overrun or late start (period > `CYCLE_DIAG_PERIOD_US` 1500) in `CycleDiag` (adc, tick, cmd, out, pub, period); the net task (step 2b) logs `ctrl: cycle <us> us, period <us> us (adc …, tick …, cmd …, out …, pub …), <n> events`, at most one line per `CYCLE_DIAG_LOG_MS` (5 s) | `control_task.h/.cpp`, `net_task.cpp`, `config.h`, `test_net_task` (+1) |
| 9 | `stress_api.py latency`: traverse with `traverse_start` / `traverse_end` from GET_CONFIG (the old fixed start 60 mm is outside the measuring zone → BAD_PARAM), waits for the axis to be idle (GET_STATE as heartbeat) before every START_TRAVERSE; `--prepare` skips INIT when homed and always ends with STOP + HV_OFF (before: HV stayed on after an abort → COMM_TIMEOUT). `fake_device.py` checks the parameter ranges of START_TRAVERSE / START_MEASURE | `scripts/` |
| 7 | Unchanged by decision: TEMP_HIGH stays latching (`ACK` via the API, after `temp` ≤ 32); `/add.html` answers 404 with `{"error":"not found"}` until `add.json` exists (Sprint A1) | – |

**Changes 1.5.24 (I1 device test part 2c, 2026-10-17, decision Norbert Walter):**

| # | Change | Where |
|---|---|---|
| 1 | **`LOCAL_HOLD_MS` in `config.h`, default 0 = LOCAL priority off**; 1 … 60000 ms = hold time (static_assert). `AppConfig::local_hold_ms` = 0: a display command still disarms the watchdog but never sets `local`. Found on the device: after ACK and INIT at the display the API was blocked for minutes (correct per V1.1, but not wanted for the demo) | `config.h`, `control_task.cpp`, `app_logic.h/.cpp`, `test_app_logic` (+1) |
| 2 | **WiFi modem sleep off** (`WiFi.setSleep(false)` right after `WiFi.mode`). Measured before: 75 … 140 ms per request, single requests over 3 s; the 5 s watchdog fired twice although the client sent GET_STATE every 2 s | `wifi_manager.h/.cpp`, WiFi shim, `test_wifi_manager` |
| 3 | Watchdog logic **unchanged** (HV_ON stays a monitored command – HV is a lasting state). New: diagnostics in the network task (step 2a): `/api` gaps > `COMM_GAP_LOG_MS` (2 s) while armed are logged, and the COMM_TIMEOUT edge with the age of the last packet and the longest gap | `net_task.h/.cpp`, `test_net_task` (+1) |
| 4 | `scripts/add_client.py shell`: sends GET_STATE every second from the start, in the background (commands can be typed at any time); message after 10 s without a GET_STATE answer and when answers resume; `hb off/on` (revised after the device test: the coupling to `/proc` `comm_armed` was dropped); `fake_device.py` simulates the watchdog | `scripts/` |

**Changes 1.5.23 (I1 device test part 2, 2026-10-17, decision Norbert Walter):**

| # | Change | Where |
|---|---|---|
| 1 | **Safety flags after every boot `interlock = true`, `cooling_flow = true`** (`temp` 18.0 unchanged; was false/false "fail-safe"). Reason: HV, measuring and profile runs were not reachable at the display without a remote client, so the complete function could not be tested locally. Faults are still simulated with `SET` (flags false, temp > 32). The flags stay non-persistent | `params.h` (`SafetyFlags`), §9, `test_params` (3 expectations), `test_app_logic` (3 tests set the flags false first, `test_hv_on_needs_flags` checks `hv_ok` after boot) |
| 2 | **Serial console** in the network task (step 0 of `netTaskStep`): `help`, `ver`, `status`, `reboot`, `wifi <ssid> <password>`, `wifi "<ssid>" <password>`, `wifi off`; one JSON line per answer; SET/REBOOT through the command queue like the API (one writer for the parameters); password never echoed; at most `CONSOLE_READ_MAX` bytes and one command per step, lines up to `CONSOLE_LINE_MAX` | `net_task.h/.cpp`, `config.h`, shim `Serial` input, `test_net_task` (+7) |
| 3 | `scripts/serial_cmd.py` waits for `boot: ready` after opening the port (USB-serial adapters reset the board on open); `scripts/add_client.py shell`: interactive REST console with automatic heartbeat | `scripts/` |

**Additions 1.5.22 (preparation of Sprint W1 – web UI, 2026-10-17):**

| # | Addition | Where |
|---|---|---|
| 1 | **Web UI split into a tested logic module and two pages:** `data/www/app.js` (pure functions, no DOM/fetch/timers, global `ADDAPP` in the browser, `require` in node), `index.html` (process view), `settings.html`; style sheet `style.css` and the Ubuntu fonts written/provided by the reviewer. Exact rules in §5.5 and in the comments of `app.js` | `data/www/*` |
| 2 | **Six more static routes** in `http_server`: `/app.js`, `/style.css`, `/font-u400.woff2`, `/font-u700.woff2`, `/font-m400.woff2`, `/font-m700.woff2` (fonts `font/woff2`, `Cache-Control: max-age=86400`). SPIFFS names are limited to 31 characters, so the fonts lie in `/www/` directly (Pflichtenheft: `/www/fonts/`). Fonts: Ubuntu / Ubuntu Mono latin 400/700 from fontsource 5.3.0, Ubuntu Font Licence 1.0 (`data/www/LICENCE-ubuntu-font.txt`) | `http_server.h/.cpp`, `test_http_server` (+1) |
| 3 | Tests in JavaScript with node's built-in runner (node ≥ 18, no packages): `test/js/test_app.js` (27 tests, logic of `app.js`) and `test/js/test_pages.js` (10 static checks: frozen markup and helpers by SHA-256, script compiles, no own protocol code, only device URLs, ids). Behaviour of the pages: reviewer check in Chromium against `scripts/fake_device.py` (simulated `/proc`, `/api`, `/add`). Mutation test `app.js`: 66 mutants, 62 caught, 4 equivalent | `test/js`, `scripts/fake_device.py` |

**Correction 1.5.21 (first flash of the complete firmware, 2026-10-16):**

| # | Correction | Where |
|---|---|---|
| 1 | **Backlight is active LOW** on the MKS TS35R at the DLC32 (GPIO 5 LOW = light on). `Light_PWM` `invert = true`. Found on the device: with `setBrightness(255)` the light went off right after the first picture; the smoketest (0a, section 6) never called `setBrightness` and ran at LovyanGFX's default 50 % duty, which is 50 % light in either polarity. Not visible in host tests (the shim has no backlight model). Verified on the device by Norbert: display on right after boot | `src/display/lgfx_config.h` |
| 2 | **Position bar mirrored**, as seen from the front of the gauge: axis 0 (home switch) on the RIGHT, service position right of centre-right, park at the right edge. `bx(mm) = 472 - (int)(clamp(mm, 0, 242) / 242 * 468)`; zones drawn from `bx(upper) + 4` to `bx(lower) + 4`. The web pages (Sprint W1) use the same direction | `display.h`, `display.cpp` (`barX`, `drawBar`), `test_display` |
| 3 | **Header:** state text `[ <name> ]` at x = **112** (was 150). `[ SAFE_START ]` (14 characters, 168 px) reached x = 318 and overwrote "LOCAL" (x = 288); now it ends at 280. `test_display` checks for every normal-screen state that the state text and "LOCAL" are both visible and do not overlap | `display.h`, `display.cpp` (`drawHeader`), `test_display` |

**Clarifications 1.5.20 (preparation of Sprint I1, 2026-10-16, two additions):**

| # | Clarification | Where |
|---|---|---|
| 1 | **Addition:** `src/app/net_task.h` (`netTaskStep`, `netTaskStart`): network task with fixed order (wifiLoop, handleClient, persist, reboot), reboot pending flag (wrap-safe, due time 0 possible), persist before reboot in the same step; exact `setup()` order in `main.cpp`, the device keeps running after a failed step (logged). Config: `NET_TASK_PERIOD_MS` 5, `DISPLAY_TASK_CORE/PRIO/STACK` 1 / 1 / 6144 | `net_task.h`, `config.h` |
| 2 | **Addition:** `displayTick(now)` in `display.h` plus exact screen layout, colours (RGB888 constants), texts and touch rules: one `getTouch` per tick; the picture is redrawn at once on a mode change BEFORE the touch is evaluated (a tap on a just-hidden button can never hit the new layout); zone cache – an unchanged snapshot issues no draw call; buttons only on a touch-down edge in NORMAL; HV toggles by `hv_on`; ACK via `ackPressUpdate` on every tick | `display.h` |
| 3 | Deviations from Pflichtenheft §5.8 (layout only): flags on two lines (`ESTOP`/`TEMP(SIM)`, `INTERLOCK(SIM)`/`COOLING(SIM)`), `ERR` not on the normal screen (a fault always shows the fault screen), no visual tap feedback (the control task's CLICK sound confirms an accepted button), DIST/THICK `---` while no valid measurement | `display.h` |
| 4 | Host shims: `LovyanGFX.hpp` (screen model: pixel colours and visible texts with boxes, touch), task creation (`xTaskCreatePinnedToCore` records, `vTaskDelay` counts), `SPIFFS.begin` result. New suites `test_net_task` (10) and `test_display` (15). Mutation test: 50 mutants, 5 survived the first run → 4 extra checks (STOP from x = 384, touch outside the ACK area, RUNNING colour, shutter colour independent of HV); remaining 1 equivalent (FAULT row cleared twice). Full firmware (`pio run -e firmware`) with the reference: RAM 20.7 %, flash 1.23 MB of 2.9 MB, `check_iram` 5/5. Reference: 22 suites, **464/464** | `test/native` |

**Scope note:** plant coordinates for several gauges (offset/sign transformation at the
API boundary, common time base) are an extension idea for later (Pflichtenheft §11.1);
they do not touch the safety logic.

### Changes 1.4.1 → 1.4.2 (smoketest 0a section 5 "distance sensor", 2026-10-11, §14: hardware measurement)

| # | Change | Where |
|---|---|---|
| 1 | Sensor wiring: **11 kΩ load Vo–GND** and 47 µF Vcc–GND directly at the sensor (R20 stays removed). Without a load the emitter-follower output cannot discharge the input; after the first ADC samples the reading hung at full scale (3145 mV) | Pflichtenheft §3 |
| 2 | Sensor curve **`168.8 · V^-1.398`** fitted to the six step centres with this wiring (max. error 0.48 mm; the V1.2 curve read 2 … 9 mm too short) | §7.1, `measure_logic.h` |
| 3 | Sensor range **35 … 300 mm** (was 40): the 40 mm step reads 40.5 mm and must not sit on the limit | §5.1, §7.1, `measure_logic.h` |
| 4 | Stair body **105 … 225 mm**, step centres 115, 135 … 215 mm (65 … 40 mm); **`measure_pos` 155** (145 was the edge between two steps) | §2, §9, `params.h` |
| 5 | Tests: new curve and measured points, range limit, simulated stair geometry in `test_app_logic` (start position axis 110 = step 1, measuring position 155) | `test/native` |

### Additions 1.4.1 (2026-10-11): nonsense values are tested

Every float input must be **finite**. NaN or ±Inf → `BAD_PARAM` in `motionStart*`
(range checks written as `!(lo <= x && x <= hi)`, because `x < lo || x > hi` lets NaN
through and `lroundf(NaN)` gives an arbitrary target), in `paramSet` and in
`apiParseRequest` (a JSON number such as `1e39` becomes +Inf as float; `NaN` /
`Infinity` literals are not JSON → `BAD_JSON`). New tests:
`test_non_finite_inputs_rejected` (motion_seq), `test_numbers_must_be_finite` (api_parse),
`test_nan_rejected` extended to ±Inf for all numeric keys (params),
`test_non_finite_command_values_rejected` (app_logic, end to end: the axis does not move).
Reference implementation re-run: 13 suites, **263/263**; mutants with a naive range
check or without the finiteness check are caught.

### Changes 1.3 → 1.4 (smoketest 0a 2026-10-11, §14: hardware measurement)

| # | Change | Where |
|---|---|---|
| 1 | Rack **40.77 mm per revolution** (measured; 42 mm was an estimate) → `STEPS_PER_MM` = 78.49; `MotionConfig::steps_per_mm` default 3200 / 40.77 | §2, §8, `config.h`, `motion_seq.h` |
| 2 | Axis 0 … **242 mm**: forward switch measured at axis 244.3 mm (246.3 mm from the backward switch), 2.3 mm margin; cannot be moved mechanically (cable chain) | §2, §5.2, §8, §9, `motion_seq.h`, `params.h` |
| 3 | Parameter ranges `measure_pos`, `traverse_start`, `traverse_end`: 21 … **242** mm (defaults unchanged) | §9 |
| 4 | `DIR_POSITIVE_HIGH` = **false** (measured) | `config.h` |
| 5 | Driver `FORCE_STOP` → `forceStopAndNewPosition(getCurrentPosition())` instead of `forceStop()`: FastAccelStepper drops a `forceStop()` that arrives while a newer move is still pending (incident in the smoketest). The per-cycle repetition of `FORCE_STOP` (§10, step 5) stays mandatory | §11, `motion_drv.h` |
| 6 | Tests: `fake_axis` forward switch at 19 334 steps (measured), conversions and boundaries for 78.49 steps/mm and 242 mm; reference implementation re-run (260/260) | `test/native` |

### Changes 1.2 → 1.3

| # | Change | Where |
|---|---|---|
| 1 | **All speeds in mm/s**: range 1 … 20 mm/s, normal speed 10 mm/s (MOVE/PARK/SERVICE default, approach, traverse default), homing 5 mm/s, homing timeout 70 s; `mmPerSecToHz`; `/proc` `vel` in mm/s | §2, §5, §8, §9 |
| 2 | E-stop wiring is the persisted parameter **`estop_nc`** (default `false` = simple switch, active LOW, no wire-break detection); settable only while idle with HV off | §9, §11 |
| 3 | Buzzer patterns (active buzzer, rhythm only) from the pure `sound_logic.h`: BOOT, REBOOT, CLICK, HOMED, FAULT, ESTOP (4 s) | §11.1 |
| 4 | Heartbeat: blue board LED GPIO 2, 0.5 Hz, driven by the control task | §11 |
| 5 | WiFi: AP always on + router connection (station) with 3 attempts × 10 s, then 2 min pause, forever; pure `wifi_logic.h` | §12.1 |
| 6 | REBOOT: the restart happens `REBOOT_DELAY_MS` (700 ms) after the request, after the REBOOT pattern | §5.2, §12 |
| 7 | NVS schema version 3 | §12 |

**Clarifications 1.3.1 (review, no behaviour change):** per-cycle command limit in §1;
exact snapshot semantics of `cmdQueueReceivePriority` (§6.7); E-stop decoding when the
polarity changes (§11); HAL writes outputs only on change (§11); version markers of all
frozen files unified to V1.3; `scripts/verify_reference.sh` reproduces the native test run.

### Changes 1.1 → 1.2 (hardware pre-test 2026-10-09, CONTRACT §14)

| # | Change | Where |
|---|---|---|
| 1 | Axis 0 … **248 mm** (forward switch at 250 mm) | §2, §5.2, §8 |
| 2 | Park zone **≤ 20 mm**; park position 2 mm; new command **`PARK`** | §2, §5.2, §6 |
| 3 | Service zone 20 … 60 mm; service position `maint_pos` (default 40, 21 … 60); new command **`SERVICE`**; manual `SHUTTER_OPEN` **only in the service zone**; automation uses the new event `SHUTTER_AUTO_OPEN` | §2, §5.2, §6.1, §6.4, §9 |
| 4 | Sensor curve fitted to the demo: `121.5 · V^-1.142` (max. 0.6 mm error at 40 … 65 mm) | §7.1 |
| 5 | Thickness reference `base_dist` = 70 mm (virtual plane at the material edge); calibration defaults 40/65 mm | §7.2, §7.3, §9 |
| 6 | Defaults: `measure_pos` 145, `traverse_start/end` 60/240 | §9 |
| 7 | HV LED on **GPIO 4** (no longer an I2S bit) | §11 |
| 8 | Display: 7 buttons `INIT · HV · MEAS · TRAV · PARK · SERV · STOP` | §13 |
| 9 | NVS schema version 2 | §12 |


### Changes 1.0 → 1.1

| # | Change | Where |
|---|---|---|
| 1 | LOCAL operator priority: display commands block remote start commands (`LOCAL_CONTROL = -10`), `/proc` field `local` | §3.3, §5.1, §6.6 |
| 2 | Priority queue for STOP / HV_OFF / SHUTTER_CLOSE; executed even in a cycle with a pending driver request; STOP latency ≤ 2 cycles | §1, §6.7, §10 |
| 3 | `/api` reply is sent **after** the outputs were applied | §5.2, §10 |
| 4 | Cycle time monitoring: `cycle_max_us`, `overruns` in the GET_STATE reply | §5.2, §10 |
| 5 | Fault screen: all buttons hidden, one full-width ACK area, 1 s hold, touch-down edge required (`display_logic.h`) | §13 |
| 6 | BUSY retry rule, simulated-flag marking, safety disclaimer | §5.2, §6.8, §6.9 |
| 7 | `cmdQueueSendAndWait` serialised by a mutex; a priority command is still queued if the mutex is busy | §1, §6.7 |
| 8 | Device-side timing: `period_max_us` (jitter) and `prio_lat_max_us` (priority command enqueue → outputs applied); `CycleStats`, `Command.t_enq_us` | §5.2, §10 |
| 9 | Overflow latch for priority commands (cannot overflow, coalesces identical requests, origin API/LOCAL stored per command); host integration test of the real `command_queue.cpp` with threads (`test_command_queue`) | §6.7, §1 |
| 10 | **All** pending priority commands are executed in the same cycle (`cmdQueueReceivePriority`, max. 7) – the 2 ms bound holds independent of how many are pending | §6.7, §10 |

This document is the single source of truth for every agent and every sprint.
The headers in `src/**/*.h` are the machine-readable part of this contract; this
file explains them. If a header and this file disagree, **stop and report** – do
not choose yourself.

---

## 1. Architecture

```
            Core 0                                   Core 1
 ┌─────────────────────────────┐        ┌─────────────────────────────────────┐
 │ network task                │        │ control task (1 kHz, prio high)     │
 │  wifiLoop()                 │ Command│  readInputs() ── HAL, motion_drv    │
 │  httpServerLoop() ──────────┼──queue─┼─► appTick() / appCommand()  (logic)│
 │  persist / reboot requests  │◄─reply─┤  applyOutputs(): shutter→motor→HV  │
 │  GET /proc ◄── snapshotGet()│        │  appFillProc() → snapshotPublish()  │
 └─────────────────────────────┘        ├─────────────────────────────────────┤
                                        │ display task (5 Hz, prio low)       │
                                        │  snapshotGet() → draw; touch → queue│
                                        ├─────────────────────────────────────┤
                                        │ FastAccelStepper task (library)     │
                                        └─────────────────────────────────────┘
```

**Single-writer principle.** Only the control task changes application state.
Every other task communicates through exactly two channels:

| Channel | Direction | Implementation (frozen, complete) |
|---|---|---|
| Command queues | HTTP / display → control | `src/app/command_queue.*` (FreeRTOS priority queue + overflow latch + normal queue; per cycle: **all** pending priority commands (max. 7) + max. **1** normal command, §6.7, §10) |
| Snapshot | control → HTTP / display | `src/app/snapshot.*` (copy of `ProcData` under one spinlock) |

No other shared mutable globals exist. Nobody but snapshot/command_queue uses
`taskENTER_CRITICAL`.

**Replies to concurrent callers.** Only callers of `cmdQueueSendAndWait()` get a
reply. That function is serialised by a mutex: one caller waits at a time, so a
caller can never consume or drop another caller's reply. With the Arduino
`WebServer` (one request at a time) there is only one caller anyway; the mutex
keeps the rule true if the server is ever replaced. If the mutex is not free in
time the result is `BUSY`; a priority command is nevertheless queued or latched
(no reply). Verified on the host by `test_command_queue` (8 threads, real
`command_queue.cpp`, FreeRTOS shim): without the mutex, replies are not mixed
up (sequence check) but most of them are lost (`BUSY`).

**Pure logic.** Everything that decides anything lives in `src/logic/` as pure
C++17 functions without Arduino/ESP-IDF/FreeRTOS. It is tested on the host
(`pio test -e native`), including complete scenarios with a simulated axis.

| Layer | Files | Tested by |
|---|---|---|
| Logic (pure) | `src/logic/*` | `test/native/*` (frozen) |
| Glue | `src/app/*` | review + `test/embedded` |
| Hardware | `src/hal/*`, `src/system/*`, `src/comm/*`, `src/display/*` | smoketest, `test/embedded`, acceptance |

---

## 2. Units and conventions

- Positions: **mm** in API, JSON and logic interfaces; **steps** inside motion_seq / driver.
  `STEPS_PER_MM = 200 · 16 / 40.77 = 78.49` (TMC2209 jumper 1/16; 40.77 mm per revolution measured, V1.4).
- Speeds: **mm/s** in API, parameters, `/proc` and display (V1.3); **steps/s (Hz)** towards the driver.
  Range 1 … 20 mm/s; normal speed 10 mm/s; homing 5 mm/s.
- Time: `uint32_t` milliseconds (`millis()`), all comparisons wrap-around safe.
- Position 0 = 2 mm in front of the backward end switch (after homing). Axis 0 … 242 mm
  (forward end switch measured at 244.3 mm, V1.4).

| Zone (axis mm) | Range | Shutter | Named position |
|---|---|---|---|
| Park zone | 0 … 20 | always closed | park position 2 mm (`PARK`) |
| Service zone | > 20 … 60 | **only manual** `SHUTTER_OPEN`; an automatic shutter closes when entering | `maint_pos`, default 40 mm (`SERVICE`), settable 21 … 60 |
| Measuring zone (V1.5) | > 60 … 242 | **only the automation** (during a measuring programme) | `measure_pos` 155, `traverse_start/end` 80/240 (settable 61 … 242) |
| – material (stair body) | 105 … 225 (six 20 mm steps centred at 115 … 215, sensor distance 65 … 40 mm) | – | measuring position 155 mm (centre of step 3) |
| Forward end | 242 | – | forward switch at 244.3 mm (measured) |

The zones are defined in **machine coordinates**; the origin is always on the park
side (homing switch). They are fixed constants (`SafetyConfig`, `MotionConfig`), not
settable via the API. A mirrored installation of the measuring frame is handled
during commissioning (`DIR_POSITIVE_HIGH`, homing switch) – never by mirroring zones.

---

## 3. Types and names (`src/logic/types.h`)

### 3.1 Safety states

| State | Meaning | HV | Shutter |
|---|---|---|---|
| `SAFE_START` | boot, or after recovery without valid homing; `INIT` required | off | closed |
| `READY` | homed, ready | off | closed |
| `RUNNING` | HV on | **on** | may be open |
| `FAULT` | fault detected this cycle (lasts one cycle) | off | closed |
| `WAIT_ACK` | waiting for `ACK` | off | closed |
| `RECOVER` | `ACK` accepted (one cycle) → `READY` (homed) or `SAFE_START` | off | closed |

### 3.2 System error codes (`/proc` field `err`, latched until ACK)

| Code | Name | Cause |
|---|---|---|
| 0 | `NONE` | – |
| 1 | `ESTOP` | E-stop pressed or E-stop wire broken |
| 2 | `TEMP_HIGH` | simulated cooling water temperature > 32.0 °C |
| 3 | `COMM_TIMEOUT` | remote operation active and no `/api` packet for > 5 s |
| 4 | `MOTOR_FAULT` | end switch hit unexpectedly during motion |
| 5 | `HOMING_FAILED` | homing timeout (70 s) or forward switch during homing |

### 3.3 API result codes (`/api` response field `err`)

| Code | Name | Meaning |
|---|---|---|
| 0 | `OK` | accepted / executed |
| -1 | `BAD_CRC` | CRC frame missing or wrong |
| -2 | `BAD_JSON` | body is not a JSON object |
| -3 | `UNKNOWN_CMD` | `cmd` missing or unknown |
| -4 | `BAD_PARAM` | parameter missing, wrong type or out of range |
| -5 | `STATE` | command not allowed in the current safety state (§6.3) |
| -6 | `PRECONDITION` | allowed in this state, but a condition is not met (e.g. HV flags, park zone, not homed) |
| -7 | `BUSY` | motion sequence active, queue full or reply timeout |
| -8 | `NOT_STABLE` | calibration capture: signal not stable for 500 ms |
| -9 | `CALIB_INVALID` | calibration points invalid |
| -10 | `LOCAL_CONTROL` | V1.1: the operator at the display has priority (§6.6); the device is fine |

---

## 4. CRC32 frame (`src/logic/crc32.h`)

CRC32 = IEEE 802.3 / zlib (`"123456789"` → `CBF43926`, `""` → `00000000`).
The firmware uses its own implementation (`crc32Ieee`) on host and target; the
smoketest compares it with `esp_rom_crc32_le`.

**Rule (requests, responses and `/proc`):**

1. Build the JSON object text **without** the crc field, compact:
   `{"cmd":"HV_ON","params":{}}`
2. CRC32 over exactly these bytes → `88F7C802`
3. Insert `,"crc":"88F7C802"` directly before the final `}`:
   `{"cmd":"HV_ON","params":{},"crc":"88F7C802"}`
4. Receiver: take the text before the **last** `,"crc":"`, append `}`, compute,
   compare (hex case-insensitive). Trailing whitespace after `}` is allowed.

No JSON re-serialisation is ever involved. Reference implementations:
C++ `src/logic/crc32.cpp`, Python `scripts/add_client.py`, JavaScript in the web pages:

```js
function crc32(str){let c,crc=0xFFFFFFFF;const b=new TextEncoder().encode(str);
 for(const x of b){c=(crc^x)&0xFF;for(let k=0;k<8;k++)c=c&1?(c>>>1)^0xEDB88320:c>>>1;crc=(crc>>>8)^c;}
 return((crc^0xFFFFFFFF)>>>0).toString(16).toUpperCase().padStart(8,'0');}
function frame(obj){const body=JSON.stringify(obj);return body.slice(0,-1)+',"crc":"'+crc32(body)+'"}';}
function verify(t){t=t.trimEnd();const i=t.lastIndexOf(',"crc":"');
 return i>=0&&t.length===i+18&&t.endsWith('"}')&&crc32(t.slice(0,i)+'}')===t.substr(i+8,8).toUpperCase();}
```

Test vectors (frozen, used in all three languages):

| Body without crc | CRC |
|---|---|
| `{"cmd":"HV_ON","params":{}}` | `88F7C802` |
| `{"cmd":"SET","params":{"key":"temp","value":35.0}}` | `C93F2D33` |
| `{"cmd":"SET","params":{"key":"temp","value":35}}` (what `JSON.stringify` produces) | `F2B0BAC5` |

The CRC always covers the text that is actually sent, so different but valid
number formatting (`35` vs `35.0`) is harmless.

---

## 5. HTTP interface

### 5.1 `GET /proc` (≤ 512 bytes incl. frame, `Cache-Control: no-store`)

Exact member order, one line, floats with one decimal:

```json
{"ts":123456,"state":"RUNNING","err":0,"local":false,
 "motion":{"pos":120.0,"vel":10.0,"state":"moving","mode":"traverse","homed":true,"limit_fwd":false,"limit_bwd":false},
 "measure":{"dist":187.7,"thick":12.3,"on_material":true,"valid":true,"in_range":true,"avg":12.3,"n":143},
 "safety":{"estop":false,"hv_on":true,"hv_ok":true,"shutter":"open","interlock":true,"cooling_flow":true,"temp":23.0,"temp_ok":true,"comm_armed":true},
 "crc":"XXXXXXXX"}
```

| Field | Meaning |
|---|---|
| `state` | safety state (§3.1) |
| `err` | system error code (§3.2) |
| `local` | V1.1: LOCAL operator priority active (§6.6) – remote start commands are rejected |
| `motion.state` | `idle` / `moving` / `homing` / `error` (error = last sequence ended with fault) |
| `motion.mode` | `none` / `measure` / `traverse` |
| `motion.homed` | axis has a valid reference |
| `measure.dist` | calibrated distance sensor → surface, mm |
| `measure.thick` | `base_dist − dist`, never negative |
| `measure.on_material` | `thick > edge_threshold` |
| `measure.valid` | **only true while the shutter is open** (simulates: no radiation → no measurement). V1.5: also true during the shutter hold time after a run – profile clients use samples with `mode == "traverse"` |
| `measure.in_range` | sensor reading within 35 … 300 mm (V1.4.2) |
| `measure.avg`, `n` | mean thickness over valid on-material samples of the current/last acquisition |
| `safety.hv_ok` | HV enable conditions met right now |
| `safety.temp_ok` | `temp ≤ 30.0 °C` |
| `safety.comm_armed` | communication watchdog active (§6.5) |

### 5.2 `POST /api`

Request: `{"cmd":"<NAME>","params":{...},"crc":"XXXXXXXX"}` – `params` optional.
Response (HTTP 200; 400 only for `BAD_CRC`/`BAD_JSON`):

```json
{"status":"ok","err":0,"cmd":"HV_ON","crc":"XXXXXXXX"}
{"status":"error","err":-6,"cmd":"HV_ON","msg":"precondition not met","crc":"XXXXXXXX"}
```

**Meaning of `OK` (V1.1):** the control task accepted the command **and the
outputs of that cycle are already set** (HV LED, shutter, driver request). The
reply is sent after `applyOutputs()`, typically within 2 ms. Long operations
(homing, traverse) continue asynchronously – their **end is only visible in
`/proc`** (`motion.state`, `state`).

**BUSY retry rule (V1.1).** `BUSY` means queue full or no reply within 100 ms.
The command **may still be executed later**. A client must therefore read
`/proc` first and only resend if the intended effect is not visible. Resending
is harmless anyway: every command is either idempotent (HV_ON in RUNNING, STOP,
SHUTTER_CLOSE, SET) or rejected by the state gate the second time (INIT while
homing → `BUSY`, ACK outside WAIT_ACK → `STATE`). STOP is never answered `BUSY`
because of a full normal queue (§6.7).

| Command | Params | Allowed in | Effect |
|---|---|---|---|
| `INIT` | – | SAFE_START, READY | homing (run backward to switch, 2 mm pull-off, zero) → READY |
| `MOVE` | `pos` (req.), `speed` (opt., default 10) | READY, RUNNING | absolute move 0…242 mm, 1…20 mm/s; requires homed |
| `PARK` | – | READY, RUNNING | move to the park position 2 mm at 10 mm/s; requires homed; the shutter closes when entering the park zone, HV is not changed |
| `SERVICE` | – | READY, RUNNING | move to `maint_pos` (service zone) at 10 mm/s; requires homed |
| `START_MEASURE` | `pos` (opt., default `measure_pos`) | RUNNING | move to pos (**60 < pos ≤ 242**, V1.5), then stationary measurement until `STOP`; shutter opens automatically |
| `START_TRAVERSE` | `start`, `end`, `speed` (opt., defaults `traverse_*`) | RUNNING | one run: approach start (10 mm/s, no measurement), traverse to end at speed, stay at end; start and end **> 60 mm** (V1.5), \|end − start\| ≥ 1 mm, end < start allowed; the shutter opens in the run and **stays open for `shutter_hold`** after it (V1.5) so that the next run can follow without closing; HV stays on |
| `STOP` | – | all | controlled stop; acquisition ends; an automatic shutter closes **in the same cycle** (V1.5); a manually opened shutter is not affected |
| `GET_STATE` | – | all | heartbeat; reply extra `"state":"READY","sys_err":0,"local":false,"cycle_max_us":312,"period_max_us":1010,"overruns":0,"prio_lat_max_us":1450` (`stateExtraToJson`, §10) |
| `GET_CONFIG` | – | all | reply extra `"config":{...}` (§5.3) |
| `SET` | `key`, `value` | all | §9 |
| `CALIB` | `action`: `near` `far` `base` `save` `reset` | SAFE_START, READY, RUNNING | §7.3 |
| `ACK` | – | WAIT_ACK | acknowledge; rejected (`PRECONDITION`) while the cause persists |
| `HV_ON` | – | READY, RUNNING | READY → RUNNING if interlock ∧ cooling_flow ∧ temp ≤ 30 ∧ ¬E-stop |
| `HV_OFF` | – | all | RUNNING → READY |
| `SHUTTER_OPEN` | – | RUNNING | V1.2: only in the service zone (20 < pos ≤ 60 mm), else `PRECONDITION` |
| `SHUTTER_CLOSE` | – | all | – |
| `REBOOT` | – | SAFE_START, READY, WAIT_ACK | REBOOT buzzer pattern, restart 700 ms after the request (reply is sent before) |

### 5.3 `GET_CONFIG` extra member

```
"config":{"measure_pos":155.0,"maint_pos":40.0,"traverse_start":80.0,"traverse_end":240.0,
"traverse_speed":10.0,"shutter_hold":3.0,"edge_threshold":3.0,"base_dist":70.0,"calib_near_raw":40.0,
"calib_near_ref":40.0,"calib_far_raw":65.0,"calib_far_ref":65.0,"calib_valid":true,"interlock":true,
"cooling_flow":true,"temp":18.0,"estop_nc":false,"wifi_ssid":"","wifi_pass_set":false}
```
(one line in reality; exact format in `params.h`, `paramsToJsonMember`)

### 5.4 Other endpoints

`/` and `/settings` (web UI), `/add` (ADD document, `Cache-Control: no-store`),
`/add.html` and `/llms.txt` (ADD spec §5.3/§5.4), `/doc` (Markdown), `/log`
(last 20 log lines). All responses carry `Access-Control-Allow-Origin: *`.
V1.5.22: `/app.js`, `/style.css` and the four font files of the web UI (§5.5).
V1.5.23: serial console on the USB port (115200 Bd) – commands and answers in `net_task.h`.

### 5.5 Web UI (V1.5.22, Sprint W1)

Language English, ADD Dashboard Style Guide v1.0 (identical CSS variables, themes `theme-dark`,
`theme-light`, `theme-night`, `theme-bw`, localStorage key `add-dashboard-theme`), fonts from the
device (no CDN – in AP mode there is no internet).

| File | Content | Written by |
|---|---|---|
| `app.js` | protocol (`parseFramed`, `apiBody`, names, `fmt`, `replyText`), process-view rules (`view`, `flags`, `buttons`, `confirmText`, `heartbeatDue`, `seriesPush`, `pill`, `validationBadges`), settings rules (`PARAMS`, `checkParam`, `configDiff`) – exact behaviour in the comment of each function | Qwen (skeleton: constants and CRC given) |
| `index.html` | process view: state, HV, shutter, mode, POS/SPEED/DIST/THICK/AVG, chart of the last 100 valid samples, flag chips with SIM, buttons INIT · HV ON/OFF · MEASURE · TRAVERSE · ACK · STOP, LOCAL badge and hint, validation strip from `/add` | Qwen (markup and helpers given) |
| `settings.html` | positions, profile, shutter hold, edge threshold, base distance, calibration references and CALIB buttons, simulated safety flags (toggles send `SET` at once), `estop_nc` with warning, router SSID/password, REBOOT | Qwen (markup and helpers given) |
| `style.css`, fonts | Style Guide components, themes, 44 px touch targets | reviewer |

Rules that are not obvious from the Pflichtenheft:
- `/proc` every 500 ms, the next request only after the previous one has finished; CRC or JSON
  error → status pill "CRC error", no data shown as valid, only STOP enabled; network error → "Offline".
- Commands are never repeated automatically (also not after `BUSY`); the next `/proc` decides.
- Confirmation dialog for `HV_ON`, `ACK`, `REBOOT` and calibration reset; never for `STOP`.
- Heartbeat `GET_STATE` every 2 s only while `safety.comm_armed`.
- While `local`: INIT, HV ON, MEASURE, TRAVERSE and ACK disabled (they would get `LOCAL_CONTROL`),
  STOP and HV OFF stay enabled; hint "Operator at display has priority".
- THICK shows `---` also off the material (`!on_material`), DIST only while `!valid`.
- Settings: invalid fields are marked and **no** `SET` is sent; otherwise one `SET` per changed key
  (in `PARAMS` order), then `GET_CONFIG` again. The router password is sent only when typed
  (empty field = keep). The live router connection state is not part of `/proc`; it is shown on
  the device display (IP line), the settings page shows the configured SSID and whether a password
  is stored.

---

## 6. Safety logic (`src/logic/safety_logic.h`)

The rules R1 … R7 and invariants I1 … I8 are defined in the header and tested
by `test_safety_logic` (incl. a randomised invariant test over 60 000 steps).

### 6.1 Conditions

- **HV enable (AND):** `cooling_flow ∧ temp ≤ 30.0 ∧ interlock ∧ ¬estop`
- **HV drop (any missing condition):** RUNNING → READY, shutter closes, **no fault, no ACK**
- **Fault (→ FAULT → WAIT_ACK, ACK required):** E-stop · temp > 32.0 · unexpected end switch ·
  communication timeout · homing failure. Priority of the reported cause:
  ESTOP > TEMP_HIGH > MOTOR_FAULT > COMM_TIMEOUT.
- **Shutter open (AND):** state RUNNING (implies HV on and no fault) ∧ position > 20 mm
  (outside the park zone). **Manual** `SHUTTER_OPEN` additionally requires position ≤ 60 mm
  (service zone, V1.2); the **automation** (`SHUTTER_AUTO_OPEN`) opens only in the
  **measuring zone** (> 60 mm, V1.5) and marks the shutter as automatic (`shutter_auto`).
- **Shutter closes** (any shutter): HV drop or `HV_OFF`, any fault, entering the park zone,
  `SHUTTER_CLOSE`. **Automatic shutter additionally (V1.5):** entering the service zone
  (pos ≤ 60), `STOP`, and the hold time `shutter_hold` after the measuring programme
  (no measuring mode active). A manually opened shutter stays open when the axis leaves
  the service zone; if an acquisition starts while it is open, the automation takes it
  over (from then on the automatic rules apply).

### 6.2 Transitions

| From | Event / condition | To | Outputs |
|---|---|---|---|
| SAFE_START | INIT done | READY | off |
| READY | HV_ON, conditions ok | RUNNING | HV on |
| RUNNING | HV_OFF or condition lost | READY | off |
| RUNNING | SHUTTER_OPEN, 20 < pos ≤ 60 | RUNNING | shutter open (manual) |
| RUNNING | SHUTTER_AUTO_OPEN, pos > 60 (V1.5) | RUNNING | shutter open (automation, `shutter_auto`) |
| RUNNING | automatic shutter: pos ≤ 60, STOP or hold time expired (V1.5) | RUNNING | shutter closed |
| any except FAULT/WAIT_ACK | fault condition or INIT failed | FAULT | off, motor emergency stop |
| FAULT | next cycle | WAIT_ACK | off |
| WAIT_ACK | ACK, cause cleared | RECOVER | off |
| RECOVER | next cycle | READY if homed, else SAFE_START | off |

**No automatic restart:** after ACK, HV and shutter stay off. Measurement modes
exist only in RUNNING: leaving RUNNING stops any measuring motion (controlled stop).
An emergency stop during motion invalidates the reference (homed = false), so
INIT is required again.

### 6.3 Command gate (`safetyCmdAllowed`)

See table in §5.2 (column "Allowed in"). Violations return `STATE`.

### 6.4 Shutter automation (V1.5)

During an acquisition (traverse run or measure hold) the control logic opens the
shutter (event `SHUTTER_AUTO_OPEN`) as soon as state is RUNNING and position > 60 mm
(measuring zone). The shutter then stays open over the whole **measuring programme**:
the end of a traverse run does not close it. It closes

- after the **hold time** `shutter_hold` (default 3 s, 0 … 10 s) in which no measuring
  mode was active (`seq.mode == NONE`; the approach phase of the next run counts as
  active, even if it takes longer than the hold time),
- immediately on `STOP` (in the STOP cycle),
- when the axis enters the service zone (pos ≤ 60) or the park zone,
- on HV loss, `HV_OFF`, any fault and `SHUTTER_CLOSE`.

A continuous back-and-forth profile scan is therefore built by the client: after
`motion.mode` returned to `none`, send the next `START_TRAVERSE` (e.g. with start and
end swapped) within the hold time – the shutter is not cycled. With `shutter_hold = 0`
the shutter closes at the end of every run (behaviour before V1.5).

### 6.5 Communication watchdog

- Armed when `INIT`, `MOVE`, `PARK`, `SERVICE`, `START_*` or `HV_ON` is accepted **from the API**.
  Accepted from the touch display → disarmed (local operation needs no network).
- Disarmed automatically when the axis is idle and HV is off.
- Armed and no valid `/api` packet for > 5 s → fault `COMM_TIMEOUT`.
- Clients keep it alive with `GET_STATE` at least every 4 s (`GET /proc` does not count).

### 6.6 LOCAL operator priority (V1.1)

The person at the machine is faster than any remote client, so the display has
priority.

- **Enter:** every command from the display (`source = LOCAL`), also one that
  the state gate rejects, sets `local = true`, restarts the 30 s hold timer and
  disarms the communication watchdog (the operator is present).
- **While local:** API commands `INIT, MOVE, PARK, SERVICE, START_MEASURE, START_TRAVERSE,
  HV_ON, SHUTTER_OPEN, CALIB, ACK, REBOOT` → `LOCAL_CONTROL (-10)`, checked
  before the state gate. Always allowed from the API: `STOP, HV_OFF,
  SHUTTER_CLOSE` (safe direction), `SET` (fault simulation), `GET_STATE`,
  `GET_CONFIG`.
- **V1.5.24:** `local_hold_ms` = `LOCAL_HOLD_MS` (`config.h`), **default 0 = priority off**: a
  display command disarms the watchdog but does not set `local`; display and API are equal.
- **Leave:** axis idle **and** HV off **and** ≥ `local_hold_ms` (V1.1: 30 s) since the
  last display command. So a locally started operation keeps priority until it
  has ended; a remote `HV_OFF` ends it after the hold time.
- Shown as `"local":true` in `/proc` and GET_STATE, as a badge on the display
  and in the web UI.

### 6.7 Priority commands and STOP latency (V1.1)

`STOP`, `HV_OFF`, `SHUTTER_CLOSE` (`appCmdIsPriority`) go into a separate
priority queue (length 4). The control task serves it first and **even in a
cycle where `appTick` already produced a driver request**.

**Overflow:** if the priority queue is full, the request is stored in a latch
(`appPrioLatchSet/Take`, one bit per command **and origin**: API and LOCAL are
kept separately, so a LOCAL request never turns an API request into a LOCAL
one). The latch cannot overflow; identical pending requests coalesce (ten
STOPs → one STOP; API + LOCAL STOP → one LOCAL STOP, the operator did press it),
different ones are all kept and served in the order STOP → HV_OFF →
SHUTTER_CLOSE, after the priority queue. A latched request gets no reply
(a waiting caller sees `BUSY`, the effect is visible in `/proc`). So a priority
request is never dropped; `cmdQueueSend()` returns `true` for every priority
command.

**All pending priority commands per cycle.** In step 3a the control task takes
every priority request that is pending at that moment
(`cmdQueueReceivePriority`: up to 4 from the queue + the whole latch, max. 7)
and executes all of them in the same cycle. Exact semantics (not one atomic
snapshot of both stores): the queue is read first (FIFO, at most the 4 entries
it can hold), then the latch is taken under its spinlock. A request that arrives
in the queue after its read, or in the latch after the latch was taken, is
executed in the next cycle. Every request that was already pending when the call
started is executed in this cycle – this is what `test_command_queue` checks. This is safe because all three are
idempotent and act in the safe direction; driver requests are merged by rank.
Consequence: a STOP never waits behind other priority commands. Requests that
arrive during step 3a are taken in the next cycle.

**Bound.** enqueue → outputs applied ≤ `period_max_us` + `cycle_max_us`. With
the acceptance values (< 1500 µs and < 500 µs) this is ≤ 2000 µs **for any number
of pending priority requests**. It is a property of the software path under
these measured timing conditions, not a safety function: the physical E-stop
acts independently of every queue (§6.9). The driver requests
are merged by rank (NONE < MOVE_TO/RUN_BACKWARD < STOP < FORCE_STOP/ZERO_HERE);
an immediate stop is never weakened.

Design bound: from entering the queue to the applied driver request **≤ 2 ms**
(see Bound above). It is a design target until measured: the device measures it
itself as `prio_lat_max_us` (`cmdQueueSend` timestamp `Command.t_enq_us` →
outputs applied) – acceptance ≤ 2000 µs. End-to-end over WLAN the HTTP round
trip dominates (reply < 150 ms, `scripts/stress_api.py latency`); the two
numbers are different quantities and are reported separately.

| Section of the reaction chain | Measured by | Target |
|---|---|---|
| physical E-stop → step pulses stop | oscilloscope (acceptance §4) | ≤ 2 ms |
| HTTP packet parsed → enqueued | not measured (µs, same task) | – |
| enqueued → outputs applied (STOP, HV_OFF, SHUTTER_CLOSE together) | `prio_lat_max_us` | ≤ 2000 µs |
| API client → reply received (incl. WLAN) | `stress_api.py latency` | < 150 ms |
| display STOP touch → enqueued | touch poll period | ≤ 20 ms |
| STOP request → axis stands | `/proc` (`stress_api.py latency`) | < 1 s (deceleration ramp) |

`prio_lat_max_us` covers all three priority commands together (conservative);
a per-command split is not planned.
The fastest stop is still the physical E-stop (≤ 2 ms, independent of software
queues) and the STOP button on the display.

### 6.8 Simulated signals (V1.1)

`interlock`, `cooling_flow` and `temp` are **simulated** demo signals set via
`SET`. Every UI (web, display) and the ADD document mark them as `SIM`
(ADD: `"simulated": true` / description text "simulated").

### 6.9 Safety disclaimer

This is a **demonstration system**. The software E-stop evaluation and the
safety logic are **not** a safety function in the sense of EN ISO 13849 /
IEC 62061 and do not replace a hardware safety circuit. There is no real
radiation source; HV and shutter are LEDs.

---

## 7. Measurement (`src/logic/measure_logic.h`)

### 7.1 Chain (50 Hz)

16 × `analogReadMilliVolts()` → sort, drop 4 lowest + 4 highest, mean →
`dist_raw = 168.8 · V^-1.398` (V in volt; **mm**; V1.4.2 fit to the six step centres measured
in smoketest 0a with an 11 kΩ load at the sensor output: 1.972 V/65 mm … 2.777 V/40 mm,
max. error 0.48 mm; far values only indicative – the lower boom reads ≈ 186 mm) → IIR (α = 0.8, first value
initialises) → 2-point calibration → `thick = max(0, base_dist − dist)` →
`on_material = thick > edge_threshold`.
Outside 35 … 300 mm (V1.4.2): `in_range = false`, IIR reset, thickness 0.

### 7.2 Geometry

`thick = max(0, base_dist − dist)`. On the demo the sensor sees no support surface
next to the material (only the lower boom: simulated at 135 mm in the tests, the real
reading is ≈ 186 mm – the curve is fitted only for 40 … 65 mm), so `base_dist` is a
**virtual reference plane at the material edge: 70 mm** (V1.2). The stair steps
(65, 60 … 40 mm) then read 5, 10 … 30 mm; next to the material the thickness is 0.
At the edges the reading changes continuously (the sensor spot is partly on the
material); `edge_threshold` (thickness, default 3 mm) decides `on_material`.
`CALIB base` still works where a surface is in range; on the demo `base_dist` is
normally set with `SET`.

### 7.3 Calibration commands

| Action | Precondition | Effect |
|---|---|---|
| `near` / `far` | axis idle, signal stable 500 ms (±0.5 mm) | `calib_near_raw` / `calib_far_raw` = current uncalibrated value. The true distances are set with `SET calib_near_ref / calib_far_ref` |
| `base` | idle, in range, stable | `base_dist` = current calibrated distance |
| `save` | calibration valid (raw span ≥ 5, ref span ≥ 5, refs 40…300) | persist all parameters |
| `reset` | idle | identity calibration (40/40, 65/65), not persisted until `save` |

Typical procedure on the demo: MOVE 215 (centre of the 40 mm step) → `CALIB near`;
MOVE 115 (centre of the 65 mm step) → `CALIB far`; `SET calib_near_ref 40`,
`calib_far_ref 65` (defaults); `CALIB save`. (Step centres V1.4.2; 105 and 205 are step edges.)

---

## 8. Motion (`src/logic/motion_seq.h`)

Non-blocking sequencer with phases `IDLE, HOME_SEEK, HOME_LATCH, HOME_PULLOFF,
MOVE, TRAV_APPROACH, TRAV_RUN, MEAS_APPROACH, MEAS_HOLD`. Exactly one driver
request per cycle (`MOVE_TO, RUN_BACKWARD, STOP, FORCE_STOP, ZERO_HERE`).

| Parameter | Value |
|---|---|
| Homing speed | 5 mm/s (seek and pull-off), pull-off 2 mm, timeout 70 s |
| Normal speed: MOVE/PARK/SERVICE default, approach moves | 10 mm/s |
| Speed range | 1 … 20 mm/s |
| Acceleration | 500 mm/s² (`ACCEL_MM_S2`, verified in smoketest 0a) |
| Measuring positions (V1.5) | `START_MEASURE`/`START_TRAVERSE`: > 60 mm (`measure_min_mm`) … 242 mm |
| Arrival | driver stopped and at target, or stopped for ≥ 100 ms |

Unexpected end switch while the driver runs (forward switch always; backward
switch outside homing) → force stop, reference lost, `MOTOR_FAULT`
(`HOMING_FAILED` during homing).

---

## 9. Parameters and safety flags (`src/logic/params.h`)

| Key | Type | Default | Range | Persisted |
|---|---|---|---|---|
| `interlock` | bool (or 0/1) | **true** (V1.5.23) | – | **no** |
| `cooling_flow` | bool (or 0/1) | **true** (V1.5.23) | – | **no** |
| `temp` | number | 18.0 | −20.0 … 99.9 °C | **no** |
| `measure_pos` | number | 155.0 | 61 … 242 mm (measuring zone, V1.5) | yes |
| `maint_pos` | number | 40.0 | 21 … 60 mm (service zone, customer-defined) | yes |
| `traverse_start` | number | 80.0 | 61 … 242 mm (V1.5) | yes |
| `traverse_end` | number | 240.0 | 61 … 242 mm (V1.5) | yes |
| `traverse_speed` | number | 10.0 | 1 … 20 mm/s | yes |
| `shutter_hold` | number | 3.0 | 0.0 … 10.0 s – automatic shutter hold time (V1.5) | yes |
| `edge_threshold` | number | 3.0 | 0.5 … 100 mm (thickness) | yes |
| `base_dist` | number | 70.0 | 40 … 300 mm (virtual reference plane) | yes |
| `calib_near_ref` | number | 40.0 | 40 … 300 mm | yes |
| `calib_far_ref` | number | 65.0 | 40 … 300 mm | yes |
| `estop_nc` | bool (or 0/1) | false | – | yes – E-stop wiring (§11); only while idle with HV off |
| `wifi_ssid` | string | "" | 0 … 32 chars (empty = no router connection) | yes (effective after REBOOT) |
| `wifi_pass` | string | "" | 0 or 8 … 63 chars | yes (never reported back) |

Safety flags are never persisted. V1.5.23: after every boot `interlock = true`,
`cooling_flow = true`, `temp = 18.0` (until V1.5.22 false/false). Persisting happens in the
network task, never in the 1 ms control cycle.

---

## 10. Application logic (`src/logic/app_logic.h`)

Order inside one control cycle:

1. one ADC sample per cycle (V1.5.25); every 20 ms `appMeasure(medianMean(samples since the last evaluation))`
2. `appTick()` – motion tick, events, safety TICK, emergency stop, mode stop,
   statistics, shutter automation, watchdog disarm, LOCAL release
3. a) `cmdQueueReceivePriority()` → `appCommand()` for **every** pending priority
      command (max. 7, §6.7)
   b) if no driver request yet: `cmdQueueReceive()` → at most one further `appCommand()`
4. outputs: **shutter → motor → HV LED**
5. replies (`cmdQueueReply`) – **after** the outputs; `prio_lat_max_us` updated
6. `appFillProc()` → `snapshotPublish()`
7. timing (`CycleStats`, maxima since `boot: ready` (V1.5.25), `esp_timer_get_time()`, uint32 µs, wrap-safe):

| Field | Measures | Acceptance (10 min) |
|---|---|---|
| `cycle_max_us` | work time of steps 1–6 | < 800 (V1.5.25, 1 ms cycle) |
| `overruns` | cycles with work time > 1000 µs | 0 new during the test |
| `period_max_us` | distance between two cycle starts (scheduling jitter) | < 1500 |
| `prio_lat_max_us` | priority command: `cmdQueueSend` → outputs applied | ≤ 2000 |

The publication of the statistics itself (< 2 µs) is not part of `cycle_max_us`.
`prio_lat_max_us` stays 0 until the first priority command was executed.

---

## 11. Hardware layer (`src/hal/*.h`)

- Inputs: limit switches debounced 5 ms. **E-stop: activation immediate, release debounced.**
- **E-stop wiring (V1.3, parameter `estop_nc`, REST `SET`, persisted):**

| `estop_nc` | Wiring | E-stop active | Wire break |
|---|---|---|---|
| `false` (default) | normally-open switch to GND, board pull-up | LOW | **not detected** (reads as "no E-stop") |
| `true` | normally-closed contact to GND, board pull-up | HIGH | detected (reads as E-stop) |

  A setting that does not match the wiring always shows a permanent E-stop
  (normal state reads as active) – the error is safe and visible. The control
  task calls `halSetEstopActiveHigh(params.estop_nc)` every cycle.
  **Decoding on a polarity change (binding for `hal_io.cpp`):** the decoded state
  is re-evaluated from the current raw level in the same `halUpdateInputs()` call.
  If the new interpretation says "active", E-stop is active **immediately** (no
  debounce, like every activation); if it says "inactive", the normal release
  debounce applies. A change can therefore never hide an E-stop for even one cycle
  longer than the release debounce. Verified on the target by
  `test_hardware: test_estop_polarity_change` and acceptance §4. The value can
  only be changed while the axis is idle and HV is off (else `PRECONDITION`).
  With `false` the display shows `ESTOP(NO)` in the warning colour.
- Outputs: shutter LED GPIO 32, **HV LED GPIO 4**, **heartbeat LED GPIO 2** (blue
  board LED, V1.3: on while `(now_ms / 1000)` is even = 0.5 Hz, driven by the
  control task, so it stops when the control loop stops); all HIGH = on, LOW from
  the first line of `halInit()`. The buzzer is a bit in the 74HC595 chain (i2so.7)
  set via `engine.i2sMuxSetBit(slot, value)` (FastAccelStepper ≥ 2.0).
  All output setters write the hardware **only when the value changes** (cached
  last value), so buzzer and heartbeat add no I2S/GPIO work in most cycles.
- Stepper: FastAccelStepper `DRIVER_I2S_MUX`, step/dir/enable as mux slots
  (`slot | PIN_I2S_FLAG`). Slot mapping `I2SO_TO_SLOT(n) = n` (determined in smoketest 0a).
- Reaction time E-stop → motor stop: ≤ 2 ms (1 ms cycle + driver); outputs off in the same cycle.

### 11.1 Buzzer signals (`src/logic/sound_logic.h`)

The buzzer is an **active** buzzer (fixed tone, on/off only), so signals differ
by rhythm. Patterns in ms (on/off/on…):

| Sound | Pattern | When |
|---|---|---|
| `BOOT` | 100 / 100 / 300 (short–long) | first control cycle after the firmware has started = firmware ready |
| `REBOOT` | 300 / 100 / 100 (long–short) | accepted REBOOT, before the restart |
| `CLICK` | 50 | display command accepted (`source = LOCAL`, result OK) |
| `HOMED` | 150 | homing finished |
| `FAULT` | 3 × 150 (pause 150) | fault entry, cause other than ESTOP |
| `ESTOP` | 4000 | fault entry with cause ESTOP (once, not repeated while pressed) |

Priority ESTOP > FAULT > REBOOT > BOOT > HOMED > CLICK: a lower-priority sound
never interrupts a higher one.

---

## 12. System (`src/system/*.h`, `src/comm/*.h`)

- Task watchdog 10 s (ESP-IDF 5 API), control task registered.
- NVS namespace `add_demo`, schema version 4 (V1.5: `shutter_hold`, measuring zone; older data → defaults), keys see `nvs_config.h`.
- REBOOT: the network task calls `systemRestart()` `REBOOT_DELAY_MS` = 700 ms after
  the request (HTTP reply sent, REBOOT pattern played).

### 12.1 WiFi (`src/comm/wifi_manager.h`, `src/logic/wifi_logic.h`)

- **Access point always on:** `ADD-Demo` / `adddemo1`, 192.168.4.1 (mode `WIFI_AP_STA`).
- **Router connection (station):** if `wifi_ssid` is set (REST `SET wifi_ssid` /
  `wifi_pass`, effective after `REBOOT`), the device logs into that network via
  DHCP – automatically after every boot – and is then also reachable there
  (and can use the internet through the router). mDNS `add-demo.local` on both.
- **Reconnect:** connection lost or failed → 3 attempts of 10 s each; if all fail
  → 2 min pause → 3 attempts again, forever (`staStep`, tested by `test_wifi_logic`).
- During connection attempts the ESP32 radio may switch channels; AP clients can
  notice short interruptions. The pause keeps this rare when the router is absent.
- Every state change is logged (`/log`); the display shows AP and station IP / state.

---

## 13. Display (`src/display/display.h`)

480 × 320 landscape, redrawn every 200 ms from the snapshot, touch polled every
20 ms. Commands go to the queue with `source = LOCAL`. All decisions come from
the pure `src/logic/display_logic.h` (tested by `test_display_logic`).
Layout details: Pflichtenheft §5.8.

| Screen mode | State | Content |
|---|---|---|
| NORMAL | SAFE_START, READY, RUNNING, RECOVER | status, 7 buttons at y = 260, h = 60: `INIT` · `HV` (toggle) · `MEAS` · `TRAV` · `PARK` · `SERV` (64 px each) · `STOP` (96 px); `LOCAL` badge; flags marked `SIM`; speed in mm/s; `ESTOP(NO)` warning if `estop_nc` is false; IP line AP + station |
| FAULT | FAULT (1 cycle) | fault banner, **no buttons** |
| ACK | WAIT_ACK | fault banner `FAULT: <name>` + remedy (English, Pflichtenheft §5.8); **all normal buttons hidden**; one ACK area 480 × 60 px at y = 260 |

ACK area (`ackCauseActive`, `ackPressUpdate`):
- cause still active (E-stop pressed, temp > 32 °C) → grey, "CAUSE ACTIVE: … - RELEASE FIRST", no command;
- otherwise yellow "ACK - HOLD 1 s" with progress bar; ACK is sent once after
  1000 ms continuous press; the press must **start** on the ACK screen
  (touch-down edge), so a finger still lying on STOP from before the fault never
  acknowledges. Nothing else can be operated in WAIT_ACK – nothing can move
  anyway (motor emergency-stopped every cycle, HV and shutter forced off).

---

## 14. Change procedure

1. Problem found → agent stops and reports.
2. Norbert decides; contract version +0.1, header and tests changed together,
   reference implementation re-run.
3. Only then the sprint continues.
