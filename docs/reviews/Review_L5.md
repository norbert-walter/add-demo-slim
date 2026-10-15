# Review Sprint L5 – proc_json, api_parse, display_logic

**Implementierung:** Qwen (Cline v4.1.24, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 15.10.2026 · Contract V1.5.13 · Branch `work-l5` → `master`, Tag `sprint-l5`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_display_logic` | 12/12 |
| `test_proc_json` | 20/20 |
| `test_api_parse` | 22/22 |
| alle Logik-Suiten mit dem gesamten Qwen-Code L1–L5 | 345/345 (+ `test_command_queue` 9/9 beim Nutzer) |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien | unverändert |
| Compiler (`-Wall -Wextra`) | ohne Warnung |
| Differenztest `ackPressUpdate`/`ackCauseActive`/`screenModeOf` (300 Folgen × 4000 Abfragen, mit uint32-Überlauf) | Rückgabe, `sent` und Fortschritt identisch |
| Differenztest `procToJson`/`responseToJson`/`stateExtraToJson` (300 × 3000 Datensätze mit NaN, ±Inf, Grenzwerten, zu kleinen Puffern) | Ausgabe Byte für Byte identisch |
| Differenztest `apiParseRequest` (300 × 20 000 zufällig erzeugte, teils verstümmelte Körper) | identisch bis auf A1/A2 |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Mutationstest: 17 von 77 Mutanten überlebten | Contract V1.5.11, +7 Tests |
| L5.1–L5.3 (51/51) | Differenztest: Fortschritt nach dem Senden 0 statt 1 (`display_logic`); fehlende Zahlenparameter übernehmen eine nicht gesetzte Hilfsvariable, z. B. `end = start` (`api_parse`). Keiner der Tests bemerkte das | Contract V1.5.12, +2 Tests, Aufgabe L5.4 mit vorgegebener Diagnose |
| L5.4 (53/53) | `api_parse` korrekt; Fortschritt nur durch Begrenzung auf 1 richtig – nach dem Loslassen ist `t0 = 0`, kurz nach dem uint32-Überlauf fällt er unter 1 (Differenztest, eine von 200 Folgen) | Contract V1.5.13, +1 Test, Aufgabe L5.5 |
| L5.5 (54/54) | korrekt; Differenztest ohne Abweichung | Freigabe |

**Diagnose vorgegeben, Umsetzung durch Qwen (L5.4, L5.5):** Beide Male in einem Durchgang ohne
Umwege umgesetzt (10,5k bzw. 11,9k Eingabe-Token). Im Vergleich zu L4.4 (Qwen sucht die Ursache
selbst, abgebrochen nach 1,8 Mio. Cache-Token) zeigt das: Qwens Grenze liegt beim **Finden** der
Ursache, nicht beim Umsetzen.

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| L5.1 display_logic | 11 600 | 6 800 | 202 900 |
| L5.2 proc_json | 16 400 | 10 400 | 114 000 |
| L5.3 api_parse | 26 500 | 11 000 | 995 700 |
| L5.4 zwei Abweichungen | 10 500 | 4 400 | 228 700 |
| L5.5 Fortschritt | 11 900 | 3 800 | 133 100 |
| **Summe L5** | **76 900** | **36 400** | **1 674 400** |

Kein Task abgebrochen. Vergleich (Ausgabe-Token / Referenzumfang): L1 267 500 / 282 Zeilen,
L2 18 800 / 143, L3 31 600 / 227, L4 287 500 / 305 (erfolgreiche Tasks 35 600), L5 36 400 / 180.

## Akzeptierte Abweichungen

- **A1:** `apiParseRequest` lehnt bei SET einen Zahlenwert ab, der als float nicht endlich ist
  (z. B. `1e39`) – wörtlich nach Header („numbers must be finite after conversion to float“). Die
  Referenz reicht ihn weiter; `paramSet` lehnt ihn dann ebenfalls mit BAD_PARAM ab.
- **A2:** SET mit leerem `key` wird vom Parser angenommen; `paramSet` lehnt ihn als unbekannten
  Schlüssel mit BAD_PARAM ab. Für den Client gleiches Ergebnis.
- **A3:** `ackPressUpdate` aktualisiert `prev_touch` im Aufruf, der das ACK auslöst, nicht (früher
  Rücksprung). Nach dem Auslösen ist `sent` gesetzt, ein neuer Druck kann ohnehin nicht beginnen;
  beim Verlassen des ACK-Bildschirms wird `prev_touch` wieder gesetzt. Ohne sichtbare Wirkung.
- **A4:** `procToJson` vergleicht `static_cast<int>(cap) <= n`; gleichwertig für Puffer unter 2 GB.

## Erkenntnisse

1. **Aufgabenschnitt wirkt:** Eine Datei pro Task, feste Lesegrenzen, Hilfsfunktionen mit
   vorgegebener Signatur – L5 lief ohne Abbruch mit einem Bruchteil der Token von L4.
2. **Grüne Tests reichen nicht:** Beide Abweichungen aus L5.1–L5.3 und die aus L5.4 fand nur der
   Differenztest. Besonders Werte, die „zufällig richtig“ sind (Begrenzung statt Regel, alte
   Variable statt 0), fallen erst bei ungewöhnlichen Eingaben (Zählerüberlauf) auf.
3. **Diagnose + Umsetzung trennen:** Gibt Claude die Diagnose (Funktion, Header-Regel, Mechanismus)
   ohne Code-Zeilen vor, setzt Qwen sie zuverlässig und billig um.
4. **Berichte:** In L5 hat Qwen die SUMMARY-Zeilen durchgehend wörtlich zitiert.
