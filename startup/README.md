# Startseite und Aufkleber – Demoanlage ADD

Einstieg am Gerät: **QR-Code und NFC-Tag** führen auf eine Startseite mit drei Einträgen
(Projektinfo, KI-Helfer, Web-UI). Ein **WLAN-QR-Code** verbindet mit dem Router der Anlage, ein
**Web-UI-QR-Code** öffnet direkt die Bedienoberfläche.

| Datei | Inhalt |
|---|---|
| `index.html` | Startseite (deutsch, eine Datei, keine externen Abhängigkeiten, Stil nach `docs/reference/ADD_Style_Guide_IMS_V1_0.md`) |
| `fonts/` | Ubuntu 400/700 mit Lizenz (Kopie aus `data/www/`) |
| `qr-startseite.svg` | Aufkleber: QR auf die Startseite, Hinweis auf NFC |
| `qr-webui.svg` | Aufkleber: QR auf `http://192.168.2.100/` (nur im WLAN der Anlage) |
| `make_qr.py` | erzeugt die beiden SVGs und – lokal – den WLAN-QR |
| `../.github/workflows/pages.yml` | veröffentlicht diesen Ordner mit GitHub Pages |

Adresse der Startseite: **https://norbert-walter.github.io/add-demo-slim/**

## 1. GitHub Pages einschalten (einmalig)

1. Auf GitHub: *Settings → Pages → Build and deployment → Source* auf **GitHub Actions** stellen.
2. Nach dem nächsten Push mit Änderungen in `startup/` (oder *Actions → Startseite veröffentlichen →
   Run workflow*) ist die Seite unter der Adresse oben erreichbar. Veröffentlicht werden nur
   `index.html`, die SVGs und `fonts/`.

Texte und Links ändern: direkt in `index.html`. Der Prompt des KI-Helfers steht im
`<textarea id="prompt">`; die Links auf Claude werden daraus beim Laden gebaut.

## 2. KI-Helfer

- **Prompt kopieren und Claude öffnen**: kopiert den Prompt und öffnet `https://claude.ai/new?q=…`
  in einem neuen Tab. Das Vorausfüllen über `?q=` ist für claude.ai im Browser nicht offiziell
  dokumentiert; klappt es nicht, fügt man den kopierten Prompt ein.
- **In Claude Desktop öffnen**: `claude://claude.ai/new?q=…` – dokumentiert, der Prompt steht im
  Eingabefeld und wird nicht automatisch gesendet (bis etwa 14 000 Zeichen).
- Der Helfer liest das ADD-Dokument aus GitHub (`data/add.json`, Raw-URL). Er sieht keine Live-Werte
  und steuert nichts; die Internetadresse der Anlage verlangt Benutzer und Passwort.

## 3. Router DIR-300 (DD-WRT)

**Feste Adresse der Anlage** – *Services → Services → DHCP Server → Static Leases → Add*:
MAC-Adresse des ESP32 (steht im DD-WRT unter *Status → LAN → DHCP Clients*), Hostname `add-demo`,
IP **192.168.2.100**. Danach die Anlage neu starten. Das Netz des Routers darf nicht `192.168.4.x`
sein – das gehört dem eigenen Access Point des ESP32.

**Name `add.demo`** (für den zweiten Knopf der Startseite) – *Services → Services → DNSMasq*:
DNSMasq einschalten, unter *Additional DNSMasq Options*:
```
address=/add.demo/192.168.2.100
```

**WLAN:** WPA2-PSK, 2,4 GHz (der ESP32 kann nur 2,4 GHz). Ein Captive Portal gibt es auf diesem
Build nicht; der Web-UI-QR-Code ersetzt es.

## 4. QR-Codes erzeugen

```
python3 -m pip install segno
```
```
python3 startup/make_qr.py
```
schreibt `qr-startseite.svg` und `qr-webui.svg` neu (nur nötig, wenn sich eine Adresse ändert).

WLAN-QR (fragt SSID und Passwort, das Passwort verdeckt):
```
python3 startup/make_qr.py --wifi
```
schreibt `tmp/qr-wlan.svg`. **Er enthält das WLAN-Passwort im Klartext:** nicht committen (`tmp/`
steht in `.gitignore`), drucken, dann löschen.

Drucken: SVG in 100 % Größe (40 mm breit) auf Etikettenfolie, möglichst matt. Unter 25 mm Breite
lesen ältere Handys den Code schlecht. Fehlerkorrektur Stufe Q (25 %) – kleine Kratzer schaden nicht.

## 5. NFC-Tag

- Tag: **NTAG213** (144 Byte reichen für die Adresse), auf Metall eine **On-Metal**-Ausführung.
- Beschreiben mit einer App, z. B. *NFC Tools* (Android, iOS): *Schreiben → Datensatz hinzufügen →
  URL* → `https://norbert-walter.github.io/add-demo-slim/` → *Schreiben*.
- Danach **sperren** (*Weitere Optionen → Tag sperren*), sonst kann ihn jeder Besucher mit einer
  App überschreiben. Das Sperren ist endgültig – vorher mit dem eigenen Handy testen.
- iPhones ab XS und aktuelle Android-Handys öffnen den Link ohne App, wenn man den Tag mit der
  Oberkante des Handys berührt.
