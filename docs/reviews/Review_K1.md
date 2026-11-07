# Review Sprint K1 – Kalibriertabelle mit 6 Stufen

**Datum:** 08.11.2026 · **Umsetzung:** Qwen3.6-35B-A3B (Cline, lokal) · **Review:** Claude
**Dateien:** `src/logic/types.cpp`, `api_parse.cpp`, `app_logic.cpp`, `measure_logic.cpp`, `params.cpp`,
`src/system/nvs_config.cpp`, `data/www/app.js` (+ eingebettete Kopie, `web-ui/`, README)
**Contract:** V1.6.0 → V1.6.2 (Review)

## Ergebnis

**Bestanden nach einer kleinen Korrektur durch Claude (K1-1).** Alle vier Aufgaben liefen ohne Abbruch
und ohne Kontext-Kompression durch – der bisher sparsamste Sprint mit Logikänderungen.

| Prüfung | Ergebnis |
|---|---|
| Geänderte Dateien | nur die zehn erwarteten |
| Eingefrorene Dateien (Header, Tests, CONTRACT, Skripte, `doc.html`, `settings.html`) | unverändert (`frozen 0`) |
| `index.html` gegenüber der Vorgabe | nur der ADDAPP-Block (Kopie von `app.js`) geändert; `embed_app.py` erneut ausgeführt: keine Änderung |
| Native Suiten mit ASan/UBSan | 495/495; nach K1-1 mit neuem Test **496/496** |
| JS-Tests (`test_app` 31, `test_pages` 10) | 41/41 |
| Differenztest `calibValid`/`calibApply`, 4000 zufällige Tabellen (gestört, permutiert, Grenzfälle 0,99/1,0/1,01 mm und 40/300 mm), 18 Messwerte je Tabelle | Gültigkeit identisch zur Referenz (C++ und JS, 1216 ungültige); Werte nur Rundungsunterschiede ≤ 0,002 mm; C++ gegen JS ≤ 0,02 mm (float/double bei 400 mm Extrapolation) |
| Browserprüfung gegen `sim_device.py` mit Qwens Logik | automatischer Lauf 6/6 in 31 s und gespeichert, STOP bricht ab, Einzelpunkt, ungültige Tabelle → `CALIB_INVALID`, Demo-Modus; keine Skriptfehler |
| Abnahmeskript T05/T18/T20 gegen den Simulator | T18 ✅ (neu ±0,5 mm); T20 ⛔ bei Werkstabelle (gewollt), ✅ mit verstellter Referenz 55,4 mm (Abweichung 0,4 mm erkannt) |
| Firmware-Build | bei Qwen `exit 0` (K1.3); im Container von Claude nicht möglich (PlatformIO-Registry gesperrt) |

## Korrektur durch Claude

| # | Datei | Befund | Korrektur |
|---|---|---|---|
| K1-1 | `api_parse.cpp`, `CALIB point` | `index` wurde als `float` gelesen und vor der Bereichsprüfung nach `int` gewandelt: `index: 3e9` ist undefiniertes Verhalten (UBSan `float-cast-overflow`; Ergebnis zufällig `BAD_PARAM`), `3.0000001` und `2.9999999` wurden als 3 angenommen. Die Tests prüften nur 3.5 und 1e9 | als `double` lesen, Bereich 1 … 6 **vor** der Wandlung prüfen, Ganzzahligkeit in `double`; neuer Test `test_v162_calib_index_exact_and_huge` (erkennt Qwens Fassung) |

## Hinweise ohne Korrektur

- `calibApply` sortiert vor der Gültigkeitsprüfung (unnötig, harmlos).
- `nvs_config.cpp`: `bool persist;` ohne Anfangswert – wird nur von `paramSet` geschrieben und nie
  gelesen, unbedenklich.
- `app.js`: `calibRows` meldet auch `Infinity` als `null` (strenger als der Kommentar, unschädlich).

## Token-Verbrauch (Norbert)

| Aufgabe | Eingabe | Ausgabe | Cache |
|---|---:|---:|---:|
| K1.1 | 20 200 | 3 700 | 255 100 |
| K1.2 | 24 400 | 9 300 | 435 100 |
| K1.3 | 28 000 | 7 400 | 479 500 |
| K1.4 | 32 600 | 9 900 | 685 600 |
| **Summe** | **105 200** | **30 300** | **1 855 300** |

Zum Vergleich: L4 brauchte bis zum Abschluss über 21 Mio. Cache-Token mit vier Abbrüchen.

## Nachgezogen (Claude)

- Contract V1.6.2, Software-Dokumentation 1.1 (3.4, Parameter, `GET_CONFIG`, 12.3 neu, Testzahlen),
  REST-API 1.1 (§2, §3, §4), Endpunkte, Hardware-Dokumentation 1.1 (6.3, Drehrichtung).
- Abnahmeskript: T18 Abstand jetzt ±0,5 mm; T20 ±0,5 mm, ⛔ bei Firmware ohne Tabelle oder bei
  unveränderter Werkstabelle.
