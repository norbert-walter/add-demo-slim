# Review Sprint K2 – Kalibrierung unter Messbedingungen

**Datum:** 08.11.2026 · **Umsetzung:** Qwen3.6-35B-A3B (Cline, lokal) · **Review:** Claude · **Contract:** V1.6.3

## Ergebnis: bestanden, ohne Korrektur

| Prüfung | Ergebnis |
|---|---|
| Geänderte Dateien | `src/logic/app_logic.cpp` (eine Zeile, identisch mit der Referenz), `README.md` |
| Eingefrorene Dateien | unverändert (`frozen 0` gegen `0696e0a`) |
| Native Suiten mit ASan/UBSan | **499/499** |
| JS-Tests | 41/41 |
| Abnahme T18K, T18, T19, T20 gegen `sim_device.py` | alle ✅ |
| Browserprüfung der Seite Kalibrierung | automatischer Lauf 6/6 mit HV und offenem Shutter, gespeichert, HV danach wieder aus; STOP bricht ab; „Messen“ (155 mm, Shutter offen) + „Übernehmen“; ungültige Tabelle → `CALIB_INVALID` |
| Firmware-Build | bei Qwen `exit 0` |

**Hinweis zum Ablauf:** Der erste Frozen-Check im Container meldete `frozen 1`, weil das lokale `master`
dort noch auf dem K1-Vorbereitungsstand stand. Gegen `origin/master` war alles unverändert. Im Container
deshalb gegen `origin/master` prüfen bzw. `master` vorher nachziehen.

## Token-Verbrauch (Norbert)

| Aufgabe | Eingabe | Ausgabe | Cache |
|---|---:|---:|---:|
| K2.1 | 29 400 | 3 100 | 163 600 |

## Offen

- Flashen, Abnahme T18K–T20 am Gerät (Norbert)
- Ursache des Shutter-Einflusses auf das Sensorsignal in der Elektronik (Norbert)
- T29: 51 ms Zykluslücke beim NVS-Speichern
