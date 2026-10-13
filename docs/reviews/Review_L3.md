# Review Sprint L3 – motion_seq, sound_logic, wifi_logic

**Implementierung:** Qwen (Cline, lokal) · **Review:** Claude · **Freigabe:** Norbert Walter
**Stand:** 14.10.2026 · Contract V1.5.8 · Branch `sprint-l3` → `master`, Tag `sprint-l3`

## Ergebnis

| Prüfung | Ergebnis |
|---|---|
| `test_motion_seq` | 38/38 |
| `test_sound_logic` | 9/9 |
| `test_wifi_logic` | 15/15 |
| `test_app_logic` mit L1–L3-Code (nur app_logic aus der Referenz) | 63/63 |
| alle Suiten unter AddressSanitizer / UndefinedBehaviorSanitizer | ohne Befund |
| eingefrorene Dateien | unverändert |
| Differenztest (Sequenzer ca. 1 Mio. Schritte, Buzzer 1,2 Mio., WLAN 4 Mio.) | Verhalten identisch; nur interne Werte A1–A3 abweichend |

## Verlauf

| Schritt | Befund | Folge |
|---|---|---|
| Vorbereitung | Mutationstest gegen Modul-Suiten und `test_app_logic`: 26 von 72 Mutanten überlebten (wifi_logic 11/14) | Contract V1.5.7, +18 Tests |
| L3.1–L3.3 (60/60) | Differenztest: Prüfreihenfolge BUSY/PRECONDITION vertauscht (Qwen erfand eine „V1.4.1“-Begründung im Kommentar); Umrechnung mit `(int)(x+0.5)` in double statt `lroundf` | Contract V1.5.8, +2 Tests, Aufgabe L3.4 |
| L3.4 (62/62) | beide Punkte korrekt behoben, nur die betroffenen Zeilen geändert | Freigabe |

## Token-Verbrauch (Cline, Qwen3.6-35B-A3B lokal)

| Task | ↑ Eingabe | ↓ Ausgabe | → Cache |
|---|---:|---:|---:|
| L3.1 motion_seq | 121 200 | 20 100 | 1 400 000 |
| L3.2 sound_logic | 15 800 | 4 000 | 95 900 |
| L3.3 wifi_logic | 26 100 | 4 900 | 261 100 |
| L3.4 Nacharbeit | 19 300 | 2 600 | 174 200 |
| **Summe L3** | **182 400** | **31 600** | **1 931 200** |

Vergleich (Ausgabe-Token / Referenzumfang): L1 267 500 / 282 Zeilen, L2 18 800 / 143,
L3 31 600 / 227.

## Akzeptierte Abweichungen (nur interne Werte, vom Header nicht festgelegt, ohne Wirkung)

- **A1:** `motionEmergencyStop` und Fehlerabbrüche setzen `phase_start_ms` anders als die Referenz
  (im Zustand IDLE ohne Bedeutung; jeder Start setzt den Wert neu).
- **A2:** Nach der Referenzfahrt bleibt `target_steps` auf dem Pull-off-Ziel (Referenz: 0);
  jeder neue Start setzt das Ziel neu.
- **A3:** `staStep` lässt `t0_ms` im Zustand OFF unverändert (Referenz: 0); OFF → CONNECTING
  setzt `t0_ms` neu.

## Erkenntnisse

1. Qwen „begründet“ Abweichungen mit erfundenen Contract-Versionen im Kommentar. Kommentare
   mit Versionsangaben im Review gezielt gegen `CONTRACT.md` prüfen.
2. Die Prüfung „busy *und* nicht referenziert“ fehlte, weil der Test beide Bedingungen nicht
   gleichzeitig verletzte. Bei Prüfreihenfolgen immer Kombinationen testen, in denen mehrere
   Bedingungen gleichzeitig verletzt sind.
3. Differenztest: Eingaben dürfen nicht vom internen Zustand der geprüften Implementierung
   abhängen, sonst erzeugen gleichwertige Abweichungen Folgeunterschiede.
4. Kopierbare Befehle ohne Platzhalter; `git checkout -b` nur einmal je Sprint.
