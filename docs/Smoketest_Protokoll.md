# Sprint 0a – Smoketest-Protokoll

**Anlage:** ADD Demo System (Slim) · **Firmware:** `pio run -e smoketest -t upload`
**Bedienung:** `pio device monitor -e smoketest` (interaktiv) oder
`python scripts/serial_cmd.py "<befehl>"` (skriptfähig, auch für Cline)

> Sicherheit: `scan`, `move`, `home` können die Achse bewegen. Not-Aus in Reichweite,
> Achse mechanisch frei, Messgut entfernt. Jeder Fahrbefehl stoppt bei Not-Aus oder Endschalter.

Ergebnis jeder Prüfung eintragen. Werte mit **→ config.h** werden danach in
`src/config.h` übernommen; erst dann wird `config.h` eingefroren (Sprint 0b).

---

## 1. Toolchain

| # | Befehl / Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 1.1 | `pio run -e smoketest` | Build ohne Fehler (pioarduino, FastAccelStepper-Commit, LovyanGFX) | |
| 1.2 | `pio test -e native -f native/test_env` | `1 Tests 0 Failures` | |
| 1.3 | Flashen, `ver` | `arduino` 3.x, `idf` ≥ v5.3, `i2s_mux: true`, `flash_mb: 4` | arduino: ___ idf: ___ |
| 1.4 | `crc` | `match: true` (eigene CRC = `esp_rom_crc32_le` = CBF43926) | |
| 1.5 | `part` | nvs 20 KB, otadata 8 KB, app0 2944 KB, spiffs 1088 KB | |
| 1.6 | `nvs` / `fs` | `nvs: true` / `spiffs: true` | |
| 1.7 | Versionen in platformio.ini festpinnen (LovyanGFX exakte Version aus Build-Log) | | LovyanGFX: ___ |

## 2. Eingänge

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 2.1 | `watch 20`, Endschalter vorn betätigen | `limit_fwd` wechselt true/false | Pegel betätigt: ___ **→ LIMIT_ACTIVE_LEVEL** |
| 2.2 | dito hinten | `limit_bwd` wechselt | |
| 2.3 | Not-Aus drücken / lösen | `estop` true / false | Pegel gedrückt: ___ |
| 2.4 | **Drahtbruch-Test:** Not-Aus-Leitung abziehen | `estop: true` (fail-safe!) | ✅/❌ – bei ❌ Öffner verdrahten |

## 3. Abstandssensor

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 3.1 | Zielfläche in 40 / 100 / 200 / 300 mm, je `adc 10` | `mm` ≈ Sollwert (±10 %) | 40: ___ 100: ___ 200: ___ 300: ___ |
| 3.2 | Multimeter direkt am Sensorausgang vs. `mv` | Abweichung < 50 mV (sonst Pull-up/Filter am Z--Eingang prüfen) | |
| 3.3 | Rauschen: `adc 20` bei fester Distanz | Streuung < ±1 mm | |
| 3.4 | Abstand Sensor → leere Auflage | – | ___ mm **→ Default `base_dist`** |

## 4. I2S-Schieberegister (wichtigster Teil)

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 4.1 | `scan 800` – beobachten, bei welchem Slot was passiert | HV-LED (i2so.3), Buzzer (i2so.7), Motor-Enable (Motor wird fest, i2so.0), Step (ein Schritt hörbar, i2so.1), Dir (i2so.2) | HV-LED Slot: ___ Buzzer Slot: ___ Enable Slot: ___ |
| 4.2 | Formel ableiten | Slot = f(i2so) | **→ `I2SO_TO_SLOT(n)`** = ___ |
| 4.3 | Nach Anpassung neu flashen: `hv 1`, `hv 0`, `beep 300` | LED / Buzzer reagieren | |

## 5. Antrieb

| # | Aktion | Erwartung | Ergebnis |
|---|---|---|---|
| 5.1 | `stepper` | `stepper: true` | |
| 5.2 | `move 10 300` – Weg mit Messschieber messen | 10,0 mm (±0,2) | gemessen: ___ mm |
| 5.3 | Richtung: positive mm = weg vom hinteren Endschalter? | ja | ✅ / ❌ **→ `DIR_POSITIVE_HIGH`** |
| 5.4 | `move 50 3000`, `move -50 3000` | läuft ruhig, kein Schrittverlust (Position danach wie vorher) | |
| 5.5 | Beschleunigung: ggf. `ACCEL_MM_S2` anpassen | kein Ruckeln | **→ `ACCEL_MM_S2`** = ___ |
| 5.6 | `home` | stoppt am hinteren Endschalter (`reason: home_switch`) | Dauer: ___ ms |
| 5.7 | Not-Aus während `move 100 300` | sofortiger Stopp, `reason: estop` | |

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
