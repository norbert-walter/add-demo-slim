# ADD Demo System (Slim) – Firmware-Projekt

1:5-Modell eines radiometrischen Dickenmessers zur Demonstration von
**ADD – AI Device Description**. ESP32 (MKS DLC32 V2.1), PlatformIO/pioarduino,
agentenbasierte Entwicklung mit code-server + Cline.

## Wo steht was

| Pfad | Inhalt |
|---|---|
| `docs/Pflichtenheft_Demoanlage_ADD_Slim_V1.7.md` | Anforderungen (deutsch) |
| `docs/CONTRACT.md` | verbindlicher Schnittstellenvertrag (englisch, für Agenten) |
| `docs/Sprint_Prompts_Demoanlage_ADD_Slim_V2.md` | Prompts und Review-Checklisten je Sprint |
| `docs/Smoketest_Protokoll.md` | Sprint 0a an der Hardware |
| `docs/reference/` | ADD Spec, Developer Guide, Style Guide (+ Auszug für S4) |
| `.clinerules` / `.clineignore` | Regeln und Ausschlüsse für Cline |
| `src/logic/*.h` | reine Logik – Verträge (eingefroren) |
| `src/app/*` | Snapshot, Command-Queue, Control-Task (vorgegeben) |
| `src/hal/*.h`, `src/system/*.h`, `src/comm/*.h`, `src/display/*.h` | Hardware-Verträge |
| `src/smoketest/smoketest.cpp` | Sprint 0a |
| `test/native/*` | eingefrorene Host-Tests (287 Testfälle, 13 Suiten, Contract V1.5.3; `test_command_queue` braucht einen Linux/macOS-Host mit pthreads) |
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

**Stand: 12.10.2026 · Contract V1.5.3 · Pflichtenheft V1.7.5**
Diese Tabelle wird **nach jedem Sprint** aktualisiert (Implementierung **und** Tests) –
Teil der Definition of Done in `.clinerules`.

| Sprint | Dateien | Implementierung | Tests auf dem Gerät / nativ | Stand |
|---|---|---|---|---|
| 0a | `src/smoketest/smoketest.cpp` (0a.3) | ✅ fertig | Smoketest-Protokoll freigegeben (5.2 bekannte Einschränkung) | ✅ 11.10.2026, Tag `sprint-0a` |
| 0b | Header, CONTRACT, Tests, `src/app/*` | ✅ fertig (Claude) | 287/287 gegen Referenzimplementierung | ✅ Contract V1.5.3 |
| S1 | system, nvs_config, wifi_manager, log_buffer, main, `logic/wifi_logic.cpp` | ⬜ offen | `test_wifi_logic` | – |
| S2.1 | `logic/types.cpp`, `crc32.cpp`, `safety_logic.cpp` | ✅ fertig | `test_types_crc` 22/22 · `test_safety_logic` 62/62 | ✅ 12.10.2026 |
| S2.2 | `logic/measure_logic.cpp`, `params.cpp` | ⬜ offen | `test_measure_logic`, `test_params` | – |
| S2.3 | `logic/motion_seq.cpp` | ⬜ offen | `test_motion_seq` | – |
| S2.4 | `logic/app_logic.cpp`, `sound_logic.cpp` | ⬜ offen | `test_app_logic`, `test_sound_logic` | – |
| S2.5 | `hal/hal_io.cpp`, `motion_drv.cpp` | ⬜ offen | Hardware-Prüfung S2.5 | – |
| S2.6 | Integration `app/control_task.cpp` (vorgegeben) | ⬜ offen | `test_command_queue`, Firmware baut | – |
| S3 | `logic/proc_json.cpp`, `api_parse.cpp`, `comm/http_server.cpp` | ⬜ offen | `test_proc_json`, `test_api_parse`, curl | – |
| S4a / S4b | `data/www/*` / `logic/display_logic.cpp`, `display/display.cpp` | ⬜ offen | `test_display_logic`, Sichtprüfung | – |
| S5 | Abnahme | ⬜ offen | Pflichtenheft Kapitel 10 | – |
| S6 | `data/add.json`, `data/doc.md`, Demo-Skripte | ⬜ offen | KI-Validierung | – |

Nativ grün im Repository (ohne Referenzimplementierung): nur `test_env`. Alle anderen
Suiten scheitern beim Linken, bis ihr Sprint die `.cpp`-Dateien liefert – das ist erwartet.

**Nachweis der Tests:** Die 287 Testfälle sind keine reine Sollvorgabe – sie laufen
vollständig grün gegen eine Referenzimplementierung (`referenz_logic.zip`, bewusst nicht
im Repository, damit der Agent sie nicht abschreibt). Reproduzierbar mit
`scripts/verify_reference.sh /pfad/zu/referenz_logic.zip`. Zusätzlich wurden 40 Fehler in die
Logik und 10 in den Queue-Code gezielt eingebaut (Mutationstest); die Tests erkennen alle
bis auf einen harmlosen (fehlendes Leeren alter Antworten – die Sequenznummer sichert die
Zuordnung trotzdem). Contract V1.5: weitere 7 gezielte Fehler in der Shutter-/Zonenlogik,
alle erkannt. Contract V1.5.2: 8 zusätzliche Tests aus dem Review von S2.1
(motor_stop in FAULT/WAIT_ACK, INIT_DONE, Ausgänge außerhalb RUNNING, lange und kurze CRC-Rahmen). Contract V1.5.3: R1-Matrix über alle Ereignisse und
vollständige API-Tabelle (Differenztest gegen die Referenz hatte eine Lücke bei `INIT_DONE` gezeigt).
