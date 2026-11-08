# Abnahme A1 – Ergänzungsblatt

**Grundlage:** Pflichtenheft V1.7.12, Kapitel 10 · Ablauf: `docs/sprints/A1.md`
Das Skript schreibt das Hauptprotokoll `Abnahme_A1_<Datum>_<Uhrzeit>.md` (Teil 1 Handprüfungen, Teil 2
automatisch, alle Messwerte). Dieses Blatt erfasst nur, was außerhalb des Skripts geprüft wird, und das
Gesamtergebnis.

**Prüfer:** ______________ · **Datum:** ______________ · **Hauptprotokoll:** ______________________

| Kriterium (Pflichtenheft Kap. 10) | Prüfung | Ergebnis | Bemerkung |
|---|---|---|---|
| Umschalten von `estop_nc`: falsche Polarität löst sofort aus | H05 `pio test -e esp32-test` | | |
| `pio test -e native` grün | H06 | ✅ | 479/479 (Reviewer, 07.11.2026) |
| Blaue LED steht still, wenn die Steuerschleife hängt | – | n. a. | ohne Testfirmware nicht auslösbar |
| Bedienvorrang (nur mit `LOCAL_HOLD_MS > 0`) | – | n. a. | `LOCAL_HOLD_MS = 0` |
| Öffner: Drücken und Drahtbruch lösen aus; falsche Einstellung = Dauer-Not-Aus | H08 | ⏭ | nach dem Umbau auf Öffner |
| Messgenauigkeit ±1 mm (T18K–T20) mit Kalibriertabelle unter Messbedingungen | nach K2 | ⏭ | Toleranz seit 08.11.2026 ±1 mm |
| Block 9 – ADD + KI | mit Sprint D1 | ⏭ | |

## Gesamtergebnis

- Hauptprotokoll: ____ bestanden · ____ nicht bestanden · ____ nicht geprüft
- Befunde und Nacharbeiten:

| Nr. | Befund | Maßnahme | Status |
|---|---|---|---|
| | | | |

**Abnahme erteilt:** ☐ ja ☐ mit Auflagen ☐ nein  ·  Datum / Unterschrift: ______________
