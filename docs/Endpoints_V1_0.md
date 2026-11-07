# HTTP-Endpunkte – Kurzübersicht

**Demoanlage ADD (Slim)** · Version 1.0 · Stand 2026-11-07 · Contract V1.5.26
Befehle für `/api`: `docs/REST_API_V1_0.md` · ausführlich: `docs/Software_Dokumentation_V1_0.md`
Kapitel 10 · verbindlich: `src/comm/http_server.h`

---

## 1. Erreichbarkeit

| Netz | Adresse | Bemerkung |
|---|---|---|
| eigener Access Point | `http://192.168.4.1` | SSID `ADD-Demo`, immer aktiv |
| Router-WLAN | `http://<IP vom Router>` | IP im Display, in `/log` oder mit dem Konsolenbefehl `status` |
| mDNS | `http://add-demo.local` | in beiden Netzen, wenn der Client mDNS unterstützt |
| Port | 80 | nur HTTP, kein HTTPS, keine Anmeldung |

---

## 2. Übersicht

| Methode | Pfad | Inhalt | Content-Type | Cache | Status |
|---|---|---|---|---|---|
| GET | `/` | Weboberfläche (IMS-Bedienoberfläche), Datei `/www/index.html` | `text/html` | – | 200 |
| GET | `/settings` | Weiterleitung auf `/#settings`, Datei `/www/settings.html` | `text/html` | – | 200 |
| GET | `/proc` | Prozessabbild mit CRC | `application/json` | `no-store` | 200 |
| POST | `/api` | Befehl mit CRC → Antwort mit CRC | `application/json` | – | 200 / 400 |
| OPTIONS | `/api` | CORS-Preflight | – | – | 204 |
| GET | `/log` | letzte 20 Logzeilen | `text/plain` | – | 200 |
| GET | `/llms.txt` | Einstieg für Sprachmodelle (4 Zeilen) | `text/plain` | – | 200 |
| GET | `/add` | ADD-Dokument, Datei `/add.json` | `application/json` | `no-store` | 200 / 404 |
| GET | `/add.html` | ADD-Dokument in HTML eingebettet | `text/html; charset=utf-8` | `no-store, no-cache, must-revalidate` | 200 / 404 |
| GET | `/doc` | Gerätehandbuch, Datei `/doc.md` | `text/markdown` | – | 200 / 404 |
| GET | `/app.js` | ADDAPP: CRC, Rahmen, Regeln für eigene Seiten | `application/javascript` | – | 200 |
| GET | `/font-u400.woff2` `/font-u700.woff2` `/font-m400.woff2` `/font-m700.woff2` | Schriften Ubuntu / Ubuntu Mono | `font/woff2` | `max-age=86400` | 200 |
| GET | `/style.css` | entfallen seit V1.5.26 (Datei entfernt) | `application/json` | – | 404 |
| beliebig | andere | `{"error":"not found"}` | `application/json` | – | 404 |

**Für alle Antworten:** Header `Access-Control-Allow-Origin: *` – Seiten auf anderen Rechnern oder
als lokale Datei geöffnete Seiten dürfen die Anlage abfragen.

**Stand Sprint D1:** `/add.json` und `/doc.md` gibt es noch nicht; `/add`, `/add.html` und `/doc`
antworten bis dahin mit 404.

---

## 3. Endpunkte im Einzelnen

### 3.1 `GET /proc` – Prozessabbild

| Eigenschaft | Wert |
|---|---|
| Inhalt | eine Zeile JSON, ≤ 512 Byte, feste Feldreihenfolge, CRC als letztes Feld |
| Abfragetakt | alle 500 ms; nächste Anfrage erst nach Ende der vorigen |
| Cache umgehen | Zeitstempel anhängen: `/proc?t=<ms>` (Parameter wird ignoriert) |
| Watchdog | hält den Kommunikations-Watchdog **nicht** am Leben (dafür `GET_STATE` über `/api`) |
| Felder | `ts`, `state`, `err`, `local`, `motion{…}`, `measure{…}`, `safety{…}`, `crc` – Tabelle in `REST_API_V1_0.md` Abschnitt 6 |

### 3.2 `POST /api` – Befehle

| Eigenschaft | Wert |
|---|---|
| Inhalt der Anfrage | `{"cmd":"<NAME>","params":{…},"crc":"XXXXXXXX"}`, max. 512 Byte |
| Content-Type der Anfrage | beliebig; Browser: `text/plain` (kein Preflight) |
| HTTP 200 | alle Antworten außer den beiden folgenden, auch Fehler wie `STATE` oder `LOCAL_CONTROL` |
| HTTP 400 | `BAD_CRC` (−1) und `BAD_JSON` (−2, auch: leer oder > 512 Byte) |
| Antwortzeit | typisch 2 ms im Regel-Task; nach 100 ms ohne Antwort `BUSY` |
| lokal beantwortet | `GET_STATE`, `GET_CONFIG` (ohne Regel-Task) |
| Watchdog | jedes Paket mit gültiger CRC und bekanntem Befehl zählt als Lebenszeichen |
| Befehle, Parameter, Fehlercodes | `REST_API_V1_0.md` |

### 3.3 `OPTIONS /api` – CORS-Preflight

| Header der Antwort | Wert |
|---|---|
| Status | 204, leerer Inhalt |
| `Access-Control-Allow-Origin` | `*` |
| `Access-Control-Allow-Methods` | `GET, POST, OPTIONS` |
| `Access-Control-Allow-Headers` | `Content-Type` |

### 3.4 `GET /log` – Geräte-Log

| Eigenschaft | Wert |
|---|---|
| Inhalt | die letzten 20 Zeilen, älteste zuerst, je Zeile ein Ereignis |
| auch seriell | jede Zeile erscheint auch auf der USB-Konsole |

| Typische Zeile | Bedeutung |
|---|---|
| `boot: ready, FW 1.0.0` | Start abgeschlossen |
| `ap: ADD-Demo 192.168.4.1` | Access Point aktiv |
| `sta: attempt 1/3` · `sta: connected <IP>` · `sta: backoff 120 s` · `sta: connection lost` | Router-WLAN |
| `nvs: saved` · `nvs: save deferred until axis idle and HV off` | Parameter gespeichert / Speichern verschoben |
| `reboot: in 700 ms` | Neustart folgt |
| `comm: /api gap … ms` · `comm: timeout, …` | Pause zwischen API-Paketen / Watchdog ausgelöst |
| `ctrl: cycle … us, period … us (…)` | langer Regelzyklus (höchstens alle 5 s) |
| `net: http … ms` · `net: gap … ms` | langsamer Schritt im Netz-Task |
| `http: idle connection closed after 500 ms (… so far)` | wartende Browser-Verbindung getrennt (höchstens alle 10 s) |

### 3.5 `GET /llms.txt` – Einstieg für Sprachmodelle

```
# AI Device Description (ADD)
- ADD (JSON): [Device Description](/add)
- ADD (HTML): [Device Description for Cloud-AI](/add.html)
- Documentation: [Device manual](/doc)
```

### 3.6 `GET /add`, `/add.html`, `/doc` – ADD-Dokument und Handbuch

| Pfad | Für wen | Aufbau |
|---|---|---|
| `/add` | KI-Agenten mit HTTP-Zugriff | ADD-Dokument nach ADD Specification v1.0, unverändert aus `/add.json` |
| `/add.html` | Cloud-KIs, die nur Webseiten lesen | dasselbe JSON zweimal: in `<script type="application/ld+json">` (mit `</` als `<\/`) und lesbar in `<pre>` (mit `&amp; &lt; &gt;`); in Blöcken gesendet |
| `/doc` | Menschen und KI | Gerätehandbuch in Markdown aus `/doc.md` |

### 3.7 Weboberfläche und Dateien

| Pfad | Größe | Bemerkung |
|---|---:|---|
| `/` | 105 kB | eine Seite für alles; enthält CSS und eine Kopie von `app.js`. Während des Ladens wartet `/api` bis zu ca. 1 s (eine Verbindung zur Zeit) |
| `/settings` | < 1 kB | alte Adresse der Einstellungsseite, leitet auf `/#settings` weiter |
| `/app.js` | 18 kB | für eigene Seiten (`ADDAPP.apiBody`, `ADDAPP.parseFramed` …) |
| `/font-*.woff2` | 26–35 kB je Datei | werden einen Tag im Browser zwischengespeichert |

Seitenadressen der Weboberfläche: `/#process` · `/#status` · `/#settings` · `/#calibration` ·
`/#log`.

---

## 4. Verhalten des Webservers

| Thema | Verhalten |
|---|---|
| Verbindungen | eine Verbindung zur Zeit; weitere warten in der Warteschlange (max. 4) |
| wartende Verbindungen | Verbindungen ohne Anfrage werden nach 500 ms getrennt (Browser öffnen Reserve-Verbindungen) |
| Zeitlimit | eine angefangene Anfrage wird höchstens 5 s abgewartet |
| Ausführung | im Netz-Task auf Kern 0 – der 1-ms-Regelzyklus auf Kern 1 wird davon nicht gebremst |
| Sicherheit | keine Anmeldung, kein HTTPS: jeder im WLAN kann alle Endpunkte benutzen (Hardware-Dokumentation 2.3) |

---

## 5. Benutzung

| Werkzeug | Beispiel |
|---|---|
| Browser | `http://192.168.4.1/` |
| curl | `curl -s http://192.168.4.1/proc` |
| curl (Befehl) | `curl -s -X POST --data '{"cmd":"GET_STATE","params":{},"crc":"4705FDB9"}' http://192.168.4.1/api` |
| Python-Client | `python3 scripts/add_client.py --host 192.168.4.1 proc` |
| Python-Client (Befehl) | `python3 scripts/add_client.py --host 192.168.4.1 cmd GET_STATE` |
| Python-Konsole | `python3 scripts/add_client.py --host 192.168.4.1 shell` (dort auch `get /log`, `get /llms.txt`) |
| ohne Hardware | `python3 scripts/fake_device.py 8080`, dann `http://localhost:8080/` |

```
curl -s http://192.168.4.1/log
```
```
curl -s http://192.168.4.1/llms.txt
```
```
curl -s -D - http://192.168.4.1/proc
```
(`-D -` zeigt zusätzlich die Header, z. B. `Cache-Control` und `Access-Control-Allow-Origin`.
`curl -I` funktioniert nicht: der Server beantwortet nur GET, POST und OPTIONS.)
