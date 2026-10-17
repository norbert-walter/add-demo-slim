# Review Sprint W1 – Weboberfläche (Process View, Settings)

**Datum:** 18.10.2026 · **Umsetzung:** Qwen3.6-35B-A3B (Cline, lokal) · **Review:** Claude
**Dateien:** `data/www/app.js`, `data/www/index.html`, `data/www/settings.html` (alle drei als Gerüst
vorgegeben) · **Contract:** V1.5.22 (§5.5), Stand V1.5.25

## Ergebnis

**Bestanden nach zwei kleinen Korrekturen durch Claude (K1, K2).** `app.js` ist auf gültigen Eingaben
gleichwertig zur Referenz; die beiden Seiten erfüllen alle Prüfpunkte der Browser-Prüfung. Gefunden
hat die Fehler erst eine zusätzliche Offline-Prüfung – die Tests und die bisherige Browser-Prüfung
waren grün.

| Prüfung | Ergebnis |
|---|---|
| JS-Tests (`test_app.js` 27, `test_pages.js` 10) | 37/37 |
| Eingefrorene Dateien (Header, Tests, CONTRACT, `style.css`, Schriften, `fake_device.py`); Markup und Helfer der Seiten per SHA-256 | unverändert (`frozen 0`) |
| Geänderte Dateien | nur `app.js`, `index.html`, `settings.html`, `README.md` |
| Differenztest `app.js` gegen Referenz, 17 Funktionen × 20 000 zufällige gültige Eingaben (`/proc`-Objekte, gerahmte Antworten, Formulare, Validierungsdaten) | identisch |
| Browser-Prüfung gegen `fake_device.py` (Chromium, 53 Prüfpunkte, 4 Themes, 400 px Breite) | 0 Fehler |
| Offline-Prüfung (neu): `/api` antwortet nicht | vor K1/K2: 4 Fehler, danach 0 |
| Native Suiten, Firmware-Build | unverändert (keine Änderung in `src/`): 479/479, `check_iram` 5/5 |
| SPIFFS-Abbild (`buildfs`) | erstellt, Webdateien 204 kB von 1088 kB |

## Korrekturen durch Claude

| # | Datei | Befund | Korrektur |
|---|---|---|---|
| K1 | `index.html`, `api()` | Bei Netzwerkfehler, HTTP-Fehler oder CRC-Fehler kam **kein** Toast – STOP ohne Verbindung blieb ohne jede Rückmeldung. Außerdem wurde bei HTTP 400 die gerahmte Fehlerantwort (BAD_CRC, BAD_JSON) verworfen. Der Kommentar verlangt den Toast `no reply` auch in diesen Fällen | Antwort bei jedem HTTP-Status lesen, Toast immer (außer `quiet`) |
| K2 | `settings.html`, `api()` | Kein `try/catch`: ein Netzwerkfehler wurde zur unbehandelten Ausnahme. Folgen: Status blieb „Online“, kein Toast `GET_CONFIG failed`, beim Speichern brachen weitere SETs stumm ab | `fetch` in `try/catch`, Antwort `null` → Status „Offline“ und Toast |

## Akzeptierte Abweichungen

- `index.html`, `poll()`: ein HTTP-Fehler von `/proc` zählt als „offline“ statt „CRC-Fehler“.
  `/proc` liefert auf dem Gerät immer 200; ohne Auswirkung.
- `app.js` bei Eingaben außerhalb der Spezifikation (`undefined` statt `null`, Zeichenkette statt
  Zahl): Qwen prüft genau auf `null`, die Referenz auf „falsy“. Die Seiten übergeben nur `null` oder
  vollständige Objekte; ohne Auswirkung.

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| W1.1 app.js Protokoll | 17 600 | 4 800 | 254 100 |
| W1.2 app.js Prozessansicht | 34 000 | 12 500 | 543 200 |
| W1.3 app.js Einstellungen | 16 700 | 4 800 | 136 300 |
| W1.4 index.html | 33 200 | 7 400 | 441 100 |
| W1.5 settings.html, Gesamtprüfung | 30 400 | 8 300 | 823 900 |
| **Summe W1** | **131 900** | **37 800** | **2 198 600** |

Etwa 35 min Ausgabe bei 18 t/s, kein Abbruch, keine veränderte eingefrorene Datei. Zum Vergleich I1:
255 000 Ausgabe-Token mit einem abgebrochenen Versuch und zwei Eingriffen in eingefrorene Dateien.

## Prozessbefunde

- Qwen hielt die Regel „nur kopierte Ausgabezeilen“ nur bei W1.3 ein; sonst Tabellen und eigene
  Beschreibungen. Zweimal stammten die Zahlen aus Teilläufen (`--test-name-pattern`), nicht aus dem
  Gesamtlauf. Norberts eigene Prüfung nach jeder Aufgabe bleibt deshalb Pflicht.
- Meine Erwartungswerte nach W1.1 waren falsch (19/18 statt 17/20): zwei `w11`-Tests prüfen die
  vorgegebenen Helfer und waren schon im Gerüst grün. Künftig die Erwartung je Aufgabe aus dem
  Gerüst ableiten, nicht aus der Testanzahl.

## Erkenntnisse

1. **Gerüste wirken weiter.** Wie in I1.2a/b: Mit vorgegebenem Markup, Helfern und genauen
   Funktionskommentaren lieferte Qwen ohne Abbruch korrekten Code – bei einem Fünftel der Token von I1.
2. **Fehlerpfade prüft niemand von selbst.** Tests und Browser-Prüfung deckten den Gutfall ab; beide
   Fehler lagen im Fall „Gerät antwortet nicht“. Die Offline-Prüfung gehört ab jetzt fest in die
   Browser-Prüfung, ebenso ein Test für jede Fehlerverzweigung, die ein Kommentar beschreibt.
3. **Der Differenztest braucht realistische Eingaben.** Mit beliebigem Unsinn (`undefined`, falsche
   Typen) meldete er 6 500 Unterschiede, die alle außerhalb der Spezifikation lagen; mit gültigen
   Eingaben 0. Er muss die Schnittstelle so bespielen, wie die Seiten sie nutzen.

## Offen (Gerät)

Flash von Firmware **und** Dateisystem und die Prüfliste am Gerät (`docs/sprints/W1.md`, Abschnitt
„Flash auf dem Dell“).
