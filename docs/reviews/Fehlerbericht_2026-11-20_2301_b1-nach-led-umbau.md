# Abnahme A1 – Fehlerbericht

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 20.11.2026 23:01 / 23:04 |
| Gerät | http://192.168.1.127/ |
| Firmware | boot: ready, FW 1.0.0 |
| Repository | 30ae60e Review B1: bestanden; doppelte Abschnittsueberschrift in types.cpp entfernt |
| Aufruf | `--host 192.168.1.127 --only T18K,T18,T19,T20 --tag b1-nach-led-umbau` |
| Ergebnis gesamt | **4 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 2 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | **nicht ausgeführt – Norbert Walter bestätigt am 20.11.2026 23:01 wahrheitsgemäß und auf eigene Verantwortung, dass der Sicherheitsblock bestanden war (Nachweis: Protokoll Abnahme_A1_2026-11-08_1519_wiederholung.md)** |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt (Aufruf) |
| Ergebnis Block 3 – automatische Prüfungen | 4 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 2 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 2:54 min |

### Symbole

| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |

## Block 3 – automatische Prüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ❌ **nicht bestanden** | Rohwert mehr als 7 mm von der Referenz: Sensor oder Stufenkörper prüfen [Punkt 1 unplausibel; Punkt 2 unplausibel; Punkt 3 unplausibel] | 30 s |
| T29 | 4 | Zeitwerte ≤ 3100 µs (inkl. Summer-Signale aus Block 1 und 2) | ❌ **nicht bestanden** | seit dem letzten Neustart | – |

## Messwerte

Nur die Werte außerhalb des Solls.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ❌ | T18K | Punkt 1 (115 mm): Rohwert (unkalibriert) gegen Referenz 65 | 76.5 | mm | 65 ± 7 |
| ❌ | T18K | Punkt 2 (135 mm): Rohwert (unkalibriert) gegen Referenz 60 | 69.1 | mm | 60 ± 7 |
| ❌ | T18K | Punkt 3 (155 mm): Rohwert (unkalibriert) gegen Referenz 55 | 62.1 | mm | 55 ± 7 |
| ❌ | T29 | period_max_us | 17423 | µs | ≤ 3100 |

## Anhang

### GET_STATE am Ende

```json
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 1955, "period_max_us": 17423, "overruns": 809, "prio_lat_max_us": 1406, "reset_reason": "POWERON", "boots": 1, "brownouts": 0, "bod_level": 7, "crc": "481946FB"}
```

### /log am Ende

```
boot: reset POWERON, boots 1, brownouts 0, brownout level 7
ap: ADD-Demo 192.168.4.1
boot: ready, FW 1.0.0
sta: attempt 1/3
sta: connected 192.168.1.127
net: http 272 ms
net: http 545 ms
net: http 396 ms
net: http 681 ms
net: http 273 ms
net: http 541 ms
net: http 435 ms
nvs: save deferred until axis idle and HV off
nvs: saved
nvs: save deferred until axis idle and HV off
nvs: saved
ctrl: cycle 230 us, period 17423 us (adc 106, tick 77, cmd 16, out 1, pub 30), 1 events
```

