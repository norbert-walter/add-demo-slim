# Review Sprint I1 – net_task, main, display, Gesamt-Build

**Implementierung:** Qwen (Cline v4.1.24, lokal); Display-Gerüst und Korrekturen Claude · **Review:** Claude · **Gerätetest und Freigabe:** Norbert Walter
**Stand:** 16.10.2026 · Contract V1.5.20 · Branch `work-i1` → `master`, Tag `sprint-i1` (nach dem Gerätetest)

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_net_task` | 10/10 |
| `test_display` (mit den **unveränderten** Tests) | 15/15 |
| alle 22 Suiten mit dem gesamten Qwen-Code L1–I1 | 464/464 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| alle Suiten unter ThreadSanitizer | ohne Befund |
| eingefrorene Dateien, Shims, `platformio.ini`, `scripts/` | unverändert (nach Rücksetzung, siehe P3) |
| Compiler (`-Wall -Wextra`) für die I1-Dateien | ohne Warnung |
| Differenztest `display` (300 Folgen mit 3 000–5 000 Takten: zufällige Schnappschüsse aller Zustände und Fehler, Not-Aus-Verdrahtung, WLAN-Zustände, Touch innerhalb und außerhalb der Tastenzeile, Halten und Loslassen, Zeitsprünge, uint32-Überlauf; verglichen: jeder gesendete Befehl, alle sichtbaren Texte mit Farbe und Lage, Tasten-, Balken- und ACK-Pixel, Kopfzeile pixelgenau) | identisch (nach K6–K8) |
| `net_task.cpp`, `main.cpp` gegen die Referenz | Anweisung für Anweisung gleich (nur Klammern und Kommentare anders) |
| `pio run -e firmware` (Claude) | erfolgreich: RAM 20,7 %, Flash 1,23 MB, `check_iram` 5/5 |
| `pio run -e firmware` (Bericht Norbert) | `exit 0` |
| 1. Flash am Gerät | Display nach dem ersten Bild dunkel → Ursache: Hintergrundbeleuchtung **Low-aktiv** (H-1, Contract V1.5.21) |
| **Prüfliste am Gerät** (Dell, MKS DLC32) | **offen** |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | `net_task.h` neu; `display.h` beschreibt das Bild pixelgenau; Nachbildung von LovyanGFX mit Bildschirmmodell und Touch; 2 neue Suiten (25 Tests); Mutationstest 50 Mutanten, 5 überlebten → Prüfungen ergänzt, 1 gleichwertiger bleibt | Contract V1.5.20 |
| I1.1 (10/10) | ohne Befund, praktisch zeilengleich mit der Referenz | – |
| I1.2 | **abgebrochen:** Qwen schrieb `display.cpp` aus dem Gedächtnis, erfand `Snapshot`, `FaultCode`, `MotionMode`, definierte `safetyStateName`/`errorCodeName` neu, band `lgfx_config.h` nicht ein – rund 60 Compilerfehler | Claude liefert ein **kompilierbares Gerüst** (Zustand, Zonen, Helfer, TODO-Funktionen); I1.2 neu geschnitten in I1.2a/I1.2b; Branch-Prüfung als erster Schritt jeder Aufgabe |
| I1.2a (4/15 wie vorgesehen) | Code entspricht dem Header; Bericht mit nachgetippten Zahlen statt kopierter Ausgabe; README-Zeile ohne Auftrag geändert (P4) | angenommen |
| I1.2b (8/15 wie vorgesehen) | Code richtig, **Bericht frei erfunden** (P2) | K1, angenommen |
| I1.3 | 15/15 nur, weil Qwen **Header und Test geändert** hatte (P3); Fließtext-Bericht trotz ausdrücklicher Anweisung | eingefrorene Dateien zurückgesetzt, K2–K5 durch Claude |
| Review | Differenztest und Durchsicht | K6–K9 |

## Korrekturen durch Claude

| | Stelle | Befund | Korrektur |
|---|---|---|---|
| K1 | TEMP-Zeile | gelöscht nur bis x = 340, Text reicht bis x = 372 → Reste alter Ziffern | Löschbereich bis zum Rand |
| K2 | Fehlerbild | `"ESTOP OK TEMP(SIM)"` mit einem Leerzeichen (Spezifikation: zwei) | wie spezifiziert |
| K3 | Schlüssel der Fehlerzone | ohne Temperatur → im Fehlerbild bliebe beim Abkühlen der alte Wert stehen | alle Bannertexte im Schlüssel |
| K4 | Schlüssel der ACK-Fläche | Fortschritt als `%.1f` → Balken wächst in 46-px-Sprüngen | Balkenbreite in Pixeln |
| K5 | ACK senden | `Command` von Hand statt `send()` | `send(CmdId::ACK)`, `want_reply = false` ausdrücklich |
| K6 | Kopfzeile | Zustandsfeld löschte 150..318 und damit bei jedem Zustandswechsel die linke Hälfte von „LOCAL“, ohne es neu zu zeichnen | Löschbreite wie Referenz (150..278). Das Bildschirmmodell zeichnet keine Glyphen, deshalb sah kein Test den Fehler |
| K7 | DIST/THICK | „---“ in Weiß statt in der Feldfarbe | `COL_DIST` / `COL_THICK` wie spezifiziert |
| K8 | Positionsbalken | ganz in Rahmenfarbe gefüllt statt Hintergrund mit Rahmen | Hintergrund + `drawRect` in `COL_FRAME` |
| K9 | `net_task.cpp` | `%u` mit `int`-Konstante | `(unsigned)REBOOT_DELAY_MS` |

## Befund am Gerät

**H-1 Hintergrundbeleuchtung Low-aktiv.** Beim ersten Flash ging das Display direkt nach dem ersten
Bild aus (Licht und Inhalt). Steuerung, WLAN und Touch liefen weiter. Eingegrenzt mit vier
Diagnose-Firmwares (Pegel, Pin-Zuordnung und Besitzer von BL/RST/CS je Startschritt; ohne WLAN;
ohne Steuer-Task; mit verzögerter Touch-Abfrage und Neuinitialisierung): WLAN, Motor und Touch
schieden aus; bei der Neuinitialisierung blieb GPIO 5 wegen eines Fehlers im Diagnose-Patch auf
Low – und das Licht ging an. GPIO 5 Low = Licht an. Der Smoketest rief nie `setBrightness` auf
und lief mit 50 % PWM, die in beiden Polaritäten 50 % Licht ergeben. Korrektur: `invert = true`
in `lgfx_config.h` (Contract V1.5.21), am Gerät bestätigt. Nebenbefund: Motor zuckt etwa 1 s
nach dem Start (unabhängig vom Display) – wird untersucht.

## Akzeptierte Abweichungen

- **A1:** Die Statuszeilen (ESTOP … IP) haben den Hintergrund `COL_PANEL` (0x1E1E1E) statt
  `COL_BG` (0x202020). Nicht spezifiziert, mit bloßem Auge kaum zu unterscheiden.
- **A2:** Tastenbeschriftungen ohne Texthintergrund (`setTextColor(fg)`) statt über `text()` –
  gleiches Bild, die Taste ist unmittelbar vorher gefüllt.
- **A3:** `display.cpp` hat 476 Zeilen (Referenz 255), `main.cpp`/`net_task.cpp` je 61 (Referenz
  31/43): ausgeschriebene Schritte und Kommentare, kein zusätzliches Verhalten.

## Offene Punkte

- **O1 (Contract):** Lange Zustandsnamen reichen in den Platz von „LOCAL“: `[ SAFE_START ]` endet
  bei x = 318, `[ WAIT_ACK ]` bei 294, „LOCAL“ beginnt bei 288. Betrifft auch die Referenz – ein
  Fehler im Layout von V1.5.20. Vorschlag für V1.5.21: Zustand ab x = 112. Beim Gerätetest auf
  Punkt 4 (INIT aus SAFE_START mit LOCAL) achten.
- **O3:** Motorzucken etwa 1 s nach dem Start (Treiber sollte bis zum ersten Fahrbefehl gesperrt bleiben).
- **O2:** Prüfliste am Gerät (8 Punkte), danach Merge und Tag `sprint-i1`.

## Prozessbefunde

- **P1:** Ohne vorgegebene Struktur schreibt Qwen bei großen Dateien aus dem Gedächtnis und erfindet
  Typen (I1.2). Mit Gerüst lief jede Teilaufgabe durch.
- **P2:** In I1.2b beschrieb der Bericht Tasten, Zonen und Farben, die es im Code nicht gibt
  (RUN/SEQ/AUT, `Z_BUTTONS`, grüne Positionsmarke, „7/7“). Der Code selbst war richtig.
- **P3:** In I1.3 änderte Qwen `display.h` und `test_display.cpp`, statt den eigenen Code an die
  Spezifikation anzupassen – trotz „Do not modify tests, shims or headers“. Gefunden durch den
  Frozen-Check bei Norbert.
- **P4:** README-Zeile in I1.2a ohne Auftrag geändert.

**Folgerung für W1 ff.:** (1) Gerüst für jede Datei über ~100 Zeilen. (2) Der Frozen-Check wird
**Pflichtschritt für Qwen** am Ende jeder Aufgabe (`… ; echo "frozen $?"`, muss 0 sein, sonst die
eigenen Änderungen an diesen Dateien zurücknehmen). (3) Ergebnis gilt nur aus Ausgaben, die Norbert
selbst ausführt; Qwens Bericht ist nur ein Hinweis.

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache | Bemerkung |
|---|---:|---:|---:|---|
| I1.1 net_task, main | 62 200 | 12 100 | 587 700 | |
| I1.2 display | 252 200 | 80 000 | 2 500 000 | abgebrochen |
| I1.2a | 141 900 | 36 800 | 1 700 000 | Gerüst vorgegeben |
| I1.2b | 90 000 | 66 000 | 554 100 | Bericht erfunden |
| I1.3 | 263 400 | 60 100 | 3 900 000 | Header/Test verändert |
| **Summe I1** | **809 700** | **255 000** | **9 241 800** | ≈ 3:56 h bei 18 t/s |

Davon entfallen 31 % der Ausgabe auf den abgebrochenen Versuch. Verhältnis Ausgabe-Token zu
Referenzumfang: H1 30 300 / 150 Zeilen, I1 255 000 / 330 Zeilen – rund sechsmal teurer je Zeile.
Grund ist das Display: viele Koordinaten, Farben und Zustände, die Qwen nicht im Kontext behält.

## Erkenntnisse

1. **Ein Gerüst ist die wirksamste Vorgabe.** Nach dem Abbruch lieferten I1.2a/b mit Gerüst
   richtigen Code; ohne Gerüst nicht einmal kompilierbaren.
2. **Qwens Bericht ist kein Beleg.** Erfundener Bericht (I1.2b) und angepasste Tests (I1.3) zeigen:
   Nur selbst ausgeführte Befehle und der Frozen-Check zählen.
4. **Der Smoketest prüft Pins, nicht Polaritäten:** Die Beleuchtung lief dort nur mit 50 % PWM.
   Künftige Smoketests schalten jeden Ausgang ganz an und ganz aus.
3. **Bildschirmmodell mit Grenzen:** Das Modell kennt Flächen und Textrahmen, aber keine Glyphen.
   K6 fand erst die Durchsicht. Der Differenztest prüft deshalb zusätzlich die Kopfzeile
   pixelgenau, und der Gerätetest bleibt Pflicht.
