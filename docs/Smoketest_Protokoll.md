# Sprint 0a – Smoketest-Protokoll

**Anlage:** ADD Demo System (Slim) · **Firmware:** `pio run -e smoketest -t upload` (Smoketest 0a.1; aktuell 0a.3)
**Bedienung:** `pio device monitor -e smoketest` (interaktiv) oder
`python scripts/serial_cmd.py "<befehl>"` (skriptfähig, auch für Cline)

> Sicherheit: `scan`, `move`, `home` können die Achse bewegen. Not-Aus in Reichweite,
> Achse mechanisch frei, Messgut entfernt. Jeder Fahrbefehl stoppt bei Not-Aus oder Endschalter.

**Vorab (09.10.2026):** Die komplette Hardware ist mit der CNC-Firmware des DLC32 vorgetestet
(Motor, Endschalter, Not-Aus, Sensor, LEDs, Buzzer, Display, Touch). Der Smoketest bestätigt
jetzt die Werte für **unsere** Firmware (Pegel, Slots, Kurve) – Hardwarefehler sind nicht zu erwarten.

Ergebnis jeder Prüfung eintragen. Werte mit **→ config.h** werden danach in
`src/config.h` übernommen; erst dann wird `config.h` eingefroren (Sprint 0b).

**Durchführung:** 11.10.2026, Flash-Rechner Dell (`/dev/ttyUSB0`). Abschnitte 1–8 erledigt; 5.2 als bekannte Einschränkung dokumentiert.

> **Vorfall „move 2010“ (11.10.2026):** Ein Tippfehler (`move 2010` statt `move 20 10`) wurde von
> Smoketest 0a als 2010 mm interpretiert; die Fahrt lief unüberwacht über Not-Aus und Endschalter
> hinweg, die Anlage wurde per Netzstecker gestoppt. Ursachen: Endschalterprüfung nicht
> richtungsabhängig, `forceStop()` wird bei anstehender Folgebewegung von FastAccelStepper
> verworfen, keine Überwachung nach Rückkehr von `waitMotion`, keine Argumentprüfung.
> **Behoben in Smoketest 0a.1** (Argumentprüfung mit `usage` für alle Befehle, richtungsabhängige
> Endschalter, `forceStopAndNewPosition()`, Dauerüberwachung in `loop()`, `stop` immer ausführbar)
> und in der Produktiv-Firmware über Contract V1.4 / V1.4.1 (FORCE_STOP wiederholt,
> Unit-Tests für unsinnige Eingaben inkl. NaN/±Inf). Nachtest: `move 2010` → `usage`-Fehler ✅.

---

## 1. Toolchain

| # | Befehl / Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 1.1 | `pio run -e smoketest` | Build ohne Fehler (pioarduino, FastAccelStepper-Commit, LovyanGFX) | ✅ |
| 1.2 | `pio test -e native -f native/test_env` | `1 Tests 0 Failures` | ✅ (übrige Native-Suites auf dem Dell erwartungsgemäß rot – keine Referenz-Implementierung) |
| 1.3 | Flashen, `ver` | `arduino` 3.x, `idf` ≥ v5.3, `i2s_mux: true`, `flash_mb: 4` | arduino 3.3.12, idf v5.5.5, chip_rev 101, i2s_mux true ✅ · `flash_mb: 8` (Chip hat 8 MB, Projekt nutzt 4 MB – unkritisch). Treiber nach Boot gesperrt; beim Reset kurzes Motorzucken (I2S-Register undefiniert, bekannt) |
| 1.4 | `crc` | `match: true` (eigene CRC = `esp_rom_crc32_le` = CBF43926) | CBF43926, match true ✅ |
| 1.5 | `part` | nvs 20 KB, otadata 8 KB, app0 2944 KB, spiffs 1088 KB | wie spezifiziert, Ende 0x400000 ✅ |
| 1.6 | `nvs` / `fs` | `nvs: true` / `spiffs: true` | nvs true ✅ · spiffs true nach Erstformatierung (einmalig `-10025`), total 1016801 Byte ✅ |
| 1.7 | Versionen in platformio.ini festpinnen (LovyanGFX exakte Version aus Build-Log) | | platform 55.03.312-1, LovyanGFX 1.2.32, ArduinoJson 7.4.3 ✅ · Upload 115200 Bd über `/dev/ttyUSB0` (in platformio.ini); `brltty` auf dem Dell entfernt (belegte den CH340) |

## 2. Eingänge

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 2.1 | `watch 20`, Endschalter vorn betätigen | `limit_fwd` wechselt true/false | Pegel betätigt: **LOW** (GPIO 36), richtiger Anschluss, kein Flackern ✅ **→ LIMIT_ACTIVE_LEVEL = LOW** |
| 2.2 | dito hinten | `limit_bwd` wechselt | LOW (GPIO 35), richtiger Anschluss, kein Flackern ✅ |
| 2.3 | Einfacher Schalter (Standard): `estop low`, `watch 20`, Schalter drücken / lösen | `estop` true / false | ✅ |
| 2.4 | Info: Leitung abziehen | mit einfachem Schalter **kein** Not-Aus (keine Drahtbrucherkennung – so gewollt für die Tests) | kein Not-Aus, wie erwartet ✅ |
| 2.5 | Später mit Öffner: `estop high`, drücken / lösen, Leitung abziehen | gedrückt **und** abgezogen → `estop: true` | entfällt – eingebaut ist ein Schließer, `estop_nc` = false. Offener Punkt: Not-Aus zusätzlich hart verdrahten (wirkt derzeit nur per Software) |

## 3. I2S-Schieberegister (wichtigster Teil)

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 3.1 | `scan 800` – beobachten, bei welchem Slot was passiert | Buzzer (i2so.7), Motor-Enable (Motor wird fest, i2so.0), Step (ein Schritt hörbar, i2so.1), Dir (i2so.2). HV-LED ist jetzt GPIO 4 (nicht im Scan) | Buzzer Slot **7**, Enable Slot **0**, Step Slot **1**, Dir Slot **2** ✅ |
| 3.2 | Formel ableiten | Slot = f(i2so) | **→ `I2SO_TO_SLOT(n)` = n** (unverändert) |
| 3.3 | Nach Anpassung neu flashen: `beep 300` | Buzzer piept | ✅ (auch `beep 4000`) |
| 3.4 | `hv 1`, `hv 0`, `shutter 1`, `shutter 0` | grüne LED (GPIO 4) / rote LED (GPIO 32) folgen; nach Reset beide aus | ✅ |
| 3.5 | Beobachten | blaue Board-LED (GPIO 2) blinkt 1 s an / 1 s aus | ✅ |

## 4. Antrieb

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 4.1 | `stepper` | `stepper: true` | ✅ |
| 4.2 | `move 10 5` (10 mm mit 5 mm/s) – Weg mit Messschieber messen | 10,0 mm (±0,2) | 10 mm ✅ · Langstrecke: mit 42 mm/U ergaben Soll 130 mm nur 126,2 mm → **`MM_PER_REV` = 40,77** (78,49 Schritte/mm, Contract V1.4); Kontrolle danach 130,25 mm (< 0,2 %) ✅ |
| 4.3 | Richtung: positive mm = weg vom hinteren Endschalter? | ja | ❌ zunächst falsch herum → **`DIR_POSITIVE_HIGH` = false**; danach ✅ (positiv = Richtung Messgut/vorn) |
| 4.4 | `move 50 20`, `move -50 20` (Maximalgeschwindigkeit 20 mm/s) | läuft ruhig, kein Schrittverlust (Position danach wie vorher) | ✅ |
| 4.4a | `move 20 1`, `move -20 1` (Minimalgeschwindigkeit 1 mm/s) | gleichmäßig, kein Rattern | ✅ · 1 … 20 mm/s ohne Schrittverlust; **Resonanz bei 5 mm/s** (= Homing-Geschwindigkeit) – offener Punkt, Resonanztest folgt |
| 4.5 | Beschleunigung: ggf. `ACCEL_MM_S2` anpassen | kein Ruckeln | **→ `ACCEL_MM_S2` = 500** (unverändert) ✅ |
| 4.6 | `home` (5 mm/s) | stoppt am hinteren Endschalter (`reason: home_switch`) | ✅ (Dauer abhängig von der Startposition, z. B. 13422 ms / 29159 ms) |
| 4.6a | Nach `home` (steht am hinteren Schalter): `move 250 20` – fährt bis zum vorderen Schalter | `reason: limit_fwd`, `pos_mm` = Lage des vorderen Schalters ab hinterem Schalter | **246,34 mm** → Achse 244,3 mm → **`axis_max_mm` = 242** (Contract V1.4) ✅ |
| 4.6b | Kontrolle nach Anpassung: `home`, `move 244 10` (= Achse 242 mm) | vorderer Endschalter löst **nicht** aus; Abstand ≈ 2,3 mm | ✅ |
| 4.7 | Not-Aus während `move 100 10` | sofortiger Stopp, `reason: estop` | ✅ in beiden Richtungen (Smoketest 0a.1) |

## 5. Abstandssensor

Voraussetzung (V1.7.2): am Sensor **11 kΩ Vo–GND** (sonst hängt der Eingang nach den ersten
Messungen am Anschlag, Emitterfolger-Ausgang ohne Last) und 47 µF Vcc–GND. R20 (4k7 Pull-up)
auf dem DLC32 bleibt entfernt, R23 ist der Serienwiderstand zu IO34.

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 5.1 | `home`, `move 117 10` (= Achse 115, Mitte Stufe 1), dann 5 × `move 20 10`; an jeder Stufenmitte `adc 10` | `mm` mit Kurve 168,8·V^-1,398 = 65, 60 … 40 mm (±1 mm) | 1972 / 2091 / 2236 / 2403 / 2588 / 2777 mV → max. Abweichung 0,48 mm ✅ · **→ Kurve `k_mm` = 168,8, `exponent` = −1,398, `min_mm` = 35** (Contract V1.4.2) |
| 5.2 | Multimeter direkt am Sensorausgang vs. `mv` | Abweichung < 50 mV, auch bei 2,8 V (ADC-Rand) | ⚠️ **Bekannte Einschränkung.** DVM (Vo gegen GND am Sensor) / ESP: 1,743 / 1,973 V (Stufe 1), 0,703 / 0,942 V (neben dem Messgut) → konstanter Versatz +230…239 mV, Steigung 0,99. `adcraw` (0a.3): raw/mV = 2250/1978, 2404/2104, 2590/2255, 2795/2422, 3037/2596, 3335/2782 (Stufen 1–6), 989/949 (Achse 65, ∞); `efuse: vref` → Umrechnung der Bibliothek plausibel (≈ 0,816 mV/LSB + 142 mV), schon der Rohwert ist erhöht. Ursache nicht abschließend geklärt (Messung am ESP-Pin zu riskant). Ohne Einfluss auf das Messergebnis, da die Kurve gegen die ESP-Werte kalibriert ist (5.1). Option für später: Zweipunkt-Kalibrierung des ADC mit Referenzquelle oder parallel messendem DVM |
| 5.3 | Rauschen: `adc 10` an den Stufen | Streuung < ±0,5 mm | σ ≤ 8,4 mV = 0,34 mm ✅ |
| 5.4 | Neben dem Messgut (Achse 242, Sensor sieht den unteren Ausleger, Boden 60 mm tiefer) | weit außerhalb des Prüfkörpers (≫ 70 mm) | 933 mV ≈ 186 mm ✅ |

Prüfkörper: Achse 105 … 225 mm, sechs Stufen à 20 mm, Stufenmitten 115 / 135 … 215 mm;
Messposition der Produktiv-Firmware 155 mm (`measure_pos`, Contract V1.4.2).

## 6. Display und Touch

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 6.1 | `disp` | Farbbalken rot/grün/blau in dieser Reihenfolge, Text lesbar Farben, Reihenfolge, Text und Lage korrekt, gelber Rahmen exakt am Displayrand ✅ **→ `invert` = false, `rgb_order` = false** (unverändert) |
| 6.2 | `touch 20`, vier Kreise antippen | `x/y` liegen auf den Kreisen (±10 px) ❌ zunächst um 180° gedreht (Punkte in der diagonal gegenüberliegenden Ecke). Nach Korrektur: alle vier Kreise auf ≤ 5 px getroffen ✅ **→ `TOUCH_X_MIN/MAX` = 130/3775, `TOUCH_Y_MIN/MAX` = 200/3900** (aus den Ecken auf den Displayrand hochgerechnet) |
| 6.3 | ggf. `offset_rotation` (0…7) in lgfx_config.h anpassen | Touch passt zum Bild | **→ Touch `offset_rotation` = 2** (Panel bleibt 0) ✅ |

## 7. WLAN

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 7.1 | `wifi`, Tablet mit `ADD-Demo` verbinden, `http://192.168.4.1/ping` | JSON `pong: true` | ✅ (AP-Ping bei gleichzeitig aktivem STA) |
| 7.2 | `sta <ssid> <passwort>` (Router) | `sta: true`, IP aus dem Heimnetz; AP weiter erreichbar | IP 192.168.1.39, RSSI −46 dBm, 3500 ms, AP 192.168.4.1 ✅ · Erster Versuch: falsches Passwort, der Hintergrund-Retry blockierte den zweiten (`ESP_ERR_WIFI_STATE`); nach Reset ✅. **Smoketest 0a.2:** `sta` bricht frühere Versuche ab, kein Auto-Reconnect, keine Zugangsdaten im NVS, Feld `reason` bei Fehlschlag |
| 7.3 | Vom PC im Heimnetz: `http://<IP>/ping` | JSON `pong: true` | ✅ |

## 8. Agenten-Probelauf (code-server + Cline + Qwen)

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 8.1 | Cline-Task: *„Füge im Smoketest den Befehl `uptime` hinzu, der `{"uptime_ms":…}` ausgibt. Bring `pio run -e smoketest -s` grün. Nicht flashen."* | Cline liest Dateien, editiert, baut, sieht Compilerausgabe | ✅ Funktion, Eintrag in `CMDS`, Dispatcher und Help-Text korrekt und im Stil der Datei; Build grün (Exit-Code 0 eigens geprüft). Nur die Befehlsübersicht im Dateikopf fehlte (nachgetragen in 0a.3). Ablauf: Erster Task brach nach dem Editieren am Build ab – auf dem Dell fand Cline das kaputte apt-Paket `/usr/bin/pio` 4.3.4; Qwen grenzte das sinnvoll ein und fragte vor `pip install` um Freigabe (abgelehnt). Behoben: apt-Paket entfernt, Symlinks `~/.local/bin/pio`, Hinweis in `.clinerules`. Der zweite Task hielt die eigene, nicht committete Änderung für „bereits vorhanden“ (frischer Task ohne Gedächtnis) |
| 8.2 | Kontextanzeige in Cline nach 8.1 | < 50 % von 64k | 17,0k von 65,5k = **26 %** ✅; Kontexttiefe im Server < 30k, keine Verdichtung |
| 8.3 | `python scripts/serial_cmd.py "uptime"` aus Cline heraus (nach manuellem Flashen) | Cline liest die JSON-Antwort | ✅ `{"uptime_ms":48570}`. Erster Aufruf fiel in den Boot nach dem Flashen (nur Boot-Log); Qwen erkannte das und wiederholte richtig |
| 8.4 | Geschwindigkeit (Tokens/s) im LLM-Server-Log | – | **18 t/s** (konstant), llama.cpp mit **Vulkan**-Backend. HIP/ROCm lief nicht stabil; mit HIP ca. 50 % schneller erwartet – später klären |

---

## Offene Punkte

- Option: Zweipunkt-Kalibrierung des ESP-ADC (Referenzquelle oder parallel messendes DVM), siehe 5.2
- llama.cpp mit HIP stabil bekommen (derzeit Vulkan, ca. halbe Tokenrate)
- Resonanz bei 5 mm/s (Homing) – Resonanztest, ggf. Homing-Geschwindigkeit ändern
- Not-Aus hart verdrahten (wirkt derzeit nur per Software)
- Motorzucken beim Reset (bekannt, I2S-Register undefiniert bis zur Initialisierung)

## Freigabe

- [x] Alle ❌ behoben oder als bekannte Einschränkung dokumentiert (5.2: ADC-Versatz)
- [x] `src/config.h` mit allen **→**-Werten aktualisiert, `VERIFY(0a)`-Kommentare entfernt
      (erledigt)
- [x] `platformio.ini`: LovyanGFX-Version exakt gepinnt (1.2.32)
- [x] Commit „Sprint 0a abgeschlossen“ (Tag `sprint-0a`) → `config.h` gilt ab jetzt als eingefroren

Datum / Unterschrift: 11.10.2026, ______________________
