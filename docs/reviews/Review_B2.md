# Review Sprint B2 – Touch entprellen, Tastensperre

**Datum:** 21.11.2026 · **Umsetzung:** Qwen3.6-35B-A3B (Cline, lokal) · **Review:** Claude · **Contract:** V1.6.8

## Ergebnis: bestanden, ohne Korrektur

| Prüfung | Ergebnis |
|---|---|
| Geänderte Dateien | `src/logic/display_logic.cpp` (B2.1), `src/display/display.cpp` (B2.2), `README.md` |
| Eingefrorene Dateien | unverändert (`frozen 0` gegen `272c3c2`) |
| Native Suiten mit ASan/UBSan | **517/517** (22 Suiten; `test_display_logic` 18, `test_display` 18) |
| JS-Tests | **65/65** |
| Firmware-Build | bei Qwen `exit 0` |

Inhaltlich gleich der Referenz:

- `touchFilterUpdate()`: Kontakt sofort, Loslassen erst nach `release_ms`, Koordinaten ohne Kontakt
  nicht überschrieben, Zeitvergleich mit `(uint32_t)(now - last)`.
- `buttonLockAllow()`: STOP nie gesperrt und startet die Sperre neu; sonst Sperre nach `lock_ms`.
- `displayTick()`: entprellter Touch für Tasten, Flanke und ACK-Bereich; Tastensperre vor jedem
  Tastenbefehl, STOP mit `is_stop = true`. Die Sperre wird vor der HV_ON/HV_OFF-Entscheidung geprüft –
  gleichwertig zur Referenz.

## Token-Verbrauch (Norbert)

| Aufgabe | Eingabe | Ausgabe | Cache |
|---|---:|---:|---:|
| B2.1 | 9 700 | 1 600 | 47 300 |
| B2.2 | 24 300 | 4 400 | 528 300 |

B2.1 ist die bisher günstigste Aufgabe aller Sprints.

## Offen (Norbert, am Gerät)

- `upload`, dann die Prüfungen aus `docs/sprints/B2.md` („Nach dem Review“): M10, leichter/zittriger
  HV-Druck schaltet nur einmal, zweite Taste erst nach etwa 0,4 s (STOP sofort), ACK-Halten mit kurzem
  Lockern.
