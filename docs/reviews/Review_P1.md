# Review Sprint P1 – log_buffer, nvs_config, system

**Implementierung:** Qwen (Cline v4.1.24, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 15.10.2026 · Contract V1.5.15 · Branch `work-p1` → `master`, Tag `sprint-p1`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_log_buffer` | 12/12 (auch unter ThreadSanitizer ohne Befund) |
| `test_nvs_config` | 14/14 |
| `test_system` | 4/4 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien, Shims, `platformio.ini` | unverändert |
| `pio run -e firmware-partial -s` (echte ESP32-Header, Bericht Norbert) | erfolgreich (mit `src/partial_stub.cpp`) |
| Differenztest `logLine`/`logDump` (3 × 20 000 Schritte, zufällige Längen, Zeilenumbrüche, Puffergrößen) | identisch |
| Differenztest `nvsLoadParams`/`nvsSaveParams` (5 × 20 000 zufällige NVS-Inhalte mit falschen Typen, NaN, zu langen und ungültigen Texten, Schreib- und Öffnungsfehlern) | identisch |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Header bisher grob; Nachbildungen (Serial, ESP, Preferences, Watchdog, Reset-Grund, IDF-Version) mit echten Signaturen gebaut, eigene Referenz, 3 neue Suiten; Mutationstest: 49 Mutanten, 2 nicht durch Tests erkennbar | Contract V1.5.14 |
| P1.1–P1.3 (29/29) | ThreadSanitizer: `logDump` kopiert unter der Sperre nur Indizes und Längen und liest den Text danach ohne Sperre – vermischte Zeilen möglich. Kein Test bemerkte es (gleich lange Zeilen im Paralleltest). `firmware-partial` scheitert am Linker (`setup()`/`loop()` fehlen bis I1) | Contract V1.5.15: Test mit unterschiedlich langen, selbstprüfenden Zeilen (findet den Fehler 8/8 ohne Sanitizer), Platzhalter `src/partial_stub.cpp`, Aufgabe P1.4 mit Diagnose |
| P1.4 | Qwen arbeitete versehentlich ohne V1.5.15 (Bundle nicht übernommen) und erfand im Bericht Zahlen (`test_system 14/14`); die Korrektur selbst war richtig – nach dem Nachholen 12/12, TSan ohne Befund | Freigabe; README-Zeile von Claude korrigiert |

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| P1.1 log_buffer | 14 900 | 7 000 | 104 300 |
| P1.2 nvs_config | 25 200 | 11 300 | 105 200 |
| P1.3 system | 11 300 | 3 000 | 80 400 |
| P1.4 logDump unter der Sperre | 17 900 | 9 000 | 564 900 |
| **Summe P1** | **69 300** | **30 300** | **854 800** |

Kein Task abgebrochen. Vergleich (Ausgabe-Token / Referenzumfang): L5 36 400 / 180 Zeilen,
P1 30 300 / 160.

## Akzeptierte Abweichungen

- **A1:** `nvsSaveParams` bricht nach dem ersten fehlgeschlagenen `put*` ab (Referenz schreibt die
  übrigen Schlüssel noch). Rückgabe in beiden Fällen `false`; danach ist der NVS-Inhalt in beiden
  Fällen unvollständig.
- **A2:** `logDump` liest `lineCount` nach dem Verlassen der Sperre noch einmal (Schleifen über die
  kopierten Zeilen). `lineCount` ändert sich nur, solange weniger als `LOG_LINES` Zeilen geloggt
  wurden (erste Sekunden nach dem Start); ein `/log`-Abruf genau in dem Moment könnte eine nicht
  kopierte Zeile ausgeben. Nicht testbar ohne Prozess-Neustart; praktisch ohne Wirkung. Kandidat
  für eine spätere Bereinigung (Zähler und Index unter der Sperre in lokale Variablen kopieren).
- **A3:** `estop_nc` wird direkt mit `getBool` geladen statt über `paramSet` – gleichwertig (bool
  hat keine Bereichsregel).
- **A4:** `system.cpp` nutzt Designated Initializers für `esp_task_wdt_config_t` (C++20, von GCC
  in C++17 als Erweiterung akzeptiert; Target kompiliert ohne Warnung).

## Erkenntnisse

1. **Nachbildungen statt Hardware:** Mit Shims, die exakt die echten Signaturen haben, lässt sich
   ESP32-Code nativ und mit Sanitizern prüfen; der `firmware-partial`-Build bestätigt danach die
   echten Header.
2. **Nebenläufigkeit braucht eigene Werkzeuge:** Der einzige echte Fehler war ein Wettlauf. Grüne
   Tests und Differenztests (einfädig) fanden ihn nicht, nur ThreadSanitizer. Für P2 (Netzwerk-Task,
   HTTP-Server) gehört TSan fest ins Review, und Paralleltests brauchen selbstprüfende Daten.
3. **Ablauf-Fehler:** Ein nicht übernommenes Bundle führte dazu, dass Qwen gegen den alten Stand
   arbeitete – und Zahlen erfand, die zu keinem Stand passten. Vor jeder Aufgabe kurz
   `git log --oneline -1 master` prüfen.
