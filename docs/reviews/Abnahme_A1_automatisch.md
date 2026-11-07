# Abnahme A1 – automatischer Teil

| | |
|---|---|
| Datum | 07.11.2026 19:09 |
| Gerät | http://192.168.1.120/ |
| Firmware | boot: ready, FW 1.0.0 |
| Ergebnis | **22 bestanden, 5 nicht bestanden, 1 übersprungen** |

| ID | Block | Prüfung | Ergebnis | Details |
|---|---|---|---|---|
| T01 | 5 | Alle Endpunkte antworten in < 1 s | ✅ bestanden | / 200 647 ms; /settings 200 164 ms; /doc 200 102 ms; /proc 200 42 ms; /log 200 41 ms; /llms.txt 200 38 ms; /app.js 200 145 ms; /font-u400.woff2 200 271 ms; /font-u700.woff2 200 320 ms; /font-m400.woff2 200 267 ms; /font-m700.woff2 200 217 ms; /add 404 102 ms; /api GET_STATE 58 ms (/add 404 bis Sprint D1) |
| T02 | 5 | /proc vollständig, CRC gültig | ✅ bestanden | 433 Byte, alle Felder vorhanden |
| T03 | 5 | Fehlerantworten: falsche CRC, kaputtes JSON, unbekannter Befehl, Parameter, Zustand | ✅ bestanden | BAD_CRC, BAD_JSON, UNKNOWN_CMD, BAD_PARAM, STATE – wie erwartet |
| T04 | 1 | Parameter nach Neustart wiederhergestellt, Sicherheitsflags zurückgesetzt | ✅ bestanden | measure_pos 150.5 gespeichert und nach Neustart da; interlock/cooling_flow/temp wieder Standard; SAFE_START |
| T05 | 3 | INIT referenziert (hinterer Endschalter, 2 mm Rückzug, Position 0) | ✅ bestanden | READY, pos 0.0, Endschalter frei, 1 s [Hand: Hat der Summer am Ende der Referenzfahrt einmal kurz gepiept ja] |
| T06 | 2 | Endschalter in /proc korrekt (Hand) | ✅ bestanden | limit_fwd ok, limit_bwd ok; kein Fehler bei stehender Achse |
| T07 | 4 | HV_ON nur mit allen Bedingungen; jede fehlende -> PRECONDITION; LED grün folgt HV | ✅ bestanden | interlock=False -> PRECONDITION; cooling_flow=False -> PRECONDITION; temp=31.0 -> PRECONDITION; alle erfüllt -> RUNNING (Not-Aus: T23) [Hand: Leuchtet die grüne LED (HV) ja; Hand: Ist die grüne LED jetzt aus ja] |
| T09 | 4 | Temperatur 31 °C -> HV aus ohne Fehler; 35 °C -> TEMP_HIGH, ACK erst nach Absenken | ✅ bestanden | 31 °C: READY ohne Fehler; 35 °C: WAIT_ACK TEMP_HIGH; ACK bei 35 °C -> PRECONDITION, bei 25 °C -> READY, HV bleibt aus [Hand: Hat der Summer 3-mal gepiept (Fehler) ja] |
| T10 | 4 | Shutter öffnet nie in der Parkzone (≤ 20 mm) | ✅ bestanden | SHUTTER_OPEN bei 10 mm -> PRECONDITION; START_MEASURE 10 -> BAD_PARAM; manuell offener Shutter schließt beim Einfahren in die Parkzone |
| T11 | 4 | SHUTTER_OPEN manuell nur in der Servicezone; bei 145 mm -> PRECONDITION; LED rot folgt Shutter | ✅ bestanden | 40 mm: auf/zu OK; 145 mm: PRECONDITION [Hand: Leuchtet die rote LED (Shutter) ja; Hand: Ist die rote LED jetzt aus ja] |
| T12 | 4 | Automatischer Shutter nie in der Servicezone (≤ 60 mm), schließt beim Einfahren | ✅ bestanden | START_MEASURE 40/60 und Profil ab 60 -> BAD_PARAM; Shutter beim Einfahren in die Servicezone geschlossen |
| T13 | 4 | Hin- und Rückfahrt innerhalb 3 s: Shutter bleibt offen; 3 s nach Ende zu; STOP schließt sofort | ✅ bestanden | Shutter über Hin- und Rückfahrt offen, 3.1 s nach Ende zu; STOP schließt sofort |
| T14 | 4 | Fahrt auf 242 mm löst den vorderen Endschalter nicht aus | ✅ bestanden | pos 242.0, Endschalter frei, READY |
| T15 | 4 | cooling_flow = false während Profilfahrt -> HV aus, Fahrt gestoppt, Shutter zu | ✅ bestanden | READY ohne Fehler, Achse steht bei 121.4 mm, Referenz bleibt |
| T16 | 4 | Kommunikations-Watchdog: COMM_TIMEOUT nach 5 s; nach ACK kein Wiederanlauf, INIT nötig | ❌ **nicht bestanden** | COMM_TIMEOUT: nach 10 s nicht erreicht (RUNNING, pos 222.4, moving/traverse, HV True, Shutter closed) |
| T17 | 4 | SET maint_pos bei fahrender Achse: Speichern verschoben bis Stillstand mit HV aus | ✅ bestanden | Log: save deferred -> nach Stillstand nvs: saved |
| T18 | 8 | Stehende Messung je Stufe: Dicke ±0,5 mm, Abstand ±1 mm | ❌ **nicht bestanden** | außerhalb der Toleranz: 115 mm (Soll 5 / 65), 135 mm (Soll 10 / 60), 155 mm (Soll 15 / 55), 175 mm (Soll 20 / 50), 195 mm (Soll 25 / 45), 215 mm (Soll 30 / 40) / 115 mm: Abstand 62.8, Dicke Mittel 7.20 (n=105); 135 mm: Abstand 57.3, Dicke Mittel 12.70 (n=102); 155 mm: Abstand 52.4, Dicke Mittel 17.80 (n=105); 175 mm: Abstand 47.6, Dicke Mittel 22.50 (n=102); 195 mm: Abstand 43.0, Dicke Mittel 27.20 (n=105); 215 mm: Abstand 38.9, Dicke Mittel 31.00 (n=102) |
| T19 | 8 | Profilfahrt: Kanten der Stufen erkannt, Statistik plausibel | ❌ **nicht bestanden** | Kanten bei 110.4 / 225.5 mm (erwartet 105 / 225 ±3) |
| T20 | 8 | Kalibrierung gespeichert und wirksam | ✅ bestanden | calib_valid, Nah 39.8 -> 40.0, Fern 63.5 -> 65.0 (wirksam: T18) |
| T21 | 3 | MOVE erreicht Ziel ±0,1 mm (Messschieber), keine Schrittverluste nach 10 Fahrten | ⏭ übersprungen | vom Prüfer übersprungen |
| T22 | 3 | Unerwarteter Endschalter während der Fahrt -> MOTOR_FAULT (Hand) | ✅ bestanden | MOTOR_FAULT bei 72.9 mm, Achse steht, nach ACK INIT nötig |
| T23 | 4 | Not-Aus in Fahrt -> ESTOP, INIT nötig; Not-Aus + cooling_flow false -> FAULT ESTOP (Hand) | ❌ **nicht bestanden** | INIT: unerwartet WAIT_ACK, Fehler ESTOP |
| T24 | 4 | STOP per API während Profilfahrt: Antwort < 150 ms, Achse steht < 1 s | ❌ **nicht bestanden** | Anlage im Fehler ESTOP |
| T25 | 4 | Queue-Flut (200 SET): nur OK/BUSY, alle CRC gültig | ✅ bestanden |     http: idle connection closed after 500 ms (1 so far) /     ctrl: cycle 517 us, period 2574 us (adc 150, tick 252, cmd 40, out 1, pub 74), 1 events / RESULT: PASS |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort | ✅ bestanden |     http: idle connection closed after 500 ms (2 so far) /     net: http 245 ms / RESULT: PASS |
| T27 | 4 | Zeitverhalten 10 min mit dauernd geladener Weboberfläche (soak --web) | ✅ bestanden |     net: http 647 ms /     net: http 225 ms / RESULT: PASS |
| T28 | 1 | Kein unerwarteter Neustart während der Abnahme | ✅ bestanden | Laufzeit 17 min, unerwartete Neustarts: 0 |
| T29 | 4 | Zeitwerte seit dem letzten Start ≤ 3000 µs (inkl. Summer-Signale) | ✅ bestanden | cycle_max_us 1950, period_max_us 2574, prio_lat_max_us 1355, overruns 293 (Info) |
