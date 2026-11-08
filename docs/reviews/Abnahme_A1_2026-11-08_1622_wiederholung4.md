# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 16:22 / 16:24 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | 2e7f53e Merge branch 'master' of /home/norbert/add-demo-slim/tmp/a1-t19t26.bundle |
| Aufruf | `--host 192.168.1.120 --only T19,T26 --tag wiederholung4` |
| Ergebnis gesamt | **3 ✅ bestanden (davon 1 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | **nicht ausgeführt – Norbert Walter bestätigt am 08.11.2026 16:22 wahrheitsgemäß und auf eigene Verantwortung, dass der Sicherheitsblock bestanden war (Nachweis: Abnahme_A1_2026-11-08_1519_wiederholung.md)** |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt (Aufruf) |
| Ergebnis Block 3 – automatische Prüfungen | 3 ✅ bestanden (davon 1 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 1:49 min |

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
| T19 | 8 | Profilfahrt hin und zurück: Kanten erkannt und aufgezeichnet, Stufen ±2 mm (in Fahrt), Shutter durchgehend offen | ❌ **nicht bestanden** | zurück: Kante Messgut ab nicht erkannt (80.1); zurück 115 mm; zurück 135 mm | 73 s |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte oder verlorene Antwort | ✅ bestanden | RESULT: PASS (Ausgabe im Anhang) | 23 s |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 2 min | – |
| T29 | 4 | Zeitwerte ≤ 3100 µs (inkl. Summer-Signale aus Block 1 und 2) | ✅ ⚠️ bestanden, Achtung | seit dem letzten Neustart | – |

## Messwerte

Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ✅ | T19 | hin: Messwerte im Lauf | 365 |  | ≥ 201 |
| – | T19 | hin: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| ✅ | T19 | hin: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 109.1 | mm | 105 ± 10 |
| ✅ | T19 | hin: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 225.6 | mm | 225 ± 10 |
| ✅ | T19 | hin: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| ✅ | T19 | hin: Stufe 115 mm: Dicke (Median) | 5.7 | mm | 5 ± 2 |
| – | T19 | hin: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 135 mm: Dicke (Median) | 9.9 | mm | 10 ± 2 |
| – | T19 | hin: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 155 mm: Dicke (Median) | 15.5 | mm | 15 ± 2 |
| – | T19 | hin: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 175 mm: Dicke (Median) | 20.0 | mm | 20 ± 2 |
| – | T19 | hin: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 195 mm: Dicke (Median) | 25.0 | mm | 25 ± 2 |
| – | T19 | hin: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 215 mm: Dicke (Median) | 30.3 | mm | 30 ± 2 |
| – | T19 | hin: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Mittelwert der Anlage | 17.9 | mm | 5 … 30 |
| ✅ | T19 | hin: Anzahl Werte der Anlage | 1167 |  | ≥ 51 |
| ✅ | T19 | zurück: Messwerte im Lauf | 364 |  | ≥ 201 |
| – | T19 | zurück: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| ❌ | T19 | zurück: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 80.1 | mm | 105 ± 10 |
| ✅ | T19 | zurück: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 224.3 | mm | 225 ± 10 |
| ✅ | T19 | zurück: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| ❌ | T19 | zurück: Stufe 115 mm: Dicke (Median) | 8.9 | mm | 5 ± 2 |
| – | T19 | zurück: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| ❌ | T19 | zurück: Stufe 135 mm: Dicke (Median) | 12.7 | mm | 10 ± 2 |
| – | T19 | zurück: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 155 mm: Dicke (Median) | 15.5 | mm | 15 ± 2 |
| – | T19 | zurück: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 175 mm: Dicke (Median) | 20.1 | mm | 20 ± 2 |
| – | T19 | zurück: Stufe 175 mm: Anzahl Werte | 22 |  |  |
| ✅ | T19 | zurück: Stufe 195 mm: Dicke (Median) | 25.0 | mm | 25 ± 2 |
| – | T19 | zurück: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 215 mm: Dicke (Median) | 30.3 | mm | 30 ± 2 |
| – | T19 | zurück: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Mittelwert der Anlage | 16.7 | mm | 5 … 30 |
| ✅ | T19 | zurück: Anzahl Werte der Anlage | 1435 |  | ≥ 51 |
| – | T19 | Kante Messgut ab: Mittel hin/zurück (Info) | 94.6 | mm |  |
| – | T19 | Kante Messgut ab: Unterschied hin – zurück (Info) | 29.0 | mm |  |
| – | T19 | Kante Messgut bis: Mittel hin/zurück (Info) | 224.95 | mm |  |
| – | T19 | Kante Messgut bis: Unterschied hin – zurück (Info) | 1.3 | mm |  |
| – | T26 | parallel | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T26 | parallel | cycles over 1 ms +17 (info; since boot ready 4035) cycle_max_us=2882 period_max_us=3008 prio_lat_max_us=2503 -> OK |  |  |
| – | T26 | parallel | slowest /api round trip: 7477.7 ms (ACK) |  |  |
| – | T26 | parallel | RESULT: PASS |  |  |
| – | T28 | Laufzeit | 1.95 | min |  |
| ✅ | T28 | unerwartete Neustarts | 0 |  | 0 |
| ✅ ⚠️ | T29 | cycle_max_us | 2882 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | period_max_us | 3008 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | prio_lat_max_us | 2503 | µs | ≤ 3100 |
| – | T29 | overruns (Info) | 4043 |  |  |

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
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2882, "period_max_us": 3008, "overruns": 4043, "prio_lat_max_us": 2503, "crc": "73A2CA55"}
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

### T26 – stress_api.py parallel --n 400 --threads 8

```
16:23:55 192.168.1.120: parallel: 400 mixed commands from 8 threads, every reply checked (about 10 ... 30 s)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  400 requests, 8 threads: 0 mismatches, 0 BUSY, 0 lost, 5 connection timeouts retried (info), worst 7477.7 ms
  cycles over 1 ms +17 (info; since boot ready 4035) cycle_max_us=2882 period_max_us=3008 prio_lat_max_us=2503 -> OK
  slowest /api round trip: 7477.7 ms (ACK)
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
RESULT: PASS
```

