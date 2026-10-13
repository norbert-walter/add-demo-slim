# Review Sprint L2 – measure_logic, params

**Implementierung:** Qwen (Cline, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 13.10.2026 · Contract V1.5.6 · Branch `sprint-l2` → `master`, Tag `sprint-l2`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_measure_logic` | 40/40 |
| `test_params` | 20/20 |
| `test_app_logic` mit L1/L2-Code und Referenz für die übrigen Module | 63/63 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien | unverändert |
| Differenztest gegen die Referenz (ca. 600 000 Zufallsfälle) | identisch bis auf A1, A2 |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Mutationstest: 24 von 43 Mutanten überlebten die Tests (fest codierte Standardwerte, JSON-Felder vertauscht, Grenzen) | Contract V1.5.5, +14 Tests |
| L2.1 + L2.2 (58/58) | `test_app_logic` scheitert: Stabilitätserkennung behält nach einem Sprung den alten Referenzwert, Kalibrierung wird nie stabil. Ursache: missverständliche V1.5.5-Formulierung, kein Test „stabil nach Sprung“. Differenztest: Bereichsprüfung in `float` (60,999999999 akzeptiert). README-Zeile falsch, Tag erfunden | Contract V1.5.6, +2 Tests, Aufgabe L2.3 |
| L2.3 (60/60) | beide Punkte korrekt behoben, README-Zeile wie vorgegeben | Freigabe |

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache | Dauer |
|---|---:|---:|---:|---|
| L2.1 measure_logic | 22 200 | 4 800 | 211 600 | ca. 2 min |
| L2.2 params | 76 400 | 11 400 | 668 300 | ca. 10 min |
| L2.3 Nacharbeit | 17 500 | 2 600 | 136 300 | < 1 min |
| **Summe L2** | **116 100** | **18 800** | **1 016 200** | ca. 13 min |
| zum Vergleich L1 | 1 028 700 | 267 500 | 10 633 600 | ca. 5 h |

L2 brauchte ca. 7 % der Ausgabe-Token von L1 bei ähnlichem Umfang (L2: 143, L1: 282 Zeilen
Referenzumfang). Wirkung von Sprint-Plan V3: eine Datei pro Task, vorab gehärtete Tests,
konkrete Anweisungen in der Nacharbeit.

## Akzeptierte Abweichungen

- **A1:** `paramsToJsonMember` schreibt für Temperaturen knapp unter 0 `"temp":-0.0`
  (Referenz: `0.0`). Gültiges JSON, vom Header nicht festgelegt.
- **A2:** `calibApply` weicht bei extremen Extrapolationen in der 3. Nachkommastelle ab
  (Reihenfolge der Gleitkommarechnung).

## Erkenntnisse

1. Die Suiten der nächsthöheren Ebene (`test_app_logic` mit Referenz für die übrigen Module)
   finden Fehler, die die Modul-Tests nicht sehen. Sie gehören in jedes Review und in den
   Mutationstest vor dem Sprint.
2. Präzisierende Header-Sätze können missverstanden werden; Qwen zitierte die V1.5.5-Formulierung
   wörtlich. Klarstellungen immer mit einem Test absichern, der genau den gemeinten Fall prüft.
3. Konkrete Vorgaben (exakte README-Zeile) werden befolgt, offene („Statuszeile aktualisieren“)
   führen zu erfundenen Einträgen.
4. Vor dem Commit den Branch prüfen (`git branch --show-current`).
