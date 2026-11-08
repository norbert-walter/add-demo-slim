# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 16:32 / 16:34 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | 0e7f4f1 Merge branch 'master' of /home/norbert/add-demo-slim/tmp/a1-t19info.bundle |
| Aufruf | `--host 192.168.1.120 --only T19 --tag wiederholung5` |
| Ergebnis gesamt | **3 ✅ bestanden (davon 1 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | **nicht ausgeführt – Norbert Walter bestätigt am 08.11.2026 16:32 wahrheitsgemäß und auf eigene Verantwortung, dass der Sicherheitsblock bestanden war (Nachweis: Abnahme_A1_2026-11-08_1519_wiederholung.md)** |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt (Aufruf) |
| Ergebnis Block 3 – automatische Prüfungen | 3 ✅ bestanden (davon 1 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 1:23 min |

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
| T19 | 8 | Profilfahrt hin und zurück: Kanten und Stufen aufgezeichnet (ohne Bewertung), Shutter durchgehend offen, Statistik plausibel | ✅ bestanden | Profil hin und zurück aufgezeichnet (Kanten und Stufen im Protokoll, ohne Bewertung) | 72 s |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 2 min | – |
| T29 | 4 | Zeitwerte ≤ 3100 µs (inkl. Summer-Signale aus Block 1 und 2) | ✅ ⚠️ bestanden, Achtung | seit dem letzten Neustart | – |

## Messwerte

Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ✅ | T19 | hin: Messwerte im Lauf | 364 |  | ≥ 201 |
| – | T19 | hin: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| – | T19 | hin: Kante Messgut ab (Info, Stufenkörper 105) | 80.7 | mm |  |
| – | T19 | hin: Kante Messgut bis (Info, Stufenkörper 225) | 239.6 | mm |  |
| ✅ | T19 | hin: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| – | T19 | hin: Stufe 115 mm: Dicke (Median, Info, Soll 5) | 7.8 | mm |  |
| – | T19 | hin: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 135 mm: Dicke (Median, Info, Soll 10) | 11.3 | mm |  |
| – | T19 | hin: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 155 mm: Dicke (Median, Info, Soll 15) | 16.1 | mm |  |
| – | T19 | hin: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 175 mm: Dicke (Median, Info, Soll 20) | 20.6 | mm |  |
| – | T19 | hin: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 195 mm: Dicke (Median, Info, Soll 25) | 25.6 | mm |  |
| – | T19 | hin: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 215 mm: Dicke (Median, Info, Soll 30) | 30.6 | mm |  |
| – | T19 | hin: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Mittelwert der Anlage | 15.6 | mm | 5 … 30 |
| ✅ | T19 | hin: Anzahl Werte der Anlage | 1570 |  | ≥ 51 |
| ✅ | T19 | zurück: Messwerte im Lauf | 363 |  | ≥ 201 |
| – | T19 | zurück: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| – | T19 | zurück: Kante Messgut ab (Info, Stufenkörper 105) | 80.8 | mm |  |
| – | T19 | zurück: Kante Messgut bis (Info, Stufenkörper 225) | 239.5 | mm |  |
| ✅ | T19 | zurück: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| – | T19 | zurück: Stufe 115 mm: Dicke (Median, Info, Soll 5) | 7.9 | mm |  |
| – | T19 | zurück: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 135 mm: Dicke (Median, Info, Soll 10) | 11.5 | mm |  |
| – | T19 | zurück: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 155 mm: Dicke (Median, Info, Soll 15) | 16.3 | mm |  |
| – | T19 | zurück: Stufe 155 mm: Anzahl Werte | 24 |  |  |
| – | T19 | zurück: Stufe 175 mm: Dicke (Median, Info, Soll 20) | 20.5 | mm |  |
| – | T19 | zurück: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 195 mm: Dicke (Median, Info, Soll 25) | 25.6 | mm |  |
| – | T19 | zurück: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 215 mm: Dicke (Median, Info, Soll 30) | 30.7 | mm |  |
| – | T19 | zurück: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Mittelwert der Anlage | 15.6 | mm | 5 … 30 |
| ✅ | T19 | zurück: Anzahl Werte der Anlage | 1566 |  | ≥ 51 |
| – | T19 | Kante Messgut ab: Mittel hin/zurück (Info) | 80.75 | mm |  |
| – | T19 | Kante Messgut ab: Unterschied hin – zurück (Info) | -0.1 | mm |  |
| – | T19 | Kante Messgut bis: Mittel hin/zurück (Info) | 239.55 | mm |  |
| – | T19 | Kante Messgut bis: Unterschied hin – zurück (Info) | 0.1 | mm |  |
| – | T19 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 0 |  |  |
| – | T28 | Laufzeit | 1.533 | min |  |
| ✅ | T28 | unerwartete Neustarts | 0 |  | 0 |
| ✅ ⚠️ | T29 | cycle_max_us | 2882 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | period_max_us | 3008 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | prio_lat_max_us | 2503 | µs | ≤ 3100 |
| – | T29 | overruns (Info) | 4194 |  |  |

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
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2882, "period_max_us": 3008, "overruns": 4194, "prio_lat_max_us": 2503, "crc": "4BDE3049"}
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

