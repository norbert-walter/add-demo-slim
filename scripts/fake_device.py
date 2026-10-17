#!/usr/bin/env python3
"""fake_device.py – a simulated ADD Demo device for the web UI (Sprint W1, V1.5.22).

Serves data/www exactly at the routes of the firmware (/, /settings, /app.js, /style.css, fonts,
/add) and simulates /proc and /api with the CRC frame of CONTRACT §4. No hardware, no motor.

    python3 scripts/fake_device.py [port]          (default 8080) -> http://localhost:8080/

The simulation is deliberately small: INIT homes in 2 s, HV_ON needs interlock, cooling flow and
temp <= 30, START_TRAVERSE runs from traverse_start to traverse_end, STOP stops, SET/GET_CONFIG/
CALIB/ACK/REBOOT answer. Test hooks (not part of the device):
    GET /fake/log            -> JSON list of every /api command received (cmd, params)
    GET /fake/set?k=v&...    -> force fields: state, err, local, estop, comm_armed, crc_error (0/1)
"""
import json, os, sys, threading, time, zlib
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

ROOT = os.environ.get('ADD_FAKE_ROOT') or os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
ROUTES = {'/': ('www/index.html', 'text/html'), '/settings': ('www/settings.html', 'text/html'),
          '/app.js': ('www/app.js', 'application/javascript'), '/style.css': ('www/style.css', 'text/css'),
          '/font-u400.woff2': ('www/font-u400.woff2', 'font/woff2'), '/font-u700.woff2': ('www/font-u700.woff2', 'font/woff2'),
          '/font-m400.woff2': ('www/font-m400.woff2', 'font/woff2'), '/font-m700.woff2': ('www/font-m700.woff2', 'font/woff2')}
ERR_MSG = {-4: 'bad parameter', -5: 'not allowed in this state', -6: 'precondition not met', -7: 'busy',
           -3: 'unknown command', -1: 'bad crc', -2: 'bad json', -10: 'local operator has priority'}
SAMPLE_ADD = {"add_version": "1.0", "validation": {"validated_by": [
    {"name": "Claude", "version": "fake-model", "validated_at": "2026-10-17T10:00:00Z", "status": "passed_with_warnings",
     "score": {"structure": "pass", "comprehensibility": "pass", "functional": "pass", "rules_compliance": "pass",
               "security": "warning", "discovery": "pass", "timing_compliance": "pass"}}]}}


def crc(s):
    return '%08X' % (zlib.crc32(s.encode()) & 0xFFFFFFFF)


def frame(obj):
    body = json.dumps(obj, separators=(',', ':'), ensure_ascii=False)
    return body[:-1] + ',"crc":"' + crc(body) + '"}'


def verify(t):
    t = t.rstrip(); i = t.rfind(',"crc":"')
    return i >= 0 and len(t) == i + 18 and t.endswith('"}') and crc(t[:i] + '}') == t[i + 8:i + 16].upper()


class Sim:
    def __init__(self):
        self.lock = threading.Lock()
        self.log = []
        self.state, self.err, self.local, self.homed = 'SAFE_START', 0, False, False
        self.pos, self.vel, self.mstate, self.mode, self.target, self.t_end = 0.0, 0.0, 'idle', 'none', 0.0, 0.0
        self.estop, self.hv_on, self.shutter, self.comm_armed, self.crc_error = False, False, 'closed', False, False
        self.cfg = {"measure_pos": 155.0, "maint_pos": 40.0, "traverse_start": 80.0, "traverse_end": 240.0,
                    "traverse_speed": 10.0, "shutter_hold": 3.0, "edge_threshold": 3.0, "base_dist": 70.0,
                    "calib_near_raw": 40.0, "calib_near_ref": 40.0, "calib_far_raw": 65.0, "calib_far_ref": 65.0,
                    "calib_valid": True, "interlock": True, "cooling_flow": True, "temp": 18.0, "estop_nc": False,
                    "wifi_ssid": "", "wifi_pass_set": False}
        self.n, self.sum, self.t0 = 0, 0.0, time.time()
        self.last_api = time.time()

    def step(self):
        now = time.time()
        if self.mstate == 'homing' and now >= self.t_end:
            self.mstate, self.homed, self.pos, self.vel, self.state = 'idle', True, 0.0, 0.0, 'READY'
        if self.mstate == 'moving':
            d = self.target - self.pos; sp = self.vel * 0.05 * (1 if d > 0 else -1)
            if abs(d) <= abs(sp): self.pos, self.mstate, self.vel = self.target, 'idle', 0.0
            else: self.pos += sp
        if self.state == 'FAULT': self.state = 'WAIT_ACK'
        # communication watchdog (CONTRACT §6.5): disarmed when idle and HV off, fault after 5 s silence
        if self.comm_armed and self.mstate == 'idle' and not self.hv_on: self.comm_armed = False
        if self.comm_armed and now - self.last_api > 5.0:
            self.comm_armed, self.hv_on, self.shutter = False, False, 'closed'
            self.mstate, self.vel, self.mode = 'idle', 0.0, 'none'
            self.state, self.err = 'FAULT', 3

    def proc(self):
        dist = 70.0 - (12.0 if 105 <= self.pos <= 225 else 0.0)
        thick = max(0.0, self.cfg['base_dist'] - dist)
        valid = self.shutter == 'open'
        if valid and thick > self.cfg['edge_threshold']: self.n += 1; self.sum += thick
        return {"ts": int((time.time() - self.t0) * 1000), "state": self.state, "err": self.err, "local": self.local,
                "motion": {"pos": round(self.pos, 1), "vel": round(self.vel, 1), "state": self.mstate, "mode": self.mode,
                           "homed": self.homed, "limit_fwd": False, "limit_bwd": False},
                "measure": {"dist": dist, "thick": thick, "on_material": thick > self.cfg['edge_threshold'], "valid": valid,
                            "in_range": True, "avg": round(self.sum / self.n, 1) if self.n else 0.0, "n": self.n},
                "safety": {"estop": self.estop, "hv_on": self.hv_on,
                           "hv_ok": self.cfg['interlock'] and self.cfg['cooling_flow'] and self.cfg['temp'] <= 30 and not self.estop,
                           "shutter": self.shutter, "interlock": self.cfg['interlock'], "cooling_flow": self.cfg['cooling_flow'],
                           "temp": self.cfg['temp'], "temp_ok": self.cfg['temp'] <= 30.0, "comm_armed": self.comm_armed}}

    def command(self, cmd, params):
        self.log.append({'cmd': cmd, 'params': params})
        self.last_api = time.time()
        st = self.state
        if cmd == 'GET_STATE': return 0, {"state": st, "sys_err": self.err, "local": self.local,   # V1.5.25: + CycleStats
                                          "cycle_max_us": 180, "period_max_us": 1040, "overruns": 0, "prio_lat_max_us": 0}
        if cmd == 'GET_CONFIG': return 0, {"config": dict(self.cfg)}
        if cmd == 'STOP':
            self.mstate, self.vel, self.mode = 'idle', 0.0, 'none'; self.shutter = 'closed'
            return 0, None
        if cmd == 'HV_OFF':
            self.hv_on = False; self.shutter = 'closed'
            if self.state == 'RUNNING': self.state = 'READY'
            return 0, None
        if self.local and cmd in ('INIT', 'HV_ON', 'START_MEASURE', 'START_TRAVERSE', 'ACK', 'CALIB', 'REBOOT'): return -10, None
        if cmd == 'INIT':
            if st not in ('SAFE_START', 'READY'): return -5, None
            if self.mstate != 'idle': return -7, None
            self.mstate, self.t_end, self.vel = 'homing', time.time() + 2.0, 5.0; self.comm_armed = True; return 0, None
        if cmd == 'HV_ON':
            if st not in ('READY', 'RUNNING'): return -5, None
            p = self.proc()['safety']
            if not p['hv_ok']: return -6, None
            self.hv_on, self.state = True, 'RUNNING'; self.comm_armed = True; return 0, None
        if cmd in ('START_MEASURE', 'START_TRAVERSE'):
            if st != 'RUNNING': return -5, None
            if not self.homed: return -6, None
            if self.mstate != 'idle': return -7, None
            self.comm_armed = True; self.shutter = 'open'; self.mstate = 'moving'
            if cmd == 'START_MEASURE': self.mode, self.target, self.vel = 'measure', self.cfg['measure_pos'], 10.0
            else: self.mode, self.pos, self.target, self.vel = 'traverse', self.cfg['traverse_start'], self.cfg['traverse_end'], self.cfg['traverse_speed']
            self.n, self.sum = 0, 0.0
            return 0, None
        if cmd == 'ACK':
            if st != 'WAIT_ACK': return -5, None
            if self.estop: return -6, None
            self.err = 0; self.state = 'READY' if self.homed else 'SAFE_START'; return 0, None
        if cmd == 'SET':
            k, v = params.get('key'), params.get('value')
            if k == 'wifi_pass':
                if not isinstance(v, str) or not (v == '' or 8 <= len(v) <= 63): return -4, None
                self.cfg['wifi_pass_set'] = v != ''; return 0, None
            if k not in self.cfg or k.startswith('calib_') and k.endswith('_raw') or k in ('calib_valid', 'wifi_pass_set'): return -4, None
            if isinstance(self.cfg[k], bool):
                if v not in (True, False, 0, 1): return -4, None
                v = bool(v)
            elif isinstance(self.cfg[k], float):
                if not isinstance(v, (int, float)) or isinstance(v, bool): return -4, None
                v = float(v)
            elif not isinstance(v, str): return -4, None
            self.cfg[k] = v; return 0, None
        if cmd == 'CALIB':
            a = params.get('action')
            if a not in ('near', 'far', 'base', 'save', 'reset'): return -4, None
            if a == 'reset': self.cfg.update(calib_near_raw=40.0, calib_far_raw=65.0, calib_valid=True)
            return 0, None
        if cmd == 'REBOOT': return 0, None
        if cmd in ('MOVE', 'PARK', 'SERVICE', 'SHUTTER_OPEN', 'SHUTTER_CLOSE'): return 0, None
        return -3, None


SIM = Sim()


def ticker():
    while True:
        with SIM.lock: SIM.step()
        time.sleep(0.05)


class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass

    def send(self, code, typ, body, extra=None):
        b = body.encode() if isinstance(body, str) else body
        self.send_response(code); self.send_header('Content-Type', typ); self.send_header('Content-Length', str(len(b)))
        self.send_header('Access-Control-Allow-Origin', '*')
        for k, v in (extra or {}).items(): self.send_header(k, v)
        self.end_headers(); self.wfile.write(b)

    def do_GET(self):
        u = urlparse(self.path); path = u.path
        if path in ROUTES:
            f, typ = ROUTES[path]
            try: return self.send(200, typ, open(os.path.join(ROOT, f), 'rb').read())
            except OSError: return self.send(404, 'application/json', '{"error":"not found"}')
        if path == '/add':
            p = os.path.join(ROOT, 'add.json')
            body = open(p, 'rb').read() if os.path.exists(p) else json.dumps(SAMPLE_ADD)
            return self.send(200, 'application/json', body, {'Cache-Control': 'no-store'})
        if path == '/proc':
            with SIM.lock:
                t = frame(SIM.proc())
                if SIM.crc_error: t = t[:-3] + ('0' if t[-3] != '0' else '1') + t[-2:]
            return self.send(200, 'application/json', t, {'Cache-Control': 'no-store'})
        if path == '/fake/log':
            with SIM.lock: return self.send(200, 'application/json', json.dumps(SIM.log))
        if path == '/fake/set':
            q = {k: v[0] for k, v in parse_qs(u.query).items()}
            with SIM.lock:
                for k, v in q.items():
                    if k in ('state',): SIM.state = v
                    elif k in ('err',): SIM.err = int(v)
                    elif k in ('local', 'estop', 'comm_armed', 'crc_error', 'homed', 'hv_on'): setattr(SIM, k, v == '1')
                    elif k == 'clear_log': SIM.log = []
            return self.send(200, 'application/json', '{"ok":true}')
        return self.send(404, 'application/json', '{"error":"not found"}')

    def do_POST(self):
        if urlparse(self.path).path != '/api': return self.send(404, 'application/json', '{"error":"not found"}')
        body = self.rfile.read(int(self.headers.get('Content-Length', 0))).decode()
        if not verify(body): return self.send(400, 'application/json', frame({"status": "error", "err": -1, "cmd": "", "msg": ERR_MSG[-1]}))
        try: req = json.loads(body)
        except ValueError: return self.send(400, 'application/json', frame({"status": "error", "err": -2, "cmd": "", "msg": ERR_MSG[-2]}))
        cmd, params = req.get('cmd', ''), req.get('params') or {}
        with SIM.lock: err, extra = SIM.command(cmd, params)
        r = {"status": "ok" if err == 0 else "error", "err": err, "cmd": cmd}
        if err: r["msg"] = ERR_MSG.get(err, 'error')
        if extra: r.update(extra)
        return self.send(200, 'application/json', frame(r))


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    threading.Thread(target=ticker, daemon=True).start()
    print('fake ADD device on http://localhost:%d/  (Ctrl+C to stop)' % port)
    ThreadingHTTPServer(('', port), H).serve_forever()
