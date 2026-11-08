#!/usr/bin/env python3
"""check_add.py – read-only check of the ADD endpoints of a running device (Sprint D1).

Checks what an AI agent sees, without moving anything and without changing any setting:
  1. /llms.txt links /add, /add.html, /openapi.json and /doc
  2. /add is the ADD document (schema "add", version "1.0"), /add.html embeds exactly the same JSON
  3. /openapi.json is OpenAPI 3.1 and its info.version matches the firmware contract in /add
  4. /proc and the replies to GET_STATE and GET_CONFIG validate against the OpenAPI schemas,
     and their CRC frames verify
  5. the device rejects a wrong CRC (-1, HTTP 400) and an out-of-range SET (-4) – nothing is changed
  6. V1.6.5: /, /add, /add.html, /openapi.json, /doc with "Accept-Encoding: gzip" come compressed and
     unpack to the same bytes; the load time of each is printed (/add.html must take < 3 s)

    python3 scripts/check_add.py --base http://192.168.1.120
    python3 scripts/check_add.py --base https://test.norbert-walter.dnshome.de --auth

With --auth user and password of the reverse proxy are asked for (or taken from the environment
variables ADD_USER / ADD_PASS); they are never printed or stored. --user NAME asks only for the password. Needs: pip install jsonschema (only for step 4; without it the step is
skipped with a note).
"""
import argparse, base64, getpass, gzip, json, os, re, sys, time, urllib.error, urllib.request, zlib

OK, BAD = 'ok  ', 'FAIL'
results = []


def check(cond, text):
    results.append(bool(cond))
    print(OK if cond else BAD, text, flush=True)
    return cond


def frame(obj):
    body = json.dumps(obj, separators=(',', ':'))
    return body[:-1] + ',"crc":"%08X"}' % (zlib.crc32(body.encode()) & 0xFFFFFFFF)


def verify(text):
    t = text.rstrip(' \t\r\n')
    i = t.rfind(',"crc":"')
    if i < 0 or len(t) != i + 18 or not t.endswith('"}'):
        return False
    return '%08X' % (zlib.crc32((t[:i] + '}').encode()) & 0xFFFFFFFF) == t[i + 8:i + 16].upper()


class Dev:
    def __init__(self, base, user, password):
        self.base = base.rstrip('/')
        self.auth = None
        if user:
            self.auth = 'Basic ' + base64.b64encode(('%s:%s' % (user, password)).encode()).decode()

    def raw(self, path, data=None, gz=False):
        """-> (status, headers, body bytes, seconds)"""
        sep = '&' if '?' in path else '?'
        url = self.base + path + ('' if data else '%st=%d' % (sep, time.time()))
        r = urllib.request.Request(url, data=data.encode() if data else None, method='POST' if data else 'GET',
                                   headers={'Content-Type': 'text/plain'} if data else {})
        if gz:
            r.add_header('Accept-Encoding', 'gzip')
        if self.auth:
            r.add_header('Authorization', self.auth)
        t0 = time.time()
        try:
            with urllib.request.urlopen(r, timeout=60) as resp:
                return resp.status, resp.headers, resp.read(), time.time() - t0
        except urllib.error.HTTPError as e:
            return e.code, e.headers, e.read(), time.time() - t0

    def req(self, path, data=None):
        st, h, b, _ = self.raw(path, data)
        return st, h.get('Content-Type', ''), b.decode('utf-8', 'replace')

    def api(self, cmd, **params):
        return self.req('/api', frame({'cmd': cmd, 'params': params}))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--base', required=True, help='e.g. http://192.168.1.120 or https://host')
    ap.add_argument('--auth', action='store_true', help='ask for Basic Auth user and password of the reverse proxy')
    ap.add_argument('--user', help='Basic Auth user (the password is asked for, or ADD_PASS)')
    a = ap.parse_args()
    if a.auth and not a.user:
        a.user = os.environ.get('ADD_USER') or input('user: ').strip()
    pw = (os.environ.get('ADD_PASS') or getpass.getpass('password: ')) if a.user else None
    d = Dev(a.base, a.user, pw)

    st, _, llms = d.req('/llms.txt')
    check(st == 200, '/llms.txt HTTP %d' % st)
    for p in ('/add)', '/add.html)', '/openapi.json)', '/doc)'):
        check(p in llms, '/llms.txt links ' + p[:-1])

    st, typ, t = d.req('/add')
    check(st == 200 and 'json' in typ, '/add HTTP %d %s' % (st, typ))
    add = json.loads(t)
    check(add.get('schema') == 'add' and add.get('version') == '1.0', '/add is an ADD v1.0 document')
    st, _, h = d.req('/add.html')
    m = re.search(r'<script type="application/ld\+json">\n(.*?)\n</script>', h, re.S)
    check(st == 200 and m and json.loads(m.group(1).replace('<\\/', '</')) == add, '/add.html embeds the same JSON')

    st, typ, t = d.req('/openapi.json')
    if not check(st == 200 and 'json' in typ, '/openapi.json HTTP %d %s' % (st, typ)):
        return finish()
    oas = json.loads(t)
    check(oas.get('openapi', '').startswith('3.1'), 'OpenAPI 3.1')
    check(('CONTRACT V' + oas['info']['version']) in add['device']['firmware'],
          'contract version %s = /add device.firmware' % oas['info']['version'])
    check(add['interfaces'][0].get('x-openapi') == '/openapi.json', '/add interfaces[0].x-openapi')

    try:
        import jsonschema
    except ImportError:
        print('note jsonschema not installed – schema validation skipped (pip install jsonschema)')
        jsonschema = None

    def valid(name, obj):
        if not jsonschema:
            return True
        schema = dict(oas['components']['schemas'][name])
        schema['components'] = oas['components']
        schema['$schema'] = 'https://json-schema.org/draft/2020-12/schema'
        try:
            jsonschema.validate(obj, schema)
            return True
        except jsonschema.ValidationError as e:
            print('     ', e.message)
            return False

    st, _, t = d.req('/proc')
    check(st == 200 and verify(t), '/proc CRC frame')
    proc = json.loads(t)
    check(valid('Proc', proc), '/proc matches schema Proc (state %s)' % proc.get('state'))
    for cmd, extra in (('GET_STATE', 'GetStateExtra'), ('GET_CONFIG', 'GetConfigExtra')):
        st, _, t = d.api(cmd)
        r = json.loads(t)
        check(st == 200 and verify(t) and r.get('err') == 0, '%s err 0, CRC frame' % cmd)
        check(valid('ApiResponse', r) and valid(extra, r), '%s matches ApiResponse + %s' % (cmd, extra))
    st, _, t = d.req('/api', '{"cmd":"GET_STATE","params":{},"crc":"00000000"}')
    r = json.loads(t)
    check(st == 400 and r.get('err') == -1 and valid('ApiResponse', r), 'wrong CRC -> HTTP 400, err -1')
    st, _, t = d.api('SET', key='measure_pos', value=999)
    r = json.loads(t)
    check(st == 200 and r.get('err') == -4 and valid('ApiResponse', r), 'SET measure_pos 999 -> err -4 (nothing changed)')

    for p in ('/', '/add', '/add.html', '/openapi.json', '/doc'):     # V1.6.5: gzip copies
        st, h, plain, t1 = d.raw(p)
        st2, h2, packed, t2 = d.raw(p, gz=True)
        enc = h2.get('Content-Encoding', '')
        body = gzip.decompress(packed) if enc == 'gzip' else packed
        check(st == st2 == 200 and enc == 'gzip' and body == plain,
              '%-14s gzip %6d of %6d bytes  %.1f s (plain %.1f s)' % (p, len(packed), len(plain), t2, t1))
        if p == '/add.html':
            check(t2 < 3.0 and t1 < 3.0, '/add.html loads in < 3 s')
    return finish()


def finish():
    n = len(results)
    print('%d/%d checks passed' % (sum(results), n))
    sys.exit(0 if all(results) else 1)


if __name__ == '__main__':
    main()
