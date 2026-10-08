# ADD Demo System – Interface Contract (Sprint 0b)

**Version:** 1.0 · **Status:** FROZEN after review by Norbert Walter · **Date:** 2026-10-08
**Basis:** Pflichtenheft Demoanlage ADD Slim V1.4

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
| Command queue | HTTP / display → control | `src/app/command_queue.*` (FreeRTOS queue, max. 1 command per cycle) |
| Snapshot | control → HTTP / display | `src/app/snapshot.*` (copy of `ProcData` under one spinlock) |

No other shared mutable globals exist. Nobody but snapshot/command_queue uses
`taskENTER_CRITICAL`.

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
  `STEPS_PER_MM = 200 · 16 / 42 = 76.19` (TMC2209 jumper 1/16).
- Speeds: **mm/min** in API; **steps/s (Hz)** towards the driver.
- Time: `uint32_t` milliseconds (`millis()`), all comparisons wrap-around safe.
- Position 0 = 2 mm in front of the backward end switch (after homing). Axis 0 … 240 mm.
- Park zone: position ≤ 5.0 mm. The shutter is never open in the park zone.

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
| 5 | `HOMING_FAILED` | homing timeout (40 s) or forward switch during homing |

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
{"ts":123456,"state":"RUNNING","err":0,
 "motion":{"pos":120.0,"vel":200.0,"state":"moving","mode":"traverse","homed":true,"limit_fwd":false,"limit_bwd":false},
 "measure":{"dist":187.7,"thick":12.3,"on_material":true,"valid":true,"in_range":true,"avg":12.3,"n":143},
 "safety":{"estop":false,"hv_on":true,"hv_ok":true,"shutter":"open","interlock":true,"cooling_flow":true,"temp":23.0,"temp_ok":true,"comm_armed":true},
 "crc":"XXXXXXXX"}
```

| Field | Meaning |
|---|---|
| `state` | safety state (§3.1) |
| `err` | system error code (§3.2) |
| `motion.state` | `idle` / `moving` / `homing` / `error` (error = last sequence ended with fault) |
| `motion.mode` | `none` / `measure` / `traverse` |
| `motion.homed` | axis has a valid reference |
| `measure.dist` | calibrated distance sensor → surface, mm |
| `measure.thick` | `base_dist − dist`, never negative |
| `measure.on_material` | `thick > edge_threshold` |
| `measure.valid` | **only true while the shutter is open** (simulates: no radiation → no measurement) |
| `measure.in_range` | sensor reading within 40 … 300 mm |
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

The response comes after the control task executed (accepted) the command,
typically within 2 ms. Long operations (homing, traverse) continue
asynchronously – watch `/proc`. On reply timeout (100 ms) the answer is `BUSY`
and the command may still be executed: check `/proc`.

| Command | Params | Allowed in | Effect |
|---|---|---|---|
| `INIT` | – | SAFE_START, READY | homing (run backward to switch, 2 mm pull-off, zero) → READY |
| `MOVE` | `pos` (req.), `speed` (opt., default 1500) | READY, RUNNING | absolute move 0…240 mm, 10…3000 mm/min; requires homed |
| `START_MEASURE` | `pos` (opt., default `measure_pos`) | RUNNING | move to pos (5 < pos ≤ 240), then stationary measurement until `STOP`; shutter opens automatically |
| `START_TRAVERSE` | `start`, `end`, `speed` (opt., defaults `traverse_*`) | RUNNING | approach start, traverse to end at speed; shutter opens during the run, closes at the end; HV stays on |
| `STOP` | – | all | controlled stop; acquisition ends, shutter closes |
| `GET_STATE` | – | all | heartbeat; reply extra `"state":"READY","sys_err":0` |
| `GET_CONFIG` | – | all | reply extra `"config":{...}` (§5.3) |
| `SET` | `key`, `value` | all | §9 |
| `CALIB` | `action`: `near` `far` `base` `save` `reset` | SAFE_START, READY, RUNNING | §7.3 |
| `ACK` | – | WAIT_ACK | acknowledge; rejected (`PRECONDITION`) while the cause persists |
| `HV_ON` | – | READY, RUNNING | READY → RUNNING if interlock ∧ cooling_flow ∧ temp ≤ 30 ∧ ¬E-stop |
| `HV_OFF` | – | all | RUNNING → READY |
| `SHUTTER_OPEN` | – | RUNNING | only if position > 5 mm |
| `SHUTTER_CLOSE` | – | all | – |
| `REBOOT` | – | SAFE_START, READY, WAIT_ACK | restart after the reply |

### 5.3 `GET_CONFIG` extra member

```
"config":{"measure_pos":120.0,"traverse_start":10.0,"traverse_end":230.0,"traverse_speed":200.0,
"edge_threshold":3.0,"base_dist":200.0,"calib_near_raw":50.0,"calib_near_ref":50.0,
"calib_far_raw":200.0,"calib_far_ref":200.0,"calib_valid":true,"interlock":false,
"cooling_flow":false,"temp":18.0,"wifi_ssid":"","wifi_pass_set":false}
```

### 5.4 Other endpoints

`/` and `/settings` (web UI), `/add` (ADD document, `Cache-Control: no-store`),
`/add.html` and `/llms.txt` (ADD spec §5.3/§5.4), `/doc` (Markdown), `/log`
(last 20 log lines). All responses carry `Access-Control-Allow-Origin: *`.

---

## 6. Safety logic (`src/logic/safety_logic.h`)

The rules R1 … R7 and invariants I1 … I6 are defined in the header and tested
by `test_safety_logic` (incl. a randomised invariant test over 60 000 steps).

### 6.1 Conditions

- **HV enable (AND):** `cooling_flow ∧ temp ≤ 30.0 ∧ interlock ∧ ¬estop`
- **HV drop (any missing condition):** RUNNING → READY, shutter closes, **no fault, no ACK**
- **Fault (→ FAULT → WAIT_ACK, ACK required):** E-stop · temp > 32.0 · unexpected end switch ·
  communication timeout · homing failure. Priority of the reported cause:
  ESTOP > TEMP_HIGH > MOTOR_FAULT > COMM_TIMEOUT.
- **Shutter open (AND):** state RUNNING (implies HV on and no fault) ∧ position > 5 mm.
  It closes automatically when HV drops, on any fault and when entering the park zone.

### 6.2 Transitions

| From | Event / condition | To | Outputs |
|---|---|---|---|
| SAFE_START | INIT done | READY | off |
| READY | HV_ON, conditions ok | RUNNING | HV on |
| RUNNING | HV_OFF or condition lost | READY | off |
| RUNNING | SHUTTER_OPEN, pos > 5 | RUNNING | shutter open |
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

### 6.4 Shutter automation

During an acquisition (traverse run or measure hold) the control logic opens the
shutter as soon as state is RUNNING and position > 5 mm; when the acquisition
ends (finished, STOP, fault) it closes the shutter.

### 6.5 Communication watchdog

- Armed when `INIT`, `MOVE`, `START_*` or `HV_ON` is accepted **from the API**.
  Accepted from the touch display → disarmed (local operation needs no network).
- Disarmed automatically when the axis is idle and HV is off.
- Armed and no valid `/api` packet for > 5 s → fault `COMM_TIMEOUT`.
- Clients keep it alive with `GET_STATE` at least every 4 s (`GET /proc` does not count).

---

## 7. Measurement (`src/logic/measure_logic.h`)

### 7.1 Chain (50 Hz)

16 × `analogReadMilliVolts()` → sort, drop 4 lowest + 4 highest, mean →
`dist_raw = 120.8 · V^-1.058` (V in volt; **mm**) → IIR (α = 0.8, first value
initialises) → 2-point calibration → `thick = max(0, base_dist − dist)` →
`on_material = thick > edge_threshold`.
Outside 40 … 300 mm: `in_range = false`, IIR reset, thickness 0.

### 7.2 Geometry

`base_dist` = distance sensor → empty support (no material). Default 200 mm,
measured with `CALIB base`. `edge_threshold` is a **thickness** threshold (default 3 mm).

### 7.3 Calibration commands

| Action | Precondition | Effect |
|---|---|---|
| `near` / `far` | axis idle, signal stable 500 ms (±0.5 mm) | `calib_near_raw` / `calib_far_raw` = current uncalibrated value. The true distances are set with `SET calib_near_ref / calib_far_ref` |
| `base` | idle, in range, stable | `base_dist` = current calibrated distance |
| `save` | calibration valid (raw span ≥ 5, ref span ≥ 5, refs 40…300) | persist all parameters |
| `reset` | idle | identity calibration (50/50, 200/200), not persisted until `save` |

---

## 8. Motion (`src/logic/motion_seq.h`)

Non-blocking sequencer with phases `IDLE, HOME_SEEK, HOME_LATCH, HOME_PULLOFF,
MOVE, TRAV_APPROACH, TRAV_RUN, MEAS_APPROACH, MEAS_HOLD`. Exactly one driver
request per cycle (`MOVE_TO, RUN_BACKWARD, STOP, FORCE_STOP, ZERO_HERE`).

| Parameter | Value |
|---|---|
| Homing speed | 600 mm/min, pull-off 2 mm, timeout 40 s |
| Approach speed / MOVE default | 1500 mm/min |
| Speed range | 10 … 3000 mm/min |
| Acceleration | 500 mm/s² (`ACCEL_MM_S2`, VERIFY 0a) |
| Arrival | driver stopped and at target, or stopped for ≥ 100 ms |

Unexpected end switch while the driver runs (forward switch always; backward
switch outside homing) → force stop, reference lost, `MOTOR_FAULT`
(`HOMING_FAILED` during homing).

---

## 9. Parameters and safety flags (`src/logic/params.h`)

| Key | Type | Default | Range | Persisted |
|---|---|---|---|---|
| `interlock` | bool (or 0/1) | false | – | **no** |
| `cooling_flow` | bool (or 0/1) | false | – | **no** |
| `temp` | number | 18.0 | −20.0 … 99.9 °C | **no** |
| `measure_pos` | number | 120.0 | 6 … 240 mm | yes |
| `traverse_start` | number | 10.0 | 6 … 240 mm | yes |
| `traverse_end` | number | 230.0 | 6 … 240 mm | yes |
| `traverse_speed` | number | 200.0 | 10 … 3000 mm/min | yes |
| `edge_threshold` | number | 3.0 | 0.5 … 100 mm (thickness) | yes |
| `base_dist` | number | 200.0 | 40 … 300 mm | yes |
| `calib_near_ref` | number | 50.0 | 40 … 300 mm | yes |
| `calib_far_ref` | number | 200.0 | 40 … 300 mm | yes |
| `wifi_ssid` | string | "" | 0 … 32 chars (empty = AP mode) | yes (effective after REBOOT) |
| `wifi_pass` | string | "" | 0 or 8 … 63 chars | yes (never reported back) |

Safety flags are **fail-safe**: after every boot `interlock = false`,
`cooling_flow = false`, `temp = 18.0`. Persisting happens in the network task,
never in the 1 ms control cycle.

---

## 10. Application logic (`src/logic/app_logic.h`)

Order inside one control cycle:

1. every 20 ms `appMeasure()`
2. `appTick()` – motion tick, events, safety TICK, emergency stop, mode stop,
   statistics, shutter automation, watchdog disarm
3. if no driver request yet and a command is queued: `appCommand()` (max. one)
4. outputs: **shutter → motor → HV LED**
5. `appFillProc()` → `snapshotPublish()`

---

## 11. Hardware layer (`src/hal/*.h`)

- Inputs: limit switches debounced 5 ms. **E-stop: activation immediate, release debounced.**
  E-stop wiring must be **normally closed to GND** (pressed or wire broken → HIGH → E-stop).
- Outputs: shutter GPIO 32; HV LED and buzzer are bits in the 74HC595 chain set via
  `engine.i2sMuxSetBit(slot, value)` (FastAccelStepper ≥ 2.0).
- Stepper: FastAccelStepper `DRIVER_I2S_MUX`, step/dir/enable as mux slots
  (`slot | PIN_I2S_FLAG`). Slot mapping `I2SO_TO_SLOT(n)` determined in Sprint 0a.
- Reaction time E-stop → motor stop: ≤ 2 ms (1 ms cycle + driver); outputs off in the same cycle.

---

## 12. System (`src/system/*.h`, `src/comm/*.h`)

- Task watchdog 10 s (ESP-IDF 5 API), control task registered.
- WiFi: STA with credentials from NVS, else / on failure AP `ADD-Demo` / `adddemo1`
  (192.168.4.1), mDNS `add-demo.local`.
- NVS namespace `add_demo`, schema version 1, keys see `nvs_config.h`.

---

## 13. Display (`src/display/display.h`)

480 × 320 landscape, one fixed view, refreshed every 200 ms from the snapshot.
Five touch buttons à 96 × 60 px at y = 260: `INIT` · `HV` (toggles HV_ON/HV_OFF)
· `MEAS` · `TRAV` · `STOP` (always active). Commands go to the queue with
`source = LOCAL`. Layout details: Pflichtenheft §5.8.

---

## 14. Change procedure

1. Problem found → agent stops and reports.
2. Norbert decides; contract version +0.1, header and tests changed together,
   reference implementation re-run.
3. Only then the sprint continues.
