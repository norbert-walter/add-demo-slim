# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 20.11.2026 22:56 / 22:59 |
| Gerät | http://192.168.1.127/ |
| Firmware | boot: ready, FW 1.0.0 |
| Repository | 30ae60e Review B1: bestanden; doppelte Abschnittsueberschrift in types.cpp entfernt |
| Aufruf | `--host 192.168.1.127 --only T18K,T18,T19,T20 --tag b1-nach-led-umbau` |
| Ergebnis gesamt | **5 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | **nicht ausgeführt – Norbert Walter bestätigt am 20.11.2026 22:56 wahrheitsgemäß und auf eigene Verantwortung, dass der Sicherheitsblock bestanden war (Nachweis: Protokoll Abnahme_A1_2026-11-08_1519_wiederholung.md)** |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt (Aufruf) |
| Ergebnis Block 3 – automatische Prüfungen | 5 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 3:21 min |

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
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ❌ **nicht bestanden** | Rohwert mehr als 7 mm von der Referenz: Sensor oder Stufenkörper prüfen [Punkt 1 unplausibel; Punkt 2 unplausibel] | 31 s |
| T18 | 2, 8 | Stehende Messung je Stufe: Mittelwert ±1 mm, Einzelwerte 3σ ≤ 2 mm | ✅ bestanden | alle Stufen in Toleranz (Einzelwerte im Anhang) – Shutter 5× geschlossen (Firmware vor V1.6.4) | 55 s |
| T19 | 8 | Profilfahrt hin und zurück: Kanten und Stufen aufgezeichnet (ohne Bewertung), Shutter durchgehend offen, Statistik plausibel | ✅ bestanden | Profil hin und zurück aufgezeichnet (Kanten und Stufen im Protokoll, ohne Bewertung) – Shutter 1× geschlossen (Firmware vor V1.6.4) | 78 s |
| T20 | 8 | Kalibriertabelle (6 Stufen) gespeichert und wirksam: Abweichung je Stufe ±1 mm (Mittelwert, Sigma) | ✅ bestanden | Tabelle gültig und kalibriert, Abweichung je Stufe ≤ 1 mm | 0 s |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 4 min | – |
| T29 | 4 | Zeitwerte ≤ 3100 µs (inkl. Summer-Signale aus Block 1 und 2) | ✅ bestanden | seit dem letzten Neustart | – |

## Messwerte

Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| – | T18K | Punkt 1 (115 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 2 (135 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 3 (155 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 4 (175 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 5 (195 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 6 (215 mm): Versuche bis stabil | 1 |  |  |
| ❌ | T18K | Punkt 1 (115 mm): Rohwert (unkalibriert) gegen Referenz 65 | 76.2 | mm | 65 ± 7 |
| – | T18K | Punkt 1 (115 mm): Änderung gegenüber der alten Tabelle | -0.3 | mm |  |
| ❌ | T18K | Punkt 2 (135 mm): Rohwert (unkalibriert) gegen Referenz 60 | 69.2 | mm | 60 ± 7 |
| – | T18K | Punkt 2 (135 mm): Änderung gegenüber der alten Tabelle | -0.7 | mm |  |
| ✅ | T18K | Punkt 3 (155 mm): Rohwert (unkalibriert) gegen Referenz 55 | 61.7 | mm | 55 ± 7 |
| – | T18K | Punkt 3 (155 mm): Änderung gegenüber der alten Tabelle | -0.4 | mm |  |
| ✅ | T18K | Punkt 4 (175 mm): Rohwert (unkalibriert) gegen Referenz 50 | 55.4 | mm | 50 ± 7 |
| – | T18K | Punkt 4 (175 mm): Änderung gegenüber der alten Tabelle | -0.5 | mm |  |
| ✅ | T18K | Punkt 5 (195 mm): Rohwert (unkalibriert) gegen Referenz 45 | 48.2 | mm | 45 ± 7 |
| – | T18K | Punkt 5 (195 mm): Änderung gegenüber der alten Tabelle | -0.9 | mm |  |
| ✅ | T18K | Punkt 6 (215 mm): Rohwert (unkalibriert) gegen Referenz 40 | 42.3 | mm | 40 ± 7 |
| – | T18K | Punkt 6 (215 mm): Änderung gegenüber der alten Tabelle | -0.8 | mm |  |
| ✅ | T18K | calib_valid | True |  | True |
| ✅ | T18 | 115 mm: Abstand Mittelwert | 65.205 | mm | 65 ± 1 |
| ✅ | T18 | 115 mm: Dicke Mittelwert | 4.795 | mm | 5 ± 1 |
| ✅ | T18 | 115 mm: Dicke Mittelwert der Anlage (avg) | 4.8 | mm | 5 ± 1 |
| ✅ | T18 | 115 mm: Dicke Sigma | 0.152 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 115 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 115 mm: Dicke Min / Max | 4.5 / 5.1 | mm |  |
| – | T18 | 115 mm: Abstand Sigma | 0.152 | mm |  |
| ✅ | T18 | 135 mm: Abstand Mittelwert | 59.953 | mm | 60 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert | 10.048 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert der Anlage (avg) | 10.0 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Sigma | 0.096 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 135 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 135 mm: Dicke Min / Max | 9.8 / 10.2 | mm |  |
| – | T18 | 135 mm: Abstand Sigma | 0.096 | mm |  |
| ✅ | T18 | 155 mm: Abstand Mittelwert | 54.828 | mm | 55 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert | 15.173 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert der Anlage (avg) | 15.2 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Sigma | 0.072 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 155 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 155 mm: Dicke Min / Max | 15.0 / 15.3 | mm |  |
| – | T18 | 155 mm: Abstand Sigma | 0.072 | mm |  |
| ✅ | T18 | 175 mm: Abstand Mittelwert | 49.998 | mm | 50 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert | 20.003 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert der Anlage (avg) | 20.0 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Sigma | 0.036 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 175 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 175 mm: Dicke Min / Max | 19.9 / 20.1 | mm |  |
| – | T18 | 175 mm: Abstand Sigma | 0.036 | mm |  |
| ✅ | T18 | 195 mm: Abstand Mittelwert | 45.027 | mm | 45 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert | 24.972 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert der Anlage (avg) | 25.0 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Sigma | 0.06 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 195 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 195 mm: Dicke Min / Max | 24.8 / 25.1 | mm |  |
| – | T18 | 195 mm: Abstand Sigma | 0.06 | mm |  |
| ✅ | T18 | 215 mm: Abstand Mittelwert | 40.015 | mm | 40 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert | 29.985 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert der Anlage (avg) | 30.0 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Sigma | 0.053 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 215 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 215 mm: Dicke Min / Max | 29.9 / 30.1 | mm |  |
| – | T18 | 215 mm: Abstand Sigma | 0.053 | mm |  |
| – | T18 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 5 |  |  |
| ✅ | T19 | hin: Messwerte im Lauf | 359 |  | ≥ 201 |
| – | T19 | hin: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| – | T19 | hin: Kante Messgut ab (Info, Stufenkörper 105) | 107.1 | mm |  |
| – | T19 | hin: Kante Messgut bis (Info, Stufenkörper 225) | 222.9 | mm |  |
| ✅ | T19 | hin: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| – | T19 | hin: Stufe 115 mm: Dicke (Median, Info, Soll 5) | 5.0 | mm |  |
| – | T19 | hin: Stufe 115 mm: Anzahl Werte | 22 |  |  |
| – | T19 | hin: Stufe 135 mm: Dicke (Median, Info, Soll 10) | 10.1 | mm |  |
| – | T19 | hin: Stufe 135 mm: Anzahl Werte | 22 |  |  |
| – | T19 | hin: Stufe 155 mm: Dicke (Median, Info, Soll 15) | 15.0 | mm |  |
| – | T19 | hin: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 175 mm: Dicke (Median, Info, Soll 20) | 20.0 | mm |  |
| – | T19 | hin: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 195 mm: Dicke (Median, Info, Soll 25) | 24.8 | mm |  |
| – | T19 | hin: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 215 mm: Dicke (Median, Info, Soll 30) | 30.0 | mm |  |
| – | T19 | hin: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Mittelwert der Anlage | 17.7 | mm | 5 … 30 |
| ✅ | T19 | hin: Anzahl Werte der Anlage | 1158 |  | ≥ 51 |
| ✅ | T19 | zurück: Messwerte im Lauf | 362 |  | ≥ 201 |
| – | T19 | zurück: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| – | T19 | zurück: Kante Messgut ab (Info, Stufenkörper 105) | 106.1 | mm |  |
| – | T19 | zurück: Kante Messgut bis (Info, Stufenkörper 225) | 221.4 | mm |  |
| ✅ | T19 | zurück: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| – | T19 | zurück: Stufe 115 mm: Dicke (Median, Info, Soll 5) | 5.0 | mm |  |
| – | T19 | zurück: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 135 mm: Dicke (Median, Info, Soll 10) | 10.2 | mm |  |
| – | T19 | zurück: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 155 mm: Dicke (Median, Info, Soll 15) | 15.2 | mm |  |
| – | T19 | zurück: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 175 mm: Dicke (Median, Info, Soll 20) | 20.1 | mm |  |
| – | T19 | zurück: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 195 mm: Dicke (Median, Info, Soll 25) | 25.2 | mm |  |
| – | T19 | zurück: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 215 mm: Dicke (Median, Info, Soll 30) | 30.0 | mm |  |
| – | T19 | zurück: Stufe 215 mm: Anzahl Werte | 22 |  |  |
| ✅ | T19 | zurück: Mittelwert der Anlage | 17.7 | mm | 5 … 30 |
| ✅ | T19 | zurück: Anzahl Werte der Anlage | 1157 |  | ≥ 51 |
| – | T19 | Kante Messgut ab: Mittel hin/zurück (Info) | 106.6 | mm |  |
| – | T19 | Kante Messgut ab: Unterschied hin – zurück (Info) | 1.0 | mm |  |
| – | T19 | Kante Messgut bis: Mittel hin/zurück (Info) | 222.15 | mm |  |
| – | T19 | Kante Messgut bis: Unterschied hin – zurück (Info) | 1.5 | mm |  |
| – | T19 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 1 |  |  |
| – | T20 | Parameter calib_raw_1 | 76.2 | mm |  |
| – | T20 | Parameter calib_raw_2 | 69.2 | mm |  |
| – | T20 | Parameter calib_raw_3 | 61.7 | mm |  |
| – | T20 | Parameter calib_raw_4 | 55.4 | mm |  |
| – | T20 | Parameter calib_raw_5 | 48.2 | mm |  |
| – | T20 | Parameter calib_raw_6 | 42.3 | mm |  |
| – | T20 | Parameter calib_ref_1 | 65.0 | mm |  |
| – | T20 | Parameter calib_ref_2 | 60.0 | mm |  |
| – | T20 | Parameter calib_ref_3 | 55.0 | mm |  |
| – | T20 | Parameter calib_ref_4 | 50.0 | mm |  |
| – | T20 | Parameter calib_ref_5 | 45.0 | mm |  |
| – | T20 | Parameter calib_ref_6 | 40.0 | mm |  |
| ✅ | T20 | calib_valid | True |  | True |
| ✅ | T20 | Stufe 115 mm: Abweichung Abstand (Mittelwert – 65 mm) | 0.205 | mm | 0 ± 1 |
| – | T20 | Stufe 115 mm: Sigma Abstand | 0.152 | mm |  |
| ✅ | T20 | Stufe 135 mm: Abweichung Abstand (Mittelwert – 60 mm) | -0.047 | mm | 0 ± 1 |
| – | T20 | Stufe 135 mm: Sigma Abstand | 0.096 | mm |  |
| ✅ | T20 | Stufe 155 mm: Abweichung Abstand (Mittelwert – 55 mm) | -0.172 | mm | 0 ± 1 |
| – | T20 | Stufe 155 mm: Sigma Abstand | 0.072 | mm |  |
| ✅ | T20 | Stufe 175 mm: Abweichung Abstand (Mittelwert – 50 mm) | -0.002 | mm | 0 ± 1 |
| – | T20 | Stufe 175 mm: Sigma Abstand | 0.036 | mm |  |
| ✅ | T20 | Stufe 195 mm: Abweichung Abstand (Mittelwert – 45 mm) | 0.027 | mm | 0 ± 1 |
| – | T20 | Stufe 195 mm: Sigma Abstand | 0.06 | mm |  |
| ✅ | T20 | Stufe 215 mm: Abweichung Abstand (Mittelwert – 40 mm) | 0.015 | mm | 0 ± 1 |
| – | T20 | Stufe 215 mm: Sigma Abstand | 0.053 | mm |  |
| – | T28 | Laufzeit | 3.5 | min |  |
| ✅ | T28 | unerwartete Neustarts | 0 |  | 0 |
| ✅ | T29 | cycle_max_us | 1955 | µs | ≤ 3100 |
| ✅ | T29 | period_max_us | 2412 | µs | ≤ 3100 |
| ✅ | T29 | prio_lat_max_us | 1406 | µs | ≤ 3100 |
| – | T29 | overruns (Info) | 460 |  |  |

## Anhang

### Konfiguration zu Beginn (GET_CONFIG)

```json
{
 "measure_pos": 155.0,
 "maint_pos": 40.0,
 "traverse_start": 80.0,
 "traverse_end": 240.0,
 "traverse_speed": 10.0,
 "shutter_hold": 5.0,
 "edge_threshold": 3.0,
 "base_dist": 70.0,
 "calib_raw_1": 76.5,
 "calib_ref_1": 65.0,
 "calib_raw_2": 69.9,
 "calib_ref_2": 60.0,
 "calib_raw_3": 62.1,
 "calib_ref_3": 55.0,
 "calib_raw_4": 55.9,
 "calib_ref_4": 50.0,
 "calib_raw_5": 49.1,
 "calib_ref_5": 45.0,
 "calib_raw_6": 43.1,
 "calib_ref_6": 40.0,
 "calib_valid": true,
 "interlock": true,
 "cooling_flow": true,
 "temp": 18.0,
 "estop_nc": false,
 "wifi_ssid": "Wireless2",
 "wifi_pass_set": true
}
```

### Konfiguration am Ende

```json
{
 "measure_pos": 155.0,
 "maint_pos": 40.0,
 "traverse_start": 80.0,
 "traverse_end": 240.0,
 "traverse_speed": 10.0,
 "shutter_hold": 5.0,
 "edge_threshold": 3.0,
 "base_dist": 70.0,
 "calib_raw_1": 76.2,
 "calib_ref_1": 65.0,
 "calib_raw_2": 69.2,
 "calib_ref_2": 60.0,
 "calib_raw_3": 61.7,
 "calib_ref_3": 55.0,
 "calib_raw_4": 55.4,
 "calib_ref_4": 50.0,
 "calib_raw_5": 48.2,
 "calib_ref_5": 45.0,
 "calib_raw_6": 42.3,
 "calib_ref_6": 40.0,
 "calib_valid": true,
 "interlock": true,
 "cooling_flow": true,
 "temp": 18.0,
 "estop_nc": false,
 "wifi_ssid": "Wireless2",
 "wifi_pass_set": true
}
```

### GET_STATE am Ende

```json
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 1955, "period_max_us": 2412, "overruns": 460, "prio_lat_max_us": 1406, "reset_reason": "POWERON", "boots": 1, "brownouts": 0, "bod_level": 7, "crc": "E1C4AB3A"}
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
```

### T18 – Einzelwerte Stufe 115 mm (Soll Abstand 65.0, Dicke 5.0)

```
Abstand [mm]: 65.3 65.1 65.1 65.0 65.1 64.9 65.0 65.3 65.0 65.1 65.2 65.2 65.2 65.2 65.5 65.4 65.5 65.3 65.0 65.2 65.3 65.4 65.2 65.3 65.4 65.5 65.3 65.3 65.2 65.0 65.2 65.1 65.0 65.1 65.3 65.1 65.2 65.1 65.3 65.3
Dicke [mm]:   4.7 4.9 4.9 5.0 4.9 5.1 5.0 4.7 5.0 4.9 4.8 4.8 4.8 4.8 4.5 4.6 4.5 4.7 5.0 4.8 4.7 4.6 4.8 4.7 4.6 4.5 4.7 4.7 4.8 5.0 4.8 4.9 5.0 4.9 4.7 4.9 4.8 4.9 4.7 4.7
```

### T18 – Einzelwerte Stufe 135 mm (Soll Abstand 60.0, Dicke 10.0)

```
Abstand [mm]: 60.1 60.1 60.1 60.1 60.0 59.9 59.9 59.9 60.1 60.0 59.8 59.9 59.9 59.9 60.0 60.0 59.9 60.0 60.0 59.9 60.2 60.0 59.9 59.8 59.8 59.9 59.9 59.9 60.0 59.9 59.8 59.9 59.9 59.9 60.0 59.9 59.9 59.9 60.0 60.1
Dicke [mm]:   9.9 9.9 9.9 9.9 10.0 10.1 10.1 10.1 9.9 10.0 10.2 10.1 10.1 10.1 10.0 10.0 10.1 10.0 10.0 10.1 9.8 10.0 10.1 10.2 10.2 10.1 10.1 10.1 10.0 10.1 10.2 10.1 10.1 10.1 10.0 10.1 10.1 10.1 10.0 9.9
```

### T18 – Einzelwerte Stufe 155 mm (Soll Abstand 55.0, Dicke 15.0)

```
Abstand [mm]: 54.9 54.8 54.8 54.8 54.8 54.8 54.9 54.8 54.9 55.0 54.8 54.9 54.8 54.8 54.8 54.9 54.9 54.8 54.9 54.9 54.9 54.8 54.7 54.9 54.7 54.8 54.7 54.8 54.8 54.9 54.9 54.8 54.8 54.8 54.8 54.7 54.7 54.9 54.8 54.9
Dicke [mm]:   15.1 15.2 15.2 15.2 15.2 15.2 15.1 15.2 15.1 15.0 15.2 15.1 15.2 15.2 15.2 15.1 15.1 15.2 15.1 15.1 15.1 15.2 15.3 15.1 15.3 15.2 15.3 15.2 15.2 15.1 15.1 15.2 15.2 15.2 15.2 15.3 15.3 15.1 15.2 15.1
```

### T18 – Einzelwerte Stufe 175 mm (Soll Abstand 50.0, Dicke 20.0)

```
Abstand [mm]: 50.0 50.0 50.0 50.0 50.0 50.0 50.1 50.0 50.0 50.0 50.0 49.9 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.1 50.0 50.0 50.0 50.0 50.0 50.0 50.0 49.9 50.0 49.9 50.0
Dicke [mm]:   20.0 20.0 20.0 20.0 20.0 20.0 19.9 20.0 20.0 20.0 20.0 20.1 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 19.9 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.1 20.0 20.1 20.0
```

### T18 – Einzelwerte Stufe 195 mm (Soll Abstand 45.0, Dicke 25.0)

```
Abstand [mm]: 45.0 45.2 45.1 45.0 45.1 45.0 45.0 45.0 45.0 45.0 45.1 45.0 45.1 45.0 45.0 45.0 45.0 44.9 45.0 45.1 45.0 45.0 45.0 45.0 45.0 45.0 45.1 45.1 45.0 45.0 44.9 45.0 45.0 45.1 45.1 45.1 45.1 45.0 45.0 45.0
Dicke [mm]:   25.0 24.8 24.9 25.0 24.9 25.0 25.0 25.0 25.0 25.0 24.9 25.0 24.9 25.0 25.0 25.0 25.0 25.1 25.0 24.9 25.0 25.0 25.0 25.0 25.0 25.0 24.9 24.9 25.0 25.0 25.1 25.0 25.0 24.9 24.9 24.9 24.9 25.0 25.0 25.0
```

### T18 – Einzelwerte Stufe 215 mm (Soll Abstand 40.0, Dicke 30.0)

```
Abstand [mm]: 40.1 40.0 40.1 40.0 40.1 40.0 40.0 40.0 40.0 40.1 40.0 40.1 40.0 40.1 40.1 40.0 40.0 39.9 40.1 39.9 40.0 40.0 40.0 40.0 40.0 40.1 40.0 40.0 40.0 40.0 40.0 40.0 40.0 40.0 39.9 40.0 40.0 40.0 40.0 40.0
Dicke [mm]:   29.9 30.0 29.9 30.0 29.9 30.0 30.0 30.0 30.0 29.9 30.0 29.9 30.0 29.9 29.9 30.0 30.0 30.1 29.9 30.1 30.0 30.0 30.0 30.0 30.0 29.9 30.0 30.0 30.0 30.0 30.0 30.0 30.0 30.0 30.1 30.0 30.0 30.0 30.0 30.0
```

