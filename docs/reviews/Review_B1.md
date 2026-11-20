# Review Sprint B1 – Brownout-Schwelle, Startgrund, Startzähler

**Datum:** 20.11.2026 · **Umsetzung:** Qwen3.6-35B-A3B (Cline, lokal) · **Review:** Claude · **Contract:** V1.6.6

## Ergebnis: bestanden; eine kosmetische Korrektur von Claude

| Prüfung | Ergebnis |
|---|---|
| Geänderte Dateien | `src/logic/types.cpp`, `src/system/system.cpp`, `src/hal/hal_io.cpp` (B1.1), `src/logic/proc_json.cpp`, `src/comm/http_server.cpp` (B1.2), `README.md` |
| Eingefrorene Dateien | unverändert (`frozen 0` gegen `a200241`) |
| Native Suiten mit ASan/UBSan | **508/508** (22 Suiten) |
| JS-Tests | **65/65** (`GET_STATE`-Felder in `openapi.json` = Firmware) |
| Simulator (`sim_device.py`) | baut |
| Firmware-Build | bei Qwen `exit 0` |

Inhaltlich gleich der Referenz:

- `systemInit()`: Brownout-Schwelle als erste Anweisung mit `REG_SET_FIELD` (andere Bits des Registers
  bleiben), Startmeldung mit dem **zurückgelesenen** Wert, Zähler, `BootInfo`, Logzeile, Watchdog – in
  der Reihenfolge von `system.h`.
- Zähler `RTC_NOINIT_ATTR` ohne Initialisierung im Code. Geprüft: Für `BootCounters` (Aggregat mit
  konstanten Vorgabewerten) erzeugt der Compiler keinen Laufzeit-Konstruktor; die Werte überleben also
  einen Reset. Am Gerät mit EN-Taste und `REBOOT` bestätigen.
- Kein NVS-Zugriff beim Start; der Flash wird durch B1 nicht zusätzlich beschrieben.
- `hal_io.cpp`: Treiberstufe 0 für GPIO 32 und 4 nach Schritt 1, vor Schritt 2.
- `proc_json.cpp`: vier Felder in der vorgegebenen Reihenfolge, Puffergrenzen wie zuvor.

**Korrektur (Claude):** In `types.cpp` stand die Abschnittsüberschrift „ApiError messages“ doppelt
(eine Kopie über dem neuen Abschnitt); entfernt. Kein Einfluss auf den Code.

## Token-Verbrauch (Norbert)

| Aufgabe | Eingabe | Ausgabe | Cache |
|---|---:|---:|---:|
| B1.1 | 49 600 | 7 700 | 922 300 |
| B1.2 | 18 900 | 3 400 | 395 600 |

Ohne Abbruch und ohne Nacharbeit – einer der günstigsten Sprints bisher.

## Offen (Norbert, am Gerät)

- `upload` und `uploadfs`, dann die Prüfungen aus `docs/sprints/B1.md` („Nach dem Review“):
  Startmeldung `Brownout level 7`, `/log` mit `boot: reset POWERON, boots 1 …`, EN-Taste → `EXT`,
  `REBOOT` → `SW`, `GET_STATE` mit den vier Feldern, `check_add.py`.
- Wiederholmessung der Abnahme mit dem umgebauten Board (Hardware-Doku 1.2).
