#!/usr/bin/env python3
"""sim_device.py – the demo device on a PC with the REAL firmware logic (src/logic).

Unlike fake_device.py (a small Python imitation for the web UI), this runs the C++ logic of the
firmware – safety state machine, motion sequencer, measuring chain, parameters, /api parser,
/proc and reply JSON – in scripts/sim/logic_sim.cpp with a simulated axis and the demo stair body.
Time runs in real time (1 ms control cycles). Not simulated: WLAN, display, timing values
(cycle_max_us ... are 0), E-stop wiring (estop_nc).

    python3 scripts/sim_device.py [port]            (default 8090) -> http://localhost:8090/

Builds the simulator on first start with g++ (C++17) and ArduinoJson 7 (searched in
.pio/libdeps/native/ArduinoJson/src, or set ADD_ARDUINOJSON=<path to ArduinoJson/src>).

Endpoints as on the device: / /settings /doc /app.js /font-*.woff2 /proc /api (POST, OPTIONS) /log
/llms.txt /add /add.html /openapi.json (D1; gzip copies like the firmware, V1.6.5). Test hooks (not on the device):
    GET /sim/set?estop=1|0&press_fwd=1|0&press_bwd=1|0&block=1|0
"""
import json, os, subprocess, sys, threading, time
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..'))
DATA = os.path.join(REPO, 'data')
BIN = os.path.join(REPO, '.pio', 'sim', 'logic_sim')
FILES = {'/': ('www/index.html', 'text/html'), '/settings': ('www/settings.html', 'text/html'),
         '/doc': ('doc.html', 'text/html'), '/app.js': ('www/app.js', 'application/javascript'),
         '/add': ('add.json', 'application/json'), '/openapi.json': ('openapi.json', 'application/json'),
         '/add.html': ('add.html', 'text/html; charset=utf-8')}   # V1.6.5: a file (scripts/build_data.py)
for f in ('u400', 'u700', 'm400', 'm700'):
    FILES['/font-%s.woff2' % f] = ('www/font-%s.woff2' % f, 'font/woff2')
FILES['/favicon.ico'] = ('www/favicon.ico', 'image/x-icon')        # V1.6.5
LLMS = ('# AI Device Description (ADD)\n- ADD (JSON): [Device Description](/add)\n'
        '- ADD (HTML): [Device Description for Cloud-AI](/add.html)\n'
        '- OpenAPI (JSON): [HTTP API, technical reference](/openapi.json)\n- Documentation: [Device manual](/doc)\n')


def build():
    src = os.path.join(REPO, 'src', 'logic')
    srcs = [os.path.join(HERE, 'sim', 'logic_sim.cpp')] + sorted(
        os.path.join(src, f) for f in os.listdir(src) if f.endswith('.cpp'))
    if os.path.exists(BIN) and os.path.getmtime(BIN) > max(os.path.getmtime(s) for s in srcs):
        return
    aj = os.environ.get('ADD_ARDUINOJSON') or next(
        (p for p in (os.path.join(REPO, '.pio', 'libdeps', 'native', 'ArduinoJson', 'src'), '/tmp/ArduinoJson/src')
         if os.path.exists(os.path.join(p, 'ArduinoJson.h'))), None)
    if not aj:
        raise SystemExit('ArduinoJson not found: run "pio test -e native" once, or set ADD_ARDUINOJSON')
    os.makedirs(os.path.dirname(BIN), exist_ok=True)
    print('building the logic simulator ...', flush=True)
    subprocess.run(['g++', '-std=c++17', '-O2', '-I', os.path.join(REPO, 'src'), '-I', aj, *srcs, '-o', BIN],
                   check=True)


class Sim:
    def __init__(self):
        self.p = subprocess.Popen([BIN], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        self.lock = threading.Lock()
        self.t0 = time.monotonic()
        self.log = []

    def req(self, line):
        with self.lock:
            now = 1000 + int((time.monotonic() - self.t0) * 1000)
            self.p.stdin.write('T %d\n' % now); self.p.stdin.flush()
            self._read()
            self.p.stdin.write(line + '\n'); self.p.stdin.flush()
            return self._read()

    def _read(self):
        r = json.loads(self.p.stdout.readline())
        for l in r.get('log', []):
            print('log:', l, flush=True)
            self.log = (self.log + [l])[-20:]
        return r

    def tick(self):
        while True:
            self.req('T 0')          # advance to the current time
            time.sleep(0.01)


SIM = None


class H(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.0'

    def log_message(self, *a): pass

    def send(self, code, typ, body, extra=None):
        b = body.encode() if isinstance(body, str) else body
        self.send_response(code)
        if typ: self.send_header('Content-Type', typ)
        self.send_header('Content-Length', str(len(b)))
        self.send_header('Access-Control-Allow-Origin', '*')
        for k, v in (extra or {}).items(): self.send_header(k, v)
        self.end_headers(); self.wfile.write(b)

    def nf(self): return self.send(404, 'application/json', '{"error":"not found"}')

    def do_GET(self):
        u = urlparse(self.path); path = u.path
        if path in FILES:
            f, typ = FILES[path]
            extra = {'Cache-Control': 'no-store'} if path in ('/add', '/openapi.json') else (
                {'Cache-Control': 'no-store, no-cache, must-revalidate', 'Pragma': 'no-cache'} if path == '/add.html' else (
                {'Cache-Control': 'max-age=86400'} if path.endswith(('.woff2', '.ico')) else {}))
            fn = os.path.join(DATA, f)                  # V1.6.5: gzip copy if accepted, like the firmware
            if not path.endswith(('.woff2', '.ico')):
                extra['Vary'] = 'Accept-Encoding'
                if 'gzip' in self.headers.get('Accept-Encoding', '') and os.path.exists(fn + '.gz'):
                    fn += '.gz'; extra['Content-Encoding'] = 'gzip'
            try: data = open(fn, 'rb').read()
            except OSError: return self.nf()
            return self.send(200, typ, data, extra or None)
        if path == '/proc':
            return self.send(200, 'application/json', SIM.req('P')['proc'], {'Cache-Control': 'no-store'})
        if path == '/log':
            return self.send(200, 'text/plain', ''.join(l + '\n' for l in SIM.log))
        if path == '/llms.txt':
            return self.send(200, 'text/plain', LLMS)
        if path == '/sim/set':
            for k, v in parse_qs(u.query).items():
                SIM.req('H %s %d' % (k, 1 if v[0] in ('1', 'true') else 0))
            return self.send(200, 'application/json', '{"ok":true}')
        return self.nf()

    def do_POST(self):
        if urlparse(self.path).path != '/api': return self.nf()
        body = self.rfile.read(int(self.headers.get('Content-Length', 0))).decode(errors='replace')
        body = body.replace('\n', ' ').replace('\r', ' ')
        if not body or len(body) > 512:
            r = SIM.req('A {}')                       # -> BAD_CRC/BAD_JSON reply of the parser
        else:
            r = SIM.req('A ' + body)
        return self.send(r['code'], 'application/json', r['body'])

    def do_OPTIONS(self):
        if urlparse(self.path).path != '/api': return self.nf()
        self.send_response(204)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.send_header('Content-Length', '0'); self.end_headers()


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8090
    build()
    SIM = Sim()
    threading.Thread(target=SIM.tick, daemon=True).start()
    print('simulated ADD device (real firmware logic) on http://localhost:%d/  (Ctrl+C to stop)' % port, flush=True)
    ThreadingHTTPServer(('', port), H).serve_forever()
