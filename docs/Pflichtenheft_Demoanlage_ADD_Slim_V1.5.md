# Pflichtenheft – Demoanlage ADD (Slim)

**Stand:** 2026-10-08
**Autor:** Norbert Walter
**Version:** 1.5
**Verbindliche Schnittstellen:** `docs/CONTRACT.md` (englisch, für Agenten) und die Header in `src/**/*.h`

---

## 0. Änderungen

### V1.4 → V1.5 (Contract V1.1, nach Review)

| Bereich | V1.4 | V1.5 |
|---|---|---|
| Bedienvorrang | nicht geregelt (Display und API gleichrangig) | **Display hat Vorrang (LOCAL)**: nach jedem Display-Befehl lehnt die Anlage Start-Befehle von der API mit `LOCAL_CONTROL (−10)` ab; STOP/HV_OFF/SHUTTER_CLOSE/SET bleiben erlaubt. Ende 30 s nach dem letzten Display-Befehl, sobald Achse steht und HV aus ist |
| STOP-Latenz | STOP wartete u. U. auf einen Zyklus ohne Fahrauftrag, volle Queue → BUSY | **Prioritäts-Queue** für STOP/HV_OFF/SHUTTER_CLOSE, Ausführung auch bei laufendem Fahrauftrag; ≤ 2 ms bis zum Treiber |
| API-Antwort | vor dem Setzen der Ausgänge | **nach** dem Setzen der Ausgänge: `OK` = angenommen und Ausgänge gesetzt; Bewegungsende nur über `/proc` |
| Zykluszeit | nicht überwacht | `cycle_max_us`, `overruns` in `GET_STATE`; Abnahme < 500 µs, 0 Überläufe in 10 min |
| Display im Fehlerfall | 5 Tasten bleiben sichtbar | **alle Tasten ausgeblendet**, Fehlertext groß, **eine** ACK-Fläche über die volle Breite, 1 s halten; grau solange die Ursache anliegt |
| Display-Logik | im Display-Code | reine Funktionen `display_logic.h`, nativ getestet |
| BUSY | Wiederholregel offen | erst `/proc` prüfen, dann wiederholen (CONTRACT §5.2) |
| Simulierte Signale | nicht gekennzeichnet | Interlock, Cooling, Temp überall als **SIM** markiert |
| Sicherheitshinweis | fehlte | Demo-Not-Aus ersetzt keinen Hardware-Sicherheitskreis (§6) |
| Tests | 189 | + LOCAL, STOP-Priorität, gleichzeitiger Not-Aus + Kühlungsausfall, Display-Logik |
| DoD | Sprint-Tests grün | zusätzlich **Regressionsregel**: alle bisher grünen Suiten bleiben grün |

### V1.3 → V1.4

| Bereich | V1.3 | V1.4 |
|---|---|---|
| Toolchain | `platform = espressif32` (Core 2.x, IDF 4.4) | **pioarduino** (Arduino Core 3.x, ESP-IDF 5.x) – Voraussetzung für FastAccelStepper-I2S |
| FastAccelStepper | `^0.30.4` | Master-Commit `96459ed` (2.0.0) mit `DRIVER_I2S_MUX` und `i2sMuxSetBit()` |
| Core-Bezeichnung | Core 1 / Core 2 | Core 0 (Netz) / Core 1 (Steuerung) |
| Schritte/mm | 4,762 (Vollschritt) | **76,19** (Mikroschritt 1/16) |
| Sensorkurve | `12.08·V^-1.058` als mm | `120.8·V^-1.058` mm (Originalformel liefert cm) |
| Dicke / Kante | `thick = calib_far_ref − dist`, Messgut wenn `dist < 60 mm` | `thick = base_dist − dist`, Messgut wenn `thick > edge_threshold` (3 mm) |
| Architektur | Globale Variablen, `taskENTER_CRITICAL` überall | **Single-Writer**: eine Control-Task, Command-Queue + Snapshot |
| Logik | in Hardware-Modulen | **reine Funktionen** in `src/logic/`, nativ getestet |
| Wiederanlauf | nach ACK automatisch HV + Shutter | **kein automatischer Wiederanlauf**; nach Not-Aus während Fahrt neue Referenzfahrt |
| Shutter | Regeln unvollständig | Shutter nur in RUNNING, nur außerhalb Parkzone (≤ 5 mm), automatisch während Messung |
| Messung | immer gültig | `measure.valid` nur bei offenem Shutter |
| Kommunikations-Watchdog | immer aktiv (→ Dauer-Not-Aus) | nur bei per API gestarteter Operation, Heartbeat `GET_STATE` |
| Not-Aus | aktiv LOW | **Öffner** – Drahtbruch löst aus; Aktivierung sofort, Freigabe entprellt |
| /proc | ohne Sicherheitszustand | mit `state`, `homed`, `valid`, `avg`, `hv_ok`, `comm_armed` |
| CRC | Regel offen | **eindeutige Rahmenregel** (CONTRACT §4), Referenz in C++, Python, JS |
| API | 12 Befehle, Fehlercodes offen | +`GET_CONFIG`, `CALIB`, `REBOOT`; Fehlercodes festgelegt |
| Display | widersprüchlich (Touch ja/nein, 1/2 Seiten) | 1 Seite, **5 Touch-Tasten** (INIT, HV, MEAS, TRAV, STOP) |
| Sicherheitsflags | in NVS gespeichert | **nie gespeichert** – nach Boot immer gesperrt (fail-safe) |
| Homing-Timeout | 30 s bei 100 mm/min (zu kurz) | 600 mm/min, Timeout 40 s |
| Tests | Nachbauten der Logik | Tests gegen echten Code, eingefroren, gegen Referenzimplementierung geprüft |
| ADD-Dokument | Feldnamen weichen von Spec ab | streng nach ADD Spec v1.0 (7 Blöcke) |

---

## 1. Zielsetzung

Die Demoanlage ist ein verkleinertes Modell (1:5) eines radiometrischen
Dickenmessers, orientiert am Messsystem M50. Sie dient ausschließlich der
Präsentation des **AI Device Description (ADD)**-Konzepts.

Die gefährliche radiometrische Messtechnik wird durch einen ungefährlichen
optischen Abstandssensor ersetzt. Die Firmware wird agentenbasiert entwickelt
(code-server + Cline + lokales Modell Qwen3.6-35B-A3B, 64k Kontext).

### Was die Demo zeigen soll

1. KI liest `/add` → versteht Anlage vollständig → erklärt sie einem Benutzer
2. KI liest `/add` → Benutzer stellt Fragen zu Bedienung und Funktion
3. Python-Skript steuert Profilfahrt über REST-API – ohne LogiCAD
4. Python erstellt individuelle Anzeigeseite aus `/proc`-Live-Daten
5. Fehlersimulation (Zahnstange auskoppeln → `MOTOR_FAULT` bzw. Positionsabweichung) → Fehlerdiagnose mit KI
6. ADD beschreibt sicherheitsrelevante Zustände → KI versteht Sicherheitslogik
7. Ein einfacher Mikrocontroller trägt die gesamte Funktionalität

### KI-Einsatzbereiche (mittelfristig)

- Kundenspezifische Steuerungsprogramme ohne LogiCAD
- Individuelle Visualisierungsseiten (Prozess- und Qualitätskontrolle)
- Anlagentest vor Auslieferung
- Fehlerdiagnose und Expertensystem
- Prozessanalyse und -optimierung

---

## 2. Zielgruppe

- Entwickler-Kollegen bei IMS
- Technische Entscheider (Vorgesetzte technischer Bereiche)
- Geschäftsführung
- Ausgewählte Schlüsselkunden
- Anthropic-Bewerbung (Constellation)

---

## 3. Hardware

### Zentrale Steuereinheit

| Komponente | Spezifikation |
|---|---|
| Steuerboard | Makerbase MKS DLC32 V2.1 |
| MCU | ESP32 (ESP32-WROOM-32U), Dual-Core 240 MHz |
| Flash | 4 MB (App 2944 KB, SPIFFS 1088 KB, NVS 20 KB) |
| RAM | 520 kB SRAM |
| Kommunikation | WiFi 802.11b/g/n |
| USB/UART | CH340C (Flashen + Konsole) |

### Antrieb X-Achse

| Komponente | Spezifikation |
|---|---|
| Schrittmotortreiber | TMC2209 (Standalone), **Mikroschritt 1/16** (Jumper) |
| Ansteuerung | Step/Dir/Enable über 74HC595-Kette am I2S (DATA 21, BCK 16, WS 17) |
| Schrittmotor | Nanotec ST4118L0804-A, 200 Schritte/U, 0,8 A |
| Übertragung | Zahnstange, 42 mm/U → **76,19 Schritte/mm**, Auflösung 0,013 mm |
| Verfahrweg | 0 … 240 mm (0 = 2 mm vor hinterem Endschalter) |
| Parkzone | 0 … 5 mm (Shutter dort immer geschlossen) |
| Endlagenschalter | GPIO 36 vorn, GPIO 35 hinten (Referenz) |

### Sensorik & Signale

| Komponente | Spezifikation |
|---|---|
| Abstandssensor | Sharp GP2Y0A41SK0F, 40–300 mm, Versorgung 5 V, Ausgang ca. 0,4–3,1 V |
| ADC | GPIO 34 (ADC1_CH6, 11 dB), `analogReadMilliVolts()` (eFuse-kalibriert) |
| LED Grün | HV eingeschaltet (i2so.3) |
| LED Rot | Shutter offen (GPIO 32) |
| Buzzer | i2so.7 |
| Not-Aus | GPIO 22, **Öffner gegen GND** (gedrückt oder Drahtbruch → HIGH → Not-Aus) |

> **Hinweis Sensor:** Im Nahbereich (40–50 mm, ≈ 2,8–3,1 V) arbeitet der ESP32-ADC
> am oberen, nichtlinearen Rand. Der Z--Eingang des DLC32 ist eigentlich ein
> Endschaltereingang – Pull-up/RC-Glied laut Schaltplan prüfen (Smoketest 3.2).
> 10 µF direkt am Sensor gegen Versorgungseinbrüche vorsehen.

### Display

| Eigenschaft | Wert |
|---|---|
| Modell | MKS TS35R, ST7796, 480×320 (Landscape, Rotation 1) |
| Touch | ADS7843E (XPT2046-kompatibel), gemeinsamer VSPI-Bus |
| Pins | SCK 18, MOSI 23, MISO 19, DC 33, CS 25, RST 27, BL 5, Touch-CS 26 (Smoketest 6) |
| Library | LovyanGFX |

### Stromversorgung

24 V DC extern über Schleppkette. USB-Kabel ebenfalls in der Schleppkette.

---

## 4. Pin- und Slot-Belegung

Verbindlich in `src/config.h`. Werte mit `VERIFY(0a)` werden im Smoketest bestätigt.

| Signal | Anschluss | Bemerkung |
|---|---|---|
| Endschalter vorn | GPIO 36 (X-) | input only |
| Endschalter hinten | GPIO 35 (Y-) | Referenzschalter |
| Abstandssensor | GPIO 34 (Z-) | analog |
| Not-Aus | GPIO 22 (PROBE) | Öffner |
| Shutter-LED | GPIO 32 (SPINDLE/TTL) | HIGH = offen |
| I2S DATA / BCK / WS | GPIO 21 / 16 / 17 | 74HC595-Kette |
| Enable / Step / Dir | i2so.0 / .1 / .2 | Enable aktiv LOW |
| HV-LED | i2so.3 | |
| Buzzer | i2so.7 | |

Die Zuordnung FluidNC-Bit `i2so.N` → FastAccelStepper-Slot ermittelt der Smoketest (`scan`).

### Simulierte Sicherheitsflags (nur per `/api SET`)

| Flag | Funktion | Startzustand nach **jedem** Boot |
|---|---|---|
| `interlock` | HV-Freigabe (Schlüsselschalter) | `false` |
| `cooling_flow` | Kühlwasserfluss | `false` |
| `temp` | Kühlwassertemperatur | `18.0 °C` |

---

## 5. Software-Architektur

### 5.1 Grundprinzipien

- PlatformIO mit **pioarduino** (Arduino Core 3.x, ESP-IDF 5.x)
- **Core 0:** WiFi, HTTP-Server, NVS-Schreiben (Netzwerk-Task)
- **Core 1:** Control-Task 1 kHz (einziger Schreiber des Anlagenzustands), Display-Task 5 Hz, FastAccelStepper-Task
- **Single-Writer-Prinzip:** Kommunikation zwischen den Tasks ausschließlich über Command-Queue (rein) und Snapshot (raus)
- **Reine Logik** in `src/logic/` ohne Arduino/IDF/FreeRTOS – vollständig nativ testbar (inkl. simulierter Achse)
- Hardware-Schicht dünn, aus dem Smoketest übernommen
- Code und Kommentare in Englisch
- Task-Watchdog 10 s, WiFi-Reconnect, AP-Fallback
- Weboberfläche: statisches HTML auf SPIFFS, Daten per JS

### 5.2 Modulstruktur

```
src/
├── config.h                 Pins, Slots, Timing (eingefroren nach 0a)
├── main.cpp                 setup(), Netzwerk-Task                       S1
├── logic/                   REINE LOGIK (nativ getestet)
│   ├── types.h/.cpp         Typen, Namen                                 S2.1
│   ├── crc32.h/.cpp         CRC32 + Rahmenregel                          S2.1
│   ├── safety_logic.h/.cpp  Zustandsautomat R1–R7                        S2.1
│   ├── measure_logic.h/.cpp Messkette, Kalibrierung, Statistik           S2.2
│   ├── params.h/.cpp        Parameter, SET-Schlüssel                     S2.2
│   ├── motion_seq.h/.cpp    Ablaufsteuerung Achse                        S2.3
│   ├── app_logic.h/.cpp     Gesamtlogik (Tick, Befehle, Prozessabbild)   S2.4
│   ├── proc_json.h/.cpp     /proc- und Antwort-JSON                      S3
│   └── api_parse.h/.cpp     /api-Parser                                  S3
├── hal/
│   ├── hal_io.h/.cpp        GPIO, ADC, LEDs, Buzzer                      S2.5
│   └── motion_drv.h/.cpp    FastAccelStepper-Adapter                     S2.5
├── app/                     (0b, vollständig vorgegeben)
│   ├── snapshot.h/.cpp      Prozessabbild über Kerne
│   ├── command_queue.h/.cpp Befehls-Queue mit Antwort
│   └── control_task.h/.cpp  Steuer-Task (Glue)                           Review S2.6
├── system/  system, nvs_config                                           S1
├── comm/    wifi_manager, log_buffer, http_server                        S1/S3
├── display/ lgfx_config.h (0a), display.cpp                              S4
└── smoketest/smoketest.cpp  Sprint 0a
test/native/<suite>/         eingefrorene Host-Tests
test/embedded/test_hardware/ On-Target-Tests                              S5
data/                        www/, add.json, doc.md                       S4/S6
scripts/                     serial_cmd.py, add_client.py
```

### 5.3 Betriebsmodi

**Stehende Messung (`START_MEASURE`)** – nur in RUNNING
- Fährt mit 1500 mm/min an die Messposition (5 < pos ≤ 240 mm)
- Shutter öffnet automatisch, Messung bis `STOP`
- Mittelwert und Anzahl gültiger Werte in `/proc` (`avg`, `n`)

**Profilfahrt (`START_TRAVERSE`)** – nur in RUNNING
- Anfahrt Startposition, dann Fahrt zur Endposition mit konstanter Geschwindigkeit
- Shutter offen während der Profilfahrt, schließt am Ende; HV bleibt an
- Kantenerkennung: Messgut, wenn `thick > edge_threshold`
- Statistik nur über gültige Werte auf Messgut (Lücken ignoriert)
- Messwerte für Profile holt der Client per `/proc`-Polling (z. B. 200 ms)

### 5.4 REST-API

Vollständige Definition: **CONTRACT §5**. Kurzfassung:

| Befehl | Parameter | Erlaubt in |
|---|---|---|
| `INIT` | – | SAFE_START, READY |
| `MOVE` | `pos`, `speed` | READY, RUNNING (homed) |
| `START_MEASURE` | `pos` | RUNNING |
| `START_TRAVERSE` | `start`, `end`, `speed` | RUNNING |
| `STOP`, `SET`, `HV_OFF`, `SHUTTER_CLOSE`, `GET_STATE`, `GET_CONFIG` | | immer |
| `CALIB` | `action` near/far/base/save/reset | SAFE_START, READY, RUNNING |
| `ACK` | – | WAIT_ACK (Ursache muss beseitigt sein) |
| `HV_ON` | – | READY, RUNNING |
| `SHUTTER_OPEN` | – | RUNNING, Position > 5 mm |
| `REBOOT` | – | SAFE_START, READY, WAIT_ACK |

Fehlercodes: Anlage `err` 0–5, API `err` 0 … −10 (CONTRACT §3).

**Bedeutung von `OK`:** Befehl angenommen und die Ausgänge des Zyklus sind gesetzt.
Lange Vorgänge (Referenzfahrt, Profilfahrt) laufen weiter; ihr Ende steht nur in `/proc`.
**`BUSY`:** Der Befehl kann trotzdem noch ausgeführt werden – Client prüft erst
`/proc`, dann ggf. erneut senden (CONTRACT §5.2).
**`LOCAL_CONTROL (−10)`:** Am Display wird gerade bedient; Start-Befehle von der API
sind gesperrt (§6 Bedienvorrang).

**Prüfsumme:** CRC32 (zlib) über den gesendeten JSON-Text ohne crc-Feld; das Feld
`,"crc":"XXXXXXXX"` wird vor der letzten `}` eingefügt (CONTRACT §4).

### 5.5 HTTP-Endpunkte

| Endpunkt | Methode | Inhalt |
|---|---|---|
| `/add` | GET | ADD-Dokument (JSON, SPIFFS) |
| `/add.html` | GET | ADD eingebettet für Cloud-KI (ADD Spec §5.3) |
| `/llms.txt` | GET | Index (ADD Spec §5.4) |
| `/proc` | GET | Prozessabbild (JSON + CRC, ≤ 512 Byte) |
| `/api` | POST | Befehlskanal (JSON + CRC) |
| `/doc` | GET | Dokumentation (Markdown) |
| `/log` | GET | letzte 20 Logzeilen |
| `/` | GET | Prozessansicht |
| `/settings` | GET | Einstellungen |

### 5.6 /proc

Siehe CONTRACT §5.1 (Feldreihenfolge verbindlich).

### 5.7 Weboberfläche (2 Seiten, SPIFFS)

**Sprache: Englisch.** Basis: ADD Dashboard Style Guide v1.0 mit **identischen
CSS-Variablen** (Werte aus Style Guide Anhang A), Theme-Klassen `theme-dark`,
`theme-light`, `theme-night`, `theme-bw`, localStorage-Schlüssel `add-dashboard-theme`.
Schriften Ubuntu / Ubuntu Mono **lokal auf SPIFFS** (`/www/fonts/*.woff2`, kein CDN –
im AP-Modus gibt es kein Internet), Fallback `sans-serif` / `monospace`.

**Seite 1 – Process View (`/`)**
- Statuszeile: Sicherheitszustand (READY/RUNNING/FAULT…), HV, Shutter, E-Stop, Fehlertext,
  Badge **LOCAL** solange `local = true` (Start-Tasten dann deaktiviert, Hinweis „Operator at display has priority“)
- Datenpanels: POS, SPEED, DIST, THICK (`---` wenn `!valid` oder nicht auf Messgut), AVG/n
- Sicherheitsflags: INTERLOCK, COOLING, TEMP mit Farbcode `--ok/--warn/--danger`,
  jeweils mit Kennzeichnung **SIM** (simuliert)
- Zeitreihe DIST/THICK (Canvas, max. 100 Punkte, clientseitig)
- Tasten: INIT, HV ON/OFF, MEASURE, TRAVERSE, STOP, ACK (ACK nur in WAIT_ACK aktiv)
  – START-Befehle ohne Parameter → Werte aus der Konfiguration
- Bestätigungsdialog (Style Guide 7.8) für HV ON und ACK
- **Heartbeat:** solange `safety.comm_armed`, alle 2 s `GET_STATE`
- Validation-Strip (Style Guide 6.10) aus `/add` → `validation.validated_by`
- `/proc`-Polling alle 500 ms, CRC wird geprüft (bei Fehler: Statuspill „CRC error“)
- Bei `BUSY`: keine automatische Wiederholung; nächster `/proc`-Stand entscheidet

**Seite 2 – Settings (`/settings`)**
- Werte beim Laden per `GET_CONFIG`
- Messposition, Profilfahrt (Start/Ende/Geschwindigkeit), Kantenschwelle, `base_dist`
- Sicherheitsflags (Toggle Interlock, Toggle Cooling, Temperatur)
- Kalibrierung: Referenzwerte Nah/Fern (`SET calib_near_ref/far_ref`),
  Tasten Capture Near / Capture Far / Capture Base / Save / Reset (`CALIB`)
- WLAN: SSID, Passwort, Reboot-Taste
- Jede Rückmeldung (`err`, `msg`) sichtbar als Toast

### 5.8 Display (1 Seite, 480×320, LovyanGFX)

Lokaler Zugang ohne WLAN. Aktualisierung alle 200 ms aus dem Snapshot,
Neuzeichnen nur geänderter Zonen. **Sprache: Englisch.**

```
┌──────────────────────────────────────────────────────────────────┐ y=0
│ ADD DEMO           [ READY ]            ● HV      ▮ SHUTTER      │ h=32
├───────────┬───────────┬───────────┬──────────────────────────────┤ y=34
│ POS       │ SPEED     │ DIST      │ THICK                        │
│ 120.3     │ 200       │ 187.7     │ 12.3                         │ h=72
│ mm        │ mm/min    │ mm        │ mm                           │
├───────────┴───────────┴───────────┴──────────────────────────────┤ y=110
│ POS |▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|  240         │ h=24
├──────────────────────────────────────────────────────────────────┤ y=140
│ ESTOP ●  INTERLOCK ●  COOLING ●  TEMP 23.0°C ●                   │ h=26
│ MODE traverse   AVG 12.3 mm (143)   ERR NONE                     │ h=26
│ IP 192.168.4.1                                                   │ h=26 (bis y=230)
├────────────┬────────────┬────────────┬────────────┬──────────────┤ y=260
│   INIT     │    HV      │   MEAS     │   TRAV     │    STOP      │ h=60
└────────────┴────────────┴────────────┴────────────┴──────────────┘ y=320
```

- Werte: `setTextSize(3)` (18×24 px/Zeichen) → max. 6 Zeichen je 120-px-Feld
- Tasten 96×60 px, Beschriftung `setTextSize(3)` (max. 5 Zeichen = 90 px)
- `HV` schaltet je nach Zustand `HV_ON`/`HV_OFF`, Farbe zeigt Zustand
- `STOP` immer aktiv; Tap-Feedback 50 ms aufhellen
- Befehle mit `source = LOCAL` (kein Kommunikations-Watchdog, setzt Bedienvorrang LOCAL)
- Badge `LOCAL` in der Kopfzeile, solange der Bedienvorrang aktiv ist
- Sicherheitsflags mit Zusatz `SIM`, z. B. `INTERLOCK(SIM) ●`
- Touch-Abfrage alle 20 ms, Neuzeichnen alle 200 ms

**Fehlerbild (FAULT / WAIT_ACK)** – alle Bedientasten werden ausgeblendet, es gibt
nur noch eine mögliche Handlung:

```
┌──────────────────────────────────────────────────────────────────┐ y=0
│ ADD DEMO           [ WAIT_ACK ]         ○ HV      ▯ SHUTTER      │ h=32
├──────────────────────────────────────────────────────────────────┤ y=34
│                                                                  │
│          FAULT: MOTOR_FAULT                    (rot, Größe 4)    │
│          Check axis and end switches, then acknowledge           │
│                                                                  │
│ POS 231.4 mm   ESTOP ○   TEMP(SIM) 23.0°C                        │
├──────────────────────────────────────────────────────────────────┤ y=260
│                  ACK – HOLD 1 s   ▓▓▓▓▓▓▓░░░░░░░                 │ h=60, 480 px
└──────────────────────────────────────────────────────────────────┘ y=320
```

- FAULT (ein Zyklus): Banner, noch keine Taste
- WAIT_ACK, Ursache liegt noch an (Not-Aus gedrückt, Temperatur > 32 °C):
  Fläche grau, Text `CAUSE ACTIVE: ESTOP – RELEASE FIRST`, keine Wirkung
- WAIT_ACK, Ursache beseitigt: Fläche gelb `#f5a623`, `ACK – HOLD 1 s`, Fortschrittsbalken;
  ACK wird **einmal** nach 1 s ununterbrochenem Drücken gesendet
- Der Druck muss **auf dem Fehlerbild beginnen**: Ein Finger, der beim Fehlereintritt
  noch auf STOP lag, quittiert nie (Schutz gegen reflexhaftes Quittieren)
- Abhilfetexte: ESTOP „Release E-stop, then acknowledge“, TEMP_HIGH „Lower temp ≤ 32 °C“,
  MOTOR_FAULT „Check axis and end switches“, COMM_TIMEOUT „Remote client lost“,
  HOMING_FAILED „Check home switch, then INIT“
- In WAIT_ACK kann sich nichts bewegen (Motor jeden Zyklus notgestoppt, HV und Shutter
  zwangsweise aus); der physische Not-Aus bleibt jederzeit wirksam
- Logik: `src/logic/display_logic.h` (`screenModeOf`, `ackCauseActive`, `ackPressUpdate`)
- Farben aus dem Style Guide (Dark): Hintergrund `#202020`, Panel `#1e1e1e`,
  Rahmen `#2e2e2e`, Text `#ffffff`, Labels `#8ab2d0`, DIST `--accent #00c8a0`,
  THICK `--accent2 #0084ff`, OK `#00c8a0`, Warnung `#f5a623`, Fehler `#e84040`

### 5.9 Messkette

16 × `analogReadMilliVolts()` alle 20 ms → 4 kleinste und 4 größte verwerfen →
Mittelwert → `dist = 120,8 · V^-1,058` mm → IIR α = 0,8 → 2-Punkt-Kalibrierung →
`thick = max(0, base_dist − dist)`. Der Sharp-Sensor liefert nur etwa alle 16,5 ms
einen neuen Wert; 50 Hz Abtastung genügen.

Kalibrierung (CONTRACT §7.3): Referenzabstände per `SET calib_near_ref/far_ref`,
Messwerte per `CALIB near/far` (nur bei stabilem Signal, 500 ms innerhalb ±0,5 mm),
`CALIB save` prüft Gültigkeit und speichert. `CALIB base` setzt `base_dist`
(Nullabgleich ohne Messgut).

### 5.10 Libraries

```ini
lib_deps =
    https://github.com/gin66/FastAccelStepper.git#96459ed15a3c7d9927f561d2edb72cc7edbbbe8a
    lovyan03/LovyanGFX    @ ^1.2.0   ; nach Smoketest exakt pinnen
    bblanchon/ArduinoJson @ ^7.4.2
```

---

## 6. Sicherheitskonzept

Vollständig: **CONTRACT §6** und `src/logic/safety_logic.h` (Regeln R1–R7, Invarianten I1–I6).

> **Sicherheitshinweis.** Dies ist eine Demonstrationsanlage. Die Auswertung des
> Not-Aus in Software und die Sicherheitslogik sind **keine Sicherheitsfunktion**
> im Sinne von EN ISO 13849 / IEC 62061 und ersetzen keinen Hardware-Sicherheitskreis.
> Es gibt keine echte Strahlenquelle; HV und Shutter sind LEDs. Interlock, Kühlung
> und Temperatur sind simuliert.

### Zustände

```
SAFE_START ──INIT ok──► READY ──HV_ON──► RUNNING
     ▲                    ▲  ◄─HV_OFF / Bedingung weg─┘
     │                    │
     └── RECOVER ◄─ACK── WAIT_ACK ◄── FAULT ◄── (Not-Aus, >32 °C, Endschalter, Kommunikation, Homing)
        (READY wenn Referenz gültig, sonst SAFE_START)
```

### HV einschalten (UND)
Kühlwasserfluss ∧ Temperatur ≤ 30 °C ∧ Interlock ∧ kein Not-Aus

### HV-Abfall ohne Fehler (eine Bedingung fehlt)
RUNNING → READY, Shutter schließt, laufende Messung stoppt. Keine Quittierung nötig.

### Fehler (Quittierung nötig)
Not-Aus · Temperatur > 32 °C · unerwarteter Endschalter · Kommunikationsausfall · Referenzfahrt gescheitert.
Reaktion im selben Zyklus: Shutter zu → Motor Not-Halt → HV aus → FAULT → WAIT_ACK.
`ACK` wird nur angenommen, wenn die Ursache beseitigt ist.

### Shutter
Öffnen nur in RUNNING und außerhalb der Parkzone (> 5 mm). Schließt bei HV-Abfall,
jedem Fehler, Einfahrt in die Parkzone und Ende der Messung.

### Wiederanlauf
**Kein automatischer Wiederanlauf.** Nach ACK bleiben HV und Shutter aus. Wurde
die Achse während einer Bewegung notgestoppt, ist die Referenz ungültig →
SAFE_START → neue Referenzfahrt.

### Kommunikationsausfall
Watchdog nur aktiv, solange eine **per API** gestartete Operation läuft (Bewegung
oder HV an). Ohne gültiges `/api`-Paket für > 5 s → Fehler `COMM_TIMEOUT`.
Clients senden `GET_STATE` mindestens alle 4 s. Bedienung am Display aktiviert
den Watchdog nicht; ein Display-Befehl während einer Fernoperation schaltet ihn ab
(der Bediener ist anwesend und übernimmt).

### Bedienvorrang (LOCAL)
Der Mensch an der Anlage ist schneller als jeder Fernzugriff, daher hat das Display
Vorrang. Jeder Display-Befehl setzt `local = true`. Solange `local` aktiv ist, werden
von der API abgelehnt (`LOCAL_CONTROL`): INIT, MOVE, START_MEASURE, START_TRAVERSE,
HV_ON, SHUTTER_OPEN, CALIB, ACK, REBOOT. Erlaubt bleiben STOP, HV_OFF, SHUTTER_CLOSE
(sichere Richtung), SET (Fehlersimulation) und Abfragen. `local` endet, wenn die Achse
steht, HV aus ist und seit dem letzten Display-Befehl 30 s vergangen sind.

### STOP-Priorität
STOP, HV_OFF und SHUTTER_CLOSE laufen über eine eigene Prioritäts-Queue und werden auch
in einem Zyklus ausgeführt, der schon einen Fahrauftrag hat. Ein Sofort-Stopp
(Not-Halt, Nullsetzen am Endschalter) wird nie zu einem gebremsten Stopp abgeschwächt.
Garantie: ≤ 2 ms von der Queue bis zum Treiber. Der schnellste Stopp bleibt der
physische Not-Aus.

### Fail-Safe-Grundsätze
- Sicherheitsflags werden nie gespeichert (nach Boot gesperrt)
- Not-Aus als Öffner, Drahtbruch = Not-Aus
- Reihenfolge der Ausgänge in jedem Zyklus: Shutter → Motor → HV

---

## 7. ADD-Integration (Sprint 6)

Das ADD-Dokument folgt **streng der ADD Specification v1.0** (sieben Blöcke
`autonomy, device, security, interfaces, actions, rules, validation` plus Header
`schema, version, spec_url, spec_license`). Zusätzlich der optionale `ui`-Block
des Style Guides. **Kein** eigener Top-Level-Block `live_data` – `/proc` wird in
`interfaces[].data` beschrieben, der Hinweis „ADD beschreibt Fähigkeiten, Live-Daten
über /proc“ steht in `rules`.

- Ablage: SPIFFS `/add.json`, Endpunkte `/add`, `/add.html`, `/llms.txt`
- `autonomy`: Einstufung über die drei Scores (bewusste Entscheidung, siehe Sprint 6).
  Konzeptionell **Level 2**: Die Demo selbst ist harmlos, modelliert aber eine
  Strahlenquelle – die KI soll sich so verhalten, wie sie es an der echten Anlage müsste.
  Eine Unterdeklaration ist nicht zulässig.
- `rules`: Bedienvorrang (bei `local = true` bzw. `LOCAL_CONTROL` nicht wiederholen,
  sondern auf den Bediener warten), BUSY-Regel, „OK heißt angenommen, Ende über /proc“
- Simulierte Signale werden im ADD als simuliert beschrieben
- `security.enforcement`: alle Prüfungen, die das Gerät selbst durchsetzt (Zustandsgate, Bereichsgrenzen, Shutter/Parkzone, Watchdog)
- `actions`: alle Befehle mit Parametern, Bereichen, `safe`, `reversible`, `requires_confirmation`
- `validation`: Validierungsprotokoll nach Spec §10.7 (zunächst `validated_by: []`)

**Demo-Ablauf KI-Validierung**
1. KI ruft `/add` ab, lädt Ethical Framework
2. KI erklärt Anlage und Bedienkonzept
3. KI liest `/proc`
4. KI startet Profilfahrt (`SET`-Flags, `INIT`, `HV_ON`, `START_TRAVERSE`) mit Benutzerbestätigung
5. KI überwacht per `/proc`, sendet Heartbeat
6. KI dokumentiert das Profil (Kanten, mittlere Dicke)

---

## 8. Tests

### Nativ (PC, ohne ESP32) – `pio test -e native`

| Suite | Modul | Inhalt |
|---|---|---|
| `test_env` | – | Testumgebung läuft (0a) |
| `test_types_crc` | types, crc32 | Namen, CRC-Vektoren, Rahmenregel |
| `test_safety_logic` | safety_logic | R1–R7, Befehlsmatrix, Automatik, Watchdog, Zufalls-Invariantentest |
| `test_measure_logic` | measure_logic | Kurve (mm!), Median, IIR, Kalibrierung, Stabilität, Statistik |
| `test_params` | params | alle SET-Schlüssel, Bereiche, Persistenz, Config-JSON |
| `test_motion_seq` | motion_seq | Homing, Timeout, Endschalterfehler, Fahren, Profil, Messen, Stopps |
| `test_app_logic` | app_logic | **Szenarien mit simulierter Achse**: Profilfahrt, Not-Aus, Temperatur, Watchdog, Kalibrierung, Bedienvorrang LOCAL, STOP-Priorität, Not-Aus + Kühlungsausfall gleichzeitig |
| `test_display_logic` | display_logic | Bildschirmmodus, „Ursache aktiv“, ACK 1 s halten, Flankenerkennung |
| `test_proc_json` | proc_json | exaktes /proc-Format, Worst Case < 512 Byte |
| `test_api_parse` | api_parse | Parser, Typen, Fehlerfälle |

Alle Tests sind **eingefroren** und gegen eine Referenzimplementierung geprüft
(216 Testfälle, alle grün; Mutationstests bestätigen die Trennschärfe).

**Regressionsregel (DoD jedes Sprints):** Alle Suiten, die vor dem Sprint grün waren,
sind danach weiterhin grün (`pio test -e native` vollständig, nicht nur die Sprint-Suite).

Zusätzlich am Gerät: `scripts/stress_api.py` (STOP-Latenz, Queue-Flut, 10-min-Dauerlauf,
Bedienvorrang).

### On-Target – `pio test -e esp32-test` (nur nach Freigabe)
`test/embedded/test_hardware`: CRC, Eingänge, Sensor, Ausgänge, NVS, 2-mm-Fahrt.

---

## 9. Entwicklungsplan

### Abhängigkeitskette

```
S0a Smoketest (👤)  ─► S0b Verträge (🤖 Claude, 👤 Review) ─► S1 System/NVS/WiFi (🤖)
                                                           └► S2.1 types/crc/safety (🤖)
                                                               └► S2.2 measure/params (🤖)
                                                                   └► S2.3 motion_seq (🤖)
                                                                       └► S2.4 app_logic (🤖)
S2.5 HAL + Motortreiber aus Smoketest (👤+🤖) ─► S2.6 Integration Control-Task (👤+🤖)
   ─► S3 JSON/Parser/HTTP (🤖) ─► S4 Web + Display (🤖) ─► S5 Abnahme (👤) ─► S6 ADD (👤+Claude)
```

S1 und S2.1–S2.4 sind voneinander unabhängig (S2.x braucht keine Hardware).

### Sprint-Übersicht

| Sprint | Inhalt | Prüfung (DoD) | Wer | Aufwand |
|---|---|---|---|---|
| S0a | Smoketest, Protokoll, config.h-Werte | Smoketest-Protokoll | 👤 | 3–4 h |
| S0b | Verträge, Header, Tests, Skelette | dieses Paket | Claude + 👤 Review | erledigt / 1 h Review |
| S1 | system, nvs_config, wifi_manager, main | firmware-partial baut; Boot-Test am Gerät | 🤖 Qwen | 2 h |
| S2.1 | types, crc32, safety_logic | test_types_crc, test_safety_logic | 🤖 Qwen | 1–2 h |
| S2.2 | measure_logic, params | test_measure_logic, test_params | 🤖 Qwen | 1–2 h |
| S2.3 | motion_seq | test_motion_seq | 🤖 Qwen | 1–2 h |
| S2.4 | app_logic | test_app_logic | 🤖 Qwen (Review Claude) | 2 h |
| S2.5 | hal_io, motion_drv | Smoketest-Funktionen über HAL | 👤 + 🤖 | 2 h |
| S2.6 | control_task + main zusammenführen | firmware baut; INIT/MOVE/Not-Aus per Serial | 👤 + 🤖 | 2 h |
| S3 | proc_json, api_parse, log_buffer, http_server | test_proc_json, test_api_parse; curl-Tests | 🤖 Qwen | 2–3 h |
| S4a/b | index.html, settings.html (4a); display_logic.cpp, display.cpp (4b) | test_display_logic; Sichtprüfung, Bedienung | 🤖 Qwen | 3–4 h |
| S5 | Integration, Abnahme | Kapitel 10 | 👤 | 3–4 h |
| S6 | add.json, doc.md, Python-Demos | KI-Validierung | 👤 + Claude | 2–3 h |

**Gesamt: ca. 25–30 h**, davon ca. 12–15 h agentenbasiert.

---

## 10. Abnahmekriterien

**1. System-Start**
- [ ] Boot fehlerfrei, Watchdog aktiv, 5 min ohne Reset
- [ ] Parameter nach Neustart wiederhergestellt; Sicherheitsflags wieder gesperrt

**2. Hardware-Signale**
- [ ] LED Grün folgt `HV_ON`/`HV_OFF`
- [ ] LED Rot folgt Shutter
- [ ] Buzzer piept bei Fehler, Referenzfahrt-Ende
- [ ] Endschalter in `/proc` korrekt
- [ ] Abstand plausibel 40–300 mm (±5 %)
- [ ] Not-Aus-Drahtbruch löst Not-Aus aus

**3. Motor**
- [ ] `INIT` referenziert (Endschalter hinten, 2 mm Rückzug, Position 0)
- [ ] `MOVE` erreicht Ziel ±0,1 mm (Messschieber), keine Schrittverluste nach 10 Fahrten
- [ ] Unerwarteter Endschalter → `MOTOR_FAULT`

**4. Sicherheitskreis**
- [ ] Not-Aus stoppt Bewegung ≤ 2 ms nach Signal (Oszilloskop: Not-Aus ↔ Step-Signal)
- [ ] `HV_ON` nur mit allen vier Bedingungen; jede einzelne fehlende → `PRECONDITION`
- [ ] Temperatur 31 °C → HV aus ohne Fehler; 35 °C → `TEMP_HIGH`, ACK erst nach Absenken
- [ ] `cooling_flow = false` während Profilfahrt → HV aus, Fahrt gestoppt, Shutter zu
- [ ] Kommunikations-Watchdog: Python-Skript während Profilfahrt abbrechen → `COMM_TIMEOUT` nach 5 s
- [ ] Nach ACK kein automatischer Wiederanlauf; nach Not-Aus in Fahrt erneutes INIT nötig
- [ ] Shutter öffnet nie in der Parkzone
- [ ] Not-Aus und `cooling_flow = false` gleichzeitig → `FAULT ESTOP`
- [ ] STOP per API während Profilfahrt: Antwort < 150 ms, Achse steht < 1 s (`stress_api.py latency`)
- [ ] Zykluszeit: `cycle_max_us` < 500, `overruns` = 0 nach 10 min (`stress_api.py soak`)
- [ ] Queue-Flut (200 SET): nur OK/BUSY, alle CRC gültig (`stress_api.py flood`)

**5. REST-API + /proc**
- [ ] Alle Endpunkte < 1 s
- [ ] `/proc` vollständig, CRC gültig (`scripts/add_client.py proc`)
- [ ] Falsche CRC → `BAD_CRC`; alle Befehle laut CONTRACT §5.2
- [ ] `pio test -e native` vollständig grün

**6. Display**
- [ ] Werte, Zustand, Indikatoren korrekt, kein Flackern
- [ ] Positionsbalken synchron
- [ ] Alle 5 Tasten wirken (lokal, ohne WLAN)
- [ ] Fehlerfall: alle Tasten ausgeblendet, nur ACK-Fläche; grau solange Not-Aus gedrückt
- [ ] ACK erst nach 1 s Halten; Finger, der beim Fehler schon auf STOP lag, quittiert nicht
- [ ] Bedienvorrang: nach Display-Befehl API `HV_ON` → `−10`, API `STOP` → OK;
      30 s nach letztem Display-Befehl (Achse steht, HV aus) wieder frei (`stress_api.py local`)

**7. Weboberfläche**
- [ ] Live-Daten ≤ 500 ms, Zeitreihe 100 Punkte
- [ ] 4 Themes, funktioniert ohne Internet (AP-Modus)
- [ ] Einstellungen laden/speichern, Kalibrierung, WLAN-Konfiguration
- [ ] Tablet über WLAN

**8. Betriebsmodi**
- [ ] Stehende Messung: Mittelwert auf Treppenstufe ±0,5 mm
- [ ] Profilfahrt: Kanten der Treppenstufen erkannt, Statistik plausibel
- [ ] Kalibrierung gespeichert und wirksam

**9. ADD + KI**
- [ ] `/add` valide nach ADD Spec v1.0 (sieben Blöcke)
- [ ] KI erklärt Anlage und Sicherheitslogik korrekt
- [ ] KI liest `/proc` korrekt
- [ ] Python-Skript fährt Profil autonom inkl. Heartbeat
- [ ] Python erzeugt Dashboard aus `/proc` nach Style Guide

---

## 11. Offene Punkte

| Punkt | Entscheidung | Status |
|---|---|---|
| Toolchain | pioarduino | ✅ |
| Mikroschritt | 1/16 | ✅ |
| Architektur | Single-Writer, reine Logik | ✅ |
| /proc-Schema, API, CRC, Fehlercodes | CONTRACT V1.1 | ✅ |
| Bedienvorrang Display, Haltezeit | LOCAL, 30 s | ✅ |
| Quittierung am Display | eine ACK-Fläche, 1 s halten | ✅ |
| i2so → Slot-Zuordnung | Smoketest `scan` | Offen (0a) |
| Display-/Touch-Pins, Farbreihenfolge | Smoketest | Offen (0a) |
| ADC am Z--Eingang (Pull-up?) | Smoketest 3.2 | Offen (0a) |
| Autonomy-Level im ADD | konzeptionell Level 2, Scores in S6 | Offen (S6) |
| Python-Demo-Skripte | S6 | Offen |
