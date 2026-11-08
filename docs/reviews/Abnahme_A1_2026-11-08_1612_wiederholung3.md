# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 16:12 / 16:15 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | 2c7d692 A1: Protokolle und Fehlerberichte 08.11.2026 |
| Aufruf | `--host 192.168.1.120 --only T18K,T18,T19,T26 --tag wiederholung3` |
| Ergebnis gesamt | **4 ✅ bestanden (davon 2 mit ⚠️ Achtung) · 2 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | **nicht ausgeführt – Norbert Walter bestätigt am 08.11.2026 16:12 wahrheitsgemäß und auf eigene Verantwortung, dass der Sicherheitsblock bestanden war (Nachweis: Abnahme_A1_2026-11-08_1519_wiederholung.md)** |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt (Aufruf) |
| Ergebnis Block 3 – automatische Prüfungen | 4 ✅ bestanden (davon 2 mit ⚠️ Achtung) · 2 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 3:32 min |

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
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ✅ bestanden | 6 Punkte mit HV an und offenem Shutter übernommen und gespeichert – Shutter 5× geschlossen (Firmware vor V1.6.4) | 32 s |
| T18 | 2, 8 | Stehende Messung je Stufe: Mittelwert ±1 mm, Einzelwerte 3σ ≤ 2 mm | ✅ ⚠️ bestanden, Achtung | alle Stufen in Toleranz (Einzelwerte im Anhang) – Shutter 6× geschlossen (Firmware vor V1.6.4) – Achtung nah am Grenzwert: 115 mm: Abstand Mittelwert, 115 mm: Dicke Mittelwert | 55 s |
| T19 | 8 | Profilfahrt hin und zurück: Kanten erkannt und aufgezeichnet, Stufen ±1 mm, Shutter durchgehend offen | ❌ **nicht bestanden** | hin 115 mm; zurück 115 mm | 78 s |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort | ❌ **nicht bestanden** | stress_api parallel:     raise URLError(err) / urllib.error.URLError: <urlopen error timed out> | 12 s |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 4 min | – |
| T29 | 4 | Zeitwerte ≤ 3100 µs (inkl. Summer-Signale aus Block 1 und 2) | ✅ ⚠️ bestanden, Achtung | seit dem letzten Neustart | – |

## Messwerte

Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| – | T18K | Punkt 1 (115 mm): Versuche bis stabil | 4 |  |  |
| – | T18K | Punkt 2 (135 mm): Versuche bis stabil | 3 |  |  |
| – | T18K | Punkt 3 (155 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 4 (175 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 5 (195 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 6 (215 mm): Versuche bis stabil | 1 |  |  |
| ✅ | T18K | Punkt 1 (115 mm): Rohwert (unkalibriert) gegen Referenz 65 | 60.2 | mm | 65 ± 7 |
| – | T18K | Punkt 1 (115 mm): Änderung gegenüber der alten Tabelle | 0.7 | mm |  |
| ✅ | T18K | Punkt 2 (135 mm): Rohwert (unkalibriert) gegen Referenz 60 | 55.3 | mm | 60 ± 7 |
| – | T18K | Punkt 2 (135 mm): Änderung gegenüber der alten Tabelle | 0.3 | mm |  |
| ✅ | T18K | Punkt 3 (155 mm): Rohwert (unkalibriert) gegen Referenz 55 | 50.8 | mm | 55 ± 7 |
| – | T18K | Punkt 3 (155 mm): Änderung gegenüber der alten Tabelle | 0.7 | mm |  |
| ✅ | T18K | Punkt 4 (175 mm): Rohwert (unkalibriert) gegen Referenz 50 | 46.1 | mm | 50 ± 7 |
| – | T18K | Punkt 4 (175 mm): Änderung gegenüber der alten Tabelle | 0.1 | mm |  |
| ✅ | T18K | Punkt 5 (195 mm): Rohwert (unkalibriert) gegen Referenz 45 | 42.0 | mm | 45 ± 7 |
| – | T18K | Punkt 5 (195 mm): Änderung gegenüber der alten Tabelle | 0.2 | mm |  |
| ✅ | T18K | Punkt 6 (215 mm): Rohwert (unkalibriert) gegen Referenz 40 | 38.6 | mm | 40 ± 7 |
| – | T18K | Punkt 6 (215 mm): Änderung gegenüber der alten Tabelle | 0.4 | mm |  |
| ✅ | T18K | calib_valid | True |  | True |
| – | T18K | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 5 |  |  |
| ✅ ⚠️ | T18 | 115 mm: Abstand Mittelwert | 64.09 | mm | 65 ± 1 |
| ✅ ⚠️ | T18 | 115 mm: Dicke Mittelwert | 5.91 | mm | 5 ± 1 |
| ✅ | T18 | 115 mm: Dicke Mittelwert der Anlage (avg) | 5.8 | mm | 5 ± 1 |
| ✅ | T18 | 115 mm: Dicke Sigma | 0.374 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 115 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 115 mm: Dicke Min / Max | 5.3 / 6.6 | mm |  |
| – | T18 | 115 mm: Abstand Sigma | 0.374 | mm |  |
| ✅ | T18 | 135 mm: Abstand Mittelwert | 59.875 | mm | 60 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert | 10.125 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert der Anlage (avg) | 10.0 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Sigma | 0.283 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 135 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 135 mm: Dicke Min / Max | 9.7 / 10.8 | mm |  |
| – | T18 | 135 mm: Abstand Sigma | 0.283 | mm |  |
| ✅ | T18 | 155 mm: Abstand Mittelwert | 54.673 | mm | 55 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert | 15.328 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert der Anlage (avg) | 15.3 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Sigma | 0.181 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 155 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 155 mm: Dicke Min / Max | 15.0 / 15.6 | mm |  |
| – | T18 | 155 mm: Abstand Sigma | 0.181 | mm |  |
| ✅ | T18 | 175 mm: Abstand Mittelwert | 49.93 | mm | 50 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert | 20.07 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert der Anlage (avg) | 20.0 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Sigma | 0.104 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 175 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 175 mm: Dicke Min / Max | 19.9 / 20.2 | mm |  |
| – | T18 | 175 mm: Abstand Sigma | 0.104 | mm |  |
| ✅ | T18 | 195 mm: Abstand Mittelwert | 44.983 | mm | 45 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert | 25.018 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert der Anlage (avg) | 25.0 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Sigma | 0.122 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 195 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 195 mm: Dicke Min / Max | 24.7 / 25.2 | mm |  |
| – | T18 | 195 mm: Abstand Sigma | 0.122 | mm |  |
| ✅ | T18 | 215 mm: Abstand Mittelwert | 39.8 | mm | 40 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert | 30.2 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert der Anlage (avg) | 30.2 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Sigma | 0.109 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 215 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 215 mm: Dicke Min / Max | 30.0 / 30.4 | mm |  |
| – | T18 | 215 mm: Abstand Sigma | 0.109 | mm |  |
| – | T18 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 6 |  |  |
| ✅ | T19 | hin: Messwerte im Lauf | 365 |  | ≥ 201 |
| – | T19 | hin: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 1 |  |  |
| ✅ | T19 | hin: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 108.2 | mm | 105 ± 10 |
| ✅ | T19 | hin: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 225.3 | mm | 225 ± 10 |
| ✅ | T19 | hin: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| ❌ | T19 | hin: Stufe 115 mm: Dicke (Median) | 6.1 | mm | 5 ± 1 |
| – | T19 | hin: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 135 mm: Dicke (Median) | 10.1 | mm | 10 ± 1 |
| – | T19 | hin: Stufe 135 mm: Anzahl Werte | 24 |  |  |
| ✅ | T19 | hin: Stufe 155 mm: Dicke (Median) | 15.3 | mm | 15 ± 1 |
| – | T19 | hin: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 175 mm: Dicke (Median) | 20.0 | mm | 20 ± 1 |
| – | T19 | hin: Stufe 175 mm: Anzahl Werte | 24 |  |  |
| ✅ | T19 | hin: Stufe 195 mm: Dicke (Median) | 25.0 | mm | 25 ± 1 |
| – | T19 | hin: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 215 mm: Dicke (Median) | 30.3 | mm | 30 ± 1 |
| – | T19 | hin: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Mittelwert der Anlage | 17.9 | mm | 5 … 30 |
| ✅ | T19 | hin: Anzahl Werte der Anlage | 1174 |  | ≥ 51 |
| ✅ | T19 | zurück: Messwerte im Lauf | 365 |  | ≥ 201 |
| – | T19 | zurück: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| ✅ | T19 | zurück: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 106.8 | mm | 105 ± 10 |
| ✅ | T19 | zurück: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 223.9 | mm | 225 ± 10 |
| ✅ | T19 | zurück: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| ❌ | T19 | zurück: Stufe 115 mm: Dicke (Median) | 6.1 | mm | 5 ± 1 |
| – | T19 | zurück: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 135 mm: Dicke (Median) | 10.2 | mm | 10 ± 1 |
| – | T19 | zurück: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 155 mm: Dicke (Median) | 15.4 | mm | 15 ± 1 |
| – | T19 | zurück: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 175 mm: Dicke (Median) | 20.0 | mm | 20 ± 1 |
| – | T19 | zurück: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 195 mm: Dicke (Median) | 25.0 | mm | 25 ± 1 |
| – | T19 | zurück: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 215 mm: Dicke (Median) | 30.3 | mm | 30 ± 1 |
| – | T19 | zurück: Stufe 215 mm: Anzahl Werte | 24 |  |  |
| ✅ | T19 | zurück: Mittelwert der Anlage | 17.9 | mm | 5 … 30 |
| ✅ | T19 | zurück: Anzahl Werte der Anlage | 1175 |  | ≥ 51 |
| – | T19 | Kante Messgut ab: Mittel hin/zurück (Info) | 107.5 | mm |  |
| – | T19 | Kante Messgut ab: Unterschied hin – zurück (Info) | 1.4 | mm |  |
| – | T19 | Kante Messgut bis: Mittel hin/zurück (Info) | 224.6 | mm |  |
| – | T19 | Kante Messgut bis: Unterschied hin – zurück (Info) | 1.4 | mm |  |
| – | T26 | parallel | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T26 | parallel | slowest /api round trip: 7391.2 ms (SHUTTER_CLOSE) |  |  |
| – | T28 | Laufzeit | 3.667 | min |  |
| ✅ | T28 | unerwartete Neustarts | 0 |  | 0 |
| ✅ ⚠️ | T29 | cycle_max_us | 2882 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | period_max_us | 3008 | µs | ≤ 3100 |
| ✅ | T29 | prio_lat_max_us | 1744 | µs | ≤ 3100 |
| – | T29 | overruns (Info) | 3872 |  |  |

## Anhang

### Konfiguration zu Beginn (GET_CONFIG)

```json
{
 "measure_pos": 155.0,
 "maint_pos": 40.0,
 "traverse_start": 80.0,
 "traverse_end": 240.0,
 "traverse_speed": 10.0,
 "shutter_hold": 3.0,
 "edge_threshold": 3.0,
 "base_dist": 70.0,
 "calib_raw_1": 59.5,
 "calib_ref_1": 65.0,
 "calib_raw_2": 55.0,
 "calib_ref_2": 60.0,
 "calib_raw_3": 50.1,
 "calib_ref_3": 55.0,
 "calib_raw_4": 46.0,
 "calib_ref_4": 50.0,
 "calib_raw_5": 41.8,
 "calib_ref_5": 45.0,
 "calib_raw_6": 38.2,
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
 "shutter_hold": 3.0,
 "edge_threshold": 3.0,
 "base_dist": 70.0,
 "calib_raw_1": 60.2,
 "calib_ref_1": 65.0,
 "calib_raw_2": 55.3,
 "calib_ref_2": 60.0,
 "calib_raw_3": 50.8,
 "calib_ref_3": 55.0,
 "calib_raw_4": 46.1,
 "calib_ref_4": 50.0,
 "calib_raw_5": 42.0,
 "calib_ref_5": 45.0,
 "calib_raw_6": 38.6,
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
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2882, "period_max_us": 3008, "overruns": 3872, "prio_lat_max_us": 1744, "crc": "47141E3A"}
```

### /log am Ende

```
net: http 261 ms
net: http 729 ms
net: http 232 ms
net: http 206 ms
net: http 605 ms
nvs: save deferred until axis idle and HV off
comm: /api gap 4146 ms
comm: /api gap 2070 ms
comm: /api gap 2093 ms
comm: timeout, last /api packet 5002 ms ago, longest gap while armed 4146 ms
nvs: saved
http: idle connection closed after 500 ms (1 so far)
ctrl: cycle 464 us, period 2514 us (adc 151, tick 211, cmd 20, out 9, pub 73), 1 events
ctrl: cycle 529 us, period 2654 us (adc 202, tick 221, cmd 21, out 9, pub 76), 1 events
http: idle connection closed after 500 ms (2 so far)
http: idle connection closed after 500 ms (4 so far)
ctrl: cycle 514 us, period 2679 us (adc 180, tick 214, cmd 30, out 9, pub 81), 1 events
nvs: save deferred until axis idle and HV off
ctrl: cycle 420 us, period 2546 us (adc 158, tick 158, cmd 18, out 9, pub 77), 1 events
nvs: saved
```

### T18 – Einzelwerte Stufe 115 mm (Soll Abstand 65.0, Dicke 5.0)

```
Abstand [mm]: 64.5 63.8 64.0 64.1 63.5 63.8 63.5 64.0 63.4 63.8 64.0 64.4 64.5 64.2 64.5 64.2 64.5 64.2 64.4 64.6 64.3 64.4 63.7 63.9 64.1 63.4 64.0 63.8 64.1 63.7 63.8 63.4 63.9 64.4 64.2 64.6 64.3 64.7 64.7 64.3
Dicke [mm]:   5.5 6.2 6.0 5.9 6.5 6.2 6.5 6.0 6.6 6.2 6.0 5.6 5.5 5.8 5.5 5.8 5.5 5.8 5.6 5.4 5.7 5.6 6.3 6.1 5.9 6.6 6.0 6.2 5.9 6.3 6.2 6.6 6.1 5.6 5.8 5.4 5.7 5.3 5.3 5.7
```

### T18 – Einzelwerte Stufe 135 mm (Soll Abstand 60.0, Dicke 10.0)

```
Abstand [mm]: 60.2 59.8 59.9 59.2 59.6 59.7 59.8 59.9 59.8 59.5 59.5 59.9 59.9 60.2 60.3 59.2 59.9 60.1 60.2 60.1 60.3 60.3 60.2 60.1 59.7 59.9 59.9 59.5 59.8 59.5 59.8 59.6 59.8 59.8 59.7 60.1 60.0 60.2 60.1 60.0
Dicke [mm]:   9.8 10.2 10.1 10.8 10.4 10.3 10.2 10.1 10.2 10.5 10.5 10.1 10.1 9.8 9.7 10.8 10.1 9.9 9.8 9.9 9.7 9.7 9.8 9.9 10.3 10.1 10.1 10.5 10.2 10.5 10.2 10.4 10.2 10.2 10.3 9.9 10.0 9.8 9.9 10.0
```

### T18 – Einzelwerte Stufe 155 mm (Soll Abstand 55.0, Dicke 15.0)

```
Abstand [mm]: 55.0 55.0 54.5 54.6 54.5 54.5 54.7 54.4 54.6 54.5 54.6 54.6 54.7 54.6 54.6 54.8 54.6 54.8 54.8 54.9 54.8 54.9 54.9 55.0 54.9 54.8 54.4 54.6 54.6 54.4 54.4 54.6 54.7 54.5 54.6 54.4 54.6 54.8 54.8 54.9
Dicke [mm]:   15.0 15.0 15.5 15.4 15.5 15.5 15.3 15.6 15.4 15.5 15.4 15.4 15.3 15.4 15.4 15.2 15.4 15.2 15.2 15.1 15.2 15.1 15.1 15.0 15.1 15.2 15.6 15.4 15.4 15.6 15.6 15.4 15.3 15.5 15.4 15.6 15.4 15.2 15.2 15.1
```

### T18 – Einzelwerte Stufe 175 mm (Soll Abstand 50.0, Dicke 20.0)

```
Abstand [mm]: 50.1 50.1 50.0 49.9 49.8 49.9 49.9 49.9 49.9 49.9 49.9 49.9 49.9 49.9 49.9 49.8 50.0 50.1 50.0 50.1 50.1 50.1 50.1 50.0 49.8 50.0 50.1 49.9 49.9 49.9 49.9 49.9 49.9 49.8 49.8 49.8 49.8 49.8 49.9 49.8
Dicke [mm]:   19.9 19.9 20.0 20.1 20.2 20.1 20.1 20.1 20.1 20.1 20.1 20.1 20.1 20.1 20.1 20.2 20.0 19.9 20.0 19.9 19.9 19.9 19.9 20.0 20.2 20.0 19.9 20.1 20.1 20.1 20.1 20.1 20.1 20.2 20.2 20.2 20.2 20.2 20.1 20.2
```

### T18 – Einzelwerte Stufe 195 mm (Soll Abstand 45.0, Dicke 25.0)

```
Abstand [mm]: 45.3 45.1 45.0 45.0 45.1 45.1 45.0 45.0 44.9 44.9 44.9 44.9 44.8 44.9 44.8 44.8 44.8 44.9 45.1 45.1 45.1 45.1 45.1 45.1 45.1 45.2 45.0 45.1 45.1 45.0 45.0 44.9 44.9 44.9 44.9 44.9 44.9 44.9 44.8 44.9
Dicke [mm]:   24.7 24.9 25.0 25.0 24.9 24.9 25.0 25.0 25.1 25.1 25.1 25.1 25.2 25.1 25.2 25.2 25.2 25.1 24.9 24.9 24.9 24.9 24.9 24.9 24.9 24.8 25.0 24.9 24.9 25.0 25.0 25.1 25.1 25.1 25.1 25.1 25.1 25.1 25.2 25.1
```

### T18 – Einzelwerte Stufe 215 mm (Soll Abstand 40.0, Dicke 30.0)

```
Abstand [mm]: 40.0 40.0 39.9 39.8 39.8 39.9 39.9 39.9 39.9 39.8 39.7 39.7 39.7 39.7 39.7 39.6 39.7 39.7 39.7 39.7 39.8 39.9 39.9 39.9 39.9 39.9 39.9 39.8 39.9 39.9 39.9 39.9 39.8 39.7 39.7 39.7 39.7 39.7 39.7 39.6
Dicke [mm]:   30.0 30.0 30.1 30.2 30.2 30.1 30.1 30.1 30.1 30.2 30.3 30.3 30.3 30.3 30.3 30.4 30.3 30.3 30.3 30.3 30.2 30.1 30.1 30.1 30.1 30.1 30.1 30.2 30.1 30.1 30.1 30.1 30.2 30.3 30.3 30.3 30.3 30.3 30.3 30.4
```

### T26 – stress_api.py parallel --n 400 --threads 8

```
16:15:30 192.168.1.120: parallel: 400 mixed commands from 8 threads, every reply checked (about 10 ... 30 s)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  slowest /api round trip: 7391.2 ms (SHUTTER_CLOSE)
  device log (ctrl/net/comm/http/sta):
    net: http 261 ms
    net: http 729 ms
    net: http 232 ms
    net: http 206 ms
    net: http 605 ms
    comm: /api gap 4146 ms
    comm: /api gap 2070 ms
    comm: /api gap 2093 ms
    comm: timeout, last /api packet 5002 ms ago, longest gap while armed 4146 ms
    http: idle connection closed after 500 ms (1 so far)
    ctrl: cycle 464 us, period 2514 us (adc 151, tick 211, cmd 20, out 9, pub 73), 1 events
    ctrl: cycle 529 us, period 2654 us (adc 202, tick 221, cmd 21, out 9, pub 76), 1 events
    http: idle connection closed after 500 ms (2 so far)
    http: idle connection closed after 500 ms (4 so far)
    ctrl: cycle 514 us, period 2679 us (adc 180, tick 214, cmd 30, out 9, pub 81), 1 events
    ctrl: cycle 420 us, period 2546 us (adc 158, tick 158, cmd 18, out 9, pub 77), 1 events
Traceback (most recent call last):
  File "/usr/lib/python3.10/urllib/request.py", line 1348, in do_open
    h.request(req.get_method(), req.selector, req.data, headers,
  File "/usr/lib/python3.10/http/client.py", line 1312, in request
    self._send_request(method, url, body, headers, encode_chunked)
  File "/usr/lib/python3.10/http/client.py", line 1358, in _send_request
    self.endheaders(body, encode_chunked=encode_chunked)
  File "/usr/lib/python3.10/http/client.py", line 1307, in endheaders
    self._send_output(message_body, encode_chunked=encode_chunked)
  File "/usr/lib/python3.10/http/client.py", line 1067, in _send_output
    self.send(msg)
  File "/usr/lib/python3.10/http/client.py", line 1005, in send
    self.connect()
  File "/usr/lib/python3.10/http/client.py", line 971, in connect
    self.sock = self._create_connection(
  File "/usr/lib/python3.10/socket.py", line 845, in create_connection
    raise err
  File "/usr/lib/python3.10/socket.py", line 833, in create_connection
    sock.connect(sa)
TimeoutError: timed out

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 390, in <module>
    main()
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 378, in main
    ok = t_parallel(a.host, a.n or 400, a.threads)
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 243, in t_parallel
    for cmd, exp, r, dt in ex.map(one, jobs):
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 621, in result_iterator
    yield _result_or_cancel(fs.pop())
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 319, in _result_or_cancel
    return fut.result(timeout)
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 458, in result
    return self.__get_result()
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 403, in __get_result
    raise self._exception
  File "/usr/lib/python3.10/concurrent/futures/thread.py", line 58, in run
    result = self.fn(*self.args, **self.kwargs)
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 238, in one
    r, dt = api(host, cmd, params, timeout=10.0)
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 68, in api
    txt = http(host, "/api", build(cmd, params or {}), timeout=timeout)
  File "/home/norbert/add-demo-slim/scripts/add_client.py", line 66, in http
    with urllib.request.urlopen(req, timeout=timeout) as r:
  File "/usr/lib/python3.10/urllib/request.py", line 216, in urlopen
    return opener.open(url, data, timeout)
  File "/usr/lib/python3.10/urllib/request.py", line 519, in open
    response = self._open(req, data)
  File "/usr/lib/python3.10/urllib/request.py", line 536, in _open
    result = self._call_chain(self.handle_open, protocol, protocol +
  File "/usr/lib/python3.10/urllib/request.py", line 496, in _call_chain
    result = func(*args)
  File "/usr/lib/python3.10/urllib/request.py", line 1377, in http_open
    return self.do_open(http.client.HTTPConnection, req)
  File "/usr/lib/python3.10/urllib/request.py", line 1351, in do_open
    raise URLError(err)
urllib.error.URLError: <urlopen error timed out>
```

