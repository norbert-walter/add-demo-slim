# Review Sprint P2 – wifi_manager, http_server

**Implementierung:** Qwen (Cline v4.1.24, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 15.10.2026 · Contract V1.5.17 · Branch `work-p2` → `master`, Tag `sprint-p2`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_wifi_manager` | 10/10 (auch unter ThreadSanitizer ohne Befund) |
| `test_http_server` | 19/19 (auch unter ThreadSanitizer ohne Befund) |
| alle 18 Suiten mit dem gesamten Qwen-Code L1–P2 | 413/413 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien, Shims, `platformio.ini` | unverändert |
| Compiler (`-Wall -Wextra`) für `src/comm/*.cpp` | ohne Warnung |
| `pio run -e firmware-partial` (echte ESP32-Header, Bericht Norbert) | `[SUCCESS]`, WebServer/SPIFFS/WiFi/ESPmDNS eingebunden |
| Differenztest `wifiLoop` (60 × 20 000 Schritte: zufällige Verbindungsverläufe, Zeitsprünge, uint32-Überlauf, Neu-Initialisierung; WiFi-Aufrufe, Status, IP, Log) | identisch |
| Differenztest HTTP (60 × 1 500 zufällige Anfragen: Methoden, Pfade, Dateien mit `< / & >`, CRC-gerahmte und verstümmelte Körper, Snapshot/Konfiguration, Antworten der Steuerung) | identisch (Code, Typ, Kopfzeilen, Inhalt, Chunk-Ende, Heartbeat) |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Header bisher grob; Nachbildungen für WiFi, mDNS, WebServer, SPIFFS/File, String, IPAddress mit echten Signaturen; eigene Referenz, 2 neue Suiten (26 Tests) mit Paralleltests; Mutationstest 47 Mutanten, 4 überlebten → 3 Tests ergänzt, 1 nur mit ThreadSanitizer erkennbar | Contract V1.5.16, `OPTIONS /api` (CORS-Preflight) ergänzt |
| P2.1–P2.3 (26/26) | (1) `wifiLoop` ruft `WiFi.status()` und `staStep` im kritischen Abschnitt auf (auf dem ESP32: FreeRTOS bei gesperrten Interrupts); (2) `/add.html` sendet jedes Zeichen als eigenen Chunk (~8 000 TCP-Schreiboperationen für 4 kB) und verliert ein `<` am Dateiende; (3) `BAD_PARAM` bei GET_STATE/GET_CONFIG wird mit OK beantwortet (nur im Differenztest). Keiner der Tests bemerkte das. Außerdem: Wettlauf im Zähler der Nachbildung selbst (`shimCritEnters`), von Claude in V1.5.16 übersehen (gekürzte Ausgabe) | Contract V1.5.17: Nachbildungen zählen WiFi-Aufrufe in der Sperre und Chunks, +3 Tests, Zähler atomar; Aufgaben P2.4/P2.5 mit Diagnose |
| P2.4, P2.5 (29/29) | korrekt; Differenztests ohne Abweichung | Freigabe |

Hinweis zum Testlauf: Scheitern Tests, meldet PlatformIO nach dem letzten Test „SIGFPE“ und zählt
einen Testfall mehr. Unity gibt die Zahl der Fehlschläge als Rückgabewert zurück (hier 8), und
PlatformIO deutet ihn als Signal 8. Mit allen Tests grün tritt das nicht auf.

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| P2.1 wifi_manager | 106 900 | 63 700 | 1 300 000 |
| P2.2 http_server ohne POST /api | 69 900 | 20 800 | 576 500 |
| P2.3 POST /api | 58 200 | 14 000 | 814 500 |
| P2.4 /add.html blockweise, BAD_PARAM | 28 100 | 8 200 | 425 300 |
| P2.5 WiFi nicht in der Sperre | 13 900 | 2 300 | 144 400 |
| **Summe P2** | **277 000** | **109 000** | **3 260 700** |

Kein Task abgebrochen. Vergleich (Ausgabe-Token / Referenzumfang): L5 36 400 / 180 Zeilen,
P1 30 300 / 160, P2 109 000 / 234. P2.1 allein brauchte 63 700 Ausgabe-Token: Qwen suchte selbst
die Ursache für fehlende „attempt“-Zeilen (Versuch läuft ab, Zustand bleibt `CONNECTING`).

## Akzeptierte Abweichungen

- **A1:** `wifiLoop` ruft `staStep` innerhalb des kritischen Abschnitts auf (ein Abschnitt statt
  zwei); der Header V1.5.17 verlangt den Aufruf außerhalb. `staStep` ist reine Logik ohne
  Systemaufrufe (wenige Vergleiche), die Sperrzeit verlängert sich um Mikrosekunden. Mit den
  Nachbildungen nicht prüfbar.
- **A2:** `sendScriptJson`/`sendHtmlEscaped` legen je 2 048 Byte Ausgabepuffer auf den Stack
  (64 Byte Eingabe → höchstens 320 Byte Ausgabe würden genügen). Netzwerk-Task: 8 192 Byte Stack.
  In I1 am Gerät mit `uxTaskGetStackHighWaterMark` prüfen; Kandidat für eine Bereinigung.
- **A3:** `OPTIONS /api` antwortet mit `send(204)` ohne Content-Type (Referenz: `text/plain`).
  Bei 204 ohne Inhalt ohne Bedeutung.
- **A4:** `wifiInit` kopiert mit `strncpy` und prüft nicht auf `nullptr` (Referenz: `""`). Die
  Aufrufer übergeben immer die Felder von `Params`.
- **A5:** `handleProc` und `reply` werten einen negativen Rückgabewert von `procToJson` /
  `responseToJson` nicht aus. Mit `PROC_JSON_MAX` bzw. 1 024 Byte Puffer nicht erreichbar.

## Erkenntnisse

1. **Was die Nachbildung nicht zählt, prüft kein Test:** Die beiden schwersten Befunde (WiFi in
   der Sperre, ein TCP-Paket pro Zeichen) sind auf dem PC funktional korrekt. Erst Zähler in den
   Nachbildungen (Aufrufe in der Sperre, Chunks) machen solche Gerätefehler zu roten Tests.
   Für H1 heißt das: die FastAccelStepper-Nachbildung muss Reihenfolge und Zeitpunkt der Aufrufe
   aufzeichnen, nicht nur das Ergebnis.
2. **Differenztest mit zufälligen Anfragen bleibt Pflicht:** `BAD_PARAM` bei GET_STATE war eine
   Reihenfolge-Frage im Code, die kein handgeschriebener Test abdeckte.
3. **Eigene Prüfungen genauso ernst nehmen:** Die Aussage „ohne Befund unter ThreadSanitizer“ in
   V1.5.16 stammte aus einer gekürzten Ausgabe. Sanitizer-Läufe werden ab jetzt auf die Zeile
   „reported N warnings“ geprüft, nicht nur auf die SUMMARY-Zeile von Unity.
4. **Diagnose + Umsetzung:** P2.4/P2.5 mit vorgegebener Diagnose kosteten zusammen 10 500
   Ausgabe-Token – wie in L5 setzt Qwen eine fertige Diagnose zuverlässig und billig um.
