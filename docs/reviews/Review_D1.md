# Review Sprint D1.1 – Route `/openapi.json`

**Datum:** 08.11.2026 · **Umsetzung:** Qwen3.6-35B-A3B (Cline, lokal) · **Review:** Claude · **Contract:** V1.6.4

## Ergebnis: bestanden, ohne Korrektur

| Prüfung | Ergebnis |
|---|---|
| Geänderte Dateien | `src/comm/http_server.cpp` (Route `/openapi.json`, Zeile 4 in `/llms.txt` – identisch mit der Referenz), `README.md` |
| Eingefrorene Dateien | unverändert (`frozen 0` gegen `cb8e5fe`) |
| Native Suiten mit ASan/UBSan | **500/500** |
| JS-Tests | 63/63 (davon `test_add.js` 22) |
| Firmware-Build | bei Qwen `exit 0` |

Die Dokumente `data/add.json` und `data/openapi.json` sowie `check_add.py` stammen von Claude und waren
schon vor D1.1 gegen den Simulator geprüft (`docs/sprints/D1.md`).

## Token-Verbrauch (Norbert)

| Aufgabe | Eingabe | Ausgabe | Cache |
|---|---:|---:|---:|
| D1.1 | 12 900 | 3 700 | 219 100 |

## Offen

- Flashen und `uploadfs`, dann `check_add.py` lokal und über die Internetadresse (Norbert)
- `/add.html` mit dem 34-kB-Dokument am Gerät (Ladezeit, Blöcke)
- Rückmeldung zu den Annahmen im Entwurf: Basic Auth auf allen Pfaden, `doc_url` auf den Contract, `max_response_time` 5 s
- KI-Validierung nach ADD Spec §13 (Block 9), danach Demo-Skripte

---

# Review Sprint D1.2 – `/add.html` als Datei, gzip-Kopien, `/favicon.ico`

**Datum:** 10.11.2026 · **Umsetzung:** Qwen3.6-35B-A3B · **Review:** Claude · **Contract:** V1.6.5

## Ergebnis: bestanden; eine Ergänzung von Claude

| Prüfung | Ergebnis |
|---|---|
| Geänderte Dateien | nur `src/comm/http_server.cpp` (README nicht – Cline hing vorher, siehe unten) |
| Umsetzung | wie im Header: `sendFile` mit gzip-Kopie, `Vary`, keine eigene `Content-Encoding`; `collectHeaders` vor den Routen; `/add.html` als Datei; `sendCached` für Schriften und `/favicon.ico`; Laufzeit-Erzeugung entfernt (−112 Zeilen) |
| Eingefrorene Dateien | unverändert (`frozen 0`) |
| Native Suiten mit ASan/UBSan | **500/500**; `test_http_server` zehnmal hintereinander 22/22 |
| JS-Tests | 64/65 mit Qwens Stand: `/favicon.ico` fehlte in `openapi.json` – **Claudes Versäumnis** in der Vorbereitung (der Test prüft jede Firmware-Route). Ergänzt, `openapi.json.gz` neu → **65/65**, OpenAPI gültig |
| Firmware-Build | bei Qwen `exit 0`, 1 255 491 Bytes |

**Warum Cline hing:** Im Auftrag stand `pio test -e native -f test_http_server`. PlatformIO nennt die Suite
`native/test_http_server`; der Filter fand nichts („0 test cases“), und Qwen wartete auf „22/22“. Künftig
`-f "native/<suite>"`.

## Token-Verbrauch (Norbert)

| Aufgabe | Eingabe | Ausgabe | Cache |
|---|---:|---:|---:|
| D1.2 | 35 000 | 8 400 | 441 500 |

## Offen

- Flashen, `uploadfs`, dann `check_add.py --auth` und `diag_http.py` (Norbert): `/add.html` < 3 s, Favicon im Tab
- Warnungen der KI-Validierung vom 09.11.2026 abarbeiten, Validierung wiederholen
- Demo-Skripte
