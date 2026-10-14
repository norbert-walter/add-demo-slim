# ADD Demo System (Slim) – Firmware-Projekt

1:5-Modell eines radiometrischen Dickenmessers zur Demonstration von
**ADD – AI Device Description**. ESP32 (MKS DLC32 V2.1), PlatformIO/pioarduino,
agentenbasierte Entwicklung mit code-server + Cline.

## Wo steht was

| Pfad | Inhalt |
|---|---|
| `docs/Pflichtenheft_Demoanlage_ADD_Slim_V1.7.md` | Anforderungen (deutsch) |
| `docs/CONTRACT.md` | verbindlicher Schnittstellenvertrag (englisch, für Agenten) |
| `docs/Sprint_Prompts_Demoanlage_ADD_Slim_V2.md` | Sprint-Plan V3 (Reihenfolge, Begründung, Ablauf), bisherige Prompts |
| `docs/sprints/` | Sprint-Dateien für Cline (eine Aufgabe pro Datei) |
| `docs/reviews/` | Review-Protokolle je Sprint |
| `docs/Smoketest_Protokoll.md` | Sprint 0a an der Hardware |
| `docs/reference/` | ADD Spec, Developer Guide, Style Guide (+ Auszug für S4) |
| `.clinerules` / `.clineignore` | Regeln und Ausschlüsse für Cline |
| `src/logic/*.h` | reine Logik – Verträge (eingefroren) |
| `src/app/*` | Snapshot, Command-Queue, Control-Task (vorgegeben) |
| `src/hal/*.h`, `src/system/*.h`, `src/comm/*.h`, `src/display/*.h` | Hardware-Verträge |
| `src/smoketest/smoketest.cpp` | Sprint 0a |
| `test/native/*` | eingefrorene Host-Tests (351 Testfälle, 13 Suiten, Contract V1.5.11; `test_command_queue` braucht einen Linux/macOS-Host mit pthreads) |
| `test/embedded/*` | On-Target-Tests (S5) |
| `scripts/serial_cmd.py` | nicht-interaktiver Serial-Client (auch für Cline) |
| `scripts/add_client.py` | REST-Client mit CRC-Rahmenregel (Referenz) |
| `scripts/verify_reference.sh` | alle nativen Tests gegen die Referenzimplementierung (`referenz_logic.zip`, separat geliefert) in einem temporären Worktree – Nachweis, dass die eingefrorenen Tests widerspruchsfrei und erfüllbar sind |
| `scripts/stress_api.py` | Abnahme V1.1: STOP-Latenz, Queue-Flut, parallele Anfragen, Dauerlauf, Bedienvorrang |

## Schnellstart

```bash
# PlatformIO Core 6.x (VS-Code-Erweiterung oder offizielles Installationsskript; NICHT das
# apt-Paket "platformio" 4.x), dazu: pip install pyserial
pio test -e native -f native/test_env     # Testumgebung prüfen
pio run  -e smoketest -t upload           # Sprint 0a
pio device monitor -e smoketest           # "help"
```

## Status der Module

**Stand: 14.10.2026 · Contract V1.5.11 · Pflichtenheft V1.7.6**
Diese Tabelle wird **nach jedem Sprint** aktualisiert (Implementierung **und** Tests) –
Teil der Definition of Done in `.clinerules`.

| Sprint | Dateien | Implementierung | Tests auf dem Gerät / nativ | Stand |
|---|---|---|---|---|
| 0a | `src/smoketest/smoketest.cpp` (0a.3) | ✅ fertig | Smoketest-Protokoll freigegeben (5.2 bekannte Einschränkung) | ✅ 11.10.2026, Tag `sprint-0a` |
| 0b | Header, CONTRACT, Tests, `src/app/*` | ✅ fertig (Claude) | 351/351 gegen Referenzimplementierung | ✅ Contract V1.5.11 |
| L1 | `logic/types.cpp`, `crc32.cpp`, `safety_logic.cpp` | ✅ fertig (Qwen) | `test_types_crc` 22/22 · `test_safety_logic` 62/62 | ✅ 12.10.2026, Tag `sprint-2.1` |
| L2 | `logic/measure_logic.cpp`, `params.cpp` | ✅ fertig (Qwen) | `test_measure_logic` 40/40 · `test_params` 20/20 | ✅ 13.10.2026, Tag `sprint-l2` |
| L3 | `logic/motion_seq.cpp`, `sound_logic.cpp`, `wifi_logic.cpp` | ✅ fertig (Qwen) | `test_motion_seq` 38/38 · `test_sound_logic` 9/9 · `test_wifi_logic` 15/15 | ✅ 14.10.2026, Tag `sprint-l3` |
| L4 | `logic/app_logic.cpp` | ✅ fertig (Qwen) | `test_app_logic` 84/84 | ✅ 14.10.2026, Tag `sprint-l4` |
| L5 | `logic/proc_json.cpp`, `api_parse.cpp`, `display_logic.cpp` | ✅ fertig (Qwen) | `test_proc_json` 20/20 · `test_api_parse` 21/21 · `test_display_logic` 10/10 | in Review |
| P1 | `comm/log_buffer.cpp`, `system/nvs_config.cpp`, `system/system.cpp` | ⬜ offen | native Tests mit Nachbildung (folgen), firmware-partial | – |
| P2 | `comm/wifi_manager.cpp`, `comm/http_server.cpp` | ⬜ offen | firmware-partial, Review | – |
| H1 | `hal/hal_io.cpp`, `hal/motion_drv.cpp` | ⬜ offen | Hardware-Prüfung | – |
| I1 | `display/display.cpp`, `main.cpp`, Gesamt-Build | ⬜ offen | `pio run -e firmware`, `test_command_queue`, Prüfliste am Gerät | – |
| W1 | `data/www/index.html`, `settings.html` | ⬜ offen | Sichtprüfung | – |
| A1 | Abnahme | ⬜ offen | Pflichtenheft Kapitel 10 | – |
| D1 | `data/add.json`, `data/doc.md`, Demo-Skripte | ⬜ offen | KI-Validierung | – |

Reihenfolge und Begründung: `docs/Sprint_Prompts_Demoanlage_ADD_Slim_V2.md`, Abschnitt
„Sprint-Plan V3“. Die Sprint-Dateien für Cline liegen in `docs/sprints/`.

Nativ grün im Repository (ohne Referenzimplementierung): nur `test_env`. Alle anderen
Suiten scheitern beim Linken, bis ihr Sprint die `.cpp`-Dateien liefert – das ist erwartet.

**Nachweis der Tests:** Die 340 Testfälle sind keine reine Sollvorgabe – sie laufen
vollständig grün gegen eine Referenzimplementierung (`referenz_logic.zip`, bewusst nicht
im Repository, damit der Agent sie nicht abschreibt). Reproduzierbar mit
`scripts/verify_reference.sh /pfad/zu/referenz_logic.zip`. Zusätzlich wurden 40 Fehler in die
Logik und 10 in den Queue-Code gezielt eingebaut (Mutationstest); die Tests erkennen alle
bis auf einen harmlosen (fehlendes Leeren alter Antworten – die Sequenznummer sichert die
Zuordnung trotzdem). Contract V1.5: weitere 7 gezielte Fehler in der Shutter-/Zonenlogik,
alle erkannt. Contract V1.5.2: 8 zusätzliche Tests aus dem Review von S2.1
(motor_stop in FAULT/WAIT_ACK, INIT_DONE, Ausgänge außerhalb RUNNING, lange und kurze CRC-Rahmen). Contract V1.5.3: R1-Matrix über alle Ereignisse und
vollständige API-Tabelle (Differenztest gegen die Referenz hatte eine Lücke bei `INIT_DONE` gezeigt). Contract V1.5.5: Mutationstest vor L2
(43 Mutanten von measure_logic/params, alle bis auf 2 gleichwertige erkannt, +14 Tests). Contract V1.5.6: +2 Tests aus dem Review von L2. Contract V1.5.7: Mutationstest vor L3 (72 Mutanten von
motion_seq/sound_logic/wifi_logic, auch gegen `test_app_logic`; +18 Tests). Contract V1.5.8: +2 Tests aus dem Review von L3. Contract V1.5.9: Mutationstest vor L4 (72 Mutanten von
app_logic, 33 überlebten; +17 Tests, danach nur noch 6 gleichwertige). Contract V1.5.10: +4 Tests aus der Zwischenprüfung von L4.1b (`appMeasure`). Contract V1.5.11: Mutationstest vor L5 (77 Mutanten von proc_json/api_parse/display_logic, 17 überlebten; +7 Tests, danach nur noch 9 gleichwertige).
