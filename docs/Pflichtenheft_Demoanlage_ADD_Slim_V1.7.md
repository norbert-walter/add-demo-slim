# Pflichtenheft – Demoanlage ADD (Slim)

**Stand:** 2026-10-12
**Autor:** Norbert Walter
**Version:** 1.7.5 (Contract V1.5: Zonen, Shutter über das Messprogramm)
**Verbindliche Schnittstellen:** `docs/CONTRACT.md` (englisch, für Agenten) und die Header in `src/**/*.h`

---

## 0. Änderungen

### V1.7.4 → V1.7.5 (Contract V1.5, 12.10.2026)

Anlass: Bei einer kontinuierlichen Profilfahrt (hin und her) öffnete und schloss der
Shutter bei jedem Durchgang – das geht auf die Lebensdauer des Shutters.

| Bereich | V1.7.4 | V1.7.5 |
|---|---|---|
| Zonen | Park 0 … 20, Wartung 20 … 60, Automatik überall außerhalb der Parkzone | **drei getrennte Zonen** in Maschinenkoordinaten (Nullpunkt immer auf der Parkseite = Referenzschalter): Park 0 … 20 (Shutter immer zu), Wartung > 20 … 60 (Shutter nur manuell), **Messzone > 60 … 242** (Shutter nur durch die Automatik). Feste Konstanten, nicht per API verstellbar |
| Shutter bei Profilfahrt | schließt am Ende jeder Fahrt | **bleibt über das Messprogramm offen**; schließt erst nach der **Nachlaufzeit** `shutter_hold` (Standard 3 s) ohne Messmodus, sofort bei `STOP`, beim Einfahren in die Wartungs- oder Parkzone, bei HV-Abfall/`HV_OFF`, Fehler, `SHUTTER_CLOSE` |
| Manueller Shutter | – | wird nie durch die Nachlaufzeit geschlossen; startet währenddessen eine Messung, übernimmt die Automatik ihn |
| Messpositionen | `START_MEASURE`/`START_TRAVERSE` > 20 mm, Parameter 21 … 242 | **> 60 mm**, Parameter `measure_pos`, `traverse_start/end` **61 … 242**; Standard-Profil **80 → 240** |
| Neuer Parameter | – | `shutter_hold` 0 … 10 s (0 = Verhalten wie bisher) |
| Seitenwechsel des Messbügels | – | bei der Inbetriebnahme über `DIR_POSITIVE_HIGH` und die Wahl des Referenzschalters (neuer Smoketest Abschnitte 2 und 4), nie durch Spiegeln der Zonen |
| Anlagenkoordinaten | – | Idee für später (§11.1): Umrechnung Maschine ↔ Anlage für mehrere Messeinrichtungen |
| Contract-Beispiel `GET_CONFIG` | `"traverse_speed":200.0` | korrigiert auf 10.0 |

### V1.7.3 → V1.7.4

| Bereich | V1.7.3 | V1.7.4 |
|---|---|---|
| Offene Punkte | – | neuer Abschnitt **11.1 Erweiterungen nach S6**: QR-Code auf dem Display, NFC-Tag, GitHub als Informationsquelle, Offline-Doku, Dokumentation per KI, Konzept KI-gestützte Serientests |

### V1.7.2 → V1.7.3 (Smoketest 0a Abschnitte 5.2 und 6)

| Bereich | V1.7.2 | V1.7.3 |
|---|---|---|
| ADC-Spannungen | als Sensorspannungen angegeben | sind **ESP-Messwerte** (`analogReadMilliVolts()`); sie liegen konstant ≈ 235 mV über der mit DVM am Sensor gemessenen Spannung (Steigung 0,99). Die Kurve ist gegen die ESP-Werte kalibriert, das Ergebnis in mm stimmt. Bekannte Einschränkung, Ursache nicht abschließend geklärt |
| ADC-Kalibrierung | – | **Option** (nicht im Contract): Zweipunkt-Kalibrierung des ESP-ADC mit unabhängiger Referenzquelle oder parallel messendem DVM |
| Display / Touch | Werte offen | `invert` = false, `rgb_order` = false, Touch `offset_rotation` = 2, Kalibrierung X 130 … 3775, Y 200 … 3900 |

### V1.7.1 → V1.7.2 (Contract V1.4.2, Smoketest 0a Abschnitt 5 „Abstandssensor“)

| Bereich | V1.7.1 | V1.7.2 |
|---|---|---|
| Sensorbeschaltung | R20 entfernt, sonst nichts | **11 kΩ zwischen Vo und GND** und **47 µF zwischen Vcc und GND**, beide direkt am Sensor angelötet. Ohne Last blieb der Eingang nach den ersten ADC-Messungen am Anschlag hängen (3145 mV) |
| Sensorkurve | `121,5·V^-1,142` | **`168,8·V^-1,398`**, angepasst an die sechs Stufenmitten (max. 0,48 mm) |
| Messbereich der Firmware | 40 … 300 mm | **35 … 300 mm** (die 40-mm-Stufe misst 40,5 mm und darf nicht auf der Grenze liegen) |
| Prüfkörper | 90 … 220 mm, Kalibrierpositionen 105 … 205 | **105 … 225 mm**, sechs Stufen à 20 mm, **Messpunkte in den Stufenmitten 115 … 215** |
| Messposition | 145 mm (Stufenkante) | **155 mm** (Mitte Stufe 3, 55 mm → 15 mm Dicke) |

### V1.7 → V1.7.1 (Contract V1.4 / V1.4.1, Smoketest 0a Abschnitt 4 „Antrieb“, 11.10.2026)

| Bereich | V1.7 | V1.7.1 |
|---|---|---|
| Übertragung | 42 mm/U (Annahme) → 76,19 Schritte/mm | **40,77 mm/U (gemessen)** → **78,49 Schritte/mm**; 126,2 mm statt 130 mm Sollweg mit dem alten Wert, nach Korrektur 130,25 mm (< 0,2 %) |
| Verfahrweg | 0 … 248 mm, vorderer Endschalter bei 250 mm | **0 … 242 mm**, vorderer Endschalter **gemessen bei Achse 244,3 mm** (246,3 mm ab hinterem Schalter); mechanisch nicht verschiebbar (Schleppkette) |
| Parameterbereiche | `measure_pos`, `traverse_start/end` 21 … 248 mm | **21 … 242 mm** (Standardwerte 145 / 60 / 240 unverändert) |
| Drehrichtung | `DIR_POSITIVE_HIGH` = true (Annahme) | **false** (gemessen: positive mm = weg von der Parkseite) |
| Nothalt im Treiber | `forceStop()` | **`forceStopAndNewPosition()`**: `forceStop()` wird von FastAccelStepper verworfen, wenn noch ein neuer Fahrbefehl ansteht (Vorfall im Smoketest, siehe Kapitel 11) |
| Smoketest | 0a | **0a.1**: Endschalter richtungsabhängig, Stopp ohne Wettlauf, Überwachung in `loop()`, strenge Argumentprüfung aller Befehle |

### V1.6 → V1.7 (Contract V1.3)

| Bereich | V1.6 | V1.7 |
|---|---|---|
| Geschwindigkeiten | mm/min, 10 … 3000, Referenzfahrt 600 mm/min | **mm/s** überall (API, Parameter, /proc, Display): 1 … 20 mm/s, Normalgeschwindigkeit 10 mm/s (Bewegungen, Profilfahrt), Referenzfahrt 5 mm/s, Referenzfahrt-Timeout 70 s |
| WLAN | Station oder AP als Rückfall | **AP immer an** + Verbindung zum Router (Station, SSID/Passwort per REST, Login nach jedem Boot automatisch); bei Verbindungsverlust 3 Versuche à 10 s, dann 2 min Pause, dann erneut |
| Herzschlag | – | blaue LED auf dem Board (**GPIO 2**) blinkt mit 0,5 Hz, solange die Steuerschleife läuft |
| Buzzer | ein kurzer Piep bei Fehler/Referenzfahrt | Signalmuster (aktiver Summer, nur Rhythmus): Start, Reboot, Tastenquittung, Referenzfahrt, Fehler, Not-Aus 4 s |
| Not-Aus-Verdrahtung | fest in `config.h` | **Parameter `estop_nc`** per REST: Standard einfacher Schalter (LOW-aktiv, ohne Drahtbrucherkennung), umschaltbar auf Öffner mit Drahtbrucherkennung |
| REBOOT | sofort nach der Antwort | Reboot-Signal, Neustart 700 ms nach dem Befehl |

### V1.5 → V1.6 (Contract V1.2, Hardware-Vortest vom 09.10.2026)

Die Hardware wurde vollständig aufgebaut und mit der CNC-Firmware des DLC32 vorgetestet
(Motor beide Richtungen bei 0,8 A, Endschalter, Not-Aus, Sensor 0,68 … 2,63 V nach Entfernen
von R20 4k7, LEDs je 20 mA, Buzzer, Display und Touch).

| Bereich | V1.5 | V1.6 |
|---|---|---|
| Verfahrweg | 0 … 240 mm | **0 … 248 mm** (vorderer Endschalter bei 250 mm) |
| Parkzone | ≤ 5 mm | **≤ 20 mm**, Parkposition 2 mm, neuer Befehl/Taste **PARK** |
| Wartungszone | – | **20 … 60 mm**, Wartungsposition `maint_pos` (Standard 40 mm, 21 … 60 mm einstellbar, kundenspezifisch), neuer Befehl `SERVICE` / Taste **SERV** |
| Shutter manuell | überall außerhalb der Parkzone | **nur in der Wartungszone**; im Messgutbereich öffnet nur die Automatik während einer Messung |
| Sensorkurve | `120,8·V^-1,058` | **`121,5·V^-1,142`**, angepasst an sechs Messpunkte (max. 0,6 mm Abweichung) |
| Dickenbezug | `base_dist` 200 mm (leere Auflage) | **70 mm** = virtuelle Bezugsebene an der Messgutkante; Stufen 5 … 30 mm |
| Standardwerte | Messposition 120, Profil 10 → 230 | Messposition **145**, Profil **60 → 240**, Kalibrierreferenzen 40/65 mm |
| HV-LED | i2so.3 (I2S) | **GPIO 4** direkt |
| Display | 5 Tasten | **7 Tasten**: INIT · HV · MEAS · TRAV · PARK · SERV · STOP |
| Versorgung | 24 V | 15 V DC, 1 A |

### V1.4 → V1.5 (Contract V1.1, nach Review)

| Bereich | V1.4 | V1.5 |
|---|---|---|
| Bedienvorrang | nicht geregelt (Display und API gleichrangig) | **Display hat Vorrang (LOCAL)**: nach jedem Display-Befehl lehnt die Anlage Start-Befehle von der API mit `LOCAL_CONTROL (−10)` ab; STOP/HV_OFF/SHUTTER_CLOSE/SET bleiben erlaubt. Ende 30 s nach dem letzten Display-Befehl, sobald Achse steht und HV aus ist |
| STOP-Latenz | STOP wartete u. U. auf einen Zyklus ohne Fahrauftrag, volle Queue → BUSY | **Prioritäts-Queue** für STOP/HV_OFF/SHUTTER_CLOSE, Ausführung auch bei laufendem Fahrauftrag; ≤ 2 ms bis zum Treiber |
| API-Antwort | vor dem Setzen der Ausgänge | **nach** dem Setzen der Ausgänge: `OK` = angenommen und Ausgänge gesetzt; Bewegungsende nur über `/proc` |
| Zykluszeit | nicht überwacht | im Gerät gemessen und in `GET_STATE` gemeldet: Rechenzeit `cycle_max_us` < 500 µs, Überläufe 0, Jitter `period_max_us` < 1500 µs, Prioritätsbefehl-Latenz `prio_lat_max_us` ≤ 2000 µs |
| Gleichzeitige API-Aufrufe | eine Antwort-Queue, Annahme „ein Aufrufer“ nur als Kommentar | Warten auf Antwort per Mutex **serialisiert**; STOP/HV_OFF/SHUTTER_CLOSE werden auch bei belegtem Mutex eingereiht; Integrationstest mit 8 Threads gegen den echten Queue-Code |
| Überlauf Prioritäts-Queue | Befehl hätte verloren gehen können | **Überlauf-Latch**: ein Bit je Befehl und Herkunft (API/LOCAL getrennt), kann nicht überlaufen, gleiche Anforderungen werden zusammengefasst, keine geht verloren |
| Abarbeitung Prioritätsbefehle | einer pro Zyklus (STOP konnte hinter anderen warten) | **alle** wartenden Prioritätsbefehle im selben Zyklus (max. 7); 2-ms-Grenze unabhängig von der Anzahl |
| Display im Fehlerfall | 5 Tasten bleiben sichtbar | **alle Tasten ausgeblendet**, Fehlertext groß, **eine** ACK-Fläche über die volle Breite, 1 s halten; grau solange die Ursache anliegt |
| Display-Logik | im Display-Code | reine Funktionen `display_logic.h`, nativ getestet |
| BUSY | Wiederholregel offen | erst `/proc` prüfen, dann wiederholen (CONTRACT §5.2) |
| Simulierte Signale | nicht gekennzeichnet | Interlock, Cooling, Temp überall als **SIM** markiert |
| Sicherheitshinweis | fehlte | Demo-Not-Aus ersetzt keinen Hardware-Sicherheitskreis (§6) |
| Tests | 189 | + LOCAL, STOP-Priorität, gleichzeitiger Not-Aus + Kühlungsausfall, Display-Logik |
| DoD | Sprint-Tests grün | zusätzlich **Regressionsregel**: alle bisher grünen Suiten bleiben grün |

### V1.3 → V1.4

| Bereich | V1.3 | V1.4 |
|---|---|---|
| Toolchain | `platform = espressif32` (Core 2.x, IDF 4.4) | **pioarduino** (Arduino Core 3.x, ESP-IDF 5.x) – Voraussetzung für FastAccelStepper-I2S |
| FastAccelStepper | `^0.30.4` | Master-Commit `96459ed` (2.0.0) mit `DRIVER_I2S_MUX` und `i2sMuxSetBit()` |
| Core-Bezeichnung | Core 1 / Core 2 | Core 0 (Netz) / Core 1 (Steuerung) |
| Schritte/mm | 4,762 (Vollschritt) | **76,19** (Mikroschritt 1/16) |
| Sensorkurve | `12.08·V^-1.058` als mm | `120.8·V^-1.058` mm (Originalformel liefert cm) |
| Dicke / Kante | `thick = calib_far_ref − dist`, Messgut wenn `dist < 60 mm` | `thick = base_dist − dist`, Messgut wenn `thick > edge_threshold` (3 mm) |
| Architektur | Globale Variablen, `taskENTER_CRITICAL` überall | **Single-Writer**: eine Control-Task, Command-Queue + Snapshot |
| Logik | in Hardware-Modulen | **reine Funktionen** in `src/logic/`, nativ getestet |
| Wiederanlauf | nach ACK automatisch HV + Shutter | **kein automatischer Wiederanlauf**; nach Not-Aus während Fahrt neue Referenzfahrt |
| Shutter | Regeln unvollständig | Shutter nur in RUNNING, nur außerhalb Parkzone (≤ 5 mm), automatisch während Messung |
| Messung | immer gültig | `measure.valid` nur bei offenem Shutter |
| Kommunikations-Watchdog | immer aktiv (→ Dauer-Not-Aus) | nur bei per API gestarteter Operation, Heartbeat `GET_STATE` |
| Not-Aus | aktiv LOW | **Öffner** – Drahtbruch löst aus; Aktivierung sofort, Freigabe entprellt |
| /proc | ohne Sicherheitszustand | mit `state`, `homed`, `valid`, `avg`, `hv_ok`, `comm_armed` |
| CRC | Regel offen | **eindeutige Rahmenregel** (CONTRACT §4), Referenz in C++, Python, JS |
| API | 12 Befehle, Fehlercodes offen | +`GET_CONFIG`, `CALIB`, `REBOOT`; Fehlercodes festgelegt |
| Display | widersprüchlich (Touch ja/nein, 1/2 Seiten) | 1 Seite, **5 Touch-Tasten** (INIT, HV, MEAS, TRAV, STOP) |
| Sicherheitsflags | in NVS gespeichert | **nie gespeichert** – nach Boot immer gesperrt (fail-safe) |
| Homing-Timeout | 30 s bei 100 mm/min (zu kurz) | 600 mm/min, Timeout 40 s |
| Tests | Nachbauten der Logik | Tests gegen echten Code, eingefroren, gegen Referenzimplementierung geprüft |
| ADD-Dokument | Feldnamen weichen von Spec ab | streng nach ADD Spec v1.0 (7 Blöcke) |

---

## 1. Zielsetzung

Die Demoanlage ist ein verkleinertes Modell (1:5) eines radiometrischen
Dickenmessers, orientiert am Messsystem M50. Sie dient ausschließlich der
Präsentation des **AI Device Description (ADD)**-Konzepts.

Die gefährliche radiometrische Messtechnik wird durch einen ungefährlichen
optischen Abstandssensor ersetzt. Die Firmware wird agentenbasiert entwickelt
(code-server + Cline + lokales Modell Qwen3.6-35B-A3B, 64k Kontext).

### Was die Demo zeigen soll

1. KI liest `/add` → versteht Anlage vollständig → erklärt sie einem Benutzer
2. KI liest `/add` → Benutzer stellt Fragen zu Bedienung und Funktion
3. Python-Skript steuert Profilfahrt über REST-API – ohne LogiCAD
4. Python erstellt individuelle Anzeigeseite aus `/proc`-Live-Daten
5. Fehlersimulation (Zahnstange auskoppeln → `MOTOR_FAULT` bzw. Positionsabweichung) → Fehlerdiagnose mit KI
6. ADD beschreibt sicherheitsrelevante Zustände → KI versteht Sicherheitslogik
7. Ein einfacher Mikrocontroller trägt die gesamte Funktionalität

### KI-Einsatzbereiche (mittelfristig)

- Kundenspezifische Steuerungsprogramme ohne LogiCAD
- Individuelle Visualisierungsseiten (Prozess- und Qualitätskontrolle)
- Anlagentest vor Auslieferung
- Fehlerdiagnose und Expertensystem
- Prozessanalyse und -optimierung

---

## 2. Zielgruppe

- Entwickler-Kollegen bei IMS
- Technische Entscheider (Vorgesetzte technischer Bereiche)
- Geschäftsführung
- Ausgewählte Schlüsselkunden
- Anthropic-Bewerbung (Constellation)

---

## 3. Hardware

### Zentrale Steuereinheit

| Komponente | Spezifikation |
|---|---|
| Steuerboard | Makerbase MKS DLC32 V2.1 |
| MCU | ESP32 (ESP32-WROOM-32U), Dual-Core 240 MHz |
| Flash | 8 MB auf dem Modul (Smoketest 0a); das Projekt nutzt bewusst 4 MB (App 2944 KB, SPIFFS 1088 KB, NVS 20 KB) |
| RAM | 520 kB SRAM |
| Kommunikation | WiFi 802.11b/g/n: eigener AP (immer an) + optional Verbindung zum Router |
| USB/UART | CH340C (Flashen + Konsole) |

### Antrieb X-Achse

| Komponente | Spezifikation |
|---|---|
| Schrittmotortreiber | TMC2209 (Standalone), **Mikroschritt 1/16** (Jumper), Strom 0,8 A eingestellt |
| Ansteuerung | Step/Dir/Enable über 74HC595-Kette am I2S (DATA 21, BCK 16, WS 17) |
| Schrittmotor | Nanotec ST4118L0804-A, 200 Schritte/U, 0,8 A |
| Übertragung | Zahnstange, **40,77 mm/U (gemessen, Smoketest 0a)** → **78,49 Schritte/mm**, Auflösung 0,013 mm |
| Verfahrweg | **0 … 242 mm** (0 = 2 mm vor hinterem Endschalter, vorderer Endschalter gemessen bei 244,3 mm) |
| Endlagenschalter | GPIO 36 vorn (Achse 244,3 mm), GPIO 35 hinten (Referenz), beide aktiv LOW |

### Zonen und Positionen der Achse

| Zone / Position | Bereich | Bedeutung |
|---|---|---|
| Parkzone | 0 … 20 mm | Shutter immer geschlossen |
| Parkposition | 2 mm | Ziel von **PARK** |
| Wartungszone | > 20 … 60 mm | Shutter **nur manuell** (Prüfung der Quelle); ein automatisch geöffneter Shutter schließt beim Einfahren; vom Kunden vorgegeben |
| Wartungsposition | 40 mm (Parameter `maint_pos`, 21 … 60 mm) | Ziel von **SERV** |
| **Messzone** (V1.7.5) | > 60 … 242 mm | Shutter **nur durch die Automatik** während eines Messprogramms; alle Mess- und Profilpositionen liegen hier (61 … 242 mm) |
| Messgutbereich | 105 … 225 mm | Stufenkörper, sechs Stufen à 20 mm, Messwerte 65 … 40 mm |
| Messposition | 155 mm (Mitte Stufe 3, Messwert 55 mm) | Normalposition, Ziel von **MEAS** |
| Profilfahrt | 80 → 240 mm (V1.7.5) | Standard für **TRAV** |

Die Zonen gelten in **Maschinenkoordinaten**; der Nullpunkt liegt immer auf der Parkseite
(Referenzschalter). Die Wartungszone liegt damit immer zwischen Park- und Messzone. Wird
der Messbügel seitenverkehrt eingebaut, werden bei der Inbetriebnahme Drehrichtung und
Referenzschalter getauscht (Smoketest 2 und 4) – die Zonen bleiben unverändert.

### Kalibrierpunkte (Smoketest 0a, 11.10.2026, mit 11 kΩ Last am Sensor)

| Position (Stufenmitte) | Abstand (Soll) | Spannung (Mittel aus 10) | σ | Kurve `168,8·V^-1,398` |
|---|---|---|---|---|
| 115 mm | 65 mm | 1,972 V | 1,2 mV | 65,32 mm |
| 135 mm | 60 mm | 2,091 V | 8,4 mV | 60,20 mm |
| 155 mm | 55 mm | 2,236 V | 8,0 mV | 54,81 mm |
| 175 mm | 50 mm | 2,403 V | 1,7 mV | 49,55 mm |
| 195 mm | 45 mm | 2,588 V | 2,0 mV | 44,66 mm |
| 215 mm | 40 mm | 2,777 V | 1,6 mV | 40,48 mm |
| 242 mm (neben dem Messgut) | unterer Ausleger | 0,933 V | 2,5 mV | ≈ 186 mm (nur Richtwert) |

(Vortest 09.10.2026 ohne Last: 1,72 … 2,63 V – die Werte sind mit der neuen Beschaltung nicht mehr gültig.)

**Hinweis (V1.7.3):** Die Spannungen sind ESP-Messwerte. Mit DVM direkt am Sensor (Vo gegen GND)
gemessen: 1,743 V bei ESP 1,973 V (Stufe 1) und 0,703 V bei ESP 0,942 V (neben dem Messgut) –
konstanter Versatz ≈ +235 mV, gleiche Steigung. `adcraw` zeigt, dass schon der Rohwert erhöht ist
(eFuse-Kalibrierung `vref` vorhanden, Umrechnung der Bibliothek plausibel). Für die Messung ohne
Bedeutung, da Kurve und Messkette gegen die ESP-Werte kalibriert sind; bei Tausch von Board,
ESP32 oder Sensor neu kalibrieren. Ein Vergleich mit dem Sharp-Datenblatt ist um diesen Versatz verschoben.

An den Messgutkanten springt der Messwert nicht, sondern ändert sich stetig, bis der
Sensorfleck wieder vollständig auf einer Fläche liegt. 70 mm entspricht etwa der Kante.

### Sensorik & Signale

| Komponente | Spezifikation |
|---|---|
| Abstandssensor | Sharp GP2Y0A41SK0F, Firmware-Messbereich 35–300 mm, Versorgung 5 V, Ausgang ca. 0,4–3,1 V; **am Sensor angelötet: 11 kΩ Vo–GND (Last, zwingend) und 47 µF Vcc–GND** |
| ADC | GPIO 34 (ADC1_CH6, 11 dB), `analogReadMilliVolts()` (eFuse-kalibriert), Pull-up R20 4k7 **entfernt**, Serienwiderstand R23 1k, Lastwiderstand 11 kΩ am Sensor (V1.7.2) |
| LED Grün | HV eingeschaltet, **GPIO 4**, HIGH aktiv, 20 mA |
| LED Rot | Shutter offen, GPIO 32, HIGH aktiv, 20 mA |
| Buzzer | i2so.7, **aktiver Summer** (feste Tonhöhe, nur Ein/Aus) – Signale unterscheiden sich im Rhythmus (§5.11) |
| LED Blau | Herzschlag, **GPIO 2** (Board-LED), HIGH aktiv, 0,5 Hz |
| Not-Aus | GPIO 22 (PROBE), Pull-up auf dem Board. Verdrahtung per Parameter `estop_nc` (§6): Standard **einfacher Schalter gegen GND, LOW-aktiv, ohne Drahtbrucherkennung** (für die ersten Tests); später Öffner gegen GND, HIGH-aktiv, **mit** Drahtbrucherkennung |

> **Hinweis Sensor:** Im Arbeitsbereich (40–65 mm) liefert der ADC 2,78–1,97 V (ESP-Werte,
> Smoketest 5.1); 2,78 V liegt noch sicher unter dem Ende des 11-dB-Bereichs (≈ 3,1 V).
> 47 µF direkt am Sensor gegen Versorgungseinbrüche sind eingebaut (V1.7.2).

### Display

| Eigenschaft | Wert |
|---|---|
| Modell | MKS TS35R, ST7796, 480×320 (Landscape, Rotation 1) |
| Touch | ADS7843E (XPT2046-kompatibel), gemeinsamer VSPI-Bus |
| Pins | SCK 18, MOSI 23, MISO 19, DC 33, CS 25, RST 27, BL 5, Touch-CS 26 (Smoketest 6) |
| Library | LovyanGFX |

### Stromversorgung

15 V DC, 1 A extern über Schleppkette. USB-Kabel ebenfalls in der Schleppkette.

---

## 4. Pin- und Slot-Belegung

Verbindlich in `src/config.h`. Alle Werte sind im Smoketest 0a bestätigt; `config.h` ist seitdem eingefroren.

| Signal | Anschluss | Bemerkung |
|---|---|---|
| Endschalter vorn | GPIO 36 (X-) | input only |
| Endschalter hinten | GPIO 35 (Y-) | Referenzschalter |
| Abstandssensor | GPIO 34 (Z-) | analog |
| Not-Aus | GPIO 22 (PROBE) | Pegel per Parameter `estop_nc` |
| Herzschlag-LED (blau) | GPIO 2 | HIGH = an, 0,5 Hz |
| Shutter-LED | GPIO 32 (SPINDLE/TTL) | HIGH = offen |
| HV-LED | GPIO 4 | HIGH = an |
| I2S DATA / BCK / WS | GPIO 21 / 16 / 17 | 74HC595-Kette |
| Enable / Step / Dir | i2so.0 / .1 / .2 | Enable aktiv LOW |
| Buzzer | i2so.7 | |

Die Zuordnung FluidNC-Bit `i2so.N` → FastAccelStepper-Slot hat der Smoketest (`scan`) ermittelt: Slot = N (`I2SO_TO_SLOT(n) = n`).

### Simulierte Sicherheitsflags (nur per `/api SET`)

| Flag | Funktion | Startzustand nach **jedem** Boot |
|---|---|---|
| `interlock` | HV-Freigabe (Schlüsselschalter) | `false` |
| `cooling_flow` | Kühlwasserfluss | `false` |
| `temp` | Kühlwassertemperatur | `18.0 °C` |

---

## 5. Software-Architektur

### 5.1 Grundprinzipien

- PlatformIO mit **pioarduino** (Arduino Core 3.x, ESP-IDF 5.x)
- **Core 0:** WiFi, HTTP-Server, NVS-Schreiben (Netzwerk-Task)
- **Core 1:** Control-Task 1 kHz (einziger Schreiber des Anlagenzustands), Display-Task 5 Hz, FastAccelStepper-Task
- **Single-Writer-Prinzip:** Kommunikation zwischen den Tasks ausschließlich über Command-Queue (rein) und Snapshot (raus)
- **Reine Logik** in `src/logic/` ohne Arduino/IDF/FreeRTOS – vollständig nativ testbar (inkl. simulierter Achse)
- Hardware-Schicht dünn, aus dem Smoketest übernommen
- Code und Kommentare in Englisch
- Task-Watchdog 10 s; WLAN: AP immer an, Router-Verbindung mit Wiederverbindung (§5.12)
- Herzschlag-LED aus dem Control-Task: blinkt nur, solange die 1-kHz-Schleife läuft
- Weboberfläche: statisches HTML auf SPIFFS, Daten per JS

### 5.2 Modulstruktur

```
src/
├── config.h                 Pins, Slots, Timing (eingefroren nach 0a)
├── main.cpp                 setup(), Netzwerk-Task                       S1
├── logic/                   REINE LOGIK (nativ getestet)
│   ├── types.h/.cpp         Typen, Namen                                 S2.1
│   ├── crc32.h/.cpp         CRC32 + Rahmenregel                          S2.1
│   ├── safety_logic.h/.cpp  Zustandsautomat R1–R7                        S2.1
│   ├── measure_logic.h/.cpp Messkette, Kalibrierung, Statistik           S2.2
│   ├── params.h/.cpp        Parameter, SET-Schlüssel                     S2.2
│   ├── motion_seq.h/.cpp    Ablaufsteuerung Achse                        S2.3
│   ├── app_logic.h/.cpp     Gesamtlogik (Tick, Befehle, Prozessabbild)   S2.4
│   ├── proc_json.h/.cpp     /proc- und Antwort-JSON                      S3
│   └── api_parse.h/.cpp     /api-Parser                                  S3
├── hal/
│   ├── hal_io.h/.cpp        GPIO, ADC, LEDs, Buzzer                      S2.5
│   └── motion_drv.h/.cpp    FastAccelStepper-Adapter                     S2.5
├── app/                     (0b, vollständig vorgegeben)
│   ├── snapshot.h/.cpp      Prozessabbild über Kerne
│   ├── command_queue.h/.cpp Befehls-Queue mit Antwort
│   └── control_task.h/.cpp  Steuer-Task (Glue)                           Review S2.6
├── system/  system, nvs_config                                           S1
├── comm/    wifi_manager, log_buffer, http_server                        S1/S3
├── display/ lgfx_config.h (0a), display.cpp                              S4
└── smoketest/smoketest.cpp  Sprint 0a
test/native/<suite>/         eingefrorene Host-Tests
test/embedded/test_hardware/ On-Target-Tests                              S5
data/                        www/, add.json, doc.md                       S4/S6
scripts/                     serial_cmd.py, add_client.py
```

### 5.3 Betriebsmodi

**Stehende Messung (`START_MEASURE`)** – nur in RUNNING
- Fährt mit 10 mm/s an die Messposition (60 < pos ≤ 242 mm, Messzone, Standard 155 mm)
- Shutter öffnet automatisch, Messung bis `STOP`
- Mittelwert und Anzahl gültiger Werte in `/proc` (`avg`, `n`)

**Parken (`PARK`) und Wartungsposition (`SERVICE`)** – in READY und RUNNING, Referenz nötig
- PARK fährt auf 2 mm (Parkzone, Shutter schließt beim Einfahren), HV bleibt unverändert
- SERVICE fährt auf `maint_pos` (Wartungszone); dort darf der Shutter manuell geöffnet werden

**Profilfahrt (`START_TRAVERSE`)** – nur in RUNNING
- Eine Fahrt: Anfahrt der Startposition mit 10 mm/s (ohne Messung), dann Fahrt zur
  Endposition mit konstanter Geschwindigkeit `speed` (mit Messung); danach steht die Achse
  an der Endposition. Start und Ende in der Messzone (> 60 mm), Abstand ≥ 1 mm, Rückwärts erlaubt
- Shutter öffnet in der Fahrt und **bleibt nach dem Ende `shutter_hold` (3 s) offen**
  (V1.7.5). Startet der Client in dieser Zeit die nächste Fahrt (z. B. Start und Ende
  vertauscht), bleibt er offen – kontinuierliche Hin- und Rückfahrt ohne Shutter-Spiel.
  Ohne Folgefahrt schließt er nach der Nachlaufzeit; `STOP` schließt sofort. HV bleibt an
- Mehrere Durchgänge, Endlos-Querprofil oder kantengeführter Scan: Ablauf im Client
  (Python/KI), Ende einer Fahrt erkennbar an `motion.mode == "none"` in `/proc`
- Kantenerkennung: Messgut, wenn `thick > edge_threshold`
- Statistik nur über gültige Werte auf Messgut (Lücken ignoriert)
- Messwerte für Profile holt der Client per `/proc`-Polling (z. B. 100–200 ms); bei per API
  gestarteter Fahrt zusätzlich `GET_STATE` mindestens alle 4 s (Watchdog, `/proc` zählt nicht)

### 5.4 REST-API

Vollständige Definition: **CONTRACT §5**. Kurzfassung:

| Befehl | Parameter | Erlaubt in |
|---|---|---|
| `INIT` | – | SAFE_START, READY |
| `MOVE` | `pos`, `speed` | READY, RUNNING (homed) |
| `PARK`, `SERVICE` | – | READY, RUNNING (homed) |
| `START_MEASURE` | `pos` | RUNNING |
| `START_TRAVERSE` | `start`, `end`, `speed` | RUNNING |
| `STOP`, `SET`, `HV_OFF`, `SHUTTER_CLOSE`, `GET_STATE`, `GET_CONFIG` | | immer |
| `CALIB` | `action` near/far/base/save/reset | SAFE_START, READY, RUNNING |
| `ACK` | – | WAIT_ACK (Ursache muss beseitigt sein) |
| `HV_ON` | – | READY, RUNNING |
| `SHUTTER_OPEN` | – | RUNNING, nur in der Wartungszone (20 < Position ≤ 60 mm) |
| `REBOOT` | – | SAFE_START, READY, WAIT_ACK (Reboot-Signal, Neustart nach 700 ms) |

Fehlercodes: Anlage `err` 0–5, API `err` 0 … −10 (CONTRACT §3).

**Bedeutung von `OK`:** Befehl angenommen und die Ausgänge des Zyklus sind gesetzt.
Lange Vorgänge (Referenzfahrt, Profilfahrt) laufen weiter; ihr Ende steht nur in `/proc`.
**`BUSY`:** Der Befehl kann trotzdem noch ausgeführt werden – Client prüft erst
`/proc`, dann ggf. erneut senden (CONTRACT §5.2).
**`LOCAL_CONTROL (−10)`:** Am Display wird gerade bedient; Start-Befehle von der API
sind gesperrt (§6 Bedienvorrang).

**Prüfsumme:** CRC32 (zlib) über den gesendeten JSON-Text ohne crc-Feld; das Feld
`,"crc":"XXXXXXXX"` wird vor der letzten `}` eingefügt (CONTRACT §4).

### 5.5 HTTP-Endpunkte

| Endpunkt | Methode | Inhalt |
|---|---|---|
| `/add` | GET | ADD-Dokument (JSON, SPIFFS) |
| `/add.html` | GET | ADD eingebettet für Cloud-KI (ADD Spec §5.3) |
| `/llms.txt` | GET | Index (ADD Spec §5.4) |
| `/proc` | GET | Prozessabbild (JSON + CRC, ≤ 512 Byte) |
| `/api` | POST | Befehlskanal (JSON + CRC) |
| `/doc` | GET | Dokumentation (Markdown) |
| `/log` | GET | letzte 20 Logzeilen |
| `/` | GET | Prozessansicht |
| `/settings` | GET | Einstellungen |

### 5.6 /proc

Siehe CONTRACT §5.1 (Feldreihenfolge verbindlich).

### 5.7 Weboberfläche (2 Seiten, SPIFFS)

**Sprache: Englisch.** Basis: ADD Dashboard Style Guide v1.0 mit **identischen
CSS-Variablen** (Werte aus Style Guide Anhang A), Theme-Klassen `theme-dark`,
`theme-light`, `theme-night`, `theme-bw`, localStorage-Schlüssel `add-dashboard-theme`.
Schriften Ubuntu / Ubuntu Mono **lokal auf SPIFFS** (`/www/fonts/*.woff2`, kein CDN –
im AP-Modus gibt es kein Internet), Fallback `sans-serif` / `monospace`.

**Seite 1 – Process View (`/`)**
- Statuszeile: Sicherheitszustand (READY/RUNNING/FAULT…), HV, Shutter, E-Stop, Fehlertext,
  Badge **LOCAL** solange `local = true` (Start-Tasten dann deaktiviert, Hinweis „Operator at display has priority“)
- Datenpanels: POS, SPEED, DIST, THICK (`---` wenn `!valid` oder nicht auf Messgut), AVG/n
- Sicherheitsflags: INTERLOCK, COOLING, TEMP mit Farbcode `--ok/--warn/--danger`,
  jeweils mit Kennzeichnung **SIM** (simuliert)
- Zeitreihe DIST/THICK (Canvas, max. 100 Punkte, clientseitig)
- Tasten: INIT, HV ON/OFF, MEASURE, TRAVERSE, STOP, ACK (ACK nur in WAIT_ACK aktiv)
  – START-Befehle ohne Parameter → Werte aus der Konfiguration
- Bestätigungsdialog (Style Guide 7.8) für HV ON und ACK
- **Heartbeat:** solange `safety.comm_armed`, alle 2 s `GET_STATE`
- Validation-Strip (Style Guide 6.10) aus `/add` → `validation.validated_by`
- `/proc`-Polling alle 500 ms, CRC wird geprüft (bei Fehler: Statuspill „CRC error“)
- Bei `BUSY`: keine automatische Wiederholung; nächster `/proc`-Stand entscheidet

**Seite 2 – Settings (`/settings`)**
- Werte beim Laden per `GET_CONFIG`
- Messposition, Wartungsposition `maint_pos` (21 … 60 mm), Profilfahrt (Start/Ende/Geschwindigkeit), Shutter-Nachlaufzeit `shutter_hold` (V1.7.5), Kantenschwelle, `base_dist`
- Sicherheitsflags (Toggle Interlock, Toggle Cooling, Temperatur)
- Kalibrierung: Referenzwerte Nah/Fern (`SET calib_near_ref/far_ref`),
  Tasten Capture Near / Capture Far / Capture Base / Save / Reset (`CALIB`)
- WLAN: SSID, Passwort des Routers (wirksam nach Reboot), Reboot-Taste; Anzeige des Verbindungszustands
- Not-Aus-Verdrahtung `estop_nc` (Umschalter mit Warnhinweis, nur bei stehender Achse und HV aus)
- Jede Rückmeldung (`err`, `msg`) sichtbar als Toast

### 5.8 Display (1 Seite, 480×320, LovyanGFX)

Lokaler Zugang ohne WLAN. Aktualisierung alle 200 ms aus dem Snapshot,
Neuzeichnen nur geänderter Zonen. **Sprache: Englisch.**

```
┌──────────────────────────────────────────────────────────────────┐ y=0
│ ADD DEMO           [ READY ]            ● HV      ▮ SHUTTER      │ h=32
├───────────┬───────────┬───────────┬──────────────────────────────┤ y=34
│ POS       │ SPEED     │ DIST      │ THICK                        │
│ 155.0     │ 10.0      │ 55.0      │ 15.0                         │ h=72
│ mm        │ mm/s      │ mm        │ mm                           │
├───────────┴───────────┴───────────┴──────────────────────────────┤ y=110
│ POS |PP▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░|  242        │ h=24
├──────────────────────────────────────────────────────────────────┤ y=140
│ ESTOP ●  INTERLOCK ●  COOLING ●  TEMP 23.0°C ●                   │ h=26
│ MODE traverse   AVG 12.3 mm (143)   ERR NONE                     │ h=26
│ IP 192.168.4.1                                                   │ h=26 (bis y=230)
├────────┬────────┬────────┬────────┬────────┬────────┬────────────┤ y=260
│  INIT  │   HV   │  MEAS  │  TRAV  │  PARK  │  SERV  │    STOP    │ h=60
└────────┴────────┴────────┴────────┴────────┴────────┴────────────┘ y=320
   64 px    64       64       64       64       64        96 px
```

- 7 Tasten (V1.6): sechs à 64 px mit `setTextSize(2)` (12×16 px, max. 5 Zeichen = 60 px),
  STOP als größte Fläche 96 px mit `setTextSize(3)`
- PARK sendet `PARK`, SERV sendet `SERVICE`
- Positionsbalken: Parkzone (0 … 20 mm) grau hinterlegt, Wartungszone (20 … 60 mm) gelb markiert

- Werte: `setTextSize(3)` (18×24 px/Zeichen) → max. 6 Zeichen je 120-px-Feld
- Tasten siehe oben (V1.6: 7 Tasten)
- `HV` schaltet je nach Zustand `HV_ON`/`HV_OFF`, Farbe zeigt Zustand
- `STOP` immer aktiv; Tap-Feedback 50 ms aufhellen
- Befehle mit `source = LOCAL` (kein Kommunikations-Watchdog, setzt Bedienvorrang LOCAL)
- Badge `LOCAL` in der Kopfzeile, solange der Bedienvorrang aktiv ist
- Sicherheitsflags mit Zusatz `SIM`, z. B. `INTERLOCK(SIM) ●`
- Touch-Abfrage alle 20 ms, Neuzeichnen alle 200 ms

**Fehlerbild (FAULT / WAIT_ACK)** – alle Bedientasten werden ausgeblendet, es gibt
nur noch eine mögliche Handlung:

```
┌──────────────────────────────────────────────────────────────────┐ y=0
│ ADD DEMO           [ WAIT_ACK ]         ○ HV      ▯ SHUTTER      │ h=32
├──────────────────────────────────────────────────────────────────┤ y=34
│                                                                  │
│          FAULT: MOTOR_FAULT                    (rot, Größe 4)    │
│          Check axis and end switches, then acknowledge           │
│                                                                  │
│ POS 231.4 mm   ESTOP ○   TEMP(SIM) 23.0°C                        │
├──────────────────────────────────────────────────────────────────┤ y=260
│                  ACK – HOLD 1 s   ▓▓▓▓▓▓▓░░░░░░░                 │ h=60, 480 px
└──────────────────────────────────────────────────────────────────┘ y=320
```

- FAULT (ein Zyklus): Banner, noch keine Taste
- WAIT_ACK, Ursache liegt noch an (Not-Aus gedrückt, Temperatur > 32 °C):
  Fläche grau, Text `CAUSE ACTIVE: ESTOP – RELEASE FIRST`, keine Wirkung
- WAIT_ACK, Ursache beseitigt: Fläche gelb `#f5a623`, `ACK – HOLD 1 s`, Fortschrittsbalken;
  ACK wird **einmal** nach 1 s ununterbrochenem Drücken gesendet
- Der Druck muss **auf dem Fehlerbild beginnen**: Ein Finger, der beim Fehlereintritt
  noch auf STOP lag, quittiert nie (Schutz gegen reflexhaftes Quittieren)
- Abhilfetexte: ESTOP „Release E-stop, then acknowledge“, TEMP_HIGH „Lower temp ≤ 32 °C“,
  MOTOR_FAULT „Check axis and end switches“, COMM_TIMEOUT „Remote client lost“,
  HOMING_FAILED „Check home switch, then INIT“
- In WAIT_ACK kann sich nichts bewegen (Motor jeden Zyklus notgestoppt, HV und Shutter
  zwangsweise aus); der physische Not-Aus bleibt jederzeit wirksam
- Logik: `src/logic/display_logic.h` (`screenModeOf`, `ackCauseActive`, `ackPressUpdate`)
- Farben aus dem Style Guide (Dark): Hintergrund `#202020`, Panel `#1e1e1e`,
  Rahmen `#2e2e2e`, Text `#ffffff`, Labels `#8ab2d0`, DIST `--accent #00c8a0`,
  THICK `--accent2 #0084ff`, OK `#00c8a0`, Warnung `#f5a623`, Fehler `#e84040`

### 5.9 Messkette

16 × `analogReadMilliVolts()` alle 20 ms → 4 kleinste und 4 größte verwerfen →
Mittelwert → `dist = 168,8 · V^-1,398` mm (angepasst an die Kalibrierpunkte, V1.7.2) → IIR α = 0,8 →
2-Punkt-Kalibrierung → `thick = max(0, base_dist − dist)` mit `base_dist` = 70 mm
(virtuelle Bezugsebene an der Messgutkante, Stufen 5 … 30 mm). Der Sharp-Sensor liefert nur etwa alle 16,5 ms
einen neuen Wert; 50 Hz Abtastung genügen.

Kalibrierung (CONTRACT §7.3): Referenzabstände per `SET calib_near_ref/far_ref`,
Messwerte per `CALIB near/far` (nur bei stabilem Signal, 500 ms innerhalb ±0,5 mm),
`CALIB save` prüft Gültigkeit und speichert. `CALIB base` setzt `base_dist`
(Nullabgleich ohne Messgut).

### 5.10 Libraries

```ini
lib_deps =
    https://github.com/gin66/FastAccelStepper.git#96459ed15a3c7d9927f561d2edb72cc7edbbbe8a
    lovyan03/LovyanGFX    @ 1.2.32   ; im Smoketest 0a exakt gepinnt
    bblanchon/ArduinoJson @ 7.4.3
; platform = pioarduino 55.03.312-1 (siehe platformio.ini)
```

### 5.11 Akustische Signale (aktiver Summer)

Der Summer hat eine feste Tonhöhe; die Signale unterscheiden sich im Rhythmus
(Muster in ms, an/aus/an …, Logik `src/logic/sound_logic.h`):

| Ereignis | Muster | Bedeutung |
|---|---|---|
| Firmware gestartet | 100 / 100 / 300 (kurz–lang) | Firmware arbeitet korrekt |
| REBOOT | 300 / 100 / 100 (lang–kurz), danach Neustart | Neustart folgt |
| Display-Taste angenommen | 50 | Quittung (nur bei Erfolg) |
| Referenzfahrt fertig | 150 | Achse referenziert |
| Fehler (außer Not-Aus) | 3 × 150 | Quittierung nötig |
| Not-Aus | 4000 (Dauerton 4 s, einmalig) | Not-Aus ausgelöst |

Ein Signal niedrigerer Priorität unterbricht nie ein höheres (Not-Aus > Fehler >
Reboot > Start > Referenzfahrt > Tastenquittung).

### 5.12 WLAN

- **Access Point immer aktiv:** `ADD-Demo` / `adddemo1`, 192.168.4.1 – das Tablet erreicht
  die Anlage immer direkt.
- **Router-Verbindung:** `SET wifi_ssid` und `SET wifi_pass` per REST-API (gespeichert),
  danach `REBOOT`. Nach jedem Boot meldet sich die Anlage automatisch an (DHCP) und ist
  zusätzlich im Heimnetz erreichbar (`http://add-demo.local`, Internet über den Router).
- **Wiederverbindung:** Bei Verbindungsverlust oder gescheitertem Aufbau 3 Versuche à 10 s;
  scheitern alle, ruht die Station 2 min, danach erneut 3 Versuche – ohne Ende.
- Während der Verbindungsversuche kann der AP kurz stocken (gemeinsames Funkmodul); die
  Pause hält das selten, wenn der Router fehlt.
- Zustand im Log (`/log`) und auf dem Display (Zeile „AP … STA …“).

---

## 6. Sicherheitskonzept

Vollständig: **CONTRACT §6** und `src/logic/safety_logic.h` (Regeln R1–R7, Invarianten I1–I8).

> **Sicherheitshinweis.** Dies ist eine Demonstrationsanlage. Die Auswertung des
> Not-Aus in Software und die Sicherheitslogik sind **keine Sicherheitsfunktion**
> im Sinne von EN ISO 13849 / IEC 62061 und ersetzen keinen Hardware-Sicherheitskreis.
> Es gibt keine echte Strahlenquelle; HV und Shutter sind LEDs. Interlock, Kühlung
> und Temperatur sind simuliert.

### Zustände

```
SAFE_START ──INIT ok──► READY ──HV_ON──► RUNNING
     ▲                    ▲  ◄─HV_OFF / Bedingung weg─┘
     │                    │
     └── RECOVER ◄─ACK── WAIT_ACK ◄── FAULT ◄── (Not-Aus, >32 °C, Endschalter, Kommunikation, Homing)
        (READY wenn Referenz gültig, sonst SAFE_START)
```

### HV einschalten (UND)
Kühlwasserfluss ∧ Temperatur ≤ 30 °C ∧ Interlock ∧ kein Not-Aus

### HV-Abfall ohne Fehler (eine Bedingung fehlt)
RUNNING → READY, Shutter schließt, laufende Messung stoppt. Keine Quittierung nötig.

### Fehler (Quittierung nötig)
Not-Aus · Temperatur > 32 °C · unerwarteter Endschalter · Kommunikationsausfall · Referenzfahrt gescheitert.
Reaktion im selben Zyklus: Shutter zu → Motor Not-Halt → HV aus → FAULT → WAIT_ACK.
`ACK` wird nur angenommen, wenn die Ursache beseitigt ist.

### Shutter
Öffnen nur in RUNNING und außerhalb der Parkzone (> 20 mm). **Manuell** nur in der
Wartungszone (20 … 60 mm); in der **Messzone (> 60 mm)** öffnet nur die Automatik während
einer Messung (V1.7.5). Jeder Shutter schließt bei HV-Abfall, `HV_OFF`, jedem Fehler,
Einfahrt in die Parkzone und `SHUTTER_CLOSE`. Der **automatische** Shutter bleibt über
das Messprogramm offen und schließt zusätzlich beim Einfahren in die Wartungszone, bei
`STOP` und nach der Nachlaufzeit `shutter_hold` ohne Messmodus. Ein in der Wartungszone
manuell geöffneter Shutter bleibt offen, wenn die Achse sie verlässt; beginnt dann eine
Messung, übernimmt ihn die Automatik.

### Wiederanlauf
**Kein automatischer Wiederanlauf.** Nach ACK bleiben HV und Shutter aus. Wurde
die Achse während einer Bewegung notgestoppt, ist die Referenz ungültig →
SAFE_START → neue Referenzfahrt.

### Kommunikationsausfall
Watchdog nur aktiv, solange eine **per API** gestartete Operation läuft (Bewegung
oder HV an). Ohne gültiges `/api`-Paket für > 5 s → Fehler `COMM_TIMEOUT`.
Clients senden `GET_STATE` mindestens alle 4 s. Bedienung am Display aktiviert
den Watchdog nicht; ein Display-Befehl während einer Fernoperation schaltet ihn ab
(der Bediener ist anwesend und übernimmt).

### Bedienvorrang (LOCAL)
Der Mensch an der Anlage ist schneller als jeder Fernzugriff, daher hat das Display
Vorrang. Jeder Display-Befehl setzt `local = true`. Solange `local` aktiv ist, werden
von der API abgelehnt (`LOCAL_CONTROL`): INIT, MOVE, PARK, SERVICE, START_MEASURE,
START_TRAVERSE, HV_ON, SHUTTER_OPEN, CALIB, ACK, REBOOT. Erlaubt bleiben STOP, HV_OFF, SHUTTER_CLOSE
(sichere Richtung), SET (Fehlersimulation) und Abfragen. `local` endet, wenn die Achse
steht, HV aus ist und seit dem letzten Display-Befehl 30 s vergangen sind.

### STOP-Priorität
STOP, HV_OFF und SHUTTER_CLOSE laufen über eine eigene Prioritäts-Queue und werden auch
in einem Zyklus ausgeführt, der schon einen Fahrauftrag hat. Ein Sofort-Stopp
(Not-Halt, Nullsetzen am Endschalter) wird nie zu einem gebremsten Stopp abgeschwächt.
Ist die Prioritäts-Queue voll, wird die Anforderung als Bit gespeichert (Latch), getrennt
nach Herkunft API/Display: Es kann nicht überlaufen, zehn STOPs werden zu einem STOP
zusammengefasst, keine Anforderung geht verloren. In jedem Zyklus werden **alle**
wartenden Prioritätsbefehle ausgeführt (höchstens 7) – ein STOP wartet also nie hinter
anderen. Auslegungsziel: ≤ 2 ms von der Queue bis zum Treiber, unabhängig von der Anzahl
wartender Befehle (gilt unter den gemessenen Zeitbedingungen Jitter < 1,5 ms, Rechenzeit
< 0,5 ms; der Not-Aus wirkt unabhängig von jeder Queue). Das Gerät misst diese Zeit
selbst (`prio_lat_max_us` in `GET_STATE`); erst die Messung bei der Abnahme ist der
Nachweis. Der schnellste Stopp bleibt der physische Not-Aus.

### Not-Aus-Verdrahtung (Parameter `estop_nc`)

| `estop_nc` | Verdrahtung | Not-Aus bei | Drahtbruch |
|---|---|---|---|
| `false` (Standard) | einfacher Schalter (Schließer) gegen GND, Pull-up auf dem Board | LOW | **nicht erkannt** |
| `true` | Öffner gegen GND | HIGH | erkannt |

Eine Einstellung, die nicht zur Verdrahtung passt, zeigt immer Dauer-Not-Aus – der Fehler
ist sicher und sofort sichtbar. Umschaltung nur bei stehender Achse und HV aus; die
Einstellung wird gespeichert. Solange `false` gilt, zeigt das Display `ESTOP(NO)` als
Warnung. Für den Demo-Einsatz wird der Öffner mit Drahtbrucherkennung empfohlen.

### Fail-Safe-Grundsätze
- Sicherheitsflags werden nie gespeichert (nach Boot gesperrt)
- Not-Aus: Öffner mit Drahtbrucherkennung empfohlen (`estop_nc = true`); der einfache Schalter (Standard) ist nur für Tests gedacht
- Reihenfolge der Ausgänge in jedem Zyklus: Shutter → Motor → HV

---

## 7. ADD-Integration (Sprint 6)

Das ADD-Dokument folgt **streng der ADD Specification v1.0** (sieben Blöcke
`autonomy, device, security, interfaces, actions, rules, validation` plus Header
`schema, version, spec_url, spec_license`). Zusätzlich der optionale `ui`-Block
des Style Guides. **Kein** eigener Top-Level-Block `live_data` – `/proc` wird in
`interfaces[].data` beschrieben, der Hinweis „ADD beschreibt Fähigkeiten, Live-Daten
über /proc“ steht in `rules`.

- Ablage: SPIFFS `/add.json`, Endpunkte `/add`, `/add.html`, `/llms.txt`
- `autonomy`: Einstufung über die drei Scores (bewusste Entscheidung, siehe Sprint 6).
  Konzeptionell **Level 2**: Die Demo selbst ist harmlos, modelliert aber eine
  Strahlenquelle – die KI soll sich so verhalten, wie sie es an der echten Anlage müsste.
  Eine Unterdeklaration ist nicht zulässig.
- `rules`: Bedienvorrang (bei `local = true` bzw. `LOCAL_CONTROL` nicht wiederholen,
  sondern auf den Bediener warten), BUSY-Regel, „OK heißt angenommen, Ende über /proc“
- Simulierte Signale werden im ADD als simuliert beschrieben
- `security.enforcement`: alle Prüfungen, die das Gerät selbst durchsetzt (Zustandsgate, Bereichsgrenzen, Shutter/Parkzone, Watchdog)
- `actions`: alle Befehle mit Parametern, Bereichen, `safe`, `reversible`, `requires_confirmation`
- `validation`: Validierungsprotokoll nach Spec §10.7 (zunächst `validated_by: []`)

**Demo-Ablauf KI-Validierung**
1. KI ruft `/add` ab, lädt Ethical Framework
2. KI erklärt Anlage und Bedienkonzept
3. KI liest `/proc`
4. KI startet Profilfahrt (`SET`-Flags, `INIT`, `HV_ON`, `START_TRAVERSE`) mit Benutzerbestätigung
5. KI überwacht per `/proc`, sendet Heartbeat
6. KI dokumentiert das Profil (Kanten, mittlere Dicke)

---

## 8. Tests

### Nativ (PC, ohne ESP32) – `pio test -e native`

| Suite | Modul | Inhalt |
|---|---|---|
| `test_env` | – | Testumgebung läuft (0a) |
| `test_types_crc` | types, crc32 | Namen, CRC-Vektoren, Rahmenregel |
| `test_safety_logic` | safety_logic | R1–R7, Befehlsmatrix, Automatik, Watchdog, Zufalls-Invariantentest |
| `test_measure_logic` | measure_logic | Kurve (mm!), Median, IIR, Kalibrierung, Stabilität, Statistik |
| `test_params` | params | alle SET-Schlüssel, Bereiche, Persistenz, Config-JSON |
| `test_motion_seq` | motion_seq | Homing, Timeout, Endschalterfehler, Fahren, Profil, Messen, Stopps |
| `test_app_logic` | app_logic | **Szenarien mit simulierter Achse**: Profilfahrt, Not-Aus, Temperatur, Watchdog, Kalibrierung, Bedienvorrang LOCAL, STOP-Priorität, Not-Aus + Kühlungsausfall gleichzeitig |
| `test_sound_logic` | sound_logic | Signalmuster, Prioritäten, Ende, Zeitüberlauf |
| `test_wifi_logic` | wifi_logic | Auto-Login, 3 Versuche à 10 s, 2 min Pause, Wiederverbindung |
| `test_display_logic` | display_logic | Bildschirmmodus, „Ursache aktiv“, ACK 1 s halten, Flankenerkennung |
| `test_command_queue` | app/command_queue.cpp (echter Code, FreeRTOS-Nachbildung mit Threads) | 8 gleichzeitige Aufrufer, Priorität vor Normal, Überlauf-Latch mit Herkunft, alle Prioritätsbefehle in einem Abruf, Last aus 8 Threads, Mutex belegt, verspätete Antworten |
| `test_proc_json` | proc_json | exaktes /proc-Format, Worst Case < 512 Byte |
| `test_api_parse` | api_parse | Parser, Typen, Fehlerfälle |

Alle Tests sind **eingefroren** und gegen eine Referenzimplementierung geprüft
(274 Testfälle, alle grün (Contract V1.5), `test_command_queue` zusätzlich mit ThreadSanitizer geprüft; Mutationstests bestätigen die Trennschärfe).

**Regressionsregel (DoD jedes Sprints):** Alle Suiten, die vor dem Sprint grün waren,
sind danach weiterhin grün (`pio test -e native` vollständig, nicht nur die Sprint-Suite).

Zusätzlich am Gerät: `scripts/stress_api.py` (STOP-Latenz, Queue-Flut, 10-min-Dauerlauf,
Bedienvorrang).

### On-Target – `pio test -e esp32-test` (nur nach Freigabe)
`test/embedded/test_hardware`: CRC, Eingänge, Sensor, Ausgänge, NVS, 2-mm-Fahrt.

---

## 9. Entwicklungsplan

### Abhängigkeitskette

```
S0a Smoketest (👤)  ─► S0b Verträge (🤖 Claude, 👤 Review) ─► S1 System/NVS/WiFi (🤖)
                                                           └► S2.1 types/crc/safety (🤖)
                                                               └► S2.2 measure/params (🤖)
                                                                   └► S2.3 motion_seq (🤖)
                                                                       └► S2.4 app_logic (🤖)
S2.5 HAL + Motortreiber aus Smoketest (👤+🤖) ─► S2.6 Integration Control-Task (👤+🤖)
   ─► S3 JSON/Parser/HTTP (🤖) ─► S4 Web + Display (🤖) ─► S5 Abnahme (👤) ─► S6 ADD (👤+Claude)
```

S1 und S2.1–S2.4 sind voneinander unabhängig (S2.x braucht keine Hardware).

### Sprint-Übersicht

| Sprint | Inhalt | Prüfung (DoD) | Wer | Aufwand |
|---|---|---|---|---|
| S0a | Smoketest, Protokoll, config.h-Werte | Smoketest-Protokoll | 👤 | 3–4 h |
| S0b | Verträge, Header, Tests, Skelette | dieses Paket | Claude + 👤 Review | erledigt / 1 h Review |
| S1 | system, nvs_config, wifi_manager, main | firmware-partial baut; Boot-Test am Gerät | 🤖 Qwen | 2 h |
| S2.1 | types, crc32, safety_logic | test_types_crc, test_safety_logic | 🤖 Qwen | 1–2 h |
| S2.2 | measure_logic, params | test_measure_logic, test_params | 🤖 Qwen | 1–2 h |
| S2.3 | motion_seq | test_motion_seq | 🤖 Qwen | 1–2 h |
| S2.4 | app_logic | test_app_logic | 🤖 Qwen (Review Claude) | 2 h |
| S2.5 | hal_io, motion_drv | Smoketest-Funktionen über HAL | 👤 + 🤖 | 2 h |
| S2.6 | control_task + main zusammenführen | firmware baut; INIT/MOVE/Not-Aus per Serial | 👤 + 🤖 | 2 h |
| S3 | proc_json, api_parse, log_buffer, http_server | test_proc_json, test_api_parse; curl-Tests | 🤖 Qwen | 2–3 h |
| S4a/b | index.html, settings.html (4a); display_logic.cpp, display.cpp (4b) | test_display_logic; Sichtprüfung, Bedienung | 🤖 Qwen | 3–4 h |
| S5 | Integration, Abnahme | Kapitel 10 | 👤 | 3–4 h |
| S6 | add.json, doc.md, Python-Demos | KI-Validierung | 👤 + Claude | 2–3 h |

**Gesamt: ca. 25–30 h**, davon ca. 12–15 h agentenbasiert.

---

## 10. Abnahmekriterien

**1. System-Start**
- [ ] Boot fehlerfrei, Watchdog aktiv, 5 min ohne Reset
- [ ] Parameter nach Neustart wiederhergestellt; Sicherheitsflags wieder gesperrt

**2. Hardware-Signale**
- [ ] LED Grün folgt `HV_ON`/`HV_OFF`
- [ ] LED Rot folgt Shutter
- [ ] Buzzer: Startsignal nach dem Booten, Quittungs-Piep bei Display-Tasten, Not-Aus 4 s Dauerton, andere Fehler 3 Pieps, Referenzfahrt-Ende ein Piep, Reboot-Signal vor dem Neustart
- [ ] Blaue LED blinkt mit 0,5 Hz (1 s an, 1 s aus); steht still, wenn die Steuerschleife hängt (Watchdog-Reset folgt)
- [ ] Endschalter in `/proc` korrekt
- [ ] Abstand plausibel 35–300 mm (±5 %); Stufen 65 … 40 mm auf ±1 mm
- [ ] Not-Aus mit `estop_nc = false` (einfacher Schalter): Drücken löst aus
- [ ] Nach Umstellung auf Öffner (`estop_nc = true`): Drücken **und** Drahtbruch lösen aus; falsche Einstellung zeigt Dauer-Not-Aus
- [ ] Umschalten von `estop_nc` bei nicht gedrücktem Not-Aus: falsche Polarität löst **sofort** Not-Aus aus (On-Target-Test `test_estop_polarity_change`; nutzt die gespeicherte Verdrahtung `estop_nc`, Voraussetzung: Not-Aus nicht gedrückt)

**3. Motor**
- [ ] `INIT` referenziert (Endschalter hinten, 2 mm Rückzug, Position 0)
- [ ] `MOVE` erreicht Ziel ±0,1 mm (Messschieber), keine Schrittverluste nach 10 Fahrten
- [ ] Unerwarteter Endschalter → `MOTOR_FAULT`

**4. Sicherheitskreis**
- [ ] Not-Aus stoppt Bewegung ≤ 2 ms nach Signal (Oszilloskop: Not-Aus ↔ Step-Signal)
- [ ] `HV_ON` nur mit allen vier Bedingungen; jede einzelne fehlende → `PRECONDITION`
- [ ] Temperatur 31 °C → HV aus ohne Fehler; 35 °C → `TEMP_HIGH`, ACK erst nach Absenken
- [ ] `cooling_flow = false` während Profilfahrt → HV aus, Fahrt gestoppt, Shutter zu
- [ ] Kommunikations-Watchdog: Python-Skript während Profilfahrt abbrechen → `COMM_TIMEOUT` nach 5 s
- [ ] Nach ACK kein automatischer Wiederanlauf; nach Not-Aus in Fahrt erneutes INIT nötig
- [ ] Shutter öffnet nie in der Parkzone (≤ 20 mm)
- [ ] `SHUTTER_OPEN` manuell nur in der Wartungszone; bei 145 mm → `PRECONDITION`
- [ ] V1.7.5: Automatischer Shutter nie in der Wartungszone (≤ 60 mm); schließt beim Einfahren
- [ ] V1.7.5: Hin- und Rückfahrt mit Folgebefehl innerhalb von 3 s → Shutter bleibt offen; 3 s nach der letzten Fahrt zu; `STOP` schließt sofort
- [ ] Fahrt auf 242 mm löst den vorderen Endschalter nicht aus
- [ ] Not-Aus und `cooling_flow = false` gleichzeitig → `FAULT ESTOP`
- [ ] STOP per API während Profilfahrt: Antwort < 150 ms, Achse steht < 1 s (`stress_api.py latency`)
- [ ] Zeitverhalten nach 10 min (`stress_api.py soak`): `cycle_max_us` < 500, `overruns` = 0,
      `period_max_us` < 1500 (Jitter), nach STOP-Tests `prio_lat_max_us` ≤ 2000
- [ ] Zeitverhalten mit und ohne Buzzer-Signal vergleichen (z. B. Not-Aus-Ton 4 s während `soak`): `cycle_max_us` bleibt < 500
- [ ] Queue-Flut (200 SET): nur OK/BUSY, alle CRC gültig (`stress_api.py flood`)
- [ ] Parallele Anfragen (8 Threads, 400 gemischte Befehle mit bekanntem Ergebnis):
      keine vertauschte Antwort, nur erwartetes Ergebnis oder BUSY (`stress_api.py parallel`)

**5. REST-API + /proc**
- [ ] Alle Endpunkte < 1 s
- [ ] `/proc` vollständig, CRC gültig (`scripts/add_client.py proc`)
- [ ] Falsche CRC → `BAD_CRC`; alle Befehle laut CONTRACT §5.2
- [ ] `pio test -e native` vollständig grün

**6. Display**
- [ ] Werte, Zustand, Indikatoren korrekt, kein Flackern
- [ ] Positionsbalken synchron
- [ ] Alle 7 Tasten wirken (lokal, ohne WLAN); PARK → 2 mm, SERV → `maint_pos`
- [ ] Fehlerfall: alle Tasten ausgeblendet, nur ACK-Fläche; grau solange Not-Aus gedrückt
- [ ] ACK erst nach 1 s Halten; Finger, der beim Fehler schon auf STOP lag, quittiert nicht
- [ ] Bedienvorrang: nach Display-Befehl API `HV_ON` → `−10`, API `STOP` → OK;
      30 s nach letztem Display-Befehl (Achse steht, HV aus) wieder frei (`stress_api.py local`)

**7. Weboberfläche**
- [ ] Live-Daten ≤ 500 ms, Zeitreihe 100 Punkte
- [ ] 4 Themes, funktioniert ohne Internet (AP-Modus)
- [ ] Einstellungen laden/speichern, Kalibrierung, WLAN-Konfiguration
- [ ] Tablet über WLAN (AP)
- [ ] Router-Verbindung: SSID/Passwort per REST setzen, REBOOT → Gerät ist im Heimnetz erreichbar (`add-demo.local`), AP bleibt erreichbar
- [ ] Router aus: 3 Versuche, dann 2 min Pause (Log), Router wieder an → Verbindung kommt automatisch zurück

**8. Betriebsmodi**
- [ ] Stehende Messung: Mittelwert auf Treppenstufe ±0,5 mm
- [ ] Profilfahrt: Kanten der Treppenstufen erkannt, Statistik plausibel
- [ ] Kalibrierung gespeichert und wirksam

**9. ADD + KI**
- [ ] `/add` valide nach ADD Spec v1.0 (sieben Blöcke)
- [ ] KI erklärt Anlage und Sicherheitslogik korrekt
- [ ] KI liest `/proc` korrekt
- [ ] Python-Skript fährt Profil autonom inkl. Heartbeat
- [ ] Python erzeugt Dashboard aus `/proc` nach Style Guide

---

## 11. Offene Punkte

| Punkt | Entscheidung | Status |
|---|---|---|
| Toolchain | pioarduino | ✅ |
| Mikroschritt | 1/16 | ✅ |
| Architektur | Single-Writer, reine Logik | ✅ |
| /proc-Schema, API, CRC, Fehlercodes | CONTRACT V1.5 | ✅ |
| Zonen, Shutter über das Messprogramm | drei getrennte Zonen, Nachlaufzeit `shutter_hold`, Messzone > 60 mm | ✅ (V1.7.5, Contract V1.5) |
| Geschwindigkeiten, Signale, WLAN, Herzschlag | V1.7 | ✅ |
| Bedienvorrang Display, Haltezeit | LOCAL, 30 s | ✅ |
| Quittierung am Display | eine ACK-Fläche, 1 s halten | ✅ |
| i2so → Slot-Zuordnung (Buzzer, Enable, Step, Dir) | Smoketest `scan`: Slot = i2so-Nummer (Enable 0, Step 1, Dir 2, Buzzer 7) | ✅ (0a) |
| Weg pro Umdrehung, Drehrichtung, Verfahrweg | Smoketest 4: 40,77 mm/U, `DIR_POSITIVE_HIGH` = false, Achse 0 … 242 mm | ✅ (0a, Contract V1.4) |
| **Not-Aus wirkt nur über die Software** | Vorfall 11.10.2026: Fahrt lief unüberwacht, Not-Aus und Endschalter ohne Wirkung, Stopp nur durch Ziehen des Netzsteckers (Ursache im Smoketest behoben, 0a.1). Not-Aus zusätzlich **hart verdrahten** (Motorspannung oder Treiber-Enable direkt unterbrechen) | **Offen – vor dem Demo-Einsatz** |
| Resonanz des Antriebs | deutlich hörbar bei 5 mm/s (= Referenzfahrt-Geschwindigkeit); Geschwindigkeits-Sweep 3 … 12 mm/s, ggf. Referenzfahrt-Geschwindigkeit verschieben | Offen |
| Materialzone / Messposition | Prüfkörper 105 … 225 mm, Messposition 155 mm, Kurve neu angepasst (Smoketest 5) | ✅ (V1.7.2) |
| Sensor-Eingang hängt am Anschlag | Ursache: keine Last am Emitterfolger-Ausgang; 11 kΩ Vo–GND am Sensor | ✅ (V1.7.2) |
| Flash 8 MB statt 4 MB | Projekt nutzt bewusst 4 MB (Partitionen wie spezifiziert); Reserve z. B. für OTA | Info |
| Motor zuckt bei Reset/Flash | Enable über das I2S-Schieberegister ist bis `setup()` undefiniert; harmlos (max. ein Vollschritt) | Bekannt |
| Not-Aus-Verdrahtung | Parameter `estop_nc`, Standard einfacher Schalter | ✅ – Umstellung auf Öffner mit Drahtbrucherkennung vor dem Demo-Einsatz empfohlen |
| Geometrie, Zonen, Sensorkurve | Hardware-Vortest 09.10.2026 (V1.6), ersetzt durch Smoketest 0a (Kurve, Prüfkörper V1.7.2) und Contract V1.5 (Zonen) | ✅ |
| Display-/Touch-Pins, Farbreihenfolge | Smoketest 6: Farben korrekt, Touch 180° gedreht → `offset_rotation` = 2, Kalibrierung X 130 … 3775, Y 200 … 3900 | ✅ (0a) |
| ADC am Z--Eingang (Pull-up?) | R20 4k7 entfernt (Vortest), 11 kΩ Last + 47 µF am Sensor (Smoketest) | ✅ |
| ADC-Versatz ESP ↔ DVM (+235 mV) | Bekannte Einschränkung (Smoketest 5.2), Kurve gegen ESP-Werte kalibriert. **Option:** Zweipunkt-Kalibrierung des ESP-ADC mit Referenzquelle oder parallel messendem DVM (z. B. als Service-Funktion mit zwei Korrekturparametern) | Option, später |
| Autonomy-Level im ADD | konzeptionell Level 2, Scores in S6 | Offen (S6) |
| Python-Demo-Skripte | S6 | Offen |

### 11.1 Erweiterungen nach S6 (Ideen, V1.7.4 / V1.7.5)

Nicht Teil des Sprintplans und nicht im Contract. Aufnahme erst nach S6, jeweils mit eigener
Contract-Version.

| Idee | Umsetzung (Vorschlag) | Nutzen / Hinweis |
|---|---|---|
| **Dynamischer QR-Code auf dem Display** | LovyanGFX `qrcode()`: (1) WLAN-Zugang `WIFI:S:ADD-Demo;T:WPA;P:…;;`, (2) aktuelle URL (AP- oder STA-IP), (3) Einstieg KI-Service-Anfrage (Seite mit Geräteidentität, Link zur ADD-Datei, fertigem Prompt) | Handy/Tablet ohne Tippen verbinden; zeigt im Gegensatz zu einem Aufkleber die aktuelle IP und den Zustand |
| **NFC-Tag am Gerät** | NDEF-URL-Eintrag: Gerätetyp, Seriennummer, Links zu Repo und Doku | Dauerhafte Identität. Tags/Aufkleber sind austauschbar → die KI prüft die geladene ADD-Datei gegen das Gerät (z. B. Hash der ADD-Beschreibung in `/proc`) |
| **GitHub als einzige Informationsquelle** | Repo, Releases mit Prüfsummen, GitHub Pages für die HTML-Doku; Handbuch, Befehlssatz und ADD-Datei aus denselben Quelldaten erzeugt | Keine auseinanderlaufenden Dokumente; jede KI findet alles an einer Stelle |
| **Offline-Doku auf der Anlage** | Kompakte HTML-Fassung im SPIFFS (≈ 1 MB frei), ausgeliefert vom eingebauten Webserver im AP-Modus | Messen haben oft kein verlässliches Internet |
| **Dokumentation per KI** | Softwareschnittstelle und Befehlssatz aus `CONTRACT.md` und Headern, technische Beschreibung aus Pflichtenheft und Smoketest-Protokoll, Bedienhandbuch mit menschlichem Input (Abläufe, Fotos) | Sicherheitshinweise und Not-Aus: KI entwirft, Mensch prüft und gibt frei |
| **KI-gestützte Serientests (Präsentationsthema)** | Grundsatz: Sicherheit hängt nicht von der KI ab – deterministische Schicht (Verriegelungen, Zäune bei HV/Shutter, Not-Aus, Grenzwerte in der Firmware), die KI arbeitet nur innerhalb dieses Rahmens. Eingefrorene Kette (Modell, Quantisierung, Inferenz-Software, Prompt, Testvorgaben; Temperatur 0), Änderung = Requalifizierung. Erste Läufe menschlich begleitet mit vorher festgelegtem Endkriterium. Stufenweise: erst Sensorik, dann gefährliche Aktionen; Stufenübergänge per fester Regel, nicht per KI-Urteil. Langsame Bewegungen, Plausibilisierung mit weiteren Größen (Strom, Kühlwasserfluss, Brandmeldung). Watchdog → sicherer Zustand, lückenloses Protokoll, Ergebnis vom Menschen freigegeben. Eigenes Sicherheitskonzept (Rahmen IEC 61508 / ISO 13849, KI als nicht sicherheitsgerichteter Teil) | Die Demoanlage zeigt das Prinzip im Kleinen: Argumentprüfung, richtungsabhängige Endschalter, Freigaben in `.clinerules`, eingefrorene Contract-Tests |
| **Anlagenkoordinaten (Ebene 2, V1.7.5)** | Zwei Ebenen: Die Zonen und alle Sicherheitsprüfungen bleiben in **Maschinenkoordinaten** (Ebene 1, Nullpunkt auf der Parkseite). Für mehrere Messeinrichtungen – z. B. zwei leicht versetzt gegenüberstehende Messbügel, die ortssynchron messen – rechnet jedes Gerät an der Schnittstelle um: `x_Anlage = coord_offset + coord_sign · x_Maschine` (Parameter `coord_offset` in mm, `coord_sign` ±1). Positionen in Befehlen werden zuerst in Maschinenkoordinaten umgerechnet und **danach** gegen die Zonen geprüft; `/proc` meldet die Position zusätzlich in Anlagenkoordinaten. Für die zeitliche Zuordnung eine gemeinsame Zeitbasis (SNTP oder Synchronimpuls) | Ein Leitsystem oder eine KI schickt allen Geräten dieselbe Anlagenposition. Ein falscher Versatz oder ein falsches Vorzeichen kann höchstens `BAD_PARAM` oder eine falsche Position bewirken, nie eine Zonenverletzung. Betrifft nur API und `/proc` (S3), nicht die Sicherheitslogik |
