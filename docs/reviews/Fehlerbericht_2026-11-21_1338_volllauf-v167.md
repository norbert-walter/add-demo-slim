# Abnahme A1 – Fehlerbericht

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 21.11.2026 13:38 / 14:09 |
| Gerät | http://192.168.1.127/ |
| Firmware | boot: ready, FW 1.0.0 |
| Repository | 1ec39b3 Sensorkennlinie 124.5*V^-1.144 nach dem LED-Umbau (Contract V1.6.7) |
| Aufruf | `--host 192.168.1.127 --tag volllauf-v167` |
| Ergebnis gesamt | **36 ✅ bestanden (davon 1 mit ⚠️ Achtung) · 3 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 7 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 4:06 min |
| Ergebnis Block 2 – weitere Handprüfungen | 6 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 9:53 min |
| Ergebnis Block 3 – automatische Prüfungen | 23 ✅ bestanden (davon 1 mit ⚠️ Achtung) · 2 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 16:47 min |

### Symbole

| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |

## Block 2 – weitere Handprüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| M10 | 6 | Display: 7 Tasten wirken, Anzeige korrekt, Positionsbalken synchron | ❌ **nicht bestanden** | Taste HV: HV ein: nach 20 s nicht erreicht (READY, pos 0.0, idle/none, HV False, Shutter closed) | 30 s |

## Block 3 – automatische Prüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| T01 | 5 | Alle Endpunkte antworten in < 1 s | ❌ **nicht bestanden** | langsamer als 1 s: ['/settings'] | 4 s |
| T13 | 4 | Hin- und Rückfahrt innerhalb 3 s: Shutter bleibt offen; 3 s nach Ende zu; STOP schließt sofort | ❌ **nicht bestanden** | Shutter nach 5.0 s zu | 32 s |

## Messwerte

Nur die Werte außerhalb des Solls.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ❌ | T01 | GET /settings (HTTP 200) | 1053.189 | ms | ≤ 1000 |
| ❌ | T13 | Shutter zu nach Laufende | 4.992 | s | 3 (2,5 … 4,0) |

## Anhang

### GET_STATE am Ende

```json
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2509, "period_max_us": 2557, "overruns": 2719, "prio_lat_max_us": 1492, "reset_reason": "SW", "boots": 3, "brownouts": 0, "bod_level": 7, "crc": "C383D594"}
```

### /log am Ende

```
net: http 704 ms
net: http 262 ms
net: http 208 ms
net: http 729 ms
net: http 231 ms
net: http 205 ms
net: http 205 ms
net: http 678 ms
net: http 261 ms
net: http 205 ms
net: http 731 ms
net: http 217 ms
net: http 202 ms
net: http 205 ms
net: http 684 ms
net: http 259 ms
net: http 216 ms
net: http 733 ms
net: http 246 ms
net: http 208 ms
```

