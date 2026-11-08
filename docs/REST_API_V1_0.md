# REST-API – Kurzübersicht

**Demoanlage ADD (Slim)** · Version 1.1 · Stand 2026-11-08 · Contract V1.6.4
Ausführlich: `docs/Software_Dokumentation_V1_0.md` Kapitel 9–11, verbindlich: `docs/CONTRACT.md` §4–§5

---

## 1. Grundlagen

| Thema | Regel |
|---|---|
| Adresse | `http://192.168.4.1` (eigener Access Point `ADD-Demo`), im Router-WLAN die vergebene IP oder `http://add-demo.local` |
| Befehle | `POST /api`, Inhalt: `{"cmd":"<NAME>","params":{…},"crc":"XXXXXXXX"}`, max. 512 Byte, `params` optional |
| Zustand lesen | `GET /proc` – alle 500 ms, nächste Anfrage erst nach Ende der vorigen |
| Prüfsumme | CRC32 (IEEE/zlib) über den JSON-Text **ohne** crc-Feld; `,"crc":"…"` vor die letzte `}` setzen. Gilt auch für alle Antworten und `/proc` |
| Content-Type | beliebig; Browser: `text/plain` (kein CORS-Preflight). Jede Antwort hat `Access-Control-Allow-Origin: *` |
| Antwort | `{"status":"ok","err":0,"cmd":"…","crc":"…"}` oder `{"status":"error","err":-6,"cmd":"…","msg":"…","crc":"…"}`; HTTP 200, nur `BAD_CRC`/`BAD_JSON` mit HTTP 400 |
| `OK` bedeutet | angenommen, Ausgänge gesetzt; Bewegungen laufen weiter – Ende in `/proc` (`motion.state`, `motion.mode`) |
| Heartbeat | solange `safety.comm_armed = true`: mindestens alle 4 s, empfohlen jede Sekunde `GET_STATE`; sonst nach 5 s Fehler `COMM_TIMEOUT`. `GET /proc` zählt nicht |
| Wiederholen | nie automatisch; bei `BUSY` erst `/proc` lesen, nur bei fehlender Wirkung erneut senden |

**Beispiel Rahmen:**

| Text ohne crc | gesendet (crc vor der **letzten** `}`) |
|---|---|
| `{"cmd":"HV_ON","params":{}}` | `{"cmd":"HV_ON","params":{},"crc":"88F7C802"}` |
| `{"cmd":"SET","params":{"key":"temp","value":35.0}}` | `{"cmd":"SET","params":{"key":"temp","value":35.0},"crc":"C93F2D33"}` |

---

## 2. Befehle

**Zustände:** S = SAFE_START · R = READY · RUN = RUNNING · W = WAIT_ACK · alle = jeder Zustand
**Prio** = eigene Prioritäts-Warteschlange, wird im nächsten Zyklus ausgeführt · **WD** = macht den Kommunikations-Watchdog scharf

| Befehl | Parameter | Erlaubt in | Prio | WD | Wirkung |
|---|---|---|:-:|:-:|---|
| `INIT` | – | S, R | | ✓ | Referenzfahrt (5 mm/s zum hinteren Schalter) → READY, `homed = true` |
| `MOVE` | `pos`, `speed` | R, RUN | | ✓ | absolute Fahrt; referenziert nötig |
| `PARK` | – | R, RUN | | ✓ | Fahrt auf 2 mm (10 mm/s); Shutter schließt in der Parkzone, HV bleibt |
| `SERVICE` | – | R, RUN | | ✓ | Fahrt auf `maint_pos` |
| `START_MEASURE` | `pos` | RUN | | ✓ | Fahrt auf pos, Shutter auf, stehende Messung bis `STOP` |
| `START_TRAVERSE` | `start`, `end`, `speed` | RUN | | ✓ | ein Profillauf start → end mit Messung; bleibt am Ende stehen |
| `STOP` | – | alle | ✓ | | kontrollierter Halt, Erfassung endet, automatisch geöffneter Shutter schließt |
| `HV_ON` | – | R, RUN | | ✓ | READY → RUNNING, wenn Freigabe erfüllt (`hv_ok`) |
| `HV_OFF` | – | alle | ✓ | | RUNNING → READY, Shutter zu |
| `SHUTTER_OPEN` | – | RUN | | | nur in der Servicezone (20 < pos ≤ 60 mm) |
| `SHUTTER_CLOSE` | – | alle | ✓ | | Shutter zu |
| `ACK` | – | W | | | Fehler quittieren; `PRECONDITION`, solange die Ursache anliegt |
| `CALIB` | `action` | S, R, RUN | | | Kalibrierung (Abschnitt 4) |
| `SET` | `key`, `value` | alle | | | Parameter setzen (Abschnitt 3) |
| `GET_CONFIG` | – | alle | | | alle Parameter als `"config":{…}` |
| `GET_STATE` | – | alle | | | Heartbeat; Zustand und Zeitwerte |
| `REBOOT` | – | S, R, W | | | Neustart 700 ms nach der Antwort |

### 2.1 Parameter der Befehle

| Befehl | Parameter | Typ | Pflicht | Bereich | Standard |
|---|---|---|:-:|---|---|
| `MOVE` | `pos` | Zahl | ✓ | 0 … 242 mm | – |
| `MOVE` | `speed` | Zahl | | 1 … 20 mm/s | 10 |
| `START_MEASURE` | `pos` | Zahl | | > 60 … 242 mm (Messzone) | `measure_pos` (155) |
| `START_TRAVERSE` | `start` | Zahl | | > 60 … 242 mm | `traverse_start` (80) |
| `START_TRAVERSE` | `end` | Zahl | | > 60 … 242 mm, ≥ 1 mm von start; end < start erlaubt | `traverse_end` (240) |
| `START_TRAVERSE` | `speed` | Zahl | | 1 … 20 mm/s | `traverse_speed` (10) |
| `CALIB` | `action` | Text | ✓ | `point` · `base` · `save` · `reset` | – |
| `CALIB` | `index` | Zahl | bei `point` ✓ | ganze Zahl 1 … 6 (Kalibrierpunkt) | – |
| `SET` | `key` | Text | ✓ | Schlüssel aus Abschnitt 3, max. 23 Zeichen | – |
| `SET` | `value` | Zahl, bool oder Text | ✓ | je Schlüssel, Text max. 63 Zeichen | – |

Bei allen anderen Befehlen werden `params` ignoriert. Zahlen müssen endlich sein (kein NaN/Inf).

### 2.2 Zusätzliche Felder der Antworten

| Befehl | Zusatzfelder |
|---|---|
| `GET_STATE` | `"state"`, `"sys_err"`, `"local"`, `"cycle_max_us"`, `"period_max_us"`, `"overruns"`, `"prio_lat_max_us"` |
| `GET_CONFIG` | `"config":{…}` mit allen Parametern aus Abschnitt 3 (ohne `wifi_pass`) und zusätzlich `calib_raw_1` … `calib_raw_6`, `calib_valid`, `wifi_pass_set` |

---

## 3. Parameter für `SET` / `GET_CONFIG`

| Schlüssel | Typ | Standard | Bereich | Bedeutung | Gespeichert |
|---|---|---|---|---|:-:|
| `interlock` | bool | true | – | simulierter Sicherheitskreis geschlossen | – |
| `cooling_flow` | bool | true | – | simulierter Kühlwasserfluss | – |
| `temp` | Zahl | 18,0 | −20,0 … 99,9 °C | simulierte Kühlwassertemperatur (HV ≤ 30 °C, Fehler > 32 °C) | – |
| `measure_pos` | Zahl | 155,0 | 61 … 242 mm | Position für `START_MEASURE` | ✓ |
| `maint_pos` | Zahl | 40,0 | 21 … 60 mm | Position für `SERVICE` | ✓ |
| `traverse_start` | Zahl | 80,0 | 61 … 242 mm | Start der Profilfahrt | ✓ |
| `traverse_end` | Zahl | 240,0 | 61 … 242 mm | Ende der Profilfahrt | ✓ |
| `traverse_speed` | Zahl | 10,0 | 1 … 20 mm/s | Geschwindigkeit der Profilfahrt | ✓ |
| `shutter_hold` | Zahl | 3,0 | 0 … 10 s | Shutter bleibt nach einem Lauf so lange offen | ✓ |
| `edge_threshold` | Zahl | 3,0 | 0,5 … 100 mm | Dicke, ab der „auf dem Messgut“ gilt | ✓ |
| `base_dist` | Zahl | 70,0 | 40 … 300 mm | Bezugsebene: Dicke = `base_dist` − Abstand | ✓ |
| `calib_ref_1` … `calib_ref_6` | Zahl | 65,0 · 60,0 · 55,0 · 50,0 · 45,0 · 40,0 | 40 … 300 mm | wahrer Abstand der Kalibrierpunkte 1 … 6 | ✓ |
| `estop_nc` | bool | false | nur bei stehender Achse und HV aus | Not-Aus als Öffner (true) oder Schließer (false) | ✓ |
| `wifi_ssid` | Text | "" | 0 … 32 Zeichen, leer = kein Router | Router-WLAN; wirksam nach `REBOOT` | ✓ |
| `wifi_pass` | Text | "" | 0 oder 8 … 63 Zeichen | Router-Passwort; wird nie zurückgemeldet | ✓ |

bool-Werte auch als `0`/`1`. Gespeicherte Werte gelten sofort und werden im Flash abgelegt, sobald
die Achse steht und die HV aus ist. Simulierte Werte (`–`) gelten nach jedem Start wieder als
Standard.

---

## 4. Kalibrierung (`CALIB`)

Kalibriertabelle mit 6 Punkten; Punkt i liegt auf der Stufe bei **95 + 20·i mm** (115 … 215 mm).

| `action` | Voraussetzung | Wirkung | mögliche Fehler |
|---|---|---|---|
| `point` + `index` 1 … 6 | Achse steht oder stehende Messung läuft (Shutter offen, seit V1.6.3), Rohabstand 500 ms innerhalb ±0,5 mm stabil | aktuellen unkalibrierten Abstand als Punkt `index` übernehmen (`calib_raw_<index>`); nicht gespeichert | `BAD_PARAM`, `BUSY`, `NOT_STABLE` |
| `base` | Achse steht, Sensor im Bereich, stabil | `base_dist` = aktueller kalibrierter Abstand | `BUSY`, `PRECONDITION`, `NOT_STABLE` |
| `save` | Achse steht; Tabelle gültig: Referenzen 40 … 300 mm, nach Referenz sortiert steigen Referenzen und Rohwerte um je ≥ 1 mm | alle Parameter speichern | `BUSY`, `CALIB_INVALID` |
| `reset` | Achse steht | Werkstabelle (Rohwert = Referenz = 65 … 40 mm), erst mit `save` dauerhaft | `BUSY` |

Die wahren Abstände stehen in `calib_ref_1` … `calib_ref_6` (`SET`). Zwischen zwei Punkten wird
linear interpoliert, außerhalb mit dem ersten bzw. letzten Abschnitt verlängert. `near`/`far`
(bis Contract V1.5) gibt es nicht mehr (`BAD_PARAM`).

Ablauf an der Demoanlage, unter Messbedingungen (der offene Shutter verschiebt das Sensorsignal):
`HV_ON`; für i = 1 … 6 `START_MEASURE pos=95+20·i` → (Achse steht, Shutter offen) →
`CALIB action=point index=i` → `STOP`; dann `CALIB action=save` und `HV_OFF`. Die Weboberfläche macht
das mit „Alle 6 Stufen automatisch“.

---

## 5. Fehlercodes

### 5.1 Antworten der API (`err`)

| Code | Name | Bedeutung |
|---:|---|---|
| 0 | `OK` | angenommen |
| −1 | `BAD_CRC` | Rahmen fehlt oder CRC falsch |
| −2 | `BAD_JSON` | kein gültiges JSON-Objekt |
| −3 | `UNKNOWN_CMD` | Befehl fehlt oder unbekannt |
| −4 | `BAD_PARAM` | Parameter fehlt, falscher Typ oder außerhalb des Bereichs |
| −5 | `STATE` | im aktuellen Zustand nicht erlaubt |
| −6 | `PRECONDITION` | Bedingung fehlt (HV-Freigabe, Zone, nicht referenziert, Fehlerursache liegt an) |
| −7 | `BUSY` | Ablauf aktiv, Warteschlange voll oder keine Antwort in 100 ms – Wirkung in `/proc` prüfen |
| −8 | `NOT_STABLE` | Kalibrierwert nicht stabil |
| −9 | `CALIB_INVALID` | Kalibrierpunkte ungültig |
| −10 | `LOCAL_CONTROL` | Bediener am Display hat Vorrang – nicht wiederholen |

### 5.2 Systemfehler (`/proc` → `err`, Zustand WAIT_ACK)

| Code | Name | Ursache | Quittieren |
|---:|---|---|---|
| 0 | `NONE` | kein Fehler | – |
| 1 | `ESTOP` | Not-Aus | Taster lösen, `ACK` |
| 2 | `TEMP_HIGH` | `temp` > 32,0 °C | `temp` ≤ 32 setzen, `ACK` |
| 3 | `COMM_TIMEOUT` | Watchdog scharf, > 5 s kein `/api`-Paket | `ACK` |
| 4 | `MOTOR_FAULT` | Endschalter unerwartet während einer Fahrt | `ACK`, dann `INIT` |
| 5 | `HOMING_FAILED` | Referenzfahrt > 70 s oder falscher Schalter | `ACK`, dann `INIT` |

Nach `ACK` bleiben HV und Shutter aus (kein automatischer Wiederanlauf).

---

## 6. Prozessabbild `GET /proc`

```json
{"ts":123456,"state":"RUNNING","err":0,"local":false,
 "motion":{"pos":120.0,"vel":10.0,"state":"moving","mode":"traverse","homed":true,"limit_fwd":false,"limit_bwd":false},
 "measure":{"dist":57.7,"thick":12.3,"on_material":true,"valid":true,"in_range":true,"avg":12.3,"n":143},
 "safety":{"estop":false,"hv_on":true,"hv_ok":true,"shutter":"open","interlock":true,"cooling_flow":true,"temp":23.0,"temp_ok":true,"comm_armed":true},
 "crc":"XXXXXXXX"}
```

| Feld | Werte | Bedeutung |
|---|---|---|
| `ts` | ms | Zeit seit dem Start |
| `state` | `SAFE_START` `READY` `RUNNING` `FAULT` `WAIT_ACK` `RECOVER` | Sicherheitszustand |
| `err` | 0 … 5 | Systemfehler (5.2) |
| `local` | bool | Bedienvorrang des Displays |
| `motion.pos` / `vel` | mm / mm/s | Position, Geschwindigkeit |
| `motion.state` | `idle` `moving` `homing` `error` | Ablauf der Achse: `moving` bei **jedem** aktiven Ablauf – auch während einer stehenden Messung (`START_MEASURE`), obwohl die Achse steht; `error` nach einem Fehler in Fahrt bis zum nächsten erfolgreichen Start. „Achse fertig“ = `idle` oder `error` und `mode = none` |
| `motion.mode` | `none` `measure` `traverse` | laufendes Messprogramm |
| `motion.homed` | bool | Referenz gültig |
| `motion.limit_fwd` / `limit_bwd` | bool | Endschalter vorn / hinten |
| `measure.dist` | mm | kalibrierter Abstand |
| `measure.thick` | mm | Dicke = `base_dist` − Abstand, ≥ 0 |
| `measure.on_material` | bool | Dicke > `edge_threshold` |
| `measure.valid` | bool | Messwert gültig (nur bei offenem Shutter) |
| `measure.in_range` | bool | Sensor im Bereich 35 … 300 mm |
| `measure.avg` / `n` | mm / Anzahl | Mittelwert und Anzahl gültiger Werte der aktuellen/letzten Erfassung |
| `safety.estop` | bool | Not-Aus aktiv |
| `safety.hv_on` / `hv_ok` | bool | HV ein / Freigabe erfüllt |
| `safety.shutter` | `open` `closed` | Shutter |
| `safety.interlock` / `cooling_flow` | bool | simulierte Signale |
| `safety.temp` / `temp_ok` | °C / bool | simulierte Temperatur, `temp ≤ 30,0` |
| `safety.comm_armed` | bool | Watchdog scharf → Heartbeat nötig |

---

## 7. Weitere Endpunkte

| Methode, Pfad | Inhalt |
|---|---|
| `GET /` | Weboberfläche (IMS-Bedienoberfläche) |
| `GET /settings` | Weiterleitung auf `/#settings` |
| `OPTIONS /api` | CORS-Preflight, Antwort 204 |
| `GET /log` | letzte 20 Logzeilen (Text) |
| `GET /llms.txt` | Einstieg für Sprachmodelle |
| `GET /add` | ADD-Dokument (JSON) – verbindlich für KI-Agenten (D1) |
| `GET /add.html` | ADD-Dokument für Cloud-KIs (D1) |
| `GET /openapi.json` | OpenAPI 3.1 dieser API – technische Referenz, Erweiterung zu ADD (V1.6.4) |
| `GET /doc` | Dokumentationsseite mit Links (HTML) |
| `GET /app.js` | ADDAPP (CRC, Rahmen, Regeln) für eigene Webseiten |

---

## 8. Zonen und feste Positionen

| Bereich | Positionen | Regel |
|---|---|---|
| Parkzone | 0 … 20 mm | Shutter immer zu; Parkposition 2 mm |
| Servicezone | > 20 … 60 mm | Shutter nur manuell (`SHUTTER_OPEN`) |
| Messzone | > 60 … 242 mm | Shutter nur automatisch bei `START_MEASURE` / `START_TRAVERSE` |
| Messgut | 105 … 225 mm | 6 Stufen à 20 mm, Dicke 5 … 30 mm |

**HV-Freigabe (`hv_ok`):** `interlock` ∧ `cooling_flow` ∧ `temp` ≤ 30,0 °C ∧ kein Not-Aus.

---

## 9. Beispiele

```
curl -s http://192.168.4.1/proc
```
```
curl -s -X POST --data '{"cmd":"HV_ON","params":{},"crc":"88F7C802"}' http://192.168.4.1/api
```
```
python3 scripts/add_client.py --host 192.168.4.1 cmd INIT
```
```
python3 scripts/add_client.py --host 192.168.4.1 cmd START_TRAVERSE start=100 end=200 speed=5
```
```
python3 scripts/add_client.py --host 192.168.4.1 cmd SET key=traverse_speed value=15
```
```
python3 scripts/add_client.py --host 192.168.4.1 shell
```

**Typische Folge für eine Profilfahrt:** `INIT` → warten auf `READY` → `HV_ON` →
`START_TRAVERSE` → `/proc` lesen und jede Sekunde `GET_STATE` → Ende bei `motion.mode = none` →
`HV_OFF` → `PARK`.

**CRC in Python:**

```python
import json, zlib
body = json.dumps({"cmd": "SET", "params": {"key": "temp", "value": 25.0}}, separators=(",", ":"))
print(body[:-1] + ',"crc":"%08X"}' % (zlib.crc32(body.encode()) & 0xFFFFFFFF))
```
