#!/usr/bin/env python3
"""make_qr.py – Aufkleber-QR-Codes für die Demoanlage ADD als SVG (zum Drucken, 40 mm breit).

    python3 startup/make_qr.py              # qr-startseite.svg und qr-webui.svg in startup/
    python3 startup/make_qr.py --wifi       # WLAN-QR: fragt SSID und Passwort, schreibt tmp/qr-wlan.svg

Der WLAN-QR enthält das Passwort im Klartext. Er wird deshalb nur nach tmp/ geschrieben (steht in
.gitignore) und gehört nicht ins Repository – drucken, dann löschen. Das Passwort wird verdeckt
abgefragt und nirgends sonst gespeichert.

Braucht:  python3 -m pip install segno   (mit demselben python3, der das Skript startet)
"""
import argparse, getpass, os, sys

try:
    import segno
except ImportError:
    sys.exit('segno fehlt für %s – installieren mit:\n  %s -m pip install segno' % (sys.executable, os.path.basename(sys.executable)))

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..'))

START_URL = 'https://norbert-walter.github.io/add-demo-slim/'
WEBUI_URL = 'http://192.168.2.100/'

WIDTH_MM = 40.0      # printed width of the sticker
QUIET = 4            # quiet zone in modules (QR standard)


def wifi_escape(s):
    """Escape \\ ; , : " in WIFI: payloads (ZXing convention, understood by iOS and Android)."""
    for ch in '\\;,:"':
        s = s.replace(ch, '\\' + ch)
    return s


def sticker_svg(data, lines, error='q'):
    """QR code with up to three caption lines below it, as a standalone SVG in millimetres."""
    qr = segno.make(data, error=error, micro=False)
    m = [list(row) for row in qr.matrix]
    n = len(m)
    size = n + 2 * QUIET
    unit = WIDTH_MM / size                      # mm per module
    cap_h = 5.2 * len(lines) + 2.0               # mm for the caption block
    h = WIDTH_MM + cap_h
    rects = []
    for y, row in enumerate(m):
        x = 0
        while x < n:
            if row[x]:
                x0 = x
                while x < n and row[x]:
                    x += 1
                rects.append('M%d %dh%dv1h-%dz' % (x0 + QUIET, y + QUIET, x - x0, x - x0))
            else:
                x += 1
    path = ''.join(rects)
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%.1fmm" height="%.1fmm" viewBox="0 0 %.3f %.3f">'
           % (WIDTH_MM, h, WIDTH_MM, h),
           '<rect width="100%" height="100%" fill="#ffffff"/>',
           '<g transform="scale(%.5f)"><path fill="#000000" d="%s"/></g>' % (unit, path)]
    y = WIDTH_MM + 1.0
    for i, text in enumerate(lines):
        bold = i == 0
        y += 5.0 if bold else 4.6
        out.append('<text x="%.2f" y="%.2f" text-anchor="middle" font-family="Ubuntu, Arial, sans-serif" '
                   'font-size="%.1f" font-weight="%s" fill="#000000">%s</text>'
                   % (WIDTH_MM / 2, y, 3.6 if bold else 3.0, '700' if bold else '400',
                      text.replace('&', '&amp;').replace('<', '&lt;')))
    out.append('</svg>')
    return '\n'.join(out) + '\n', qr.version


def write(path, svg):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(svg)
    print('geschrieben:', os.path.relpath(path, REPO))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--wifi', action='store_true', help='WLAN-QR erzeugen (fragt SSID und Passwort)')
    a = ap.parse_args()
    if a.wifi:
        ssid = input('SSID des Demo-WLANs: ').strip()
        pw = getpass.getpass('WLAN-Passwort (wird nicht angezeigt): ')
        if not ssid or len(pw) < 8:
            sys.exit('SSID leer oder Passwort kürzer als 8 Zeichen')
        data = 'WIFI:T:WPA;S:%s;P:%s;;' % (wifi_escape(ssid), wifi_escape(pw))
        svg, ver = sticker_svg(data, ['WLAN der Anlage', 'scannen und verbinden', 'dann Web-UI: 192.168.2.100'])
        os.makedirs(os.path.join(REPO, 'tmp'), exist_ok=True)
        write(os.path.join(REPO, 'tmp', 'qr-wlan.svg'), svg)
        print('Achtung: enthält das Passwort – nicht committen, nach dem Drucken löschen.')
        return
    svg, ver = sticker_svg(START_URL, ['Demoanlage ADD', 'Info · KI-Helfer · Web-UI', 'oder NFC hier antippen'])
    write(os.path.join(HERE, 'qr-startseite.svg'), svg)
    svg, ver = sticker_svg(WEBUI_URL, ['Web-UI', 'nur im WLAN der Anlage', '192.168.2.100'])
    write(os.path.join(HERE, 'qr-webui.svg'), svg)


if __name__ == '__main__':
    main()
