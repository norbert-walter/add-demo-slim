# Sprint-Prompts – Demoanlage ADD (Slim) · V3.0

**Stand:** 2026-10-13
**Autor:** Norbert Walter
**Basis:** Pflichtenheft V1.7.6, `docs/CONTRACT.md` V1.5.8 (Änderungen seit V1.3: Anhänge am Ende)
**Werkzeuge:** code-server + Cline, Modell Qwen3.6-35B-A3B (64k Kontext), Review mit Claude
---

## Sprint-Plan V3 (ab 13.10.2026)

### Warum ein neuer Plan?

Sprint S2.1 (types, crc32, safety_logic) hat gezeigt, wie Qwen mit 64k Kontext arbeitet
(`docs/reviews/Review_S2.1.md`):

- **Drei Dateien mit ca. 300 Zeilen Referenzumfang** sind die passende Größe für einen Sprint.
  S2.1 hatte 3 Dateien, 19 Funktionen und 282 Zeilen Referenzumfang (Qwen: ca. 750 Zeilen).
- **Qwen erfüllt die Tests, nicht den Header-Text.** Was kein Test prüft, ist ungesichert.
  Die Nacharbeit machte 55–68 % des Token-Verbrauchs aus, überwiegend wegen Testlücken.
- **Frische Tasks mit genau einer Aufgabe sind am effizientesten.** Der Abschluss-Task mit
  einer konkreten Codezeile kostete ca. 1 % des Sprints.

Die Prüfung der alten Reihenfolge gegen die tatsächlichen Abhängigkeiten im Code ergab drei
Fehler:

1. **S1 kam zu früh.** `nvs_config` braucht `params.cpp` (S2.2), `main.cpp` ruft HTTP-Server
   (S3) und Display (S4b) auf. S1 hätte nur kompiliert, ohne geprüft zu werden.
2. **S2.6 verlangte eine vollständige Firmware**, die vor S3 und S4b nicht linken kann.
3. **`log_buffer`** stand im Header unter S3, im Prompt unter S1.

Außerdem waren die Sprints ungleich groß: S2.4 bündelte die größte Datei (`app_logic`,
279 Zeilen) mit `sound_logic`, während `wifi_logic` (45 Zeilen) in S1 steckte.

### Grundsätze

1. **Reihenfolge nach Abhängigkeiten.** Ein Sprint baut nur auf fertigem, geprüftem Code auf.
2. **Erst die reine Logik, dann Plattform, dann Hardware, dann Integration.**
   Die reine Logik ist die Hälfte der Arbeit und vollständig nativ prüfbar – dort ist Qwen am
   stärksten. Plattform und Hardware bauen danach nur noch auf fertigem Code auf. Der erste
   Flash der vollständigen Firmware kommt erst, wenn sie wirklich vollständig linkt.
3. **Größe:** höchstens 3 Dateien und ca. 300 Zeilen Referenzumfang pro Sprint.
4. **Eine Cline-Task pro Datei** (frischer Kontext). Jede Aufgabe steht als eigener Abschnitt
   in der Sprint-Datei `docs/sprints/<Sprint>.md` und beginnt mit einer „Stand:“-Zeile.
5. **Tests vor dem Start absichern (Claude).** Vor jedem Sprint werden die Tests gegen gezielt
   fehlerhafte Varianten der Referenzimplementierung geprüft (Mutationstest) und bei Lücken
   ergänzt (Contract-Version). Was nicht nativ testbar ist, bekommt eine Nachbildung
   (wie `test_command_queue`) oder eine Prüfliste am Gerät.
6. **Review nach dem Sprint (Claude):** alle Suiten unter AddressSanitizer/UBSan, eingefrorene
   Dateien unverändert, Differenztest gegen die Referenz (Zufallsfälle), Review-Protokoll in
   `docs/reviews/`. Erst dann Merge nach `master` und Tag `sprint-<name>`.
7. **Berichte prüfbar machen:** Qwen zitiert die SUMMARY-Zeilen der Testausgabe wörtlich.
   Konkrete Anweisungen (eine Codezeile) statt Hinweisen, wo es auf eine Stelle ankommt.

### Sprints

| Sprint | Dateien | Ref.-Umfang | Prüfung | Wer |
|---|---|---:|---|---|
| ✅ **L1** (= S2.1) | `logic/types.cpp`, `crc32.cpp`, `safety_logic.cpp` | 282 | `test_types_crc`, `test_safety_logic` | Qwen |
| **L2** | `logic/measure_logic.cpp`, `params.cpp` | 143 | `test_measure_logic`, `test_params` | Qwen |
| **L3** | `logic/motion_seq.cpp`, `sound_logic.cpp`, `wifi_logic.cpp` | 227 | `test_motion_seq`, `test_sound_logic`, `test_wifi_logic` | Qwen |
| **L4** | `logic/app_logic.cpp` (allein; Aufgaben nach Funktionsgruppen) | 279 | `test_app_logic` | Qwen, Review Claude |
| **L5** | `logic/proc_json.cpp`, `api_parse.cpp`, `display_logic.cpp` | 169 | `test_proc_json`, `test_api_parse`, `test_display_logic` | Qwen |
| **P1** | `comm/log_buffer.cpp`, `system/nvs_config.cpp`, `system/system.cpp` | klein | neue native Tests mit Nachbildung für log_buffer und nvs_config; firmware-partial | Qwen |
| **P2** | `comm/wifi_manager.cpp`, `comm/http_server.cpp` | mittel | firmware-partial, Review | Qwen |
| **H1** | `hal/hal_io.cpp`, `hal/motion_drv.cpp` | mittel | Smoketest-Code als Vorlage; Prüfung am Gerät | du + Qwen |
| **I1** | `display/display.cpp`, `main.cpp`, Gesamt-Build | mittel | `pio run -e firmware`, erster Flash, Prüfliste am Gerät | du + Qwen |
| **W1** | `data/www/index.html`, `settings.html` | – | Sichtprüfung im Browser | Qwen |
| **A1** | Abnahme (Pflichtenheft Kapitel 10) | – | Abnahmeprotokoll | du |
| **D1** | `data/add.json`, `data/doc.md`, Demo-Skripte | – | KI-Validierung | du + Claude |

Abhängigkeiten: L1 → L2 → L3 → L4 (app_logic braucht safety, measure, params, motion_seq,
sound) → L5 → P1 (nvs_config braucht params) → P2 (http_server braucht api_parse, proc_json,
log_buffer) → H1 → I1 (alles) → W1 → A1 → D1. W1 ist inhaltlich unabhängig und kann bei
Bedarf vorgezogen werden.

Umfang gesamt: 21 C++-Dateien mit ca. 107 öffentlichen Funktionen (davon 12 Dateien reine
Logik mit ca. 72 Funktionen und ca. 1 100 Zeilen Referenzumfang), dazu 2 Web-Seiten und das
ADD-Dokument.

### Zuordnung alt → neu

| Datei | bisher | neu |
|---|---|---|
| types, crc32, safety_logic | S2.1 | L1 (erledigt, Tag `sprint-2.1`) |
| measure_logic, params | S2.2 | L2 |
| motion_seq | S2.3 | L3 |
| sound_logic | S2.4 | L3 |
| wifi_logic | S1 | L3 |
| app_logic | S2.4 | L4 |
| proc_json, api_parse | S3 | L5 |
| display_logic | S4b | L5 |
| log_buffer, nvs_config, system | S1 (log_buffer im Header: S3) | P1 |
| wifi_manager | S1 | P2 |
| http_server | S3 | P2 |
| hal_io, motion_drv | S2.5 | H1 |
| display.cpp | S4b | I1 |
| main.cpp, Integration control_task | S1, S2.6 | I1 |
| Web-Seiten | S4a | W1 |
| Abnahme | S5 | A1 |
| ADD-Dokument, Demo-Skripte | S6 | D1 |

Die Prompts der alten Sprints weiter unten bleiben als Quelle für die Sprint-Dateien in
`docs/sprints/` erhalten. Für die Arbeit mit Cline gilt immer die Sprint-Datei.

### Ablauf eines Sprints

1. **Claude:** Tests des Sprints absichern (Mutationstest, ggf. Contract-Version), Sprint-Datei
   `docs/sprints/<Sprint>.md` mit einer Aufgabe pro Datei schreiben, Bundle.
2. **Du:** Bundle einspielen, Branch `sprint-<name>` anlegen.
3. **Qwen:** je Aufgabe eine frische Cline-Task (Plan → Act), Bericht mit SUMMARY-Zeilen.
4. **Du:** Bericht prüfen, committen, Bundle `git bundle create … <branch> ^master` an Claude.
5. **Claude:** Review (Tests, Sanitizer, Differenztest, eingefrorene Dateien), bei Befund
   Nacharbeits-Prompt, sonst Review-Protokoll.
6. **Du:** Merge nach `master` mit `--no-ff`, Tag `sprint-<name>`, Push.
7. Token-Verbrauch je Task (↑ ↓ →) in die Tabelle im Review-Protokoll eintragen.

---

## Arbeitsweise

### Grundidee
Die Denkarbeit steckt in den eingefrorenen Verträgen (Header, `CONTRACT.md`) und
den eingefrorenen Tests. Ein Sprint-Prompt beschreibt nur noch **welche Dateien**
zu implementieren sind und **welche Tests** grün werden müssen. Cline liest
Header, Tests und Contract selbst aus dem Repository – deshalb sind die Prompts kurz.

`.clinerules` (Projekt-Root) wird von Cline bei jeder Task automatisch geladen
und enthält die harten Regeln (eingefrorene Dateien, IDF-5-APIs, keine
blockierenden Aufrufe, kein Flashen ohne Freigabe, Definition of Done).
Bei Nutzung **ohne Cline**: den Inhalt von `.clinerules` vor jeden Prompt setzen
und die genannten Header/Tests mit in den Prompt kopieren.

### Ablauf je Cline-Task
1. Neue Cline-Task (frischer Kontext). Modus **Plan**: Prompt einfügen.
   Cline liest die Dateien und legt einen Plan vor → prüfen.
2. Modus **Act**: Cline implementiert, baut, testet, iteriert.
3. Cline berichtet. Du prüfst mit der **Review-Checkliste** des Sprints.
   **Regressionsregel:** `pio test -e native` komplett laufen lassen – jede Suite, die vor
   dem Sprint grün war, muss danach noch grün sein (steht auch in `.clinerules`).
4. README-Status prüfen: Cline hat die Statuszeile des Sprints aktualisiert (Implementierung
   und grüne Suiten, DoD Punkt 4 in `.clinerules`) – stimmt sie mit dem Testlauf überein?
5. Frozen-Check und Commit:
   ```bash
   git diff --exit-code -- src/config.h ':(glob)src/**/*.h' test docs/CONTRACT.md
   git add -A && git commit -m "Sprint L2: measure_logic, params"
   ```

### Auto-Approve in Cline
| Erlauben | Nie automatisch |
|---|---|
| Dateien lesen/bearbeiten | `-t upload`, `-t uploadfs` |
| `pio test -e native …` | `pio test -e esp32-test` |
| `pio run -e firmware-partial -s`, `pio run -e firmware -s` | `pio device monitor` |
| `python scripts/add_client.py selftest` | alles mit `rm -rf` |

### Modell-Empfehlung

| Sprint | Modell | Grund |
|---|---|---|
| L1–L3, L5, P1 | Qwen3.6-35B-A3B | eng umrissen, Tests als Ziel |
| L4 | Qwen, Review Claude | viele Querbeziehungen |
| P2 | Qwen, Review Claude | Netzwerk, Nebenläufigkeit |
| H1, I1 | du + Qwen/Claude | Hardware, erster Flash |
| W1 | Qwen | viel UI-Code |
| D1 | Claude | ADD-Expertise |

### Wenn Tests nicht grün werden
- Erst die Fehlermeldung des Tests lesen lassen („read the failing assertion and the rule in the header it refers to“).
- Nach 3 erfolglosen Iterationen: Task stoppen, Ausgabe an Claude zur Analyse geben.
- **Nie** Tests oder Header ändern lassen. Ist ein Test wirklich falsch → Contract-Änderung (CONTRACT §14).

---

## Bisherige Sprint-Prompts (V2.4, Quelle für `docs/sprints/`)

Die folgenden Abschnitte verwenden die alten Sprint-Namen (Zuordnung siehe oben). Sie
werden nicht mehr direkt an Cline gegeben.

## Sprint 0a – Smoketest (du, an der Hardware)

Ablauf und Protokoll: `docs/Smoketest_Protokoll.md`. **Erledigt** (11.10.2026, Tag `sprint-0a`):
alle Werte in `src/config.h` bestätigt, `config.h` eingefroren.

### PROMPT S0a-Probe (Agenten-Probelauf, Protokoll Punkt 8.1) – erledigt

```
Add a serial command "uptime" to src/smoketest/smoketest.cpp that prints exactly one
JSON line: {"uptime_ms":<millis()>}. Follow the existing style of dispatch() and the
other cmd* functions. Also add "uptime" to the help text.
Then run: pio run -e smoketest -s
Fix all errors until the build is clean. Do NOT flash. Report the files you changed.
```

---

## Sprint 0b – Verträge (Claude, Review durch dich)

Bereits erledigt in diesem Paket. **Review-Checkliste:**

- [ ] `docs/CONTRACT.md` §6 (Sicherheitslogik) fachlich korrekt? Insbesondere: kein automatischer Wiederanlauf, HV-Abfall ohne Fehler (30–32 °C), Parkzone, Watchdog-Regeln
- [ ] `src/logic/safety_logic.h` Regeln R1–R7 und Invarianten I1–I8 entsprechen deiner Sicherheitsphilosophie
- [ ] Befehlsmatrix (`test_safety_logic.cpp` → `test_cmd_gate_matrix`) passt
- [ ] Szenarien in `test_app_logic.cpp` spiegeln die gewünschten Abläufe
- [ ] Parameterbereiche (`params.h`) sinnvoll
- [ ] `/proc`-Format (CONTRACT §5.1) passt zu Display, Web und ADD
- [ ] V1.1: Bedienvorrang LOCAL (CONTRACT §6.6), STOP-Priorität (§6.7), Fehlerbild am Display (§13)
- [ ] Nach 0a: `config.h` aktualisiert
- [ ] Commit „Sprint 0b: contract frozen“

---

## Sprint 1 – System, NVS, WiFi, main

### PROMPT S1

```
Sprint S1 of the ADD Demo System firmware. Read first:
- .clinerules, docs/CONTRACT.md §1, §9, §12, §12.1
- src/logic/wifi_logic.h and test/native/test_wifi_logic/test_wifi_logic.cpp
- src/config.h, src/system/system.h, src/system/nvs_config.h, src/comm/wifi_manager.h,
  src/comm/log_buffer.h, src/app/control_task.h, src/app/snapshot.h, src/comm/http_server.h,
  src/display/display.h, src/logic/params.h

Implement these files (and nothing else):
1. src/system/system.cpp      – exactly as documented in system.h. Task watchdog with the
                                ESP-IDF 5 API (esp_task_wdt_config_t; use esp_task_wdt_reconfigure()
                                if the core already initialised the TWDT, else esp_task_wdt_init()).
                                Boot banner: FW_VERSION, ESP_ARDUINO_VERSION_STR, esp_get_idf_version(),
                                esp_reset_reason().
0. src/logic/wifi_logic.cpp   – pure C++ (like all of src/logic), rules exactly as in wifi_logic.h.
                                DoD: pio test -e native -f native/test_wifi_logic green.
2. src/system/nvs_config.cpp  – Preferences, keys exactly as listed in nvs_config.h, "ver" = 4.
                                nvsLoadParams: start from paramsDefaults(), read each key, replace any
                                value outside the CONTRACT §9 range by its default. Never store flags.
3. src/comm/wifi_manager.cpp  – as documented: WiFi.mode(WIFI_AP_STA), softAP always on,
                                station via staStep() every 100 ms (BEGIN -> WiFi.begin(ssid, pass)
                                without waiting; DISCONNECT -> WiFi.disconnect(false, false) so the
                                AP stays up; link_up = WiFi.status() == WL_CONNECTED), log every
                                state change, mDNS "add-demo". wifiLoop never blocks > 50 ms.
4. src/comm/log_buffer.cpp    – ring buffer LOG_LINES x LOG_LINE_LEN, spinlock, vsnprintf,
                                echo to Serial.
5. src/main.cpp:
   setup():
     systemInit(); SPIFFS.begin(true);
     Params p; nvsLoadParams(p);
     cmdQueueInit();
     if (!controlTaskStart(p)) logLine("FATAL: control task start failed");
     displayStart();
     wifiInit(p.wifi_ssid, p.wifi_pass);
     httpServerInit();
     start the network task: xTaskCreatePinnedToCore(netTask, "net", NET_TASK_STACK, nullptr,
                                                     NET_TASK_PRIO, nullptr, NET_TASK_CORE);
   netTask loop (every 5 ms with vTaskDelay):
     wifiLoop(millis()); httpServerLoop();
     if (snapshotTakePersistRequest()) { Params q; SafetyFlags f; snapshotGetConfig(q, f);
                                         bool ok = nvsSaveParams(q); logLine(...); }
     if (snapshotTakeRebootRequest())  { reboot_at = millis() + REBOOT_DELAY_MS; }   // V1.3
     if (reboot_at && (int32_t)(millis() - reboot_at) >= 0) systemRestart();        // pattern played
   loop(): vTaskDelay(portMAX_DELAY);   // Arduino loop task is not used

Rules: ESP-IDF 5 APIs only. No String in log_buffer. English comments.
Definition of done:
- pio run -e firmware-partial -s   -> compiles; unresolved symbols of later sprints are fine
- frozen check clean
Report: files, decisions, anything in the headers that was unclear.
```

**Review-Checkliste S1**
- [ ] Watchdog mit IDF-5-API, Control-Task wird in `control_task.cpp` registriert (nicht doppelt)
- [ ] NVS: Bereichsprüfung beim Laden, Flags nie gespeichert
- [ ] `wifiLoop` blockiert nicht (kein `WiFi.begin` mit Warteschleife im Loop)
- [ ] AP bleibt aktiv, auch während der Station-Versuche und in der 2-min-Pause
- [ ] NVS `ver` = 4, Schlüssel `estnc` und `shold` vorhanden (V1.5)
- [ ] Persist/Reboot nur im Netzwerk-Task
- [ ] Nach S2.6 am Gerät: Boot-Banner, AP sichtbar, Startsignal, Reboot über `REBOOT` (Signal, dann Neustart)
- [ ] Router-Login: `SET wifi_ssid/wifi_pass`, `REBOOT` → IP im Log; Router aus → 3 Versuche, 2 min Pause

---

## Sprint 2.1 – Typen, CRC32, Sicherheitslogik

### PROMPT S2.1

```
Sprint S2.1. Pure C++17 logic, host-tested. Read first:
- .clinerules (rule 3: src/logic is pure C++, no Arduino/IDF/heap)
- docs/CONTRACT.md §3, §4, §6
- src/logic/types.h, src/logic/crc32.h, src/logic/safety_logic.h
- test/native/test_types_crc/test_types_crc.cpp
- test/native/test_safety_logic/test_safety_logic.cpp

Implement exactly:
- src/logic/types.cpp         all name functions of types.h
- src/logic/crc32.cpp         crc32Ieee (bitwise or table, zlib-compatible), crc32Hex,
                              crcFrameAppend, crcFrameVerify – exactly as documented
- src/logic/safety_logic.cpp  safetyStep must follow rules R1..R7 in the documented ORDER.
                              Implement it as a switch over cur.state after the R1 check.
                              Keep it readable; no if/else chains spanning states.

Example of the expected style (rule R6, READY + HV_ON):
    case SafetyState::READY:
        outputsOff(n);   // hv_on = shutter_open = shutter_auto = false (V1.5)
        if (ev == SafetyEvent::HV_ON) {
            if (safetyHvConditionsOk(in, cfg)) { n.state = SafetyState::RUNNING; n.hv_on = true; r.api = ApiError::OK; }
            else r.api = ApiError::PRECONDITION;
        } else if (...) ...

Hints:
- crcFrameAppend adds exactly 17 characters; it needs cap >= len + 18.
- crcFrameVerify uses the LAST occurrence of ,"crc":" and accepts trailing whitespace.
- The random invariant test (test_invariants_random_sequences) fails if any rule lets the
  shutter be open without HV, HV on outside RUNNING, or outputs on in FAULT/WAIT_ACK/RECOVER.
- V1.5: EVERY way of closing the shutter must also clear shutter_auto (use one helper).
  SHUTTER_AUTO_OPEN needs pos > maint_max_mm (measuring zone) and sets shutter_auto (also
  when the shutter was already open: take-over). In RUNNING an automatic shutter closes
  when pos <= maint_max_mm. safetyShutterAutomation has the new V1.5 signature (hold time).

Definition of done:
  pio test -e native -f native/test_types_crc -f native/test_safety_logic   -> all green
  frozen check clean
```

**Review-Checkliste S2.1**
- [ ] R1 vor allen Zustandsregeln; FAULT/WAIT_ACK von R1 ausgenommen, RECOVER nicht
- [ ] WAIT_ACK: `motor_stop` in jedem Schritt
- [ ] RUNNING prüft Bedingungen bei **jedem** Ereignis zuerst
- [ ] Keine statischen Variablen in `src/logic`

---

## Sprint 2.2 – Messkette, Parameter

### PROMPT S2.2

```
Sprint S2.2. Pure C++17 logic. Read first:
- .clinerules, docs/CONTRACT.md §7, §9
- src/logic/measure_logic.h, src/logic/params.h, src/logic/types.h
- test/native/test_measure_logic/test_measure_logic.cpp
- test/native/test_params/test_params.cpp

Implement exactly:
- src/logic/measure_logic.cpp
- src/logic/params.cpp

Hints:
- Sensor curve gives MILLIMETRES: dist_mm = c.k_mm * powf(mv / 1000.0f, c.exponent). Use the
  SensorCurve fields (defaults 168.8 / -1.398, V1.4.2) – never hard-code the constants.
- medianMean sorts in place (std::sort from <algorithm> is allowed, it does not allocate).
- params: table-driven implementation recommended (array of {key, member pointer, lo, hi}).
  On ANY error nothing changes and *persist stays untouched.
- paramsToJsonMember must produce the exact string of test_config_json_exact_defaults
  (snprintf, "%.1f"). Never write wifi_pass.
- String rule for wifi_ssid / wifi_pass: printable ASCII 0x20..0x7E without '"' and '\'.

Definition of done:
  pio test -e native -f native/test_measure_logic -f native/test_params   -> all green
  frozen check clean
```

**Review-Checkliste S2.2**
- [ ] Kurve in mm, `mv ≤ 1` → 9999
- [ ] Kalibrierung: ungültig → Rohwert unverändert
- [ ] Flags nie `persist = true`

---

## Sprint 2.3 – Ablaufsteuerung Achse

### PROMPT S2.3

```
Sprint S2.3. Pure C++17 logic. Read first:
- .clinerules, docs/CONTRACT.md §2, §8
- src/logic/motion_seq.h (the documentation of motionTick is the specification)
- test/native/test_motion_seq/test_motion_seq.cpp and test/native/fake_axis.h
  (fake_axis.h shows how the driver reacts to each DrvOp)

Implement exactly: src/logic/motion_seq.cpp

Structure:
- small static helpers: setPhase(s, phase, now) (also sets phase_start_ms),
  moveTo(s, target, hz) (sets s.target_steps, returns the DrvRequest), isHoming(phase)
- motionTick in this order: (1) homing timeout, (2) unexpected limit switch while
  drv_running, (3) compute "arrived", (4) switch over phase.
- Every start function: reset `out` first; on error leave the sequencer unchanged.

Definition of done:
  pio test -e native -f native/test_motion_seq   -> all green
  frozen check clean
```

**Review-Checkliste S2.3**
- [ ] Endschalter hinten in allen HOME-Phasen erlaubt, sonst Fehler
- [ ] Not-Halt während Bewegung → `homed = false`
- [ ] genau ein `DrvRequest` pro Aufruf

---

## Sprint 2.4 – Anwendungslogik

### PROMPT S2.4

```
Sprint S2.4. Pure C++17 logic that combines all logic modules. Read first:
- .clinerules, docs/CONTRACT.md §6.4, §6.5, §10
- src/logic/app_logic.h (step lists of appTick/appCommand are the specification)
- src/logic/safety_logic.h, motion_seq.h, measure_logic.h, params.h (already implemented)
- test/native/test_app_logic/test_app_logic.cpp and test/native/fake_axis.h

Implement exactly: src/logic/app_logic.cpp

Recommended private helpers (static, in the .cpp):
- MotionInputs motionIn(const HwInputs&)
- SafetyInputs safetyIn(const AppState&, const HwInputs&, const AppConfig&, bool motor_fault)
- ApiError applySafety(AppState&, SafetyEvent, const SafetyInputs&, const HwInputs&,
                       const AppConfig&, DrvRequest& d, bool& acqEnded, bool& enteredFault)
  -> calls safetyStep, stores the new status, and on motor_stop with active motion calls
     motionEmergencyStop (sets d = FORCE_STOP).

Pay attention to:
- appTick: follow steps 1..10 of the header in this exact order.
- Events raised inside appTick by motionStop / motionEmergencyStop (seq.ev.acq_ended) must be
  consumed in the SAME appTick before the shutter automation (step 8).
- appCommand: STOP leaves seq.ev.acq_ended pending for the next appTick.
- V1.5 shutter over a measuring programme: step 8 uses prog_active_ms (updated while
  seq.mode != NONE) and params.shutter_hold (seconds -> ms) for safetyShutterAutomation;
  acq_ended does NOT close the shutter any more. STOP closes an AUTOMATIC shutter at once
  (safetyStep SHUTTER_CLOSE inside appCommand); a manual shutter is left alone.
- Comm watchdog: arm only on SUCCESS of INIT, MOVE, PARK, SERVICE, START_*, HV_ON (API -> true, LOCAL -> false).
- V1.2: PARK -> motionStartMove(cfg.motion.park_target_mm, approach_speed);
  SERVICE -> motionStartMove(params.maint_pos, approach_speed).
- V1.3: also implement src/logic/sound_logic.cpp (test_sound_logic). appTick step 11:
  BOOT on the first tick, ESTOP/FAULT on fault entry, HOMED; out.buzzer, out.sound,
  out.heartbeat. appCommand: CLICK for LOCAL + OK, REBOOT pattern, SET estop_nc only
  while idle and HV off. All speeds are mm/s.
- V1.1 LOCAL priority (CONTRACT §6.6): at the start of appCommand, after GET_STATE/GET_CONFIG:
  source LOCAL -> local = true, local_last_ms = now, comm_armed = false (even if the gate
  rejects the command afterwards); source API && local && appLocalBlocksApi(id) -> return
  LOCAL_CONTROL before the state gate. Release in appTick step 9b (wrap-around safe).
- V1.1 driver request merge: replace out.drv only if rank(d) >= rank(out.drv);
  rank NONE 0, MOVE_TO/RUN_BACKWARD 1, STOP 2, FORCE_STOP/ZERO_HERE 3.
- appCmdIsPriority / appLocalBlocksApi are simple switch tables (header lists them).
- appFillProc: p.local = s.local.

Definition of done:
  pio test -e native   -> ALL suites up to test_app_logic green
  frozen check clean
```

**Review-Checkliste S2.4** (zusätzlich von Claude prüfen lassen: `app_logic.cpp` + Header)
- [ ] Not-Halt überschreibt jede andere Fahranforderung
- [ ] Modus endet, sobald RUNNING verlassen wird
- [ ] Watchdog-Ent-/Bewaffnung wie CONTRACT §6.5
- [ ] LOCAL-Prüfung vor dem Zustandsgate; LOCAL-Befehl setzt `local` auch bei Ablehnung
- [ ] STOP schwächt FORCE_STOP / ZERO_HERE nie ab
- [ ] V1.5: Ende einer Profilfahrt schließt den Shutter **nicht**; Nachlaufzeit aus `params.shutter_hold`, Zeitbasis `prog_active_ms` auch in der Anfahrphase

---

## Sprint 2.5 – Hardware-Schicht (du + KI)

Grundlage ist der funktionierende Smoketest-Code. Die KI überträgt ihn in die
HAL-Struktur; du prüfst an der Hardware.

### PROMPT S2.5

```
Sprint S2.5. Hardware layer. Read first:
- .clinerules, docs/CONTRACT.md §11, src/config.h
- src/hal/hal_io.h, src/hal/motion_drv.h
- src/smoketest/smoketest.cpp – the WORKING reference for FastAccelStepper I2S_MUX,
  i2sMuxSetBit, inputs and ADC on this board. Reuse its calls; do not invent other APIs.
- .pio/libdeps/firmware/FastAccelStepper/src/FastAccelStepper.h (real API)

Implement exactly:
- src/hal/motion_drv.cpp – engine/stepper setup as in the header comment, motionDrvApply
  mapping exactly as documented, speed conversion |getCurrentSpeedInMilliHz()| / 1000 / STEPS_PER_MM (mm/s).
- src/hal/hal_io.cpp     – inputs with debounce (limit switches both directions,
  E-stop: activation immediate, release after INPUT_DEBOUNCE_MS), outputs, non-blocking
  halSetBuzzer(on) via engine->i2sMuxSetBit(I2SO_TO_SLOT(I2SO_BUZZER), on) (the pattern
  comes from sound_logic, the HAL only sets the bit). HV LED (GPIO 4), shutter LED (GPIO 32)
  and heartbeat LED (GPIO 2) are plain digitalWrite outputs, set LOW first in halInit().
  halSetEstopActiveHigh(bool) stores the active level used by halUpdateInputs (default LOW).
  motionDrvSpeedMmS(): |getCurrentSpeedInMilliHz()| / 1000 / STEPS_PER_MM.

No delay(), no Serial, no heap in any of these functions.
Definition of done:
  pio run -e firmware-partial -s -> compiles
  frozen check clean
Do NOT flash. The user tests on hardware.
```

**Hardware-Prüfung S2.5 (du)**
- [ ] Temporär `pio test -e esp32-test` (Tests ohne Motorlauf zuerst), dann 2-mm-Fahrt
- [ ] Not-Aus-Aktivierung ohne Verzögerung (Oszilloskop: Not-Aus ↔ letzter Step-Puls)

---

## Sprint 2.6 – Integration Control-Task (du + KI)

### PROMPT S2.6

```
Sprint S2.6. Integration. Read first: .clinerules, docs/CONTRACT.md §1, §10,
src/app/control_task.cpp (provided, FROZEN except for compile fixes), src/main.cpp.

Task:
1. Build the complete firmware: pio run -e firmware -s
2. Fix compile/link errors. control_task.cpp may only be changed to fix compile errors;
   list every change with the reason. No behavioural changes.
3. Review control_task.cpp against CONTRACT §10 and §6.7 and report deviations (do not fix
   them). Check in particular: step 3a executes ALL commands from
   cmdQueueReceivePriority(), step 3b at most one from cmdQueueReceive() only if
   out.drv.op == NONE, replies AFTER applyOutputs, CycleStats (work time, period between cycle
   starts, overruns, priority latency from cmd.t_enq_us) published every cycle,
   cmdQueueSendAndWait serialised by its mutex.
Definition of done: firmware builds; ALL native tests green (incl. test_command_queue,
which compiles the real src/app/command_queue.cpp against a FreeRTOS shim); report with changes.
Do NOT flash.
```

**Hardware-Prüfung S2.6 (du)** – über `scripts/add_client.py` erst ab S3; bis dahin Display/Serial:
- [ ] Boot, Watchdog läuft, kein Reset in 5 min
- [ ] Nach S3: `INIT`, `MOVE`, Not-Aus → FAULT → ACK → SAFE_START
- [ ] Nach S3: `GET_STATE` zeigt `cycle_max_us` < 500, `overruns` 0, `period_max_us` < 1500

---

## Sprint 3 – JSON, Parser, HTTP

### PROMPT S3

```
Sprint S3. Read first:
- .clinerules, docs/CONTRACT.md §4, §5
- src/logic/proc_json.h, src/logic/api_parse.h, src/comm/http_server.h, src/comm/log_buffer.h,
  src/app/snapshot.h, src/app/command_queue.h, src/logic/params.h
- test/native/test_proc_json/test_proc_json.cpp, test/native/test_api_parse/test_api_parse.cpp
- docs/reference/ADD_Specification_v1_0.md sections 5.3 (/add.html) and 5.4 (llms.txt) only

Implement exactly:
1. src/logic/proc_json.cpp  – snprintf only; float sanitising exactly as in the header.
2. src/logic/api_parse.cpp  – ArduinoJson 7 (JsonDocument). Check order exactly as documented.
3. src/comm/http_server.cpp – WebServer on HTTP_PORT, all routes of http_server.h.
   POST /api: read server.arg("plain"); reject bodies > API_BODY_MAX with BAD_JSON.
   Response buffers: static char[1024] (GET_CONFIG needs ~420 bytes extra). Never String
   concatenation for JSON. /add.html: HTML page with the add.json content in
   <script type="application/ld+json"> and in <pre> (HTML-escape < > &), headers
   Cache-Control: no-store, no-cache, must-revalidate and Pragma: no-cache.
   GET_STATE extra: stateExtraToJson(snapshot.state, snapshot.err, snapshot.local,
   cycle stats from snapshotGetCycleStats()) – V1.1, implemented in proc_json.cpp.
   LOCAL_CONTROL (-10) is answered with HTTP 200 like every other command error.
   Only this (network) task may call cmdQueueSendAndWait().

Definition of done:
  pio test -e native  -> all suites green
  pio run -e firmware -s -> clean
  python scripts/add_client.py selftest -> ok
  frozen check clean
Do NOT flash.
```

**Hardware-Prüfung S3 (du)**
```bash
python scripts/add_client.py --host 192.168.4.1 proc
python scripts/add_client.py cmd SET key=interlock value=true
python scripts/add_client.py cmd SET key=cooling_flow value=true
python scripts/add_client.py cmd INIT
python scripts/add_client.py cmd HV_ON
python scripts/add_client.py cmd START_TRAVERSE start=80 end=240 speed=10   # V1.5: Messzone > 60 mm, mm/s
# innerhalb von 5 s: GET_STATE als Heartbeat, sonst COMM_TIMEOUT (gewollt!)

# V1.1 – Last und Zykluszeit (keine Bewegung):
python scripts/stress_api.py --host 192.168.4.1 flood
python scripts/stress_api.py --host 192.168.4.1 parallel     # 8 Threads, keine vertauschten Antworten
python scripts/stress_api.py --host 192.168.4.1 soak --minutes 10
# STOP-Latenz – BEWEGT DIE ACHSE, nur in RUNNING, Not-Aus in Reichweite:
python scripts/stress_api.py --host 192.168.4.1 latency
```

---

## Sprint 4a – Weboberfläche

### PROMPT S4a

```
Sprint S4a. Web UI (vanilla HTML/CSS/JS, no framework, no CDN). Read first:
- .clinerules, docs/CONTRACT.md §4 (JavaScript crc32/frame/verify – copy them verbatim),
  §5 (all JSON formats), §6.5 (heartbeat), §9 (parameters)
- docs/Pflichtenheft_Demoanlage_ADD_Slim_V1.7.md §5.7 (content of both pages)
- docs/reference/style_guide_excerpt_S4.md (CSS variables, themes, components – use them exactly)

Deliver:
- data/www/index.html, data/www/settings.html – each self-contained (inline CSS/JS).
- Fonts: @font-face from /www/fonts/ubuntu-regular.woff2, ubuntu-bold.woff2,
  ubuntu-mono-regular.woff2 (the user copies the files), fallback sans-serif/monospace.
- Themes: body classes theme-dark/theme-light/theme-night/theme-bw, localStorage key
  "add-dashboard-theme" (wrap in try/catch), variables exactly from Appendix A.

Behaviour (index.html):
- poll GET /proc every 500 ms, verify CRC, show "CRC error" pill on failure, "Offline" on fetch error
- values: THICK shows "---" if !measure.valid or !measure.on_material
- buttons: INIT, HV ON/OFF (toggle by safety.hv_on), MEASURE, TRAVERSE (no params ->
  device defaults), STOP, ACK (enabled only if state == "WAIT_ACK")
- V1.1: if /proc "local" is true: show a "LOCAL" pill with the text "Operator at display
  has priority" and disable INIT, HV ON, MEASURE, TRAVERSE, ACK (STOP and HV OFF stay
  enabled). err -10 (LOCAL_CONTROL) -> toast "Operator at display has priority".
- V1.1: interlock, cooling and temp are labelled "SIM" (simulated) next to their values.
- V1.1: on err -7 (BUSY) never resend automatically; the next /proc decides.
- confirmation dialog (style guide 7.8) for HV ON and ACK
- while safety.comm_armed: send GET_STATE every 2 s
- every /api response: show toast with msg if err != 0
- chart: canvas, last 100 values of dist and thick, auto-scale, no library
- validation strip: fetch /add once, show validation.validated_by entries (or "not validated")
settings.html: load GET_CONFIG on open; one input per parameter with its range from
CONTRACT §9; send SET on change; calibration buttons (CALIB near/far/base/save/reset);
WiFi ssid/pass + REBOOT button with confirmation.
V1.3: speeds are mm/s everywhere (SPEED panel, traverse_speed 1..20). Settings page:
estop_nc toggle with a warning text ("simple switch, no wire-break detection" for false;
"normally-closed contact with wire-break detection" for true), enabled only when
/proc shows state != RUNNING and motion.state == "idle"; WiFi section explains that the
AP stays on and the router login is effective after REBOOT.

Size budget: each page < 40 KB. Test locally: open the files with a small mock is optional.
Definition of done: files exist, no external URLs (grep -n "http" data/www/*.html shows only
relative paths), frozen check clean.
```

**Review-Checkliste S4a**
- [ ] Ohne Internet (AP-Modus) vollständig nutzbar
- [ ] CRC-Prüfung von `/proc` und Antworten
- [ ] Heartbeat aktiv, solange `comm_armed`
- [ ] B&W-Theme: alle Zustände ohne Farbe unterscheidbar (Text/Symbol)
- [ ] LOCAL-Pill, Start-Tasten gesperrt solange `local`; SIM-Kennzeichnung

---

## Sprint 4b – Display

### PROMPT S4b

```
Sprint S4b. Local display. Two parts: pure logic first (tested), then drawing.
Read first:
- .clinerules, docs/CONTRACT.md §13, src/display/display.h, src/display/lgfx_config.h
- src/logic/display_logic.h and test/native/test_display_logic/test_display_logic.cpp
- docs/Pflichtenheft_Demoanlage_ADD_Slim_V1.7.md §5.8 (layout, fault screen, colours – binding)
- src/app/snapshot.h, src/app/command_queue.h, src/logic/types.h
- .pio/libdeps/firmware/LovyanGFX/src/ (real API; examples/ for getTouch)

Part 1 – src/logic/display_logic.cpp (pure C++17, like all of src/logic):
  screenModeOf, ackCauseActive, ackPressUpdate exactly as documented in the header.
  DoD part 1: pio test -e native -f native/test_display_logic green.

Part 2 – src/display/display.cpp
- displayStart(): lcd.init(), setRotation(1), draw static frame, create task
  (core 1, priority 1, stack 6144). Loop every DISPLAY_TOUCH_POLL_MS (20 ms):
  read touch; every DISPLAY_PERIOD_MS (200 ms) also snapshotGet() and redraw.
- Redraw only zones whose content changed (cache last values); setTextDatum + fillRect
  background (no full-screen clear, no flicker). A change of ScreenMode redraws the
  lower part of the screen completely once.
- ScreenMode NORMAL: layout of §5.8, 7 buttons (INIT HV MEAS TRAV PARK SERV: 64 px, text
  size 2; STOP: 96 px, text size 3). Label SERV sends CmdId::SERVICE. Button press = touch-down edge inside the
  button; brighten 50 ms; send Command{source = LOCAL, want_reply = false} via
  cmdQueueSend(). HV button: HV_OFF if snapshot.safety.hv_on else HV_ON. STOP always active.
  "LOCAL" badge in the header while snapshot.local. Flags labelled "(SIM)".
- ScreenMode FAULT and ACK: hide all 7 buttons; big fault banner
  "FAULT: <errorCodeName(err)>" in red with the remedy text of §5.8.
  ACK mode: one area x 0..479, y 260..319.
    cause = ackCauseActive(snapshot, SafetyConfig{})
    fire  = ackPressUpdate(state, mode, cause, touch inside area, millis(), ACK_HOLD_MS, &progress)
    cause -> grey area "CAUSE ACTIVE: <name> - RELEASE FIRST"
    else  -> yellow area "ACK - HOLD 1 s" + progress bar (redraw progress every poll)
    fire  -> send ACK with source LOCAL (exactly once).
  Call ackPressUpdate on EVERY poll in every mode (it needs the touch history).
- A touch on a hidden button never sends anything.
- IP line: wifiGetIp() (comm/wifi_manager.h).
No calls into hal/ or app/safety logic (display_logic.h is the only logic). No Serial in the loop.
Definition of done: all native suites green (regression rule); pio run -e firmware -s clean;
frozen check clean. Do NOT flash.
```

**Hardware-Prüfung S4b (du)**
- [ ] Alle 7 Tasten wirken (PARK fährt auf 2 mm, SERV auf `maint_pos`); `LOCAL`-Badge erscheint nach Tastendruck, verschwindet nach 30 s Ruhe
- [ ] Not-Aus drücken: Tasten verschwinden, Banner `FAULT: ESTOP`, ACK-Fläche grau
- [ ] Not-Aus lösen: Fläche gelb; kurzer Tipp quittiert nicht; 1 s halten quittiert genau einmal
- [ ] STOP gedrückt halten, dabei Not-Aus auslösen und wieder lösen: Finger bleibt liegen → **keine** Quittierung

---

## Sprint 5 – Integration und Abnahme (du)

- Abnahme nach Pflichtenheft Kapitel 10, in der dort genannten Reihenfolge.
- V1.1-Messungen mit `scripts/stress_api.py`: `flood`, `parallel`, `soak` (10 min), `local`,
  `latency` (bewegt die Achse – nur mit Not-Aus in Reichweite).
- `pio test -e esp32-test` nach Freigabe.
- Fehleranalyse mit Claude: Log (`/log`), `/proc`-Mitschnitt und betroffene Datei übergeben.

---

## Sprint 6 – ADD-Dokument und Demo-Skripte (du + Claude)

### PROMPT S6

```
You are an expert in the ADD (AI Device Description) standard v1.0.
Read completely before writing anything:
- docs/reference/ADD_Specification_v1_0.md (normative: top-level schema §9, blocks §10)
- docs/reference/ADD_Developer_Guide_v1_0.md
- docs/reference/style_guide_excerpt_S4.md (only for the optional "ui" block)
- docs/CONTRACT.md (the device behaviour – every statement in add.json must match it)
- docs/Pflichtenheft_Demoanlage_ADD_Slim_V1.7.md §1, §6, §7

Task 1 – data/add.json (served at /add, max. 32 KB, valid JSON):
- Exactly the header fields schema, version, spec_url, spec_license and the seven blocks
  autonomy, device, security, interfaces, actions, rules, validation – plus the optional
  "ui" block of the style guide. No other top-level keys (no "live_data").
- autonomy: compute the three scores honestly for THIS demo device and propose the level.
  Pflichtenheft §7 states the concept: Level 2 (the demo models a radiation source; the AI
  shall behave as it would at the real gauge; under-declaration is not allowed).
  Before writing, show me the scores and the reasoning and wait for my confirmation.
- device: include "ip" (ask me), firmware "1.0.0", hardware "ESP32 (MKS DLC32 V2.1)",
  doc_url "/doc" with doc_url_note.
- security: network_scope "local", authentication "none", remote_access false, and an
  "enforcement" text listing everything the device enforces itself (CONTRACT §5.2 gate,
  ranges, zones park/service/measuring and shutter rules incl. hold time, HV conditions,
  comm watchdog, fail-safe flags).
- interfaces: one HTTP interface with data entries /proc, /api, /add, /add.html, /llms.txt,
  /doc, /log, /, /settings. Describe the CRC frame rule (CONTRACT §4) in the interface
  description, including the two test vectors.
- actions: one action per API command (and one "read_process_data" for GET /proc) with
  parameters, ranges, units, safe, reversible, idempotent, requires_confirmation,
  confirmation_scope, actor. HV_ON, START_*, INIT, ACK require confirmation.
- rules: mandatory rules 1 and 2 at positions 1 and 2, standard rules, the Level-2
  session-integrity rules if level >= 2, then device rules: heartbeat GET_STATE every
  <= 4 s during remote operation, verify /proc after each command, stop and report if
  err != 0, ACK only after the user confirmed the situation is safe, never retry HV_ON
  automatically, "/add describes capabilities only, live data via /proc".
  V1.1 rules: "OK means accepted and outputs set; the end of a motion is only visible in
  /proc"; "BUSY: read /proc before resending"; "if /proc local is true or a command returns
  -10 LOCAL_CONTROL, a person operates the device locally: do not retry, wait and inform
  the user; STOP and HV_OFF remain available"; "interlock, cooling_flow and temp are
  simulated demo signals". Error codes 0..-10 in the action descriptions.
- validation: add_version "1.0", validated_by [] (draft).
- ui: style_guide_url, style_guide_version "1.0", theme_default "dark".

Task 2 – data/doc.md: operator documentation (English, < 15 KB) with state diagram,
error codes and their remedies, calibration procedure.

Task 3 – scripts/demo_traverse.py (stdlib only, reuse scripts/add_client.py functions):
scenario: SET flags, INIT (wait homed), HV_ON, START_TRAVERSE, poll /proc every 200 ms
with GET_STATE heartbeat every 2 s, record ts/pos/dist/thick/on_material/valid (only samples
with motion.mode == "traverse"), HV_OFF at the end, detect material edges, print step
thicknesses and write profile.csv. Option --passes N: back-and-forth runs (start/end swapped,
next START_TRAVERSE within shutter_hold, so the shutter stays open – CONTRACT §6.4).

Task 4 – scripts/demo_safety.py: traverse, then SET temp=31 (HV drop, no fault),
SET temp=35 (fault), show ACK rejected, SET temp=18, ACK, show state SAFE_START/READY.

Then run the ADD validation (spec §13) against the live device together with me.
```

---

## Anhang: Contract V1.5 (Zonen, Shutter über das Messprogramm, 12.10.2026)

Keine neuen Prompts – die Regeln stehen in den Headern und Tests:
- **S2.1:** `SafetyStatus::shutter_auto`, Invarianten I7/I8, `SHUTTER_AUTO_OPEN` nur in der
  Messzone (> 60 mm) inkl. Übernahme eines manuell geöffneten Shutters, automatischer Shutter
  schließt beim Einfahren in die Wartungszone, neue Signatur von `safetyShutterAutomation`.
- **S2.2:** Bereiche `measure_pos`/`traverse_*` 61 … 242 mm, `traverse_start` 80, neuer
  Parameter `shutter_hold` (0 … 10 s, 3.0) – auch in `paramsToJsonMember` nach `traverse_speed`.
- **S2.3:** `MotionConfig::measure_min_mm` (60) statt `park_pos_mm` für Messpositionen.
- **S2.4:** Nachlaufzeit (`prog_active_ms`), STOP schließt den automatischen Shutter sofort.
- **S1:** NVS-Schema 4, Schlüssel `shold`.
**Review aller Sprints ab S2.1:** Zonengrenzen nie hart codieren, immer `SafetyConfig`/`MotionConfig`.

## Anhang: Contract V1.4 (Smoketest 0a, 11.10.2026)

Keine neuen Prompts – die Prompts verweisen auf die Header, die Werte stehen dort:
Achse 0 … 242 mm (`MotionConfig::axis_max_mm`), 78,49 Schritte/mm (`MM_PER_REV` 40,77),
`DIR_POSITIVE_HIGH` = false, Parameterbereiche 21 … 242 mm.
**S2.5 Review zusätzlich:** `FORCE_STOP` → `forceStopAndNewPosition(getCurrentPosition())`,
**nicht** `forceStop()` (siehe `motion_drv.h`); der Nothalt wird in jedem Zyklus wiederholt,
solange der Treiber läuft – nicht zu einem einmaligen Befehl „optimieren“.
**V1.4.2 – S2.2:** Sensorkurve 168,8·V^-1,398, Messbereich 35 … 300 mm, `measure_pos` 155 –
alles als Standardwerte in `measure_logic.h` / `params.h`, nichts davon in der Implementierung hart codieren.
**V1.4.1 – Review S2.2, S2.3, S2.4, S3 zusätzlich:** Bereichsprüfungen für Gleitkommawerte als
`!(lo <= x && x <= hi)` schreiben (NaN fällt sonst durch); `apiParseRequest` lehnt Zahlen ab,
die als float nicht endlich sind. Die neuen Tests prüfen genau das.

## Anhang: Änderungen V2.2 → V2.3 (Contract V1.3)

| Sprint | Neu |
|---|---|
| S0a | `move` in mm/s, `home` mit 5 mm/s, `estop <low\|high>`, `sta <ssid> <pass>`, Herzschlag-LED |
| S1 | `wifi_logic.cpp` (getestet), AP immer an + Station mit Wiederverbindung, NVS ver 3, Reboot-Verzögerung |
| S2.2 | `traverse_speed` in mm/s (1 … 20), Parameter `estop_nc` |
| S2.3 | alle Geschwindigkeiten mm/s, `mmPerSecToHz`, Referenzfahrt 5 mm/s, Timeout 70 s |
| S2.4 | `sound_logic.cpp` (getestet), Signale, Herzschlag, `estop_nc` nur im Stillstand |
| S2.5 | `halSetBuzzer`, `halSetHeartbeat`, `halSetEstopActiveHigh`, `motionDrvSpeedMmS` |
| S4a/b | mm/s, `ESTOP(NO)`-Warnung, WLAN-Zustand, Einstellung `estop_nc` |

## Anhang: Änderungen V2.1 → V2.2 (Contract V1.2, Hardware-Vortest)

| Sprint | Neu |
|---|---|
| S0a | HV-LED an GPIO 4 (`hv`-Befehl schaltet GPIO); `scan` identifiziert nur noch Buzzer, Enable, Step, Dir |
| S2.1 | Ereignis `SHUTTER_AUTO_OPEN`; manuelles Öffnen nur in der Wartungszone; Befehle PARK/SERVICE im Gate |
| S2.2 | Sensorkurve 121,5·V^-1,142; `maint_pos`; neue Defaults und Bereiche |
| S2.3 | Achse 0 … 248 mm, Parkzone ≤ 20 mm |
| S2.4 | PARK/SERVICE in `appCommand`, LOCAL-Sperre, Watchdog |
| S2.5 | `halSetHvLed` über GPIO 4 |
| S4b | 7 Tasten |

## Anhang: Änderungen V2 → V2.1 (Contract V1.1)

| Sprint | Neu |
|---|---|
| alle | Regressionsregel in DoD und `.clinerules` |
| S2.4 | LOCAL-Vorrang, Prioritätstabellen, Fahrauftrags-Rang; 13 neue Szenariotests |
| S2.6 | Prioritäts-Queue mit Überlauf-Latch, Antwort nach den Ausgängen, Zykluszeitmessung (Review-Punkte); `test_command_queue` muss grün sein |
| S3 | `stateExtraToJson` (mit `CycleStats`) für GET_STATE; `stress_api.py` flood/parallel/soak/latency |
| S4a | LOCAL-Pill, SIM-Kennzeichnung, keine automatische BUSY-Wiederholung |
| S4b | `display_logic.cpp` (getestet) + Fehlerbild mit einer ACK-Fläche, 1 s halten |
| S6 | Level 2 als Konzept, Regeln zu LOCAL/BUSY/OK, Fehlercode −10 |

## Anhang: Was sich gegenüber V1 geändert hat

| V1 | V2 |
|---|---|
| Jeder Prompt enthält alles, Modell erfindet Schnittstellen | Prompts verweisen auf eingefrorene Header und Tests im Repository |
| Tests vom implementierenden Modell | Tests vorab eingefroren, gegen Referenzimplementierung geprüft |
| S2 mit 13 Dateien | S2.1–S2.6, je 1–2 Dateien |
| `platform = espressif32`, IDF-4-APIs | pioarduino, IDF-5-APIs, Verbotsliste in `.clinerules` |
| Style Guide per URL | lokaler Auszug `docs/reference/style_guide_excerpt_S4.md` (~6k Tokens) |
| ADD-Feldnamen frei erfunden | streng nach ADD Spec v1.0, Spec liegt lokal bei |
