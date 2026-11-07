# Abnahmeprotokoll A1 – Demoanlage ADD (Slim)

**Grundlage:** Pflichtenheft V1.7.12, Kapitel 10 · Ablauf: `docs/sprints/A1.md`
**Datum:** ______________ · **Prüfer:** Norbert Walter · **Firmware/Commit:** ______________
**Automatischer Teil:** `docs/reviews/Abnahme_A1_automatisch.md`

Ergebnis je Zeile: ✅ bestanden · ❌ nicht bestanden · ⏭ nicht geprüft · n. a. nicht anwendbar.
Txx = Abnahmeskript, Hxx = Handprüfung (Sprint-Datei Abschnitt 3).

## 1. System-Start

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| Boot fehlerfrei, Watchdog aktiv, 5 min ohne Reset | H01, T28 | | |
| Parameter nach Neustart wiederhergestellt; Sicherheitsflags zurückgesetzt | T04 | | |

## 2. Hardware-Signale

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| LED grün folgt HV_ON/HV_OFF | T07 | | |
| LED rot folgt Shutter | T11 | | |
| Summer: Startsignal | H01 | | |
| Summer: Klick bei Display-Tasten | H03 | | |
| Summer: Not-Aus 4 s Dauerton | T23 | | |
| Summer: andere Fehler 3 Pieps | T09, T16 | | |
| Summer: Referenzfahrt-Ende 1 Piep | T05 | | |
| Summer: Reboot-Signal vor dem Neustart | H01 | | |
| Blaue LED 0,5 Hz | H01 | | |
| Blaue LED steht still, wenn die Steuerschleife hängt | H01 | n. a. | ohne Testfirmware nicht auslösbar |
| Endschalter in `/proc` korrekt | T06 | | |
| Abstand plausibel 35–300 mm (±5 %) | H02 | | |
| Stufen 65 … 40 mm auf ±1 mm | T18 | | |
| Not-Aus mit `estop_nc = false`: Drücken löst aus | T23 | | |
| Öffner: Drücken und Drahtbruch lösen aus; falsche Einstellung = Dauer-Not-Aus | H08 | ⏭ | nach dem Umbau auf Öffner |
| Umschalten von `estop_nc`: falsche Polarität löst sofort aus (`test_estop_polarity_change`) | H05 | | |

## 3. Motor

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| INIT referenziert (hinterer Endschalter, 2 mm Rückzug, Position 0) | T05 | | |
| MOVE ±0,1 mm (Messschieber), keine Schrittverluste nach 10 Fahrten | T21 | | vorher ____ mm, nachher ____ mm |
| Unerwarteter Endschalter → MOTOR_FAULT | T22 | | |

## 4. Sicherheitskreis

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| Not-Aus stoppt Bewegung ≤ 2 ms (Oszilloskop) | H07 | | ____ ms, Bild `A1_estop_oszi.png` |
| HV_ON nur mit allen vier Bedingungen; jede fehlende → PRECONDITION | T07 (Not-Aus: T23 → FAULT ESTOP) | | |
| 31 °C → HV aus ohne Fehler; 35 °C → TEMP_HIGH, ACK erst nach Absenken | T09 | | |
| `cooling_flow = false` während Profilfahrt → HV aus, Fahrt gestoppt, Shutter zu | T15 | | |
| Watchdog: Skript abbrechen → COMM_TIMEOUT nach 5 s | T16 | | |
| Nach ACK kein Wiederanlauf; nach Not-Aus in Fahrt INIT nötig | T16, T23 | | |
| Shutter öffnet nie in der Parkzone | T10 | | |
| SHUTTER_OPEN nur in der Servicezone; 145 mm → PRECONDITION | T11 | | |
| Automatischer Shutter nie in der Servicezone; schließt beim Einfahren | T12 | | |
| Hin-/Rückfahrt innerhalb 3 s: Shutter bleibt offen; 3 s danach zu; STOP schließt sofort | T13 | | |
| Fahrt auf 242 mm löst den vorderen Endschalter nicht aus | T14 | | |
| Not-Aus und `cooling_flow = false` gleichzeitig → FAULT ESTOP | T23 | | |
| STOP per API: Antwort < 150 ms, Achse steht < 1 s | T24 | | |
| 10 min mit Weboberfläche: kein COMM_TIMEOUT, Zeitwerte ≤ 3000 µs | T27 | | |
| Zeitverhalten mit Summer-Signal | T29 | | |
| `SET maint_pos` in Fahrt → Speichern erst im Stillstand | T17 | | |
| Queue-Flut: nur OK/BUSY, alle CRC gültig | T25 | | |
| Parallele Anfragen: keine vertauschte Antwort | T26 | | |

## 5. REST-API + /proc

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| Alle Endpunkte < 1 s | T01 | | |
| `/proc` vollständig, CRC gültig | T02 | | |
| Falsche CRC → BAD_CRC; Befehle laut CONTRACT §5.2 | T03 und T05–T23 | | |
| `pio test -e native` grün | H06 | ✅ | 479/479 (Reviewer, 07.11.2026) |

## 6. Display

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| Werte, Zustand, Indikatoren korrekt, kein Flackern | H03 | | |
| Positionsbalken synchron | H03 | | |
| Alle 7 Tasten wirken (ohne WLAN); PARK → 2 mm, SERV → `maint_pos` | H03 | | |
| Fehlerfall: nur ACK-Fläche; grau solange Not-Aus gedrückt | H03 | | |
| ACK erst nach 1 s Halten; Finger auf STOP quittiert nicht | H03 | | |
| Bedienvorrang (nur mit `LOCAL_HOLD_MS > 0`) | – | n. a. | `LOCAL_HOLD_MS = 0` |

## 7. Weboberfläche

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| Live-Daten ≤ 500 ms, Zeitreihe 100 Punkte | H04 | | |
| IMS-Bedienoberfläche ohne Internet (AP); lokal mit Verbindungsdialog | H04 | | |
| Hilfe-Symbol öffnet `/doc` in neuem Tab | H04 | | |
| Einstellungen laden/speichern, Kalibrierung, WLAN-Konfiguration | H04 | | |
| Tablet über WLAN (AP) | H04 | | |
| Router-Verbindung, AP bleibt erreichbar | H04 | | |
| Router aus: 3 Versuche, 2 min Pause, Verbindung kommt zurück | H04 | | |

## 8. Betriebsmodi

| Kriterium | Prüfung | Ergebnis | Bemerkung / Messwert |
|---|---|---|---|
| Stehende Messung: Mittelwert auf Stufe ±0,5 mm | T18 | | |
| Profilfahrt: Kanten erkannt, Statistik plausibel | T19 | | |
| Kalibrierung gespeichert und wirksam | T20, H04 | | |

## 9. ADD + KI

| Kriterium | Prüfung | Ergebnis | Bemerkung |
|---|---|---|---|
| Block 9 vollständig | mit Sprint D1 | ⏭ | Entscheidung 07.11.2026 |

## Gesamtergebnis

- Bestanden: ____ · Nicht bestanden: ____ · Offen: ____
- Befunde und Nacharbeiten:

| Nr. | Befund | Maßnahme | Status |
|---|---|---|---|
| | | | |

**Abnahme erteilt:** ☐ ja ☐ mit Auflagen ☐ nein  ·  Datum / Unterschrift: ______________
