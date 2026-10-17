# Review Sprint I1 – Gerätetest Teil 2 (lokal, Heimnetz, REST)

**Datum:** 17.10.2026 · **Prüfer:** Norbert Walter (Gerät), Claude (Auswertung, Korrekturen)
**Firmware/Contract:** V1.5.23 → V1.5.25 · **Pflichtenheft:** V1.7.7 → V1.7.9
**Tests:** 22 native Suiten, 479 Testfälle, grün mit ASan/UBSan und ThreadSanitizer; Firmware-Build
mit `check_iram` 5/5

## Ergebnis

**Bestanden.** Alle Punkte von 2a (lokal am Display), 2b (Heimnetz) und 2c (REST über den Router) sind
erfüllt. Der Gerätetest hat vier echte Fehler der Firmware gefunden (K1–K4), die alle behoben und auf
dem Gerät nachgeprüft sind. Die Zeitgrenzen der Regelschleife wurden auf Grundlage der Messungen neu
festgelegt (E1).

| Teil | Inhalt | Ergebnis |
|---|---|---|
| 2a | Display-Bedienung, HV, Messen, Traverse, Park/Service, Not-Aus, Fehler/ACK | 10/10 ok |
| 2b | WLAN-Einstellung, Anmeldung im Heimnetz, IP-Anzeige | ok |
| 2c 1–6 | Zustand nach Reset, HV-Vorbedingungen, INIT, Interlock, Messen, Traverse rückwärts | ok (COMM_TIMEOUT-Befunde → K1, K3) |
| 2c 7 | Watchdog: `hb off` → COMM_TIMEOUT, `ACK` | ok nach K2 |
| 2c 8 | TEMP_HIGH, Quittierung per `ACK` | ok, Verhalten bleibt (E3) |
| 2c 9 | SERVICE, Shutter manuell, PARK | ok nach K1/K3 |
| 2c 10 | Display-STOP, danach API-INIT | ok (`LOCAL_HOLD_MS` = 0) |
| 2c 11 | Parameter speichern, Neustart | ok |
| 2c 12 | `/log`, `/llms.txt`, `/add.html` | ok; `/add.html` 404 bis A1 (E4) |
| 2c 13 | `flood`, `parallel`, 3 × `soak --minutes 10 --web` | PASS (Grenze 3 ms) |
| 2c 14 | `latency --prepare`, 3 × 10 STOPs bei fahrender Achse | PASS |
| Zusatz | Einstellwert bei HV an → `nvs: save deferred …`, nach STOP/HV_OFF `nvs: saved` | ok |

### Messwerte (Abnahme)

| Größe | gemessen max. | Grenze |
|---|---|---|
| Zykluszeit `cycle_max_us` (Soak mit Weblast) | 2220 µs | ≤ 3000 µs |
| Abstand zweier Zyklusstarts `period_max_us` | 2405 µs | ≤ 3000 µs |
| Prioritätsbefehl-Latenz `prio_lat_max_us` | 1725 µs | ≤ 3000 µs |
| Zyklen über 1 ms (`overruns`, Information) | ≈ 0,04 % unter Weblast | – |
| STOP-Antwort über REST | 52 ms | < 150 ms (ohne gleichzeitiges Seitenladen) |
| Achse steht nach STOP | 103 ms | < 1 s |
| REST-Antwortzeit (WLAN-Stromsparmodus aus) | 35–60 ms, unter Weblast bis 0,3 s | – |
| COMM_TIMEOUT in Soak/Latenz | keiner | keiner |

## Korrekturen (Firmware)

| # | Befund am Gerät | Ursache | Korrektur |
|---|---|---|---|
| K1 | COMM_TIMEOUT trotz Heartbeat, Lücken von 5–11 s | Der Arduino-`WebServer` bedient eine Verbindung nach der anderen und wartet bis 5 s auf Verbindungen ohne Anfrage (offener Browser-Tab) | Verbindungen ohne Anfrage werden nach 500 ms getrennt (`AddWebServer`, V1.5.25) |
| K2 | Nach Display-ACK blieb die API minutenlang mit `LOCAL_CONTROL` gesperrt | Bedienvorrang 30 s nach dem letzten Display-Befehl, verlängert durch jede Berührung | `LOCAL_HOLD_MS` in `config.h`, Standard 0 = kein Vorrang (V1.5.24) |
| K3 | COMM_TIMEOUT bei häufigen Anfragen, Log: `last /api packet 4294967285 ms ago` | Die Netz-Task trug die Paketzeit während des Regelzyklus ein; die vorzeichenlose Differenz `now − last_rx` wurde bei einem „jüngeren“ Paket ≈ 4,29·10⁹ | Paketalter vorzeichenbehaftet (`safetyCommTimeout`, V1.5.25), Test für genau diesen Fall |
| K4 | `overruns` +600 je 200 Anfragen, Zykluszeit bis 3 ms | 16 ADC-Lesungen am Stück in einem Zyklus; WLAN-Treiber schrieb bei jedem Verbinden in den Flash | ein ADC-Wert je Zyklus, `WiFi.persistent(false)`; Statistik ab `boot: ready` (V1.5.25) |

Weitere Änderungen ohne Fehlerursache: WLAN-Stromsparmodus aus (Antwortzeit 75–140 ms → 35–60 ms),
Sicherheitsflags nach Boot `true` (V1.5.23), serielle Konsole (V1.5.23), Diagnosezeilen in `/log`
(`comm:`, `net:`, `http:`, `ctrl:`), Speichern von Einstellwerten nur im Stillstand (V1.5.25).

## Korrekturen (Werkzeuge)

- `add_client.py shell`: Heartbeat `GET_STATE` jede Sekunde ab Start, Meldung nach 10 s ohne Antwort,
  `hb` zeigt die Statistik (zuerst an `comm_armed` gekoppelt – zu fehleranfällig, verworfen).
- `stress_api.py`: stammte aus V1.1 (feste Traverse ab 60 mm außerhalb der Messzone, 8 Threads gegen
  einen Server mit Warteschlange 4, „0 Überläufe seit Boot“). Jetzt Messzone aus `GET_CONFIG`,
  4 Threads, Grenze 3 ms, `--prepare`, `--web`, Diagnose aus `/log` nach jedem Test.
- `fake_device.py`: Watchdog, Parameterbereiche und CycleStats wie das Gerät.

## Entscheidungen (Norbert Walter)

- **E1 Zeitverhalten:** Nenntakt 1 ms; Zykluszeit, Zyklusabstand und Prioritätsbefehl-Latenz jeweils
  ≤ 3 ms, nachgewiesen unter den Abnahmebedingungen. Zyklen über 1 ms nur als Information – eine
  Häufigkeit von Ausreißern lässt sich durch Messen nicht absichern. Die V1.5-Werte (500 µs / 1,5 ms /
  2 ms, 0 Überläufe) galten ohne Motortask und WLAN-Last.
- **E2 Speichern nur im Stillstand:** Ein Flash-Schreibvorgang hält beide Kerne für einige 10 ms an.
  Einstellwerte gelten sofort im RAM und werden gespeichert, wenn die Achse steht und HV aus ist.
- **E3 TEMP_HIGH bleibt quittierpflichtig** (`ACK` per API ist möglich).
- **E4 `/add.html` 404** mit `{"error":"not found"}`, bis `add.json` mit Sprint A1 kommt.
- **E5 Webdateien bleiben im SPIFFS:** Die 3-ms-Grenze hält auch unter dauernder Weblast.
- **E6 Not-Aus:** In der Demoanlage Software (unterliegt den 3 ms), in der echten Anlage ein Relais,
  das direkt und ohne Software wirkt. Als Hinweis im Sicherheitskonzept (§6) festgehalten.

## Akzeptierte Abweichungen

- Während eine Seite der Weboberfläche lädt, wartet ein `/api`-Befehl bis etwa 0,3 s (eine Verbindung
  nach der anderen). Die 150-ms-Grenze für die STOP-Antwort über REST gilt ohne gleichzeitiges
  Seitenladen; Display-STOP, Not-Aus und die Reaktion im Gerät sind nicht betroffen.
- Ausfall der Versorgung vor dem Stillstand: der zuletzt geänderte Einstellwert ist nach dem
  Neustart wieder der alte.

## Offene Punkte

1. **Einzelne Messwerte `dist = 34.0`, `thick = 0.0`** (2c Nr. 5 und 6, auch nach dem verteilten
   ADC-Lesen): Ausreißer filtern (Plausibilität / Sprungbegrenzung) – eigener kleiner Sprint in der
   Messlogik.
2. **Grüne LED flackert sehr kurz** (2a Nr. 3), auch ohne WLAN-Befehle – vermutlich elektrisch.
3. **Ruck des Motors beim Start von I2S** (O3) – braucht eine Erweiterung im FastAccelStepper-Fork.
4. **Pull Request** des IRAM-Fixes an FastAccelStepper (gin66) steht aus.
5. Webdateien einbetten nur, falls eine spätere Messung die 3 ms mit Weblast verletzt.

## Erkenntnisse

1. **Diagnose im Gerät hat die Ursachen gefunden, nicht Vermutungen.** K1 und K3 waren erst mit den
   `/log`-Zeilen (`comm: … 4294967285 ms ago`, `net: http … ms`) eindeutig; vorher lagen WLAN,
   Stromsparmodus und Tool gleichermaßen im Verdacht.
2. **Werte aus zwei Kernen brauchen vorzeichenbehaftete Differenzen.** Eine Zeit, die ein anderer
   Kern schreibt, kann „jünger“ sein als die eigene. Die bisherigen Tests prüften nur den Überlauf
   der Uhr, nicht diesen Fall.
3. **Testwerkzeuge altern mit.** `stress_api.py` war nach drei Contract-Versionen falsch (Messzone,
   Thread-Zahl, Grenzwerte) und meldete Fehler, die keine waren. Werkzeuge gehören in jede
   Contract-Änderung mit geprüft.
4. **Grenzwerte erst nach der Messung auf der echten Hardware festlegen** und als einen Wert, der
   immer gilt – nicht als Häufigkeit.
