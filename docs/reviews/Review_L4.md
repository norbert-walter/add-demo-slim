# Review Sprint L4 – app_logic

**Implementierung:** Qwen (Cline, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 14.10.2026 · Contract V1.5.10 · Branch `sprint-l4` → `master`, Tag `sprint-l4`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_app_logic` | 84/84 |
| alle Logik-Suiten mit dem gesamten Qwen-Code L1–L4 (L5-Module aus der Referenz) | 335/335 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien | unverändert |
| Compiler (`-Wall -Wextra`) | ohne Warnung |
| Differenztest `app_logic` (Qwen) gegen Referenz, 400 Zufallsfolgen × 240 000 Zyklen | Ausgänge und Zustand identisch |
| Differenztest gesamter Qwen-Code L1–L4 gegen Referenz | Zustände, Ausgänge, Befehlsergebnisse identisch; Abstand/Mittelwert weichen um ≤ 0,001 mm ab (Gleitkomma-Rundung im L2-Code, ohne Wirkung) |

Der Differenztest erzeugt Befehle, Not-Aus, Freigaben, Temperatur, Sensorwerte und Zeitsprünge
(auch über den uint32-Überlauf) nur aus dem Zufallsgenerator; die simulierte Achse folgt den
Ausgängen. Abgedeckt: alle sechs Sicherheitszustände, alle Sequenzer-Phasen bis MEAS_APPROACH,
offener Shutter, alle Töne, alle Fahraufträge, LOCAL-Betrieb. Kontrolle der Trennschärfe mit
absichtlich eingebauten Fehlern (Watchdog-Freigabe, LOCAL-Haltezeit): erkannt in 15–20 von 40
Folgen; das fehlende Statistik-Rücksetzen erkennt der Differenztest nicht, wohl aber ein
eingefrorener Test.

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Mutationstest: 33 von 72 Mutanten überlebten | Contract V1.5.9, +17 Tests, Sprint-Datei mit vier Aufgaben |
| L4.1 (zweimal) | erste Fassung aus dem Gedächtnis (falsche Prioritätstabelle, Messkette in mV, fest verdrahtete Eingänge); zweiter Versuch flickte den alten Entwurf und las die ganze Testdatei | beide abgebrochen, Datei gelöscht, Aufgabe dreigeteilt mit festen Lesegrenzen |
| L4.1a | Tabellen, Überlauf-Latch, `appInit` | korrekt |
| L4.1b | `appMeasure` mit vier Abweichungen vom Header, alle 80 Tests trotzdem grün | Contract V1.5.10, +4 Tests, Aufgabe L4.1c |
| L4.1c | `appMeasure` korrigiert | korrekt |
| L4.2 | `appTick`: Differenztest 300 × 240 000 Zyklen ohne Abweichung | korrekt |
| L4.3 | `appCommand` geschrieben, 17 Tests rot, Qwen verlor sich im Ausprobieren | abgebrochen (67/84) |
| L4.4 | Selbstversuch nur mit den Namen der roten Tests: 3 von 11 Ursachen selbst gefunden (ACK-Ereignis, CALIB NEAR/FAR mit `raw_mm`, Ergebnis von START_MEASURE/TRAVERSE); `estop_nc` mit `cmd.key == "..."` (Adressvergleich) versucht | nach mehrfacher Kontext-Kompression abgebrochen (71/84) |
| L4.4b | Restliste mit konkreten Code-Zeilen (von Claude an einer Kopie geprüft) | 84/84, Freigabe |

**Anteil der Fehlersuche bei `appCommand`:** Die Ursachen hat zu etwa einem Viertel Qwen, zu drei
Vierteln Claude bestimmt; die Umsetzung stammt vollständig von Qwen.

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache | |
|---|---:|---:|---:|---|
| L4.1 (1. Versuch) | 268 900 | 35 800 | 2 700 000 | abgebrochen |
| L4.1 (2. Versuch) | 439 200 | 78 500 | 9 900 000 | abgebrochen |
| L4.1a Tabellen, Latch, appInit | 15 600 | 4 000 | 235 400 | |
| L4.1b appMeasure, appFillProc | 27 800 | 10 300 | 857 300 | |
| L4.1c appMeasure nach Header | 18 100 | 7 500 | 280 600 | |
| L4.2 appTick | 61 000 | 7 900 | 784 400 | |
| L4.3 appCommand | 387 100 | 96 100 | 3 500 000 | abgebrochen |
| L4.4 Selbstversuch | 219 300 | 41 500 | 1 800 000 | abgebrochen |
| L4.4b Restliste | 22 700 | 5 900 | 236 500 | mit konkreter Hilfestellung |
| **Summe L4** | **1 459 700** | **287 500** | **20 294 200** | |
| davon erfolgreiche Tasks | 145 200 | 35 600 | 2 394 200 | |

Die vier abgebrochenen Tasks verursachten 88 % der Ausgabe- und 88 % der Cache-Token.
Vergleich (Ausgabe-Token / Referenzumfang): L1 267 500 / 282 Zeilen, L2 18 800 / 143,
L3 31 600 / 227, L4 287 500 / 305 (erfolgreiche Tasks allein: 35 600).

## Akzeptierte Abweichungen (ohne Wirkung)

- **A1:** `appTick` Schritt 6 setzt nach `motionStop` einen NONE-Auftrag auf STOP. Nie erreicht:
  `mode != NONE` gilt nur bei `phase != IDLE`, dann liefert `motionStop` bereits STOP.
- **A2:** `acq_ended` aus einem Stopp im selben Zyklus bleibt in `seq.ev` und wird im nächsten
  `appTick` abgeholt (Referenz: sofort verworfen). `acq_ended` steuert seit V1.5 nichts mehr.
- **A3:** `appFillProc` setzt `out` nicht vorher zurück; alle Felder werden einzeln geschrieben.
- **A4:** Zusammenführung der Fahraufträge ersetzt auch NONE durch NONE; Klickton prüft
  zusätzlich `cmd.id != NONE`. Beides ohne Wirkung.

## Erkenntnisse

1. **Aufgabengröße:** Eine Funktion mit Header, Zieltests und fester Lesegrenze pro Task gelingt
   (1a, 1b, 1c, 2: zusammen 30k Ausgabe-Token). Mehrere eng gekoppelte Funktionen in einem Task
   führen zum Kontextüberlauf und danach zu erfundenem Code.
2. **Tests statt Header:** Qwen erfüllt Tests, nicht die Spezifikation (L4.1b: vier Abweichungen
   bei 80 grünen Tests; L4.3: erfundene INIT-BUSY-Prüfung). Ein Review mit Differenztest gegen die
   Referenz ist unverzichtbar.
3. **Fehlersuche:** Örtlich begrenzte Fehler findet Qwen selbst. Eine gemeinsame Ursache vieler
   roter Tests (verlorene Rückgabewerte) erkennt es nicht; es probiert Test für Test, liest die
   Testfunktionen wiederholt und läuft über. Abbruch spätestens bei der ersten Kontext-Kompression.
4. **Berichte:** Zahlen in Qwens Berichten nur glauben, wenn sie wörtlich aus der pio-Ausgabe
   zitiert sind (L4.4b: zum ersten Mal korrekt zitiert).
5. **Für spätere Projekte:** Befehlsattribute (Sicherheitsereignis, Fahrstart, Watchdog, Klick)
   als Tabelle im eingefrorenen Header; Kopplungen zwischen Sicherheit, Sequenzer und Shutter als
   benannte Regelliste. Dann bleibt für Qwen nur ein kleiner Interpreter.
