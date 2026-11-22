# Abnahme A1 – Prüfprotokoll

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

## Block 1 – Sicherheit

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| S01 | 2, 4, 6 | Not-Aus im Stillstand: ESTOP, HV aus, Shutter zu, Dauerton, ACK erst nach Lösen am Display (1 s) | ✅ bestanden | ESTOP, HV aus, Shutter zu, ACK über API abgewiesen, Quittierung am Display | 46 s |
| S02 | 4 | Shutter im Stillstand: zu, rote LED aus, Öffnen ohne HV abgewiesen | ✅ bestanden | Shutter zu, Öffnen ohne HV abgewiesen (STATE) | 9 s |
| S03 | 2, 3 | Fahrtrichtung: Referenzfahrt zum hinteren Endschalter (Parkseite), +mm weg davon, Summer 1 Piep | ✅ bestanden | INIT zur Parkseite, +mm weg davon | 37 s |
| S04 | 2 | Endlagen: Endschalter vorn/hinten in /proc, kein Fehler bei stehender Achse | ✅ bestanden | beide Endschalter erkannt, kein Fehler | 18 s |
| S05 | 3 | Endlagen: unerwarteter Endschalter während der Fahrt -> MOTOR_FAULT, Achse steht | ✅ bestanden | MOTOR_FAULT bei 132.5 mm, nach ACK INIT nötig | 20 s |
| S06 | 2, 4, 6 | Not-Aus in Fahrt (HV an, Shutter offen): ESTOP, HV aus, Shutter zu, Referenz verloren | ✅ bestanden | ESTOP in Fahrt, HV aus, Shutter zu; mit cooling_flow false weiter ESTOP; danach INIT nötig | 58 s |
| S07 | 4 | Not-Aus stoppt die Bewegung ≤ 2 ms (Oszilloskop) | ✅ bestanden | 1.10 ms | 58 s |

## Block 2 – weitere Handprüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| M01 | 1, 2 | Neustart: Summer lang–kurz, Startsignal kurz–lang, SAFE_START, blaue LED 0,5 Hz | ✅ bestanden | SAFE_START, boot: ready im Log | 35 s |
| M04 | 2 | LED grün folgt HV, LED rot folgt Shutter | ✅ bestanden | HV und Shutter geschaltet | 28 s |
| M05 | 2 | Fehlersignal: Summer 3 Pieps (Übertemperatur) | ✅ bestanden | TEMP_HIGH, quittiert | 21 s |
| M06 | 2 | Abstand neben dem Messgut aufzeichnen (Maßband, ohne Bewertung) | ✅ bestanden | aufgezeichnet: Anlage 173.5 mm, Maßband 134.0 mm (ohne Bewertung) | 76 s |
| M07 | 3 | MOVE ±0,1 mm, keine Schrittverluste nach 10 Fahrten (Messschieber) | ✅ bestanden | Differenz 0.00 mm | 156 s |
| M10 | 6 | Display: 7 Tasten wirken, Anzeige korrekt, Positionsbalken synchron | ❌ **nicht bestanden** | Taste HV: HV ein: nach 20 s nicht erreicht (READY, pos 0.0, idle/none, HV False, Shutter closed) | 30 s |
| M11 | 7 | Weboberfläche (Tablet im AP, lokale Datei, Hilfe, Einstellungen, Kalibrierseite, Router) | ✅ bestanden | Weboberfläche geprüft | 247 s |

## Block 3 – automatische Prüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| T01 | 5 | Alle Endpunkte antworten in < 1 s | ❌ **nicht bestanden** | langsamer als 1 s: ['/settings'] | 4 s |
| T02 | 5 | /proc vollständig, CRC gültig, ≤ 512 Byte | ✅ bestanden | alle Felder vorhanden | 0 s |
| T03 | 5 | Fehlerantworten: falsche CRC, kaputtes JSON, unbekannter Befehl, Parameter, Zustand | ✅ bestanden | alle Fehlerantworten wie erwartet | 0 s |
| T04 | 1 | Parameter nach Neustart wiederhergestellt, Sicherheitsflags zurückgesetzt | ✅ bestanden | gespeicherter Wert da, Sicherheitsflags Standard, SAFE_START | 8 s |
| T05 | 3 | INIT referenziert (hinterer Endschalter, 2 mm Rückzug, Position 0) | ✅ bestanden | READY, Endschalter frei | 1 s |
| T07 | 4 | HV_ON nur mit allen Bedingungen; jede fehlende -> PRECONDITION | ✅ bestanden | jede fehlende Bedingung -> PRECONDITION (Not-Aus: S01, S06) | 1 s |
| T09 | 4 | Temperatur 31 °C -> HV aus ohne Fehler; 35 °C -> TEMP_HIGH, ACK erst nach Absenken | ✅ bestanden | wie erwartet | 1 s |
| T10 | 4 | Shutter öffnet nie in der Parkzone (≤ 20 mm) | ✅ bestanden | Parkzone immer zu | 11 s |
| T11 | 4 | SHUTTER_OPEN manuell nur in der Servicezone; bei 145 mm -> PRECONDITION | ✅ bestanden | wie erwartet | 14 s |
| T12 | 4 | Automatischer Shutter nie in der Servicezone (≤ 60 mm), schließt beim Einfahren | ✅ bestanden | Servicezone immer zu | 11 s |
| T13 | 4 | Hin- und Rückfahrt innerhalb 3 s: Shutter bleibt offen; 3 s nach Ende zu; STOP schließt sofort | ❌ **nicht bestanden** | Shutter nach 5.0 s zu | 32 s |
| T14 | 4 | Fahrt auf 242 mm löst den vorderen Endschalter nicht aus | ✅ bestanden | Endschalter frei | 14 s |
| T15 | 4 | cooling_flow = false während Profilfahrt -> HV aus, Fahrt gestoppt, Shutter zu | ✅ bestanden | READY ohne Fehler, Referenz bleibt | 21 s |
| T16 | 4 | Kommunikations-Watchdog: COMM_TIMEOUT nach 5 s; nach ACK kein Wiederanlauf, INIT nötig | ✅ bestanden | COMM_TIMEOUT, kein Wiederanlauf, INIT nötig | 7 s |
| T17 | 4 | SET maint_pos bei fahrender Achse: Speichern verschoben bis Stillstand mit HV aus | ✅ bestanden | Speichern erst im Stillstand | 60 s |
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ✅ bestanden | 6 Punkte mit HV an und offenem Shutter übernommen und gespeichert – Shutter 5× geschlossen (Firmware vor V1.6.4) | 29 s |
| T18 | 2, 8 | Stehende Messung je Stufe: Mittelwert ±1 mm, Einzelwerte 3σ ≤ 2 mm | ✅ bestanden | alle Stufen in Toleranz (Einzelwerte im Anhang) – Shutter 6× geschlossen (Firmware vor V1.6.4) | 55 s |
| T19 | 8 | Profilfahrt hin und zurück: Kanten und Stufen aufgezeichnet (ohne Bewertung), Shutter durchgehend offen, Statistik plausibel | ✅ bestanden | Profil hin und zurück aufgezeichnet (Kanten und Stufen im Protokoll, ohne Bewertung) – Shutter 1× geschlossen (Firmware vor V1.6.4) | 78 s |
| T20 | 8 | Kalibriertabelle (6 Stufen) gespeichert und wirksam: Abweichung je Stufe ±1 mm (Mittelwert, Sigma) | ✅ bestanden | Tabelle gültig und kalibriert, Abweichung je Stufe ≤ 1 mm | 0 s |
| T24 | 4 | STOP per API während Profilfahrt: Antwort < 150 ms, Achse steht < 1 s | ✅ bestanden | RESULT: PASS (Ausgabe im Anhang) | 17 s |
| T25 | 4 | Queue-Flut (200 SET): nur OK/BUSY, alle CRC gültig | ✅ bestanden | RESULT: PASS (Ausgabe im Anhang) | 11 s |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte oder verlorene Antwort | ✅ bestanden | RESULT: PASS (Ausgabe im Anhang) | 24 s |
| T27 | 4 | Zeitverhalten 10 min mit dauernd geladener Weboberfläche (soak --web) | ✅ bestanden | RESULT: PASS (Ausgabe im Anhang) | 601 s |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 31 min | – |
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
| ✅ | S01 | Zeit bis Quittierung am Display | 4.821 | s | ≥ 1 |
| ✅ | S02 | Shutter laut /proc | closed |  | closed |
| ✅ | S02 | SHUTTER_OPEN ohne HV | STATE |  | abgewiesen |
| ✅ | S02 | Shutter mechanisch zu und rote LED aus? | ja |  | ja |
| – | S03 | Dauer INIT | 1.293 | s |  |
| ✅ | S03 | vorderer Endschalter während INIT | nein |  | nein |
| ✅ | S03 | Position nach INIT | 0.0 | mm | 0 ± 0.05 |
| ✅ | S03 | Ist der Schlitten zur Parkseite (hinterer Endschalter) gefahren? | ja |  | ja |
| ✅ | S03 | Hat der Summer am Ende der Referenzfahrt einmal kurz gepiept? | ja |  | ja |
| ✅ | S03 | Ist der Schlitten von der Parkseite weg gefahren? | ja |  | ja |
| ✅ | S04 | limit_fwd gedrückt / losgelassen erkannt | ja |  | ja |
| ✅ | S04 | limit_bwd gedrückt / losgelassen erkannt | ja |  | ja |
| ✅ | S05 | Fehler | MOTOR_FAULT |  | MOTOR_FAULT |
| – | S05 | Halteposition | 132.5 | mm |  |
| ✅ | S06 | Fehler | ESTOP |  | ESTOP |
| – | S06 | Halteposition | 128.4 | mm |  |
| ✅ | S06 | HV / Shutter | aus / closed |  | aus / closed |
| ✅ | S06 | Fehler mit cooling_flow = false | ESTOP |  | ESTOP |
| ✅ | S06 | Zustand nach ACK | SAFE_START |  | SAFE_START |
| ✅ | S07 | Not-Aus -> letzter Schritt | 1.1 | ms | ≤ 2 |
| ✅ | M01 | Zustand nach Neustart | SAFE_START |  | SAFE_START |
| ✅ | M01 | Vor dem Neustart Summer lang–kurz? | ja |  | ja |
| ✅ | M01 | Nach dem Neustart Summer kurz–lang? | ja |  | ja |
| ✅ | M01 | Display zeigt SAFE_START? | ja |  | ja |
| ✅ | M01 | Blaue LED blinkt gleichmäßig 1 s an / 1 s aus (10 Perioden ≈ 20 s)? | ja |  | ja |
| ✅ | M04 | HV ein: leuchtet die grüne LED? | ja |  | ja |
| ✅ | M04 | Shutter offen: leuchtet die rote LED? | ja |  | ja |
| ✅ | M04 | Shutter zu: ist die rote LED aus? | ja |  | ja |
| ✅ | M04 | HV aus: ist die grüne LED aus? | ja |  | ja |
| ✅ | M05 | Fehler | TEMP_HIGH |  | TEMP_HIGH |
| ✅ | M05 | Hat der Summer 3-mal gepiept? | ja |  | ja |
| ✅ | M05 | Zeigt das Display den Fehler TEMP_HIGH mit ACK-Fläche? | ja |  | ja |
| – | M06 | Abstand laut Anlage bei 242 mm | 173.5 | mm |  |
| – | M06 | Abstand Maßband | 134.0 | mm |  |
| – | M06 | Abweichung Anlage – Maßband (Info) | 39.5 | mm |  |
| – | M07 | Messschieber vorher | 50.0 | mm |  |
| – | M07 | Messschieber nachher | 50.0 | mm |  |
| ✅ | M07 | Differenz | 0.0 | mm | ≤ 0.1 |
| – | M07 | Fahrten (Ziel mm @ mm/s) | 85.1@5 106.7@20 197.2@20 177.6@10 77.8@5 220.9@20 138.5@5 185.6@15 190.2@10 166.8@5 |  |  |
| ✅ | M10 | Taste INIT -> Referenzfahrt fertig | 0.0 | s | Wirkung in /proc |
| ✅ | M11 | Seite lädt ohne Internet, Werte aktualisieren sich mindestens zweimal pro Sekunde? | ja |  | ja |
| ✅ | M11 | Verlauf auf der Prozessseite zeigt bis zu 100 Punkte? | ja |  | ja |
| ✅ | M11 | Hilfe-Symbol öffnet /doc in einem neuen Tab mit Beschreibung und Linktabelle? | ja |  | ja |
| ✅ | M11 | Einstellungen: Wert ändern, Speichern, Neu laden -> Wert bleibt? | ja |  | ja |
| ✅ | M11 | Einstellungen: ungültiger Wert wird rot markiert und nicht gesendet? | ja |  | ja |
| ✅ | M11 | web-ui/index.html am PC lokal geöffnet: Verbindungsdialog, Verbindung klappt? | ja |  | ja |
| ✅ | M11 | Router-Test durchführen (SSID/Passwort setzen, Neustart, Router aus/an – ca. 10 min)? | ja |  | ja |
| ✅ | M11 | Anlage im Heimnetz erreichbar (IP im Display), AP ADD-Demo bleibt erreichbar? | ja |  | ja |
| ✅ | M11 | Router aus: Log zeigt sta: attempt 1/3 … 3/3, dann sta: backoff 120 s? | ja |  | ja |
| ✅ | M11 | Router wieder an: Verbindung kommt von selbst zurück (sta: connected)? | ja |  | ja |
| ✅ ⚠️ | T01 | GET / (HTTP 200) | 809.745 | ms | ≤ 1000 |
| ❌ | T01 | GET /settings (HTTP 200) | 1053.189 | ms | ≤ 1000 |
| ✅ | T01 | GET /doc (HTTP 200) | 146.206 | ms | ≤ 1000 |
| ✅ | T01 | GET /proc (HTTP 200) | 117.258 | ms | ≤ 1000 |
| ✅ | T01 | GET /log (HTTP 200) | 32.838 | ms | ≤ 1000 |
| ✅ | T01 | GET /llms.txt (HTTP 200) | 42.033 | ms | ≤ 1000 |
| ✅ | T01 | GET /app.js (HTTP 200) | 199.007 | ms | ≤ 1000 |
| ✅ | T01 | GET /font-u400.woff2 (HTTP 200) | 286.135 | ms | ≤ 1000 |
| ✅ | T01 | GET /font-u700.woff2 (HTTP 200) | 273.496 | ms | ≤ 1000 |
| ✅ | T01 | GET /font-m400.woff2 (HTTP 200) | 266.151 | ms | ≤ 1000 |
| ✅ | T01 | GET /font-m700.woff2 (HTTP 200) | 242.569 | ms | ≤ 1000 |
| ✅ | T01 | GET /add (HTTP 200) | 355.393 | ms | ≤ 1000 |
| ✅ | T01 | POST /api GET_STATE | 72.775 | ms | ≤ 1000 |
| ✅ | T02 | Länge /proc | 434 | Byte | ≤ 512 |
| ✅ | T03 | Antwort auf BAD_CRC-Fall | HTTP 400, err -1 |  | HTTP 400, err -1 |
| ✅ | T03 | Antwort auf BAD_JSON-Fall | HTTP 400, err -2 |  | HTTP 400, err -2 |
| ✅ | T03 | Antwort auf UNKNOWN_CMD-Fall | HTTP 200, err -3 |  | HTTP 200, err -3 |
| ✅ | T03 | Antwort auf BAD_PARAM-Fall | HTTP 200, err -4 |  | HTTP 200, err -4 |
| ✅ | T03 | ACK außerhalb WAIT_ACK | err -5 |  | err -5 |
| ✅ | T04 | measure_pos nach Neustart | 150.5 | mm | 150.5 ± 0.05 |
| ✅ | T04 | interlock / cooling_flow / temp nach Neustart | True / True / 18.0 |  | True / True / 18.0 |
| – | T05 | Dauer INIT | 0.816 | s |  |
| ✅ | T05 | Position nach INIT | 0.0 | mm | 0 ± 0.05 |
| ✅ | T07 | HV_ON mit interlock = False | PRECONDITION |  | PRECONDITION |
| ✅ | T07 | HV_ON mit cooling_flow = False | PRECONDITION |  | PRECONDITION |
| ✅ | T07 | HV_ON mit temp = 31.0 | PRECONDITION |  | PRECONDITION |
| ✅ | T07 | HV_ON mit allen Bedingungen | RUNNING |  | RUNNING |
| ✅ | T09 | 31 °C: Zustand / Fehler | READY / NONE |  | READY / NONE |
| ✅ | T09 | 35 °C: Fehler | TEMP_HIGH |  | TEMP_HIGH |
| ✅ | T09 | ACK bei 35 °C | PRECONDITION |  | PRECONDITION |
| ✅ | T09 | nach ACK bei 25 °C: HV | False |  | False |
| ✅ | T10 | SHUTTER_OPEN bei 10 mm | PRECONDITION |  | PRECONDITION |
| ✅ | T10 | START_MEASURE bei 10 mm | BAD_PARAM |  | BAD_PARAM |
| – | T10 | manueller Shutter schließt bei | 19.9 | mm | Info: erste Abfrage mit Shutter zu (Abtastung ≈ 0,5 mm) |
| ✅ | T10 | Abfragen mit offenem Shutter in der Parkzone | 0 |  | 0 |
| ✅ | T11 | SHUTTER_OPEN bei 40 mm | OK |  | OK |
| ✅ | T11 | SHUTTER_OPEN bei 145 mm | PRECONDITION |  | PRECONDITION |
| ✅ | T12 | START_MEASURE {'pos': 40} | BAD_PARAM |  | BAD_PARAM |
| ✅ | T12 | START_MEASURE {'pos': 60} | BAD_PARAM |  | BAD_PARAM |
| ✅ | T12 | START_TRAVERSE {'start': 60, 'end': 100, 'speed': 10} | BAD_PARAM |  | BAD_PARAM |
| – | T12 | automatischer Shutter schließt bei | 59.8 | mm | Info: erste Abfrage mit Shutter zu (Abtastung ≈ 0,5 mm) |
| ✅ | T12 | Abfragen mit offenem Shutter in der Servicezone | 0 |  | 0 |
| ✅ | T13 | Zeit bis zum Folgebefehl | 0.059 | s | ≤ 3 |
| ❌ | T13 | Shutter zu nach Laufende | 4.992 | s | 3 (2,5 … 4,0) |
| ✅ | T14 | Position | 242.0 | mm | 242 ± 0.1 |
| ✅ | T14 | vorderer Endschalter | False |  | False |
| ✅ | T15 | HV aus / Shutter zu nach | 88.971 | ms | ≤ 1000 |
| ✅ | T15 | Zustand / Fehler | READY / NONE |  | READY / NONE |
| – | T15 | Halteposition | 121.3 | mm |  |
| ✅ | T16 | Fehler | COMM_TIMEOUT |  | COMM_TIMEOUT |
| ✅ | T16 | Zeit seit dem letzten /api-Paket | 4.998 | s | 5 (4,8 … 6,5) |
| ✅ | T16 | nach ACK: Zustand / homed / HV | SAFE_START / False / False |  | SAFE_START / False / False |
| ✅ | T17 | 'save deferred' während der Fahrt | True |  | True |
| ✅ | T17 | 'nvs: saved' während der Fahrt | False |  | False |
| ✅ | T17 | 'nvs: saved' nach dem Stillstand | True |  | True |
| – | T18K | Punkt 1 (115 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 2 (135 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 3 (155 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 4 (175 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 5 (195 mm): Versuche bis stabil | 1 |  |  |
| – | T18K | Punkt 6 (215 mm): Versuche bis stabil | 1 |  |  |
| ✅ | T18K | Punkt 1 (115 mm): Rohwert (unkalibriert) gegen Referenz 65 | 65.3 | mm | 65 ± 7 |
| – | T18K | Punkt 1 (115 mm): Änderung gegenüber der alten Tabelle | 0.0 | mm |  |
| ✅ | T18K | Punkt 2 (135 mm): Rohwert (unkalibriert) gegen Referenz 60 | 60.0 | mm | 60 ± 7 |
| – | T18K | Punkt 2 (135 mm): Änderung gegenüber der alten Tabelle | 0.0 | mm |  |
| ✅ | T18K | Punkt 3 (155 mm): Rohwert (unkalibriert) gegen Referenz 55 | 54.9 | mm | 55 ± 7 |
| – | T18K | Punkt 3 (155 mm): Änderung gegenüber der alten Tabelle | 0.0 | mm |  |
| ✅ | T18K | Punkt 4 (175 mm): Rohwert (unkalibriert) gegen Referenz 50 | 50.1 | mm | 50 ± 7 |
| – | T18K | Punkt 4 (175 mm): Änderung gegenüber der alten Tabelle | 0.0 | mm |  |
| ✅ | T18K | Punkt 5 (195 mm): Rohwert (unkalibriert) gegen Referenz 45 | 44.9 | mm | 45 ± 7 |
| – | T18K | Punkt 5 (195 mm): Änderung gegenüber der alten Tabelle | 0.1 | mm |  |
| ✅ | T18K | Punkt 6 (215 mm): Rohwert (unkalibriert) gegen Referenz 40 | 40.2 | mm | 40 ± 7 |
| – | T18K | Punkt 6 (215 mm): Änderung gegenüber der alten Tabelle | 0.0 | mm |  |
| ✅ | T18K | calib_valid | True |  | True |
| – | T18K | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 5 |  |  |
| ✅ | T18 | 115 mm: Abstand Mittelwert | 64.89 | mm | 65 ± 1 |
| ✅ | T18 | 115 mm: Dicke Mittelwert | 5.11 | mm | 5 ± 1 |
| ✅ | T18 | 115 mm: Dicke Mittelwert der Anlage (avg) | 5.1 | mm | 5 ± 1 |
| ✅ | T18 | 115 mm: Dicke Sigma | 0.101 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 115 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 115 mm: Dicke Min / Max | 4.9 / 5.3 | mm |  |
| – | T18 | 115 mm: Abstand Sigma | 0.101 | mm |  |
| ✅ | T18 | 135 mm: Abstand Mittelwert | 59.89 | mm | 60 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert | 10.11 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Mittelwert der Anlage (avg) | 10.1 | mm | 10 ± 1 |
| ✅ | T18 | 135 mm: Dicke Sigma | 0.078 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 135 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 135 mm: Dicke Min / Max | 9.9 / 10.3 | mm |  |
| – | T18 | 135 mm: Abstand Sigma | 0.078 | mm |  |
| ✅ | T18 | 155 mm: Abstand Mittelwert | 54.935 | mm | 55 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert | 15.065 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Mittelwert der Anlage (avg) | 15.1 | mm | 15 ± 1 |
| ✅ | T18 | 155 mm: Dicke Sigma | 0.08 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 155 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 155 mm: Dicke Min / Max | 15.0 / 15.3 | mm |  |
| – | T18 | 155 mm: Abstand Sigma | 0.08 | mm |  |
| ✅ | T18 | 175 mm: Abstand Mittelwert | 50.0 | mm | 50 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert | 20.0 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Mittelwert der Anlage (avg) | 20.0 | mm | 20 ± 1 |
| ✅ | T18 | 175 mm: Dicke Sigma | 0.032 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 175 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 175 mm: Dicke Min / Max | 19.9 / 20.1 | mm |  |
| – | T18 | 175 mm: Abstand Sigma | 0.032 | mm |  |
| ✅ | T18 | 195 mm: Abstand Mittelwert | 44.903 | mm | 45 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert | 25.098 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Mittelwert der Anlage (avg) | 25.1 | mm | 25 ± 1 |
| ✅ | T18 | 195 mm: Dicke Sigma | 0.058 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 195 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 195 mm: Dicke Min / Max | 25.0 / 25.2 | mm |  |
| – | T18 | 195 mm: Abstand Sigma | 0.058 | mm |  |
| ✅ | T18 | 215 mm: Abstand Mittelwert | 39.945 | mm | 40 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert | 30.055 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Mittelwert der Anlage (avg) | 30.0 | mm | 30 ± 1 |
| ✅ | T18 | 215 mm: Dicke Sigma | 0.055 | mm | ≤ 0.67 (Einzelwerte 3σ ≤ 2) |
| ✅ | T18 | 215 mm: Anzahl Einzelwerte | 40 |  | ≥ 30 |
| – | T18 | 215 mm: Dicke Min / Max | 30.0 / 30.2 | mm |  |
| – | T18 | 215 mm: Abstand Sigma | 0.055 | mm |  |
| – | T18 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 6 |  |  |
| ✅ | T19 | hin: Messwerte im Lauf | 361 |  | ≥ 201 |
| – | T19 | hin: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| – | T19 | hin: Kante Messgut ab (Info, Stufenkörper 105) | 110.1 | mm |  |
| – | T19 | hin: Kante Messgut bis (Info, Stufenkörper 225) | 225.2 | mm |  |
| ✅ | T19 | hin: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| – | T19 | hin: Stufe 115 mm: Dicke (Median, Info, Soll 5) | 5.3 | mm |  |
| – | T19 | hin: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 135 mm: Dicke (Median, Info, Soll 10) | 10.2 | mm |  |
| – | T19 | hin: Stufe 135 mm: Anzahl Werte | 22 |  |  |
| – | T19 | hin: Stufe 155 mm: Dicke (Median, Info, Soll 15) | 15.2 | mm |  |
| – | T19 | hin: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 175 mm: Dicke (Median, Info, Soll 20) | 20.0 | mm |  |
| – | T19 | hin: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| – | T19 | hin: Stufe 195 mm: Dicke (Median, Info, Soll 25) | 25.1 | mm |  |
| – | T19 | hin: Stufe 195 mm: Anzahl Werte | 22 |  |  |
| – | T19 | hin: Stufe 215 mm: Dicke (Median, Info, Soll 30) | 30.0 | mm |  |
| – | T19 | hin: Stufe 215 mm: Anzahl Werte | 22 |  |  |
| ✅ | T19 | hin: Mittelwert der Anlage | 17.9 | mm | 5 … 30 |
| ✅ | T19 | hin: Anzahl Werte der Anlage | 1155 |  | ≥ 51 |
| ✅ | T19 | zurück: Messwerte im Lauf | 363 |  | ≥ 201 |
| – | T19 | zurück: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info) | 0 |  |  |
| – | T19 | zurück: Kante Messgut ab (Info, Stufenkörper 105) | 108.9 | mm |  |
| – | T19 | zurück: Kante Messgut bis (Info, Stufenkörper 225) | 223.8 | mm |  |
| ✅ | T19 | zurück: Shutter geschlossen im Messgutbereich (Messpunkte) | 0 |  | 0 |
| – | T19 | zurück: Stufe 115 mm: Dicke (Median, Info, Soll 5) | 5.3 | mm |  |
| – | T19 | zurück: Stufe 115 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 135 mm: Dicke (Median, Info, Soll 10) | 10.2 | mm |  |
| – | T19 | zurück: Stufe 135 mm: Anzahl Werte | 22 |  |  |
| – | T19 | zurück: Stufe 155 mm: Dicke (Median, Info, Soll 15) | 15.3 | mm |  |
| – | T19 | zurück: Stufe 155 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 175 mm: Dicke (Median, Info, Soll 20) | 20.0 | mm |  |
| – | T19 | zurück: Stufe 175 mm: Anzahl Werte | 23 |  |  |
| – | T19 | zurück: Stufe 195 mm: Dicke (Median, Info, Soll 25) | 25.3 | mm |  |
| – | T19 | zurück: Stufe 195 mm: Anzahl Werte | 22 |  |  |
| – | T19 | zurück: Stufe 215 mm: Dicke (Median, Info, Soll 30) | 30.1 | mm |  |
| – | T19 | zurück: Stufe 215 mm: Anzahl Werte | 23 |  |  |
| ✅ | T19 | zurück: Mittelwert der Anlage | 18.0 | mm | 5 … 30 |
| ✅ | T19 | zurück: Anzahl Werte der Anlage | 1155 |  | ≥ 51 |
| – | T19 | Kante Messgut ab: Mittel hin/zurück (Info) | 109.5 | mm |  |
| – | T19 | Kante Messgut ab: Unterschied hin – zurück (Info) | 1.2 | mm |  |
| – | T19 | Kante Messgut bis: Mittel hin/zurück (Info) | 224.5 | mm |  |
| – | T19 | Kante Messgut bis: Unterschied hin – zurück (Info) | 1.4 | mm |  |
| – | T19 | Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4) | 1 |  |  |
| – | T20 | Parameter calib_raw_1 | 65.3 | mm |  |
| – | T20 | Parameter calib_raw_2 | 60.0 | mm |  |
| – | T20 | Parameter calib_raw_3 | 54.9 | mm |  |
| – | T20 | Parameter calib_raw_4 | 50.1 | mm |  |
| – | T20 | Parameter calib_raw_5 | 44.9 | mm |  |
| – | T20 | Parameter calib_raw_6 | 40.2 | mm |  |
| – | T20 | Parameter calib_ref_1 | 65.0 | mm |  |
| – | T20 | Parameter calib_ref_2 | 60.0 | mm |  |
| – | T20 | Parameter calib_ref_3 | 55.0 | mm |  |
| – | T20 | Parameter calib_ref_4 | 50.0 | mm |  |
| – | T20 | Parameter calib_ref_5 | 45.0 | mm |  |
| – | T20 | Parameter calib_ref_6 | 40.0 | mm |  |
| ✅ | T20 | calib_valid | True |  | True |
| ✅ | T20 | Stufe 115 mm: Abweichung Abstand (Mittelwert – 65 mm) | -0.11 | mm | 0 ± 1 |
| – | T20 | Stufe 115 mm: Sigma Abstand | 0.101 | mm |  |
| ✅ | T20 | Stufe 135 mm: Abweichung Abstand (Mittelwert – 60 mm) | -0.11 | mm | 0 ± 1 |
| – | T20 | Stufe 135 mm: Sigma Abstand | 0.078 | mm |  |
| ✅ | T20 | Stufe 155 mm: Abweichung Abstand (Mittelwert – 55 mm) | -0.065 | mm | 0 ± 1 |
| – | T20 | Stufe 155 mm: Sigma Abstand | 0.08 | mm |  |
| ✅ | T20 | Stufe 175 mm: Abweichung Abstand (Mittelwert – 50 mm) | 0.0 | mm | 0 ± 1 |
| – | T20 | Stufe 175 mm: Sigma Abstand | 0.032 | mm |  |
| ✅ | T20 | Stufe 195 mm: Abweichung Abstand (Mittelwert – 45 mm) | -0.097 | mm | 0 ± 1 |
| – | T20 | Stufe 195 mm: Sigma Abstand | 0.058 | mm |  |
| ✅ | T20 | Stufe 215 mm: Abweichung Abstand (Mittelwert – 40 mm) | -0.055 | mm | 0 ± 1 |
| – | T20 | Stufe 215 mm: Sigma Abstand | 0.055 | mm |  |
| – | T24 | latency | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T24 | latency | #1: STOP reply   56.1 ms, idle in /proc after  110.8 ms |  |  |
| – | T24 | latency | #2: STOP reply   59.6 ms, idle in /proc after  115.6 ms |  |  |
| – | T24 | latency | #3: STOP reply   49.1 ms, idle in /proc after  139.5 ms |  |  |
| – | T24 | latency | #4: STOP reply   55.7 ms, idle in /proc after   97.5 ms |  |  |
| – | T24 | latency | #5: STOP reply   53.9 ms, idle in /proc after   94.2 ms |  |  |
| – | T24 | latency | #6: STOP reply   45.5 ms, idle in /proc after   85.4 ms |  |  |
| – | T24 | latency | #7: STOP reply   37.6 ms, idle in /proc after   88.0 ms |  |  |
| – | T24 | latency | #8: STOP reply   60.3 ms, idle in /proc after   98.7 ms |  |  |
| – | T24 | latency | #9: STOP reply   51.8 ms, idle in /proc after   95.1 ms |  |  |
| – | T24 | latency | #10: STOP reply   36.2 ms, idle in /proc after   96.4 ms |  |  |
| – | T24 | latency | reply  min/median/max: 36.2 / 52.8 / 60.3 ms |  |  |
| – | T24 | latency | idle   min/median/max: 85.4 / 97.0 / 139.5 ms |  |  |
| – | T24 | latency | (HTTP round trip incl. WLAN; the device-internal STOP latency is prio_lat_max_us below) |  |  |
| – | T24 | latency | cycles over 1 ms +21 (info; since boot ready 703) cycle_max_us=1922 period_max_us=2446 prio_lat_max_us=1492 -> OK |  |  |
| – | T24 | latency | STOP, HV_OFF sent |  |  |
| – | T24 | latency | slowest /api round trip: 93.5 ms (GET_STATE) |  |  |
| – | T24 | latency | RESULT: PASS |  |  |
| – | T25 | flood | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T25 | flood | cycles over 1 ms +9 (info; since boot ready 712) cycle_max_us=1922 period_max_us=2446 prio_lat_max_us=1492 -> OK |  |  |
| – | T25 | flood | slowest /api round trip: 108.3 ms (SET) |  |  |
| – | T25 | flood | RESULT: PASS |  |  |
| – | T26 | parallel | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T26 | parallel | cycles over 1 ms +8 (info; since boot ready 721) cycle_max_us=1922 period_max_us=2446 prio_lat_max_us=1492 -> OK |  |  |
| – | T26 | parallel | slowest /api round trip: 7507.0 ms (SET) |  |  |
| – | T26 | parallel | RESULT: PASS |  |  |
| – | T27 | soak | limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info |  |  |
| – | T27 | soak | 1 min: cycles over 1 ms +221, cycle_max_us=2077, period_max_us=2446, web files 221 ok / 0 failed |  |  |
| – | T27 | soak | 2 min: cycles over 1 ms +419, cycle_max_us=2077, period_max_us=2446, web files 441 ok / 0 failed |  |  |
| – | T27 | soak | 3 min: cycles over 1 ms +631, cycle_max_us=2077, period_max_us=2446, web files 659 ok / 0 failed |  |  |
| – | T27 | soak | 4 min: cycles over 1 ms +823, cycle_max_us=2358, period_max_us=2458, web files 875 ok / 0 failed |  |  |
| – | T27 | soak | 5 min: cycles over 1 ms +1015, cycle_max_us=2358, period_max_us=2458, web files 1093 ok / 0 failed |  |  |
| – | T27 | soak | 6 min: cycles over 1 ms +1228, cycle_max_us=2358, period_max_us=2458, web files 1313 ok / 0 failed |  |  |
| – | T27 | soak | 7 min: cycles over 1 ms +1427, cycle_max_us=2358, period_max_us=2458, web files 1533 ok / 0 failed |  |  |
| – | T27 | soak | 8 min: cycles over 1 ms +1631, cycle_max_us=2509, period_max_us=2557, web files 1751 ok / 0 failed |  |  |
| – | T27 | soak | 9 min: cycles over 1 ms +1816, cycle_max_us=2509, period_max_us=2557, web files 1971 ok / 0 failed |  |  |
| – | T27 | soak | cycles over 1 ms +1987 (info; since boot ready 2708) cycle_max_us=2509 period_max_us=2557 prio_lat_max_us=1492 -> OK |  |  |
| – | T27 | soak | slowest /api round trip: 783.3 ms (GET_STATE) |  |  |
| – | T27 | soak | RESULT: PASS |  |  |
| – | T28 | Laufzeit | 30.933 | min |  |
| ✅ | T28 | unerwartete Neustarts | 0 |  | 0 |
| ✅ ⚠️ | T29 | cycle_max_us | 2509 | µs | ≤ 3100 |
| ✅ ⚠️ | T29 | period_max_us | 2557 | µs | ≤ 3100 |
| ✅ | T29 | prio_lat_max_us | 1492 | µs | ≤ 3100 |
| – | T29 | overruns (Info) | 2719 |  |  |

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
 "calib_raw_5": 44.9,
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

### T18 – Einzelwerte Stufe 115 mm (Soll Abstand 65.0, Dicke 5.0)

```
Abstand [mm]: 64.9 65.1 64.9 64.9 64.9 64.9 64.8 64.7 64.8 64.8 65.0 64.9 64.7 65.0 65.0 65.0 64.9 65.0 65.0 65.0 65.0 64.9 64.9 64.9 64.9 65.0 65.0 64.8 64.8 64.8 64.8 64.9 65.0 64.9 64.9 64.8 64.8 64.7 64.9 64.7
Dicke [mm]:   5.1 4.9 5.1 5.1 5.1 5.1 5.2 5.3 5.2 5.2 5.0 5.1 5.3 5.0 5.0 5.0 5.1 5.0 5.0 5.0 5.0 5.1 5.1 5.1 5.1 5.0 5.0 5.2 5.2 5.2 5.2 5.1 5.0 5.1 5.1 5.2 5.2 5.3 5.1 5.3
```

### T18 – Einzelwerte Stufe 135 mm (Soll Abstand 60.0, Dicke 10.0)

```
Abstand [mm]: 59.8 59.8 59.9 59.9 59.9 60.0 59.9 59.9 59.9 60.0 59.8 59.9 59.9 59.8 59.9 59.7 59.8 59.9 59.9 59.8 59.8 60.0 59.9 59.9 60.0 59.8 59.9 60.0 59.9 59.8 59.9 59.9 59.9 60.1 60.0 59.9 59.9 59.8 59.9 59.9
Dicke [mm]:   10.2 10.2 10.1 10.1 10.1 10.0 10.1 10.1 10.1 10.0 10.2 10.1 10.1 10.2 10.1 10.3 10.2 10.1 10.1 10.2 10.2 10.0 10.1 10.1 10.0 10.2 10.1 10.0 10.1 10.2 10.1 10.1 10.1 9.9 10.0 10.1 10.1 10.2 10.1 10.1
```

### T18 – Einzelwerte Stufe 155 mm (Soll Abstand 55.0, Dicke 15.0)

```
Abstand [mm]: 55.0 55.0 55.0 55.0 54.8 54.9 55.0 54.9 55.0 55.0 54.8 54.9 54.9 55.0 54.8 54.9 54.7 54.9 54.9 55.0 54.9 55.0 55.0 54.9 54.9 55.0 55.0 55.0 55.0 55.0 55.0 55.0 54.8 54.9 55.0 55.0 54.8 54.9 55.0 54.9
Dicke [mm]:   15.0 15.0 15.0 15.0 15.2 15.1 15.0 15.1 15.0 15.0 15.2 15.1 15.1 15.0 15.2 15.1 15.3 15.1 15.1 15.0 15.1 15.0 15.0 15.1 15.1 15.0 15.0 15.0 15.0 15.0 15.0 15.0 15.2 15.1 15.0 15.0 15.2 15.1 15.0 15.1
```

### T18 – Einzelwerte Stufe 175 mm (Soll Abstand 50.0, Dicke 20.0)

```
Abstand [mm]: 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.1 50.0 50.0 50.1 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 49.9 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 50.0 49.9
Dicke [mm]:   20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 19.9 20.0 20.0 19.9 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.1 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.1
```

### T18 – Einzelwerte Stufe 195 mm (Soll Abstand 45.0, Dicke 25.0)

```
Abstand [mm]: 44.9 44.9 45.0 44.9 45.0 44.9 44.8 44.9 45.0 44.9 44.9 44.9 44.9 44.9 44.8 44.9 44.9 44.9 44.9 45.0 44.9 45.0 45.0 44.9 44.9 44.8 44.9 44.8 44.9 44.9 44.9 44.9 44.8 44.9 44.9 44.9 44.9 45.0 44.9 44.8
Dicke [mm]:   25.1 25.1 25.0 25.1 25.0 25.1 25.2 25.1 25.0 25.1 25.1 25.1 25.1 25.1 25.2 25.1 25.1 25.1 25.1 25.0 25.1 25.0 25.0 25.1 25.1 25.2 25.1 25.2 25.1 25.1 25.1 25.1 25.2 25.1 25.1 25.1 25.1 25.0 25.1 25.2
```

### T18 – Einzelwerte Stufe 215 mm (Soll Abstand 40.0, Dicke 30.0)

```
Abstand [mm]: 40.0 40.0 40.0 40.0 40.0 40.0 39.9 39.9 40.0 39.9 40.0 40.0 40.0 40.0 39.9 40.0 39.9 40.0 40.0 40.0 40.0 39.9 39.9 40.0 39.9 39.9 39.9 40.0 39.8 39.9 39.9 39.9 39.9 39.9 39.9 39.9 39.9 39.9 39.9 40.0
Dicke [mm]:   30.0 30.0 30.0 30.0 30.0 30.0 30.1 30.1 30.0 30.1 30.0 30.0 30.0 30.0 30.1 30.0 30.1 30.0 30.0 30.0 30.0 30.1 30.1 30.0 30.1 30.1 30.1 30.0 30.2 30.1 30.1 30.1 30.1 30.1 30.1 30.1 30.1 30.1 30.1 30.0
```

### T24 – stress_api.py latency --n 10 --prepare

```
13:58:16 192.168.1.127: latency: 10 x START_TRAVERSE / STOP, measures STOP reply and time until the axis is idle (prepare: INIT if not homed, HV_ON; STOP + HV_OFF at the end) - THE AXIS MOVES, E-stop in reach
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  prepare: homed, axis idle, HV on
  traverse 80.0 -> 240.0 mm at 10.0 mm/s
  #1: STOP reply   56.1 ms, idle in /proc after  110.8 ms
  #2: STOP reply   59.6 ms, idle in /proc after  115.6 ms
  #3: STOP reply   49.1 ms, idle in /proc after  139.5 ms
  #4: STOP reply   55.7 ms, idle in /proc after   97.5 ms
  #5: STOP reply   53.9 ms, idle in /proc after   94.2 ms
  #6: STOP reply   45.5 ms, idle in /proc after   85.4 ms
  #7: STOP reply   37.6 ms, idle in /proc after   88.0 ms
  #8: STOP reply   60.3 ms, idle in /proc after   98.7 ms
  #9: STOP reply   51.8 ms, idle in /proc after   95.1 ms
  #10: STOP reply   36.2 ms, idle in /proc after   96.4 ms
reply  min/median/max: 36.2 / 52.8 / 60.3 ms
idle   min/median/max: 85.4 / 97.0 / 139.5 ms
  (HTTP round trip incl. WLAN; the device-internal STOP latency is prio_lat_max_us below)
  cycles over 1 ms +21 (info; since boot ready 703) cycle_max_us=1922 period_max_us=2446 prio_lat_max_us=1492 -> OK
  STOP, HV_OFF sent
  slowest /api round trip: 93.5 ms (GET_STATE)
  device log (ctrl/net/comm/http/sta):
    sta: attempt 1/3
    sta: connected 192.168.1.127
    net: http 218 ms
    comm: timeout, last /api packet 5001 ms ago, longest gap while armed 89 ms
RESULT: PASS
```

### T25 – stress_api.py flood --n 200

```
13:58:32 192.168.1.127: flood: 200 x SET temp=18.0 one after the other, then the cycle timing (a few seconds)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  results by err: {0: 200}, worst round trip 108.3 ms
  cycles over 1 ms +9 (info; since boot ready 712) cycle_max_us=1922 period_max_us=2446 prio_lat_max_us=1492 -> OK
  slowest /api round trip: 108.3 ms (SET)
  device log (ctrl/net/comm/http/sta):
    sta: attempt 1/3
    sta: connected 192.168.1.127
    net: http 218 ms
    comm: timeout, last /api packet 5001 ms ago, longest gap while armed 89 ms
RESULT: PASS
```

### T26 – stress_api.py parallel --n 400 --threads 8

```
13:58:43 192.168.1.127: parallel: 400 mixed commands from 8 threads, every reply checked (about 10 ... 30 s)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  400 requests, 8 threads: 0 mismatches, 0 BUSY, 0 lost, 3 connection timeouts retried (info), worst 7507.0 ms
  cycles over 1 ms +8 (info; since boot ready 721) cycle_max_us=1922 period_max_us=2446 prio_lat_max_us=1492 -> OK
  slowest /api round trip: 7507.0 ms (SET)
  device log (ctrl/net/comm/http/sta):
    sta: attempt 1/3
    sta: connected 192.168.1.127
    net: http 218 ms
    comm: timeout, last /api packet 5001 ms ago, longest gap while armed 89 ms
RESULT: PASS
```

### T27 – stress_api.py soak --minutes 10 --web

```
13:59:07 192.168.1.127: soak: GET_STATE every second for 10 min, meanwhile the web UI files are loaded without pause; a line every minute, a dot every 10 s; Ctrl+C aborts
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
.....
  1 min: cycles over 1 ms +221, cycle_max_us=2077, period_max_us=2446, web files 221 ok / 0 failed
.....
  2 min: cycles over 1 ms +419, cycle_max_us=2077, period_max_us=2446, web files 441 ok / 0 failed
.....
  3 min: cycles over 1 ms +631, cycle_max_us=2077, period_max_us=2446, web files 659 ok / 0 failed
.....
  4 min: cycles over 1 ms +823, cycle_max_us=2358, period_max_us=2458, web files 875 ok / 0 failed
.....
  5 min: cycles over 1 ms +1015, cycle_max_us=2358, period_max_us=2458, web files 1093 ok / 0 failed
.....
  6 min: cycles over 1 ms +1228, cycle_max_us=2358, period_max_us=2458, web files 1313 ok / 0 failed
.....
  7 min: cycles over 1 ms +1427, cycle_max_us=2358, period_max_us=2458, web files 1533 ok / 0 failed
.....
  8 min: cycles over 1 ms +1631, cycle_max_us=2509, period_max_us=2557, web files 1751 ok / 0 failed
.....
  9 min: cycles over 1 ms +1816, cycle_max_us=2509, period_max_us=2557, web files 1971 ok / 0 failed
.....
  469 heartbeats, web load: 2190 files (78809 kB) ok, 0 failed
  cycles over 1 ms +1987 (info; since boot ready 2708) cycle_max_us=2509 period_max_us=2557 prio_lat_max_us=1492 -> OK
  slowest /api round trip: 783.3 ms (GET_STATE)
  device log (ctrl/net/comm/http/sta):
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
RESULT: PASS
```

