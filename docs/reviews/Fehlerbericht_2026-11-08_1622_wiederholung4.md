# Abnahme A1 – Fehlerbericht

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

## Messwerte

Nur die Werte außerhalb des Solls.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ❌ | T19 | zurück: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 80.1 | mm | 105 ± 10 |
| ❌ | T19 | zurück: Stufe 115 mm: Dicke (Median) | 8.9 | mm | 5 ± 2 |
| ❌ | T19 | zurück: Stufe 135 mm: Dicke (Median) | 12.7 | mm | 10 ± 2 |

## Anhang

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

