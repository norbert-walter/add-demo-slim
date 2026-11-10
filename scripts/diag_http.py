#!/usr/bin/env python3
"""diag_http.py – where does the time go when loading /add.html? (D1.2, 10.11.2026)

Loads some files from the device directly (local network) and, with --remote, through the reverse
proxy, each once plain and once with "Accept-Encoding: gzip". Prints per request: HTTP status,
bytes received, Content-Encoding, Content-Length/chunked, time to the first byte, the times at which
25/50/75 % of the body had arrived, total time and throughput. Only GET requests, nothing is changed.

    python3 scripts/diag_http.py --local http://192.168.1.120
    python3 scripts/diag_http.py --local http://192.168.1.120 --remote https://test.norbert-walter.dnshome.de

With --remote user and password of the proxy are asked for; they are not printed or stored.
"""
import argparse, base64, getpass, ssl, sys, time, urllib.error, urllib.request

PATHS = ['/add.html', '/', '/add', '/openapi.json', '/proc']


def fetch(url, gz, auth):
    r = urllib.request.Request(url + ('&' if '?' in url else '?') + 't=%d' % (time.time() * 1000))
    if gz:
        r.add_header('Accept-Encoding', 'gzip')
    if auth:
        r.add_header('Authorization', auth)
    t0 = time.time()
    try:
        resp = urllib.request.urlopen(r, timeout=120, context=ssl.create_default_context())
    except urllib.error.HTTPError as e:
        resp = e
    except Exception as e:  # noqa: BLE001 – report and go on
        return '%s' % e
    ttfb = time.time() - t0
    h = resp.headers
    marks, n, total = [], 0, int(h.get('Content-Length') or 0)
    while True:
        b = resp.read(1024)
        if not b:
            break
        n += len(b)
        marks.append((n, time.time() - t0))
    t = time.time() - t0
    q = lambda f: next((s for m, s in marks if m >= f * n), t) if n else t
    return ('%3d %7d B %-5s %-9s ttfb %5.2f s  25%% %5.2f  50%% %5.2f  75%% %5.2f  total %6.2f s  %6.1f kB/s  server=%s' %
            (resp.status if hasattr(resp, 'status') else resp.code, n, h.get('Content-Encoding') or '-',
             'len=%d' % total if total else ('chunked' if 'chunked' in (h.get('Transfer-Encoding') or '') else 'close'),
             ttfb, q(.25), q(.5), q(.75), t, n / 1024 / t if t else 0, h.get('Server') or '-'))


def run(base, auth, label):
    print('\n== %s  %s' % (label, base))
    for p in PATHS:
        for gz in (False, True):
            print('%-14s %-5s %s' % (p, 'gzip' if gz else 'plain', fetch(base.rstrip('/') + p, gz, auth)), flush=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--local', required=True, help='e.g. http://192.168.1.120')
    ap.add_argument('--remote', help='e.g. https://test.norbert-walter.dnshome.de (asks for user and password)')
    a = ap.parse_args()
    run(a.local, None, 'direkt')
    if a.remote:
        user = input('Benutzer: ').strip()
        pw = getpass.getpass('Passwort: ')
        run(a.remote, 'Basic ' + base64.b64encode(('%s:%s' % (user, pw)).encode()).decode(), 'Proxy')


if __name__ == '__main__':
    main()
