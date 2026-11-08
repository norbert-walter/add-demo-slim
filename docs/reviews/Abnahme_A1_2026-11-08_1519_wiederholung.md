# Abnahme A1 – Prüfprotokoll

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 15:19 / 15:27 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | c22b0b3 Abnahmegrenzen an den optischen Sensor angepasst (Pflichtenheft V1.7.16) |
| Aufruf | `--host 192.168.1.120 --only M06,M11,T18K,T18,T19,T26 --tag wiederholung` |
| Ergebnis gesamt | **12 ✅ bestanden (davon 2 mit ⚠️ Achtung) · 3 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 7 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 3:24 min |
| Ergebnis Block 2 – weitere Handprüfungen | 2 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 1:58 min |
| Ergebnis Block 3 – automatische Prüfungen | 3 ✅ bestanden (davon 2 mit ⚠️ Achtung) · 3 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 2:55 min |

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
| S01 | 2, 4, 6 | Not-Aus im Stillstand: ESTOP, HV aus, Shutter zu, Dauerton, ACK erst nach Lösen am Display (1 s) | ✅ bestanden | ESTOP, HV aus, Shutter zu, ACK über API abgewiesen, Quittierung am Display | 44 s |
| S02 | 4 | Shutter im Stillstand: zu, rote LED aus, Öffnen ohne HV abgewiesen | ✅ bestanden | Shutter zu, Öffnen ohne HV abgewiesen (STATE) | 5 s |
| S03 | 2, 3 | Fahrtrichtung: Referenzfahrt zum hinteren Endschalter (Parkseite), +mm weg davon, Summer 1 Piep | ✅ bestanden | INIT zur Parkseite, +mm weg davon | 32 s |
| S04 | 2 | Endlagen: Endschalter vorn/hinten in /proc, kein Fehler bei stehender Achse | ✅ bestanden | beide Endschalter erkannt, kein Fehler | 7 s |
| S05 | 3 | Endlagen: unerwarteter Endschalter während der Fahrt -> MOTOR_FAULT, Achse steht | ✅ bestanden | MOTOR_FAULT bei 69.9 mm, nach ACK INIT nötig | 15 s |
| S06 | 2, 4, 6 | Not-Aus in Fahrt (HV an, Shutter offen): ESTOP, HV aus, Shutter zu, Referenz verloren | ✅ bestanden | ESTOP in Fahrt, HV aus, Shutter zu; mit cooling_flow false weiter ESTOP; danach INIT nötig | 55 s |
| S07 | 4 | Not-Aus stoppt die Bewegung ≤ 2 ms (Oszilloskop) | ✅ bestanden | 1.00 ms | 46 s |

## Block 2 – weitere Handprüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| M06 | 2 | Abstand neben dem Messgut aufzeichnen (Maßband, ohne Bewertung) | ✅ bestanden | aufgezeichnet: Anlage 69.5 mm, Maßband 123.0 mm (ohne Bewertung) | 76 s |
| M11 | 7 | Weboberfläche (Tablet im AP, lokale Datei, Hilfe, Einstellungen, Kalibrierseite, Router) | ✅ bestanden | Weboberfläche geprüft | 42 s |

## Block 3 – automatische Prüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ❌ **nicht bestanden** | Stufe 115 mm: CALIB point -> NOT_STABLE | 17 s |
| T18 | 2, 8 | Stehende Messung je Stufe: Mittelwert ±1 mm, Einzelwerte 3σ ≤ 2 mm | ✅ ⚠️ bestanden, Achtung | alle Stufen in Toleranz (Einzelwerte im Anhang) – Shutter 5× geschlossen (Firmware vor V1.6.4) – Achtung nah am Grenzwert: 115 mm: Abstand Mittelwert, 115 mm: Dicke Mittelwert, 115 mm: Dicke Mittelwert der Anlage (avg), 115 mm: Dicke Sigma | 45 s |
| T19 | 8 | Profilfahrt hin und zurück: Kanten erkannt und aufgezeichnet, Stufen ±1 mm, Shutter durchgehend offen | ❌ **nicht bestanden** | hin: Kante Messgut ab nicht erkannt (81.1); hin: Kante Messgut bis nicht erkannt (238.9); zurück: Kante Messgut ab nicht erkannt (91.4); zurück: Kante Messgut bis nicht erkannt (239.6) | 78 s |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort | ❌ **nicht bestanden** | stress_api parallel:     raise URLError(err) / urllib.error.URLError: <urlopen error timed out> | 12 s |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 8 min | – |
| T29 | 4 | Zeitwerte ≤ 3100 µs (inkl. Summer-Signale aus Block 1 und 2) | ✅ ⚠️ bestanden, Achtung | seit dem letzten Neustart | – |

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
| ✅ | S01 | Zeit bis Quittierung am Display | 5.266 | s | ≥ 1 |
| ✅ | S02 | Shutter laut /proc | closed |  | closed |
| ✅ | S02 | SHUTTER_OPEN ohne HV | STATE |  | abgewiesen |
| ✅ | S02 | Shutter mechanisch zu und rote LED aus? | ja |  | ja |
| – | S03 | Dauer INIT | 0.874 | s |  |
| ✅ | S03 | vorderer Endschalter während INIT | nein |  | nein |
| ✅ | S03 | Position nach INIT | 0.0 | mm | 0 ± 0.05 |
| ✅ | S03 | Ist der Schlitten zur Parkseite (hinterer Endschalter) gefahren? | ja |  | ja |
| ✅ | S03 | Hat der Summer am Ende der Referenzfahrt einmal kurz gepiept? | ja |  | ja |
| ✅ | S03 | Ist der Schlitten von der Parkseite weg gefahren? | ja |  | ja |
| ✅ | S04 | limit_fwd gedrückt / losgelassen erkannt | ja |  | ja |
| ✅ | S04 | limit_bwd gedrückt / losgelassen erkannt | ja |  | ja |
| ✅ | S05 | Fehler | MOTOR_FAULT |  | MOTOR_FAULT |
| – | S05 | Halteposition | 69.9 | mm |  |
| ✅ | S06 | Fehler | ESTOP |  | ESTOP |
| – | S06 | Halteposition | 83.2 | mm |  |
| ✅ | S06 | HV / Shutter | aus / closed |  | aus / closed |
| ✅ | S06 | Fehler mit cooling_flow = false | ESTOP |  | ESTOP |
| ✅ | S06 | Zustand nach ACK | SAFE_START |  | SAFE_START |
| ✅ | S07 | Not-Aus -> letzter Schritt | 1.0 | ms | ≤ 2 |
| – | M06 | Abstand laut Anlage bei 242 mm | 69.5 | mm |  |
| – | M06 | Abstand Maßband | 123.0 | mm |  |
| – | M06 | Abweichung Anlage – Maßband (Info) | -53.5 | mm |  |
| ✅ | M11 | Seite lädt ohne Internet, Werte aktualisieren sich mindestens zweimal pro Sekunde? | ja |  | ja |
| ✅ | M11 | Verlauf auf der Prozessseite zeigt bis zu 100 Punkte? | ja |  | ja |
| ✅ | M11 | Hilfe-Symbol öffnet /doc in einem neuen Tab mit Beschreibung und Linktabelle? | ja |  | ja |
| ✅ | M11 | Einstellungen: Wert ändern, Speichern, Neu laden -> Wert bleibt? | ja |  | ja |
| ✅ | M11 | Einstellungen: ungültiger Wert wird rot markiert und nicht gesendet? | ja |  | ja |
| ✅ | M11 | web-ui/ims-demo-ui.html am PC lokal geöffnet: Verbindungsdialog, Verbindung klappt? | ja |  | ja |
| ✅ | M11 | Router-Test durchführen (SSID/Passwort setzen, Neustart, Router aus/an – ca. 10 min)? | ja |  | ja |
| ✅ | M11 | Anlage im Heimnetz erreichbar (IP im Display), AP ADD-Demo bleibt erreichbar? | ja |  | ja |
| ✅ | M11 | Router aus: Log zeigt sta: attempt 1/3 … 3/3, dann sta: backoff 120 s? | ja |  | ja |
| ✅ | M11 | Router wieder an: Verbindung kommt von selbst zurück (sta: connected)? | ja |  | ja |
| ✅ ⚠️ | T18 | 115 mm: Abstand Mittelwert | 64.007 | mm | 65 ± 1 |
| ✅ ⚠️ | T18 | 115 mm: Dicke Mittelwert | 5.993 | mm | 5 ± 1 |
| ✅ ⚠️ | T18 | 115 mm: Dicke Mittelwert der Anlage (avg) | 6.0 | mm | 5 ± 1 |
| ✅ ⚠️ | T18 | 115 mm: Dicke Sigma | 0.641 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 115 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 115 mm: Dicke Min / Max | 5.0 / 8.3 | mm |  |
| – | T18 | 115 mm: Abstand Sigma | 0.641 | mm |  |
| ✅ | T18 | 135 mm: Abstand Mittelwert | 59.9 | mm | 60 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert | 10.1 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert der Anlage (avg) | 10.0 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Sigma | 0.29 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 135 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 135 mm: Dicke Min / Max | 9.6 / 10.7 | mm |  |
| – | T18 | 135 mm: Abstand Sigma | 0.29 | mm |  |
| ✅ | T18 | 155 mm: Abstand Mittelwert | 55.147 | mm | 55 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert | 14.852 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert der Anlage (avg) | 14.8 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Sigma | 0.168 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 155 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 155 mm: Dicke Min / Max | 14.4 / 15.3 | mm |  |
| – | T18 | 155 mm: Abstand Sigma | 0.168 | mm |  |
| ✅ | T18 | 175 mm: Abstand Mittelwert | 49.89 | mm | 50 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert | 20.11 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert der Anlage (avg) | 20.1 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Sigma | 0.11 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 175 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 175 mm: Dicke Min / Max | 19.8 / 20.3 | mm |  |
| – | T18 | 175 mm: Abstand Sigma | 0.11 | mm |  |
| ✅ | T18 | 195 mm: Abstand Mittelwert | 45.107 | mm | 45 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert | 24.892 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert der Anlage (avg) | 24.8 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Sigma | 0.11 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 195 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 195 mm: Dicke Min / Max | 24.7 / 25.1 | mm |  |
| – | T18 | 195 mm: Abstand Sigma | 0.11 | mm |  |
| ✅ | T18 | 215 mm: Abstand Mittelwert | 40.232 | mm | 40 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert | 29.768 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert der Anlage (avg) | 29.7 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Sigma | 0.102 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 215 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 215 mm: Dicke Min / Max | 29.5 / 30.0 | mm |  |
| – | T18 | 215 mm: Abstand Sigma | 0.102 | mm |  |
| – | T18 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 5 |  |  |
| ✅ | T19 | hin: Messwerte im Lauf | 363 |  | ≥ 201 |
| ❌ | T19 | hin: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 81.1 | mm | 105 ± 10 |
| ❌ | T19 | hin: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 238.9 | mm | 225 ± 10 |
| ✅ | T19 | hin: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| ✅ ⚠️ | T19 | hin: Stufe 115 mm: Dicke (Median) | 5.9 | mm | 5 ± 1 |
| – | T19 | hin: Stufe 115 mm: Anzahl Werte | 22 |  |  |
| ✅ | T19 | hin: Stufe 135 mm: Dicke (Median) | 10.0 | mm | 10 ± 1 |
| – | T19 | hin: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 155 mm: Dicke (Median) | 15.0 | mm | 15 ± 1 |
| – | T19 | hin: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 175 mm: Dicke (Median) | 20.0 | mm | 20 ± 1 |
| – | T19 | hin: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 195 mm: Dicke (Median) | 24.9 | mm | 25 ± 1 |
| – | T19 | hin: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Stufe 215 mm: Dicke (Median) | 29.8 | mm | 30 ± 1 |
| – | T19 | hin: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | hin: Mittelwert der Anlage | 17.4 | mm | 5 … 30 |
| ✅ | T19 | hin: Anzahl Werte der Anlage | 1204 |  | ≥ 51 |
| ✅ | T19 | zurück: Messwerte im Lauf | 364 |  | ≥ 201 |
| ❌ | T19 | zurück: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 91.4 | mm | 105 ± 10 |
| ❌ | T19 | zurück: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 239.6 | mm | 225 ± 10 |
| ✅ | T19 | zurück: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| ✅ ⚠️ | T19 | zurück: Stufe 115 mm: Dicke (Median) | 5.9 | mm | 5 ± 1 |
| – | T19 | zurück: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 135 mm: Dicke (Median) | 10.1 | mm | 10 ± 1 |
| – | T19 | zurück: Stufe 135 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 155 mm: Dicke (Median) | 15.0 | mm | 15 ± 1 |
| – | T19 | zurück: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 175 mm: Dicke (Median) | 20.0 | mm | 20 ± 1 |
| – | T19 | zurück: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 195 mm: Dicke (Median) | 24.9 | mm | 25 ± 1 |
| – | T19 | zurück: Stufe 195 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Stufe 215 mm: Dicke (Median) | 29.9 | mm | 30 ± 1 |
| – | T19 | zurück: Stufe 215 mm: Anzahl Werte | 22 |  |  |
| ✅ | T19 | zurück: Mittelwert der Anlage | 17.5 | mm | 5 … 30 |
| ✅ | T19 | zurück: Anzahl Werte der Anlage | 1191 |  | ≥ 51 |
| – | T19 | Kante Messgut ab: Mittel hin/zurück (Info) | 86.25 | mm |  |
| – | T19 | Kante Messgut ab: Unterschied hin – zurück (Info) | -10.3 | mm |  |
| – | T19 | Kante Messgut bis: Mittel hin/zurück (Info) | 239.25 | mm |  |
| – | T19 | Kante Messgut bis: Unterschied hin – zurück (Info) | -0.7 | mm |  |
| – | T26 | parallel | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T26 | parallel | slowest /api round trip: 7317.0 ms (HV_OFF) |  |  |
| – | T28 | Laufzeit | 8.433 | min |  |
| ✅ | T28 | unerwartete Neustarts | 0 |  | 0 |
| ✅ ⚠️ | T29 | cycle_max_us | 2882 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | period_max_us | 3008 | µs | ≤ 3100 |
| ✅ | T29 | prio_lat_max_us | 1744 | µs | ≤ 3100 |
| – | T29 | overruns (Info) | 3382 |  |  |

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
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2882, "period_max_us": 3008, "overruns": 3382, "prio_lat_max_us": 1744, "crc": "BBE8B3E9"}
```

### /log am Ende

```
net: http 705 ms
net: http 255 ms
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
```

### T18 – Einzelwerte Stufe 115 mm (Soll Abstand 65.0, Dicke 5.0)

```
Abstand [mm]: 64.7 63.4 63.9 63.2 63.9 63.5 63.9 64.1 63.1 64.1 64.3 64.7 64.8 64.1 64.5 64.2 64.7 64.3 64.6 65.0 64.0 64.2 63.2 64.0 63.6 63.3 63.7 63.5 63.8 63.1 63.8 61.7 63.9 64.7 64.4 64.6 64.1 64.3 64.8 64.6
Dicke [mm]:   5.3 6.6 6.1 6.8 6.1 6.5 6.1 5.9 6.9 5.9 5.7 5.3 5.2 5.9 5.5 5.8 5.3 5.7 5.4 5.0 6.0 5.8 6.8 6.0 6.4 6.7 6.3 6.5 6.2 6.9 6.2 8.3 6.1 5.3 5.6 5.4 5.9 5.7 5.2 5.4
```

### T18 – Einzelwerte Stufe 135 mm (Soll Abstand 60.0, Dicke 10.0)

```
Abstand [mm]: 60.0 59.9 59.9 59.4 59.7 59.6 59.8 59.5 59.5 59.8 59.7 59.9 60.0 60.3 60.4 60.0 60.3 60.2 60.3 60.1 60.3 60.4 59.9 59.8 59.4 59.9 59.8 59.3 59.7 59.7 59.9 59.4 59.8 60.0 60.1 60.2 60.0 60.2 60.0 59.9
Dicke [mm]:   10.0 10.1 10.1 10.6 10.3 10.4 10.2 10.5 10.5 10.2 10.3 10.1 10.0 9.7 9.6 10.0 9.7 9.8 9.7 9.9 9.7 9.6 10.1 10.2 10.6 10.1 10.2 10.7 10.3 10.3 10.1 10.6 10.2 10.0 9.9 9.8 10.0 9.8 10.0 10.1
```

### T18 – Einzelwerte Stufe 155 mm (Soll Abstand 55.0, Dicke 15.0)

```
Abstand [mm]: 55.6 55.4 54.7 55.0 55.0 55.0 55.2 55.1 55.1 55.0 55.2 55.1 55.0 55.3 55.1 55.3 55.4 55.4 55.4 55.2 55.3 55.0 55.3 55.2 55.2 55.1 55.0 55.0 54.9 55.1 55.1 55.0 55.1 55.1 55.2 55.0 55.2 55.1 55.2 55.3
Dicke [mm]:   14.4 14.6 15.3 15.0 15.0 15.0 14.8 14.9 14.9 15.0 14.8 14.9 15.0 14.7 14.9 14.7 14.6 14.6 14.6 14.8 14.7 15.0 14.7 14.8 14.8 14.9 15.0 15.0 15.1 14.9 14.9 15.0 14.9 14.9 14.8 15.0 14.8 14.9 14.8 14.7
```

### T18 – Einzelwerte Stufe 175 mm (Soll Abstand 50.0, Dicke 20.0)

```
Abstand [mm]: 50.2 50.0 49.9 49.8 49.8 49.9 49.9 49.9 49.8 49.8 49.8 49.8 49.8 49.8 49.8 49.9 50.0 50.0 50.0 50.0 50.0 50.1 50.0 50.0 50.0 50.1 49.8 49.7 49.8 49.8 49.8 49.8 49.9 49.9 49.8 49.8 49.9 49.8 49.8 49.9
Dicke [mm]:   19.8 20.0 20.1 20.2 20.2 20.1 20.1 20.1 20.2 20.2 20.2 20.2 20.2 20.2 20.2 20.1 20.0 20.0 20.0 20.0 20.0 19.9 20.0 20.0 20.0 19.9 20.2 20.3 20.2 20.2 20.2 20.2 20.1 20.1 20.2 20.2 20.1 20.2 20.2 20.1
```

### T18 – Einzelwerte Stufe 195 mm (Soll Abstand 45.0, Dicke 25.0)

```
Abstand [mm]: 45.3 45.3 45.1 45.2 45.3 45.1 45.0 45.0 45.0 45.0 45.0 45.0 45.0 45.1 44.9 45.0 45.1 45.2 45.2 45.2 45.2 45.2 45.2 45.2 45.3 45.3 45.1 45.2 45.1 45.0 45.0 45.1 45.1 45.0 45.0 45.1 45.1 45.0 45.0 45.1
Dicke [mm]:   24.7 24.7 24.9 24.8 24.7 24.9 25.0 25.0 25.0 25.0 25.0 25.0 25.0 24.9 25.1 25.0 24.9 24.8 24.8 24.8 24.8 24.8 24.8 24.8 24.7 24.7 24.9 24.8 24.9 25.0 25.0 24.9 24.9 25.0 25.0 24.9 24.9 25.0 25.0 24.9
```

### T18 – Einzelwerte Stufe 215 mm (Soll Abstand 40.0, Dicke 30.0)

```
Abstand [mm]: 40.5 40.3 40.3 40.3 40.3 40.4 40.3 40.2 40.2 40.2 40.2 40.2 40.1 40.1 40.0 40.1 40.2 40.2 40.3 40.3 40.3 40.3 40.4 40.4 40.3 40.3 40.2 40.3 40.3 40.2 40.2 40.2 40.1 40.1 40.2 40.2 40.2 40.2 40.1 40.1
Dicke [mm]:   29.5 29.7 29.7 29.7 29.7 29.6 29.7 29.8 29.8 29.8 29.8 29.8 29.9 29.9 30.0 29.9 29.8 29.8 29.7 29.7 29.7 29.7 29.6 29.6 29.7 29.7 29.8 29.7 29.7 29.8 29.8 29.8 29.9 29.9 29.8 29.8 29.8 29.8 29.9 29.9
```

### T26 – stress_api.py parallel --n 400 --threads 8

```
15:27:21 192.168.1.120: parallel: 400 mixed commands from 8 threads, every reply checked (about 10 ... 30 s)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  slowest /api round trip: 7317.0 ms (HV_OFF)
  device log (ctrl/net/comm/http/sta):
    net: http 705 ms
    net: http 255 ms
    net: http 207 ms
    net: http 731 ms
    net: http 238 ms
    net: http 706 ms
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

