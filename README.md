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
| `test/native/*` | eingefrorene Host-Tests (260 Testfälle, 13 Suiten; `test_command_queue` braucht einen Linux/macOS-Host mit pthreads) |
| `test/embedded/*` | On-Target-Tests (S5) |
| `scripts/serial_cmd.py` | nicht-interaktiver Serial-Client (auch für Cline) |
| `scripts/add_client.py` | REST-Client mit CRC-Rahmenregel (Referenz) |
| `scripts/verify_reference.sh` | alle nativen Tests gegen die Referenzimplementierung (`referenz_logic.zip`, separat geliefert) in einem temporären Worktree – Nachweis, dass die eingefrorenen Tests widerspruchsfrei und erfüllbar sind |
| `scripts/stress_api.py` | Abnahme V1.1: STOP-Latenz, Queue-Flut, parallele Anfragen, Dauerlauf, Bedienvorrang |

## Schnellstart

```bash
pip install platformio pyserial
pio test -e native -f native/test_env     # Testumgebung prüfen
pio run  -e smoketest -t upload           # Sprint 0a
pio device monitor -e smoketest           # "help"
```

## Status der Module

| Sprint | Dateien | Stand |
|---|---|---|
| 0a | smoketest | bereit zum Flashen |
| 0b | Header, CONTRACT V1.3, Tests, app/* | erledigt (V1.3: mm/s, Signale, WLAN-Wiederverbindung, Herzschlag, Not-Aus-Parameter) |
| S1 | system, nvs_config, wifi_manager, log_buffer, main | offen |
| S2.1–S2.4 | logic/*.cpp | offen (Tests rot bis Implementierung) |
| S2.5 | hal_io, motion_drv | offen |
| S3 | proc_json, api_parse, http_server | offen |
| S4a / S4b | data/www / display_logic, display | offen |
| S6 | data/add.json, data/doc.md, Demo-Skripte | offen |

Bis zur Implementierung der Logik schlagen die nativen Test-Suiten beim Linken
fehl – das ist erwartet. Jede Suite wird mit ihrem Sprint grün.

**Nachweis der Tests:** Die 260 Testfälle sind keine reine Sollvorgabe – sie laufen
vollständig grün gegen eine Referenzimplementierung (`referenz_logic.zip`, bewusst nicht
im Repository, damit der Agent sie nicht abschreibt). Reproduzierbar mit
`scripts/verify_reference.sh /pfad/zu/referenz_logic.zip`. Zusätzlich wurden 40 Fehler in die
Logik und 10 in den Queue-Code gezielt eingebaut (Mutationstest); die Tests erkennen alle
bis auf einen harmlosen (fehlendes Leeren alter Antworten – die Sequenznummer sichert die
Zuordnung trotzdem).
