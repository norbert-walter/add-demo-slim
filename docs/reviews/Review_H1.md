# Review Sprint H1 – hal_io, motion_drv

**Implementierung:** Qwen (Cline v4.1.24, lokal) · **Review:** Claude · **Gerätetest und Freigabe:** Norbert Walter
**Stand:** 16.10.2026 · Contract V1.5.19 · Branch `work-h1` → `master`, Tag `sprint-h1`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_motion_drv` | 10/10 (auch unter ThreadSanitizer ohne Befund) |
| `test_hal_io` | 16/16 (auch unter ThreadSanitizer ohne Befund) |
| alle 20 Suiten mit dem gesamten Qwen-Code L1–H1 | 439/439 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien, Shims, `platformio.ini` | unverändert |
| Compiler (`-Wall -Wextra -Wpedantic`) für `src/hal/*.cpp` | ohne Warnung |
| Differenztest `hal_io` (200 × 30 000 Schritte: zufällige Pegel, Zeitsprünge, uint32-Überlauf, Polaritätswechsel, erneutes `halInit`, Setter-Folgen, ADC; jeder aufgezeichnete Hardware-Zugriff) | identisch |
| Differenztest `motion_drv` (100 × 5 000 zufällige Anforderungen, Initialisierung mit Fehlern) | identisch |
| `pio run -e firmware-partial` (Bericht Norbert) | erfolgreich |
| **`test_hardware` am Gerät** (MKS DLC32, Dell, 16.10.2026) | **8/8** – CRC, Eingänge, Sensor, Ausgänge, Not-Aus-Polaritätswechsel, NVS, Motor 2 mm vor/zurück, `/proc`-JSON |
| `check_iram.py` beim Gerätebuild | 5/5 I2S-Interrupt-Funktionen im IRAM |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Header bisher grob; Nachbildungen für GPIO/ADC und FastAccelStepper (Signaturen der Version 96459ed, am Quelltext geprüft), die **jeden Hardware-Zugriff in Aufrufreihenfolge aufzeichnen**; eigene Referenz; 2 neue Suiten (26 Tests); Mutationstest 64 Mutanten, 3 überlebten → 1 Test + 1 Prüfung ergänzt, danach alle erkannt. Gerätetest `test_nvs_roundtrip` überschrieb die gespeicherten Parameter → stellt sie jetzt wieder her | Contract V1.5.18 |
| H1.1, H1.2 (26/26) | ohne Befund; `motion_drv` praktisch zeilengleich mit der Referenz | Gerätetest freigegeben |
| 1. Gerätetest | Container ohne USB-Zugriff → Dell. Danach: 5 Tests grün, dann Absturz in `test_nvs_roundtrip` und Neustart-Schleife („Cache disabled but cached memory region accessed“, PC in `StepperQueue::fill_i2s_buffer`). Ursache in **FastAccelStepper 96459ed**: Der I2S-Interrupt läuft im Mux-Modus ständig (auch bei stehendem Motor) und ist IRAM-sicher registriert, ruft aber zwei Funktionen im Flash auf. Jeder Flash-Zugriff (NVS, SPIFFS, OTA) stürzte ab | Contract V1.5.19: Fork `norbert-walter/FastAccelStepper` @ a64ceff (2 × `IRAM_ATTR`), Build-Prüfung `scripts/check_iram.py`; Pull-Request an gin66 |
| 2. Gerätetest (nach Flash-Löschen) | 8/8, sichtbare Funktionen (LEDs, Summer, Fahrt) wie erwartet | Freigabe |

**Wie der Fehler gefunden wurde:** Claude hat die ESP32-Toolchain in der eigenen Umgebung
eingerichtet, die Firmware gebaut und mit `nm`/`objdump` die Lage aller Funktionen geprüft, die
der Interrupt erreicht (Aufrufziele und Literale). Vor dem Fix lagen `fill_i2s_buffer`
(0x400d4508) und `I2sManager::i2sMuxSetBit` (0x401247fc) im Flash, danach im IRAM. Die
Build-Prüfung schlägt mit 96459ed an und läuft mit a64ceff durch.

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| H1.1 motion_drv | 15 100 | 4 800 | 140 200 |
| H1.2 hal_io | 73 400 | 25 500 | 571 200 |
| **Summe H1** | **88 500** | **30 300** | **711 400** |

Kein Task abgebrochen, keine Nacharbeit. Vergleich (Ausgabe-Token / Referenzumfang): P1 30 300 /
160 Zeilen, P2 109 000 / 234, H1 30 300 / 150. Bisher günstigster Sprint im Verhältnis zum Umfang.

## Akzeptierte Abweichungen

- **A1:** `halUpdateInputs` merkt sich die Polarität des Not-Aus (`used_high`) nur bei einer
  inaktiven Abtastung, die Referenz bei jeder. Bei einer aktiven Abtastung ist ohnehin keine
  Freigabe-Entprellung offen; der Differenztest (200 Folgen mit Polaritätswechseln) zeigt keinen
  Unterschied.
- **A2:** `hal_io.cpp` hat 200 Zeilen (Referenz 90) – ausgeschriebene Schritte und Kommentare,
  keine zusätzlichen Hardware-Zugriffe (die Nachbildungen würden jeden zählen).
- **A3:** README-Zeile H1 von Qwen mit verschobenen Spalten; beim Merge von Claude korrigiert.

## Erkenntnisse

1. **Aufzeichnende Nachbildungen wirken:** Die Lehre aus P2 (was die Nachbildung nicht zählt,
   prüft kein Test) wurde in H1 umgesetzt. Qwens Code war ohne einen einzigen Befund – die exakten
   Aufruffolgen ließen keinen Spielraum für Zusatzaufrufe.
2. **Nativ grün heißt nicht gerätefest:** Den schwersten Fehler bisher konnte kein Host-Test
   finden – er lag in einer Bibliothek und hängt am Flash-Cache des ESP32. Gefunden hat ihn erst
   der Gerätetest; eingegrenzt hat ihn die Absturzmeldung mit Exception-Decoder. Der Gerätetest
   gehört deshalb in jeden Hardware-Sprint, und `check_iram.py` sichert die Ursache dauerhaft ab.
3. **Gepinnte Bibliotheksstände:** Der Pin auf einen Master-Commit mit neuer Funktion (I2S_MUX)
   brachte einen Fehler mit, den der Autor offenbar noch nicht gesehen hatte. Der Fork mit
   Pull-Request hält die Änderung nachvollziehbar; sobald gin66 den Fix übernimmt, wird zurück
   auf das Original gewechselt.
4. **Ablauf:** Zwei Umwege (falscher Rechner, Pfad der Bundles) – Flash- und Gerätetests laufen
   künftig immer auf dem Dell; die Sprint-Dateien nennen das ab I1 ausdrücklich.
