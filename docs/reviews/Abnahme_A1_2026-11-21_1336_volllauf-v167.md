# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 21.11.2026 13:36 / 13:37 |
| Gerät | http://192.168.1.127/ |
| Firmware | boot: ready, FW 1.0.0 |
| Repository | 1ec39b3 Sensorkennlinie 124.5*V^-1.144 nach dem LED-Umbau (Contract V1.6.7) |
| Aufruf | `--host 192.168.1.127 --tag volllauf-v167` |
| Ergebnis gesamt | **0 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 0 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 0:59 min |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt – **gesperrt: Sicherheitsprüfung nicht bestanden (S01)** |
| Ergebnis Block 3 – automatische Prüfungen | nicht ausgeführt – **gesperrt: Sicherheitsprüfung nicht bestanden (S01)** |

### Symbole

| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |

> **Sicherheitsprüfung nicht bestanden (S01).** Alle weiteren Prüfungen wurden nicht ausgeführt. Erst die Fehler beheben, dann die Abnahme neu starten.

## Block 1 – Sicherheit

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| S01 | 2, 4, 6 | Not-Aus im Stillstand: ESTOP, HV aus, Shutter zu, Dauerton, ACK erst nach Lösen am Display (1 s) | ❌ **nicht bestanden** | Messwert außerhalb Soll: Zeit bis Quittierung am Display | 60 s |

## Messwerte

Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ✅ | S01 | Fehler | ESTOP |  | ESTOP |
| ✅ | S01 | HV / Shutter | aus / closed |  | aus / closed |
| ✅ | S01 | ACK per API bei gedrücktem Not-Aus | PRECONDITION |  | PRECONDITION |
| ✅ | S01 | Dauerton des Summers etwa 4 s? | ja |  | ja |
| ✅ | S01 | Display: alle Tasten ausgeblendet, nur die ACK-Fläche, grau solange Not-Aus gedrückt? | ja |  | ja |
| ✅ | S01 | Quittiert der liegende Finger NICHT (ACK-Fläche bleibt)? | ja |  | ja |
| ❌ | S01 | Zeit bis Quittierung am Display | 0.048 | s | ≥ 1 |

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
 "calib_raw_1": 65.3,
 "calib_ref_1": 65.0,
 "calib_raw_2": 60.0,
 "calib_ref_2": 60.0,
 "calib_raw_3": 54.9,
 "calib_ref_3": 55.0,
 "calib_raw_4": 50.1,
 "calib_ref_4": 50.0,
 "calib_raw_5": 44.8,
 "calib_ref_5": 45.0,
 "calib_raw_6": 40.2,
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
 "calib_raw_1": 65.3,
 "calib_ref_1": 65.0,
 "calib_raw_2": 60.0,
 "calib_ref_2": 60.0,
 "calib_raw_3": 54.9,
 "calib_ref_3": 55.0,
 "calib_raw_4": 50.1,
 "calib_ref_4": 50.0,
 "calib_raw_5": 44.8,
 "calib_ref_5": 45.0,
 "calib_raw_6": 40.2,
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
{}
```

### /log am Ende

```
boot: reset POWERON, boots 1, brownouts 0, brownout level 7
ap: ADD-Demo 192.168.4.1
boot: ready, FW 1.0.0
sta: attempt 1/3
ctrl: cycle 386 us, period 2881 us (adc 84, tick 171, cmd 18, out 1, pub 112), 1 events
sta: connected 192.168.1.127
net: http 224 ms
ctrl: cycle 625 us, period 2615 us (adc 198, tick 282, cmd 44, out 1, pub 100), 1 events
```

