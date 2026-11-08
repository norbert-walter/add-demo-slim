# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 16:06 / 16:06 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | 5beb790 A1: Protokoll Wiederholung 15:19 |
| Aufruf | `--host 192.168.1.120 --only T18K,T18,T19,T26 --tag wiederholung2` |
| Ergebnis gesamt | **0 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 0 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 0:17 min |
| Ergebnis Block 2 – weitere Handprüfungen | nicht ausgeführt (Aufruf) |
| Ergebnis Block 3 – automatische Prüfungen | nicht ausgeführt (Aufruf) |

### Symbole

| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |

## Block 1 – Sicherheit

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| – | – | Abbruch durch den Prüfer | ❌ **nicht bestanden** | Strg+C | – |

## Messwerte

Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|

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

### GET_STATE am Ende

```json
{}
```

### /log am Ende

```
net: http 207 ms
net: http 731 ms
net: http 238 ms
net: http 706 ms
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
```

