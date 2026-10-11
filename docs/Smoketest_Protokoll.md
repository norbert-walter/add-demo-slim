# Sprint 0a – Smoketest-Protokoll

**Anlage:** ADD Demo System (Slim) · **Firmware:** `pio run -e smoketest -t upload`
**Bedienung:** `pio device monitor -e smoketest` (interaktiv) oder
`python scripts/serial_cmd.py "<befehl>"` (skriptfähig, auch für Cline)

> Sicherheit: `scan`, `move`, `home` können die Achse bewegen. Not-Aus in Reichweite,
> Achse mechanisch frei, Messgut entfernt. Jeder Fahrbefehl stoppt bei Not-Aus oder Endschalter.

**Vorab (09.10.2026):** Die komplette Hardware ist mit der CNC-Firmware des DLC32 vorgetestet
(Motor, Endschalter, Not-Aus, Sensor, LEDs, Buzzer, Display, Touch). Der Smoketest bestätigt
jetzt die Werte für **unsere** Firmware (Pegel, Slots, Kurve) – Hardwarefehler sind nicht zu erwarten.

Ergebnis jeder Prüfung eintragen. Werte mit **→ config.h** werden danach in
`src/config.h` übernommen; erst dann wird `config.h` eingefroren (Sprint 0b).

---

## 1. Toolchain

| # | Befehl / Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 1.1 | `pio run -e smoketest` | Build ohne Fehler (pioarduino, FastAccelStepper-Commit, LovyanGFX) | ✅ |
| 1.2 | `pio test -e native -f native/test_env` | `1 Tests 0 Failures` | ✅ |
| 1.3 | Flashen, `ver` | `arduino` 3.x, `idf` ≥ v5.3, `i2s_mux: true`, `flash_mb: 4` | arduino: ___ idf: ___ |
| 1.4 | `crc` | `match: true` (eigene CRC = `esp_rom_crc32_le` = CBF43926) | |
| 1.5 | `part` | nvs 20 KB, otadata 8 KB, app0 2944 KB, spiffs 1088 KB | |
| 1.6 | `nvs` / `fs` | `nvs: true` / `spiffs: true` | |
| 1.7 | Versionen in platformio.ini festpinnen (LovyanGFX exakte Version aus Build-Log) | | LovyanGFX: 1.2.32 |

## 2. Eingänge

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 2.1 | `watch 20`, Endschalter vorn betätigen | `limit_fwd` wechselt true/false | Pegel betätigt: ___ **→ LIMIT_ACTIVE_LEVEL** |
| 2.2 | dito hinten | `limit_bwd` wechselt | |
| 2.3 | Einfacher Schalter (Standard): `estop low`, `watch 20`, Schalter drücken / lösen | `estop` true / false | ✅/❌ |
| 2.4 | Info: Leitung abziehen | mit einfachem Schalter **kein** Not-Aus (keine Drahtbrucherkennung – so gewollt für die Tests) | |
| 2.5 | Später mit Öffner: `estop high`, drücken / lösen, Leitung abziehen | gedrückt **und** abgezogen → `estop: true` | ✅/❌ **→ Parameter `estop_nc` = true** (per REST, nicht in config.h) |

## 3. Abstandssensor

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
Voraussetzung (V1.7.2): am Sensor **11 kΩ Vo–GND** (sonst hängt der Eingang nach den ersten
Messungen am Anschlag) und 47 µF Vcc–GND.

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 3.1 | `home`, `move 117 10` (= Achse 115, Mitte Stufe 1), dann 5 × `move 20 10`; an jeder Stufenmitte `adc 10` | `mm` mit Kurve 168,8·V^-1,398 = 65, 60 … 40 mm (±1 mm) | 0a: 1972 / 2091 / 2236 / 2403 / 2588 / 2777 mV → max. Abweichung 0,48 mm ✅ |
| 3.2 | Multimeter direkt am Sensorausgang vs. `mv` | Abweichung < 50 mV, auch bei 2,8 V (ADC-Rand) | |
| 3.3 | Rauschen: `adc 10` an den Stufen | Streuung < ±0,5 mm | 0a: σ ≤ 8,4 mV = 0,34 mm ✅ |
| 3.4 | Neben dem Messgut (Achse 242, unterer Ausleger) | weit außerhalb des Prüfkörpers (≫ 70 mm) | 0a: 933 mV ≈ 186 mm ✅ |

## 4. I2S-Schieberegister (wichtigster Teil)

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 4.1 | `scan 800` – beobachten, bei welchem Slot was passiert | Buzzer (i2so.7), Motor-Enable (Motor wird fest, i2so.0), Step (ein Schritt hörbar, i2so.1), Dir (i2so.2). HV-LED ist jetzt GPIO 4 (nicht im Scan) | Buzzer Slot: ___ Enable Slot: ___ Step Slot: ___ |
| 4.2 | Formel ableiten | Slot = f(i2so) | **→ `I2SO_TO_SLOT(n)`** = ___ |
| 4.3 | Nach Anpassung neu flashen: `beep 300` | Buzzer piept | |
| 4.4 | `hv 1`, `hv 0`, `shutter 1`, `shutter 0` | grüne LED (GPIO 4) / rote LED (GPIO 32) folgen; nach Reset beide aus | |
| 4.5 | Beobachten | blaue Board-LED (GPIO 2) blinkt 1 s an / 1 s aus | |

## 5. Antrieb

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 5.1 | `stepper` | `stepper: true` | |
| 5.2 | `move 10 5` (10 mm mit 5 mm/s) – Weg mit Messschieber messen | 10,0 mm (±0,2) | gemessen: ___ mm |
| 5.3 | Richtung: positive mm = weg vom hinteren Endschalter? | ja | ✅ / ❌ **→ `DIR_POSITIVE_HIGH`** |
| 5.4 | `move 50 20`, `move -50 20` (Maximalgeschwindigkeit 20 mm/s) | läuft ruhig, kein Schrittverlust (Position danach wie vorher) | |
| 5.4a | `move 20 1`, `move -20 1` (Minimalgeschwindigkeit 1 mm/s) | gleichmäßig, kein Rattern | |
| 5.5 | Beschleunigung: ggf. `ACCEL_MM_S2` anpassen | kein Ruckeln | **→ `ACCEL_MM_S2`** = ___ |
| 5.6 | `home` (5 mm/s) | stoppt am hinteren Endschalter (`reason: home_switch`) | Dauer: ___ ms |
| 5.6a | Nach `home` (steht am hinteren Schalter): `move 250 20` – fährt bis zum vorderen Schalter | `reason: limit_fwd`, `pos_mm` = Lage des vorderen Schalters ab hinterem Schalter | 0a: **246,34 mm** → Achse 244,3 mm → **`axis_max_mm` = 242** (Contract V1.4) |
| 5.6b | Kontrolle nach Anpassung: `home`, `move 244 10` (= Achse 242 mm) | vorderer Endschalter löst **nicht** aus; Abstand ≈ 2,3 mm | ✅/❌ |
| 5.7 | Not-Aus während `move 100 10` | sofortiger Stopp, `reason: estop` | |

## 6. Display und Touch

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 6.1 | `disp` | Farbbalken rot/grün/blau in dieser Reihenfolge, Text lesbar | Farben korrekt? **→ `invert`, `rgb_order` in lgfx_config.h** |
| 6.2 | `touch 20`, vier Kreise antippen | `x/y` liegen auf den Kreisen (±10 px) | raw min/max: X ___/___ Y ___/___ **→ TOUCH_*** |
| 6.3 | ggf. `offset_rotation` (0…7) in lgfx_config.h anpassen | Touch passt zum Bild | |

## 7. WLAN

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 7.1 | `wifi`, Tablet mit `ADD-Demo` verbinden, `http://192.168.4.1/ping` | JSON `pong: true` | |
| 7.2 | `sta <ssid> <passwort>` (Router) | `sta: true`, IP aus dem Heimnetz; AP weiter erreichbar | IP: ___ |
| 7.3 | Vom PC im Heimnetz: `http://<IP>/ping` | JSON `pong: true` | |

## 8. Agenten-Probelauf (code-server + Cline + Qwen)

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 8.1 | Cline-Task: *„Füge im Smoketest den Befehl `uptime` hinzu, der `{"uptime_ms":…}` ausgibt. Bring `pio run -e smoketest -s` grün. Nicht flashen."* | Cline liest Dateien, editiert, baut, sieht Compilerausgabe | ✅ / ❌ |
| 8.2 | Kontextanzeige in Cline nach 8.1 | < 50 % von 64k | ___ % |
| 8.3 | `python scripts/serial_cmd.py "ver"` aus Cline heraus (nach manuellem Flashen) | Cline liest die JSON-Antwort | |
| 8.4 | Geschwindigkeit (Tokens/s) im LLM-Server-Log | – | ___ t/s |

---

## Freigabe

- [ ] Alle ❌ behoben oder als bekannte Einschränkung dokumentiert
- [ ] `src/config.h` mit allen **→**-Werten aktualisiert, `VERIFY(0a)`-Kommentare entfernt
- [ ] `platformio.ini`: LovyanGFX-Version exakt gepinnt
- [ ] Commit „Sprint 0a abgeschlossen“ → `config.h` gilt ab jetzt als eingefroren

Datum / Unterschrift: ______________________
