# Abnahme A1 – Fehlerbericht

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 13:49 / 13:51 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | f17c52d A1.md: Prüfungsnummern S/M nachgezogen |
| Aufruf | `--host 192.168.1.120 --tag voll2` |
| Ergebnis gesamt | **0 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 0 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 2:12 min |
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

Nur die Werte außerhalb des Solls.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| – | – | keine Messwerte außerhalb des Solls (Fehler siehe Bemerkung oben) |  |  |  |

## Anhang

### GET_STATE am Ende

```json
{}
```

### /log am Ende

```
net: http 204 ms
net: http 720 ms
net: http 252 ms
net: http 209 ms
net: http 711 ms
net: http 250 ms
net: http 208 ms
net: http 711 ms
net: http 250 ms
net: http 202 ms
net: http 694 ms
net: http 718 ms
comm: /api gap 2147 ms
comm: /api gap 2001 ms
ctrl: cycle 529 us, period 2603 us (adc 214, tick 197, cmd 20, out 9, pub 89), 1 events
ctrl: cycle 503 us, period 2631 us (adc 179, tick 201, cmd 20, out 9, pub 94), 1 events
nvs: save deferred until axis idle and HV off
comm: timeout, last /api packet 5000 ms ago, longest gap while armed 1582 ms
nvs: saved
ctrl: cycle 272 us, period 49767 us (adc 111, tick 104, cmd 14, out 6, pub 37), 1 events
```

