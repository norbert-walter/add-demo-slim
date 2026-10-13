# Review Sprint S2.1 – types, crc32, safety_logic

**Implementierung:** Qwen (Cline, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 12.10.2026 · Contract V1.5.3 · Branch `s2.1-review` → `master`, Tag `sprint-2.1`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_types_crc` | 22/22 |
| `test_safety_logic` | 62/62 |
| `test_app_logic` mit S2.1-Code und Referenz für die übrigen Module | 63/63 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien (Header, Tests, CONTRACT, `config.h`) | unverändert |
| Differenztest gegen die Referenz (400 000 Zufallsfälle, nur konsistente Ausgangszustände) | identisch bis auf Abweichung A1 |

## Verlauf

| Runde | Befund | Folge |
|---|---|---|
| 1 (71/71) | `motor_stop` in FAULT/WAIT_ACK nicht bei jedem Ereignis; `INIT_DONE` prüft `homed`; Ausgänge außerhalb RUNNING nicht zwangsweise aus; CRC-Prüfung mit 512-Byte-Puffer (lange Rahmen mit Fake-CRC `00000000` akzeptiert); verzögert initialisierte statische CRC-Tabelle | Contract V1.5.2: +8 Tests |
| 2 | nur `crc32.cpp` bearbeitet, `safety_logic.cpp` unverändert; Bericht nannte die fehlschlagende Suite nicht | neuer Task nur für `safety_logic.cpp` |
| 3 (79/79) | Differenztest: `INIT_DONE` umging die Fehlererkennung R1 (FAULT nicht erkannt, HV blieb in RUNNING an, einmaliger Motorfehler ging verloren); falsche API-Codes in WAIT_ACK/RECOVER | Contract V1.5.3: R1-Matrix, vollständige API-Tabelle (+5 Tests) |
| 4 (84/84) | Verhalten korrekt; `len < 8` und README-Zeile offen | Abschluss-Task |
| 5 | erledigt | Freigabe |

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

↑ = Eingabe (neu eingelesen) · ↓ = Ausgabe (inkl. Denken) · → = aus dem Prompt-Cache gelesen

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| Pure C++ (Erstimplementierung) | 119 200 | 49 800 | 1 700 000 |
| Continuation (nach Kontextüberlauf) | 208 500 | 68 400 | 2 500 000 |
| Nacharbeit Teil 1 (V1.5.2) | 175 400 | 34 800 | 1 200 000 |
| Nacharbeit Teil 2 (safety_logic) | 272 000 | 48 800 | 2 900 000 |
| Nacharbeit Teil 3 (V1.5.3) | 243 600 | 64 500 | 2 300 000 |
| Abschluss (len < 8, README) | 10 000 | 1 200 | 33 600 |
| **Summe** | **1 028 700** | **267 500** | **10 633 600** |

| Phase | ↑ | ↓ | → |
|---|---:|---:|---:|
| Erstimplementierung | 32 % | 44 % | 39 % |
| Nacharbeit (Teil 1–3, Abschluss) | 68 % | 56 % | 61 % |
| davon Abschluss | 1,0 % | 0,4 % | 0,3 % |

- 91 % des gesamten Kontexts (Eingabe + Cache, 11,7 Mio. Token) kamen aus dem Prompt-Cache.
- Rechenzeit ca. 5 h (ca. 18 Token/s Ausgabe), Wanduhrzeit ca. 6 h inkl. Pausen.
- Vergleich API-Preise (Stand Oktober 2026, gleiche Tokenmengen): Haiku 5.5 ≈ 0,34 $,
  Gemini 3.5 Flash-Lite ≈ 1,30 $, GPT-5.4 mini ≈ 2,77 $, Sonnet 5.5 ≈ 5,80 $,
  Opus 5.5 ≈ 11,59 $, GPT-5.5 ≈ 18,49 $ (ohne Cache das 3- bis 5-Fache).
  Programmierqualität von Qwen3.6-35B-A3B laut Benchmarks etwa auf dem Niveau von
  GPT-5.4 mini / Gemini 3.5 Flash-Lite.

## Akzeptierte Abweichungen und bekannte Schwächen

- **A1:** In RECOVER setzt `safetyStep` bei `TICK`, `HV_OFF`, `SHUTTER_CLOSE` zusätzlich
  `motor_stop = true`. Vom Header nicht verlangt, sicherheitlich unkritisch (Achse steht in RECOVER).
- **S1 Aufbau:** `safetyStep` enthält vor R1 Sonderblöcke (FAULT+TICK, `INIT_FAILED`,
  RUNNING+`INIT_DONE`, WAIT_ACK+`ACK`), die Teile von R1 und R7 duplizieren. Verhalten korrekt,
  aber Regeländerungen müssen an beiden Stellen nachgezogen werden. Abgesichert durch R1-Matrix
  und API-Tabelle. Bei der nächsten Änderung an `safety_logic.cpp` umbauen: erst R1, dann die
  Zustandsregeln, ohne Kopien. Ein Kommentar verweist auf eine nicht existierende „V1.5.5“.
- **S2:** Compiler-Warnung `-Wswitch` (RUNNING im `INIT_DONE`-switch nicht behandelt; wird vorher
  abgefangen).

## Erkenntnisse für die nächsten Sprints

1. Qwen erfüllt die Tests, nicht den Header-Text. Jede Regel, die kein Test prüft, ist ungesichert.
2. Hinweise ohne Test (z. B. `len < 8`) wurden zweimal übergangen; erst eine konkrete Codezeile im
   Prompt half.
3. Abschlussberichte ohne Gegenprüfung sind nicht verlässlich → SUMMARY-Zeilen wörtlich zitieren
   lassen und selbst nachprüfen.
4. Der Differenztest gegen die Referenz findet Lücken, die die eingefrorenen Tests nicht abdecken.
   Er wird für jeden Logik-Sprint wiederholt.
5. Ein Task pro Datei passt besser in 64k Kontext als mehrere Dateien gleichzeitig.
