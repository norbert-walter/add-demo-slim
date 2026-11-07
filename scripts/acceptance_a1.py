#!/usr/bin/env python3
"""acceptance_a1.py – Abnahme Sprint A1 (Pflichtenheft Kapitel 10) über die REST-API.

Prüft alle Abnahmepunkte, die sich über /api, /proc und /log prüfen lassen, und schreibt ein
Protokoll (Markdown). Punkte, die Sinne oder Hände brauchen (LEDs, Summer, Not-Aus, Endschalter,
Messschieber), fragt das Skript an der passenden Stelle ab (j = ja, n = nein, ü = überspringen).
Alles Weitere (Display, Weboberfläche, Oszilloskop, Router) steht in docs/sprints/A1.md.

ACHTUNG: Das Skript fährt die Achse über den ganzen Verfahrweg, schaltet die (simulierte) HV und
startet die Anlage einmal neu. Fahrbereich frei, Not-Aus in Reichweite.

    python3 scripts/acceptance_a1.py --host 192.168.4.1
    python3 scripts/acceptance_a1.py --host 192.168.4.1 --only T05,T18      (einzelne Prüfungen)
    python3 scripts/acceptance_a1.py --host 192.168.4.1 --no-manual         (ohne Handprüfungen)
    python3 scripts/acceptance_a1.py --host localhost:8090 --sim --yes      (gegen sim_device.py)

--sim: Gegenüber scripts/sim_device.py werden Not-Aus und Endschalter über die Test-Hooks des
Simulators betätigt; Fragen nach LEDs/Summer/Messschieber entfallen (übersprungen).
Ergebnis: Protokoll abnahme_a1_<Datum>_<Uhrzeit>.md im aktuellen Verzeichnis (oder --out).
"""
import argparse, datetime, json, os, random, subprocess, sys, threading, time, urllib.error, urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from add_client import build, frame, verify   # noqa: E402

ERR = {0: "OK", -1: "BAD_CRC", -2: "BAD_JSON", -3: "UNKNOWN_CMD", -4: "BAD_PARAM", -5: "STATE",
       -6: "PRECONDITION", -7: "BUSY", -8: "NOT_STABLE", -9: "CALIB_INVALID", -10: "LOCAL_CONTROL"}
SYS_ERR = ["NONE", "ESTOP", "TEMP_HIGH", "COMM_TIMEOUT", "MOTOR_FAULT", "HOMING_FAILED"]
STEPS = [(115, 65.0, 5.0), (135, 60.0, 10.0), (155, 55.0, 15.0), (175, 50.0, 20.0), (195, 45.0, 25.0),
         (215, 40.0, 30.0)]                      # step centre (mm), distance (mm), thickness (mm)
PROC_KEYS = {"ts", "state", "err", "local", "motion", "measure", "safety"}
MOTION_KEYS = {"pos", "vel", "state", "mode", "homed", "limit_fwd", "limit_bwd"}
MEASURE_KEYS = {"dist", "thick", "on_material", "valid", "in_range", "avg", "n"}
SAFETY_KEYS = {"estop", "hv_on", "hv_ok", "shutter", "interlock", "cooling_flow", "temp", "temp_ok", "comm_armed"}


class Fail(Exception):
    pass


class Skip(Exception):
    pass


def check(cond, msg):
    if not cond:
        raise Fail(msg)


# --------------------------------------------------------------------------------------- device
class Device:
    def __init__(self, host, sim):
        self.host, self.sim = host, sim
        self.io = threading.Lock()
        self.last_api = 0.0
        self.hb = True                      # heartbeat GET_STATE every second
        self.run = True
        self.last_ts = None
        self.resets = 0                     # unexpected restarts (ts went backwards)
        self.expect_reboot = False
        threading.Thread(target=self._heartbeat, daemon=True).start()

    def raw(self, path, data=None, timeout=4.0):
        """-> (http code, text). HTTP errors are returned, not raised."""
        req = urllib.request.Request("http://%s%s" % (self.host, path), data=data.encode() if data else None,
                                     headers={"Content-Type": "text/plain"} if data else {})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.status, r.read().decode(errors="replace")
        except urllib.error.HTTPError as e:
            return e.code, e.read().decode(errors="replace")

    def api(self, cmd, **params):
        """-> err code (int). Raises Fail on a broken frame."""
        with self.io:
            self.last_api = time.time()
            code, txt = self.raw("/api", build(cmd, params))
            self.last_api = time.time()
        check(verify(txt), "%s: Antwort ohne gültige CRC: %s" % (cmd, txt[:120]))
        r = json.loads(txt)
        self.last_reply = r
        return r["err"]

    def ok(self, cmd, **params):
        e = self.api(cmd, **params)
        check(e == 0, "%s %s -> %s (erwartet OK)" % (cmd, params or "", ERR.get(e, e)))

    def expect(self, err, cmd, **params):
        e = self.api(cmd, **params)
        check(e == err, "%s %s -> %s (erwartet %s)" % (cmd, params or "", ERR.get(e, e), ERR.get(err, err)))

    def proc(self):
        code, txt = self.raw("/proc?t=%d" % time.time_ns())
        check(code == 200 and verify(txt), "/proc ungültig (HTTP %s): %s" % (code, txt[:120]))
        p = json.loads(txt)
        if self.last_ts is not None and p["ts"] < self.last_ts and not self.expect_reboot:
            self.resets += 1
        self.last_ts = p["ts"]
        return p

    def config(self):
        self.ok("GET_CONFIG")
        return self.last_reply["config"]

    def log(self):
        return self.raw("/log")[1].splitlines()

    def count(self, text):
        """How often `text` occurs in /log (the device keeps only the last 20 lines)."""
        return sum(1 for l in self.log() if text in l)

    def hook(self, **kv):
        if not self.sim:
            raise Skip("nur mit --sim")
        self.raw("/sim/set?" + "&".join("%s=%d" % (k, int(v)) for k, v in kv.items()))

    def _heartbeat(self):
        while self.run:
            time.sleep(0.1)
            if self.hb and time.time() - self.last_api >= 1.0:
                try:
                    self.api("GET_STATE")
                except Exception:
                    self.last_api = time.time()

    # ---- waiting ----------------------------------------------------------------------------
    def wait(self, cond, timeout, what, allow_fault=False, period=0.05):
        t0 = time.time()
        while True:
            p = self.proc()
            if not allow_fault and p["state"] in ("FAULT", "WAIT_ACK"):
                raise Fail("%s: unerwartet %s, Fehler %s" % (what, p["state"], SYS_ERR[p["err"]]))
            if cond(p):
                return p
            if time.time() - t0 > timeout:
                raise Fail("%s: nach %.0f s nicht erreicht (%s, pos %.1f, %s/%s, HV %s, Shutter %s)" % (
                    what, timeout, p["state"], p["motion"]["pos"], p["motion"]["state"], p["motion"]["mode"],
                    p["safety"]["hv_on"], p["safety"]["shutter"]))
            time.sleep(period)

    @staticmethod
    def still(p):
        """No sequence active. motion.state is "moving" for every active sequence (also while a stationary
        measurement holds its position) and "error" (latched) after a fault until the next start."""
        return p["motion"]["state"] in ("idle", "error") and p["motion"]["mode"] == "none"

    def idle(self, timeout=90, what="Achse steht"):
        return self.wait(self.still, timeout, what)

    def move(self, pos, speed=10):
        self.ok("MOVE", pos=pos, speed=speed)
        p = self.idle(timeout=abs(pos - self.proc()["motion"]["pos"]) / speed + 15, what="MOVE %s" % pos)
        check(abs(p["motion"]["pos"] - pos) <= 0.1, "MOVE %s: Position %.1f" % (pos, p["motion"]["pos"]))
        return p

    def ready(self, hv=False):
        """READY (or RUNNING with hv=True), axis homed and idle, simulated signals at their defaults."""
        p = self.proc()
        check(p["state"] not in ("FAULT", "WAIT_ACK"), "Anlage im Fehler %s" % SYS_ERR[p["err"]])
        if not self.still(p):
            self.ok("STOP")
            p = self.idle(30, "STOP")
        if p["safety"]["shutter"] == "open":
            self.ok("SHUTTER_CLOSE")
        if p["safety"]["hv_on"] and not hv:
            self.ok("HV_OFF")
        s = p["safety"]
        if not s["interlock"]: self.ok("SET", key="interlock", value=True)
        if not s["cooling_flow"]: self.ok("SET", key="cooling_flow", value=True)
        if s["temp"] != 18.0: self.ok("SET", key="temp", value=18.0)
        if not p["motion"]["homed"] or p["state"] == "SAFE_START":
            self.ok("INIT")
            self.wait(lambda q: q["state"] == "READY" and q["motion"]["homed"] and self.still(q), 120, "INIT")
        if hv and not self.proc()["safety"]["hv_on"]:
            self.ok("HV_ON")
        return self.proc()


# ---------------------------------------------------------------------------------- the checks
TESTS = []


def test(tid, block, title):
    def deco(f):
        TESTS.append((tid, block, title, f))
        return f
    return deco


class Ctx:
    def __init__(self, dev, args):
        self.d, self.args = dev, args
        self.notes = []

    def ask(self, question):
        """Manual check inside an automatic test. -> True / False / None (not checked: --no-manual, --sim
        or 'ü'). Never aborts the test, so the device always ends in a defined state."""
        if not (self.args.no_manual or self.args.sim):
            while True:
                a = input("\n  >>> %s [j/n/ü] " % question).strip().lower()
                if a in ("j", "ja", "y"):
                    self.notes.append("Hand: %s ja" % question.rstrip("?"))
                    return True
                if a in ("n", "nein"): return False
                if a in ("ü", "u", "s"): break
        self.notes.append("Hand offen: %s" % question.rstrip("?"))
        return None

    def prompt(self, text):
        if self.args.no_manual or self.args.sim:
            return
        input("\n  >>> %s – weiter mit Enter " % text)

    def number(self, question):
        if self.args.no_manual or self.args.sim:
            raise Skip("Handprüfung übersprungen")
        while True:
            a = input("\n  >>> %s (Zahl, leer = überspringen) " % question).strip().replace(",", ".")
            if not a: raise Skip("vom Prüfer übersprungen")
            try: return float(a)
            except ValueError: pass


@test("T01", "5", "Alle Endpunkte antworten in < 1 s")
def t01(c):
    d, out, slow = c.d, [], []
    paths = ["/", "/settings", "/doc", "/proc", "/log", "/llms.txt", "/app.js", "/font-u400.woff2",
             "/font-u700.woff2", "/font-m400.woff2", "/font-m700.woff2", "/add"]
    for path in paths:
        t0 = time.perf_counter(); code, _ = d.raw(path, timeout=5); dt = time.perf_counter() - t0
        out.append("%s %d %.0f ms" % (path, code, dt * 1000))
        if path == "/add" and code == 404:
            continue                          # until Sprint D1
        check(code == 200, "%s -> HTTP %d" % (path, code))
        if dt >= 1.0: slow.append(path)
    t0 = time.perf_counter(); d.ok("GET_STATE"); out.append("/api GET_STATE %.0f ms" % ((time.perf_counter() - t0) * 1000))
    check(not slow, "langsamer als 1 s: %s" % slow)
    return "; ".join(out) + " (/add 404 bis Sprint D1)"


@test("T02", "5", "/proc vollständig, CRC gültig")
def t02(c):
    code, txt = c.d.raw("/proc")
    check(code == 200 and verify(txt), "CRC ungültig")
    check(len(txt.encode()) <= 512, "länger als 512 Byte (%d)" % len(txt.encode()))
    p = json.loads(txt)
    check(set(p) - {"crc"} == PROC_KEYS, "Felder: %s" % sorted(p))
    check(set(p["motion"]) == MOTION_KEYS and set(p["measure"]) == MEASURE_KEYS and set(p["safety"]) == SAFETY_KEYS,
          "Unterfelder unvollständig")
    return "%d Byte, alle Felder vorhanden" % len(txt.encode())


@test("T03", "5", "Fehlerantworten: falsche CRC, kaputtes JSON, unbekannter Befehl, Parameter, Zustand")
def t03(c):
    d = c.d
    cases = [('{"cmd":"GET_STATE","params":{},"crc":"00000000"}', 400, -1),
             (frame('{"cmd":}'), 400, -2), (build("FOO", {}), 200, -3), (build("MOVE", {}), 200, -4)]
    res = []
    for body, code, err in cases:
        hc, txt = d.raw("/api", body)
        check(verify(txt), "Antwort ohne CRC bei %s" % ERR[err])
        r = json.loads(txt)
        check(hc == code and r["err"] == err, "%s: HTTP %d err %s (erwartet HTTP %d %s)" % (
            body[:40], hc, r["err"], code, ERR[err]))
        res.append(ERR[err])
    p = d.proc()
    if p["state"] != "WAIT_ACK":
        d.expect(-5, "ACK"); res.append("STATE")
    return ", ".join(res) + " – wie erwartet"


@test("T04", "1", "Parameter nach Neustart wiederhergestellt, Sicherheitsflags zurückgesetzt")
def t04(c):
    d = c.d
    p = d.proc()
    if p["safety"]["hv_on"]: d.ok("HV_OFF")
    if p["motion"]["state"] != "idle": d.ok("STOP"); d.idle(30)
    check(d.proc()["state"] in ("SAFE_START", "READY"), "REBOOT nur in SAFE_START/READY")
    orig = d.config()["measure_pos"]
    new = 150.5 if orig != 150.5 else 151.5
    d.ok("SET", key="measure_pos", value=new)
    d.ok("SET", key="interlock", value=False)
    d.ok("SET", key="temp", value=25.0)
    time.sleep(1.0)                           # saved at once (axis idle, HV off), at the latest before the reboot
    ts = d.proc()["ts"]
    d.expect_reboot = True
    try:
        d.ok("REBOOT")
        t0 = time.time()
        while True:
            time.sleep(0.5)
            try:
                if d.proc()["ts"] < ts: break
            except Exception:
                pass
            check(time.time() - t0 < 40, "Anlage nach 40 s nicht wieder erreichbar")
        time.sleep(1.0)
        d.last_ts = d.proc()["ts"]
    finally:
        d.expect_reboot = False
    cfg = d.config(); p = d.proc()
    try:
        check(abs(cfg["measure_pos"] - new) < 0.05, "measure_pos nach Neustart %s (erwartet %s)" % (cfg["measure_pos"], new))
        check(cfg["interlock"] is True and cfg["cooling_flow"] is True and cfg["temp"] == 18.0,
              "Sicherheitsflags nach Neustart: interlock %s, cooling_flow %s, temp %s" % (
                  cfg["interlock"], cfg["cooling_flow"], cfg["temp"]))
        check(p["state"] == "SAFE_START" and not p["motion"]["homed"], "nach Neustart %s" % p["state"])
    finally:
        d.ok("SET", key="measure_pos", value=orig)
    time.sleep(1.0)
    return "measure_pos %.1f gespeichert und nach Neustart da; interlock/cooling_flow/temp wieder Standard; SAFE_START" % new


@test("T05", "3", "INIT referenziert (hinterer Endschalter, 2 mm Rückzug, Position 0)")
def t05(c):
    d = c.d
    p = d.proc()
    if p["safety"]["hv_on"]: d.ok("HV_OFF")
    if p["motion"]["homed"]:
        d.move(30)
    d.ok("INIT")
    seen_bwd = False
    t0 = time.time()
    while True:
        p = d.proc()
        seen_bwd = seen_bwd or p["motion"]["limit_bwd"]
        if p["state"] == "READY" and p["motion"]["homed"] and p["motion"]["state"] == "idle": break
        check(p["state"] not in ("FAULT", "WAIT_ACK"), "Fehler %s" % SYS_ERR[p["err"]])
        check(time.time() - t0 < 90, "INIT nicht fertig nach 90 s")
        time.sleep(0.05)
    check(abs(p["motion"]["pos"]) <= 0.05, "Position nach INIT %.2f" % p["motion"]["pos"])
    check(not p["motion"]["limit_bwd"], "hinterer Endschalter nach INIT noch betätigt (kein Rückzug)")
    res = "READY, pos %.1f, Endschalter frei, %.0f s" % (p["motion"]["pos"], time.time() - t0)
    if not seen_bwd:
        res += " (Schalter beim Abfragen nicht gesehen – 50-ms-Raster)"
    if c.ask("Hat der Summer am Ende der Referenzfahrt einmal kurz gepiept?") is False:
        raise Fail("kein Piep nach der Referenzfahrt")
    return res


@test("T06", "2", "Endschalter in /proc korrekt (Hand)")
def t06(c):
    d = c.d
    d.ready()
    res = []
    for name, key, hook in (("vorderen", "limit_fwd", "press_fwd"), ("hinteren", "limit_bwd", "press_bwd")):
        if c.args.sim:
            d.hook(**{hook: 1})
        else:
            c.prompt("Achse steht. %s Endschalter von Hand drücken und halten" % name.capitalize())
            if c.args.no_manual: raise Skip("Handprüfung übersprungen")
        p = d.wait(lambda q: q["motion"][key], 15, "%s Endschalter gedrückt" % name)
        if c.args.sim:
            d.hook(**{hook: 0})
        else:
            c.prompt("%s Endschalter loslassen" % name.capitalize())
        d.wait(lambda q: not q["motion"][key], 15, "%s Endschalter losgelassen" % name)
        check(p["state"] not in ("FAULT", "WAIT_ACK"), "Endschalter bei stehender Achse löst Fehler aus")
        res.append("%s ok" % key)
    return ", ".join(res) + "; kein Fehler bei stehender Achse"


@test("T07", "4", "HV_ON nur mit allen Bedingungen; jede fehlende -> PRECONDITION; LED grün folgt HV")
def t07(c):
    d = c.d
    d.ready()
    res = []
    for key, bad, good in (("interlock", False, True), ("cooling_flow", False, True), ("temp", 31.0, 18.0)):
        d.ok("SET", key=key, value=bad)
        check(not d.proc()["safety"]["hv_ok"], "hv_ok trotz %s=%s" % (key, bad))
        d.expect(-6, "HV_ON")
        check(not d.proc()["safety"]["hv_on"], "HV an trotz %s=%s" % (key, bad))
        d.ok("SET", key=key, value=good)
        res.append("%s=%s -> PRECONDITION" % (key, bad))
    d.ok("HV_ON")
    check(d.proc()["state"] == "RUNNING", "HV_ON mit allen Bedingungen -> nicht RUNNING")
    if c.ask("Leuchtet die grüne LED (HV)?") is False: raise Fail("grüne LED aus bei HV ein")
    d.ok("HV_OFF")
    check(d.proc()["state"] == "READY", "HV_OFF -> nicht READY")
    if c.ask("Ist die grüne LED jetzt aus?") is False: raise Fail("grüne LED an bei HV aus")
    return "; ".join(res) + "; alle erfüllt -> RUNNING (Not-Aus: T23)"


@test("T09", "4", "Temperatur 31 °C -> HV aus ohne Fehler; 35 °C -> TEMP_HIGH, ACK erst nach Absenken")
def t09(c):
    d = c.d
    d.ready(hv=True)
    d.ok("SET", key="temp", value=31.0)
    p = d.wait(lambda q: not q["safety"]["hv_on"], 2, "HV aus bei 31 °C")
    check(p["state"] == "READY" and p["err"] == 0, "31 °C: %s, Fehler %s" % (p["state"], SYS_ERR[p["err"]]))
    d.ok("SET", key="temp", value=18.0)
    d.ok("HV_ON")
    d.ok("SET", key="temp", value=35.0)
    p = d.wait(lambda q: q["state"] == "WAIT_ACK", 2, "TEMP_HIGH", allow_fault=True)
    check(p["err"] == 2 and not p["safety"]["hv_on"], "35 °C: Fehler %s, HV %s" % (SYS_ERR[p["err"]], p["safety"]["hv_on"]))
    if c.ask("Hat der Summer 3-mal gepiept (Fehler)?") is False: raise Fail("kein Fehlersignal")
    d.expect(-6, "ACK")
    d.ok("SET", key="temp", value=25.0)
    d.ok("ACK")
    p = d.wait(lambda q: q["state"] == "READY", 2, "READY nach ACK")
    check(not p["safety"]["hv_on"], "HV nach ACK wieder an")
    d.ok("SET", key="temp", value=18.0)
    return "31 °C: READY ohne Fehler; 35 °C: WAIT_ACK TEMP_HIGH; ACK bei 35 °C -> PRECONDITION, bei 25 °C -> READY, HV bleibt aus"


@test("T10", "4", "Shutter öffnet nie in der Parkzone (≤ 20 mm)")
def t10(c):
    d = c.d
    d.ready(hv=True)
    d.move(10)
    d.expect(-6, "SHUTTER_OPEN")
    d.expect(-4, "START_MEASURE", pos=10)
    check(d.proc()["safety"]["shutter"] == "closed", "Shutter offen in der Parkzone")
    d.move(40)
    d.ok("SHUTTER_OPEN")
    d.ok("MOVE", pos=10, speed=5)
    bad = []
    while True:
        p = d.proc()
        if p["safety"]["shutter"] == "open" and p["motion"]["pos"] <= 20.0: bad.append(p["motion"]["pos"])
        if Device.still(p): break
        time.sleep(0.03)
    check(not bad, "Shutter offen bei pos %s" % bad[:3])
    check(p["safety"]["shutter"] == "closed", "Shutter in der Parkzone offen")
    return "SHUTTER_OPEN bei 10 mm -> PRECONDITION; START_MEASURE 10 -> BAD_PARAM; manuell offener Shutter schließt beim Einfahren in die Parkzone"


@test("T11", "4", "SHUTTER_OPEN manuell nur in der Servicezone; bei 145 mm -> PRECONDITION; LED rot folgt Shutter")
def t11(c):
    d = c.d
    d.ready(hv=True)
    d.move(40)
    d.ok("SHUTTER_OPEN")
    check(d.proc()["safety"]["shutter"] == "open", "Shutter nicht offen")
    if c.ask("Leuchtet die rote LED (Shutter)?") is False: raise Fail("rote LED aus bei offenem Shutter")
    d.ok("SHUTTER_CLOSE")
    check(d.proc()["safety"]["shutter"] == "closed", "Shutter nicht zu")
    if c.ask("Ist die rote LED jetzt aus?") is False: raise Fail("rote LED an bei geschlossenem Shutter")
    d.move(145)
    d.expect(-6, "SHUTTER_OPEN")
    return "40 mm: auf/zu OK; 145 mm: PRECONDITION"


@test("T12", "4", "Automatischer Shutter nie in der Servicezone (≤ 60 mm), schließt beim Einfahren")
def t12(c):
    d = c.d
    d.ready(hv=True)
    d.expect(-4, "START_MEASURE", pos=40)
    d.expect(-4, "START_MEASURE", pos=60)
    d.expect(-4, "START_TRAVERSE", start=60, end=100, speed=10)
    d.ok("START_TRAVERSE", start=100, end=70, speed=10)
    bad, opened = [], False
    d.wait(lambda q: q["motion"]["mode"] == "traverse", 3, "Profilfahrt startet")
    while True:
        p = d.proc()
        if p["safety"]["shutter"] == "open":
            opened = True
            if p["motion"]["pos"] <= 60.0: bad.append(p["motion"]["pos"])
        if p["motion"]["mode"] == "none": break
        time.sleep(0.03)
    check(opened, "Shutter hat sich bei der Profilfahrt 100 -> 70 nicht geöffnet")
    check(p["safety"]["shutter"] == "open", "Shutter direkt nach dem Lauf zu (Nachlaufzeit 3 s erwartet)")
    d.ok("MOVE", pos=40, speed=10)
    while True:
        p = d.proc()
        if p["safety"]["shutter"] == "open" and p["motion"]["pos"] <= 60.0: bad.append(p["motion"]["pos"])
        if Device.still(p): break
        time.sleep(0.03)
    check(not bad, "Shutter offen bei pos %s" % bad[:3])
    check(p["safety"]["shutter"] == "closed", "Shutter in der Servicezone offen")
    return "START_MEASURE 40/60 und Profil ab 60 -> BAD_PARAM; Shutter beim Einfahren in die Servicezone geschlossen"


@test("T13", "4", "Hin- und Rückfahrt innerhalb 3 s: Shutter bleibt offen; 3 s nach Ende zu; STOP schließt sofort")
def t13(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=100, end=200, speed=10)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["shutter"] == "open", 30, "Shutter offen im Lauf")
    d.wait(lambda q: q["motion"]["mode"] == "none", 30, "Ende Hinfahrt")
    d.ok("START_TRAVERSE", start=200, end=100, speed=10)
    closed_at = []
    d.wait(lambda q: q["motion"]["mode"] == "traverse", 3, "Rückfahrt startet")
    while True:
        p = d.proc()
        if p["safety"]["shutter"] != "open": closed_at.append(p["motion"]["pos"])
        if p["motion"]["mode"] == "none": break
        time.sleep(0.03)
    check(not closed_at, "Shutter zwischendurch zu bei pos %s" % closed_at[:3])
    t_end = time.time()
    d.wait(lambda q: q["safety"]["shutter"] == "closed", 6, "Shutter schließt nach der Nachlaufzeit")
    dt = time.time() - t_end
    check(2.5 <= dt <= 4.0, "Shutter nach %.1f s zu (erwartet 3 s)" % dt)
    d.ok("START_MEASURE", pos=155)
    d.wait(lambda q: q["motion"]["mode"] == "measure" and q["safety"]["shutter"] == "open", 30, "Messung, Shutter offen")
    d.ok("STOP")
    p = d.proc()
    check(p["safety"]["shutter"] == "closed", "Shutter nach STOP noch offen")
    return "Shutter über Hin- und Rückfahrt offen, %.1f s nach Ende zu; STOP schließt sofort" % dt


@test("T14", "4", "Fahrt auf 242 mm löst den vorderen Endschalter nicht aus")
def t14(c):
    d = c.d
    d.ready()
    p = d.move(242)
    check(not p["motion"]["limit_fwd"] and p["state"] == "READY", "Endschalter %s, Zustand %s" % (
        p["motion"]["limit_fwd"], p["state"]))
    return "pos %.1f, Endschalter frei, READY" % p["motion"]["pos"]


@test("T15", "4", "cooling_flow = false während Profilfahrt -> HV aus, Fahrt gestoppt, Shutter zu")
def t15(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=80, end=240, speed=10)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["shutter"] == "open" and q["motion"]["pos"] > 120,
           60, "Profilfahrt (Messlauf) bei 120 mm")
    d.ok("SET", key="cooling_flow", value=False)
    p = d.wait(lambda q: not q["safety"]["hv_on"] and q["safety"]["shutter"] == "closed" and q["motion"]["mode"] == "none",
               1.0, "HV aus, Shutter zu, Programm beendet")
    check(p["state"] == "READY" and p["err"] == 0, "Zustand %s, Fehler %s" % (p["state"], SYS_ERR[p["err"]]))
    p = d.idle(2, "Achse steht")
    d.ok("SET", key="cooling_flow", value=True)
    check(p["motion"]["homed"], "Referenz verloren (kontrollierter Halt erwartet)")
    return "READY ohne Fehler, Achse steht bei %.1f mm, Referenz bleibt" % p["motion"]["pos"]


@test("T16", "4", "Kommunikations-Watchdog: COMM_TIMEOUT nach 5 s; nach ACK kein Wiederanlauf, INIT nötig")
def t16(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=240, end=80, speed=5)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["comm_armed"], 40, "Profilfahrt läuft")
    d.hb = False
    with d.io:
        t_last = d.last_api
    try:
        p = d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 10, "COMM_TIMEOUT", allow_fault=True, period=0.05)
        dt = time.time() - t_last
    finally:
        d.hb = True
    check(p["err"] == 3, "Fehler %s (erwartet COMM_TIMEOUT)" % SYS_ERR[p["err"]])
    check(4.8 <= dt <= 6.5, "Fehler nach %.1f s (erwartet 5 s)" % dt)
    check(not p["safety"]["hv_on"] and p["safety"]["shutter"] == "closed", "HV/Shutter nach Fehler nicht aus")
    if c.ask("Hat der Summer 3-mal gepiept?") is False: raise Fail("kein Fehlersignal")
    d.ok("ACK")
    time.sleep(2.0)
    p = d.proc()
    check(not p["safety"]["hv_on"] and Device.still(p),
          "Wiederanlauf nach ACK")
    check(p["state"] == "SAFE_START" and not p["motion"]["homed"], "nach Fehler in Fahrt: %s, homed %s (INIT nötig erwartet)" % (
        p["state"], p["motion"]["homed"]))
    d.expect(-5, "HV_ON")
    return "COMM_TIMEOUT nach %.1f s; nach ACK SAFE_START ohne Referenz, kein Wiederanlauf, HV_ON -> STATE" % dt


@test("T17", "4", "SET maint_pos bei fahrender Achse: Speichern verschoben bis Stillstand mit HV aus")
def t17(c):
    d = c.d
    d.ready()
    start = d.proc()["motion"]["pos"]
    target = 220 if start < 120 else 20
    orig = d.config()["maint_pos"]
    new = 41.0 if orig != 41.0 else 42.0
    d.ok("MOVE", pos=target, speed=10)
    d.wait(lambda q: q["motion"]["state"] == "moving", 3, "Fahrt läuft")
    deferred0, saved0 = d.count("save deferred"), d.count("nvs: saved")
    d.ok("SET", key="maint_pos", value=new)
    time.sleep(1.0)
    check(d.count("save deferred") > deferred0, "keine neue Zeile 'nvs: save deferred' im Log")
    check(d.count("nvs: saved") == saved0, "während der Fahrt gespeichert")
    d.idle(40)
    time.sleep(1.0)
    check(d.count("nvs: saved") > saved0, "nach dem Stillstand nicht gespeichert")
    check(d.config()["maint_pos"] == new, "maint_pos nicht übernommen")
    d.ok("SET", key="maint_pos", value=orig)
    time.sleep(1.0)
    return "Log: save deferred -> nach Stillstand nvs: saved"


@test("T18", "8", "Stehende Messung je Stufe: Dicke ±0,5 mm, Abstand ±1 mm")
def t18(c):
    d = c.d
    d.ready(hv=True)
    rows, bad = [], []
    for pos, dist, thick in STEPS:
        d.ok("START_MEASURE", pos=pos)
        d.wait(lambda q: q["motion"]["mode"] == "measure" and abs(q["motion"]["pos"] - pos) <= 0.1
               and q["motion"]["vel"] == 0 and q["safety"]["shutter"] == "open", 40, "Messung bei %d mm" % pos)
        time.sleep(2.0)
        p = d.proc()
        m = p["measure"]
        d.ok("STOP")
        rows.append("%d mm: Abstand %.1f, Dicke Mittel %.2f (n=%d)" % (pos, m["dist"], m["avg"], m["n"]))
        if abs(m["avg"] - thick) > 0.5 or abs(m["dist"] - dist) > 1.0 or m["n"] < 10:
            bad.append("%d mm (Soll %.0f / %.0f)" % (pos, thick, dist))
    check(not bad, "außerhalb der Toleranz: %s | %s" % (", ".join(bad), "; ".join(rows)))
    return "; ".join(rows)


@test("T19", "8", "Profilfahrt: Kanten der Stufen erkannt, Statistik plausibel")
def t19(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=80, end=240, speed=5)
    d.wait(lambda q: q["motion"]["mode"] == "traverse", 3, "Profilfahrt startet")
    samples = []
    while True:
        p = d.proc()
        if p["motion"]["mode"] == "none": break
        m = p["measure"]
        if m["valid"]:
            samples.append((p["motion"]["pos"], m["thick"], m["on_material"]))
        time.sleep(0.05)
    avg, n = p["measure"]["avg"], p["measure"]["n"]
    check(len(samples) > 200, "zu wenige Messwerte (%d)" % len(samples))
    on = [s[0] for s in samples if s[2]]
    check(on, "kein Messgut erkannt")
    first, last = min(on), max(on)
    rows, bad = [], []
    for pos, _, thick in STEPS:
        v = sorted(s[1] for s in samples if abs(s[0] - pos) <= 5.0)
        med = v[len(v) // 2] if v else float("nan")
        rows.append("%d: %.1f" % (pos, med))
        if not v or abs(med - thick) > 1.0: bad.append(pos)
    check(abs(first - 105) <= 3 and abs(last - 225) <= 3, "Kanten bei %.1f / %.1f mm (erwartet 105 / 225 ±3)" % (first, last))
    check(not bad, "Stufen außerhalb ±1 mm: %s (%s)" % (bad, ", ".join(rows)))
    check(5.0 <= avg <= 30.0 and n > 50, "Statistik: Mittel %.2f, n %d" % (avg, n))
    return "Messgut %.1f … %.1f mm; Stufen (Median) %s; Mittel %.2f mm, n=%d" % (first, last, ", ".join(rows), avg, n)


@test("T20", "8", "Kalibrierung gespeichert und wirksam")
def t20(c):
    cfg = c.d.config()
    check(cfg["calib_valid"], "calib_valid = false")
    return "calib_valid, Nah %.1f -> %.1f, Fern %.1f -> %.1f (wirksam: T18)" % (
        cfg["calib_near_raw"], cfg["calib_near_ref"], cfg["calib_far_raw"], cfg["calib_far_ref"])


@test("T21", "3", "MOVE erreicht Ziel ±0,1 mm (Messschieber), keine Schrittverluste nach 10 Fahrten")
def t21(c):
    d = c.d
    d.ready()
    d.move(50)
    a = c.number("Achse steht bei 50 mm. Abstand Schlitten – fester Bezugspunkt mit dem Messschieber messen [mm]")
    rnd = random.Random(1)
    for _ in range(10):
        d.move(round(rnd.uniform(61, 240), 1), speed=rnd.choice([5, 10, 15, 20]))
    d.move(50)
    b = c.number("Wieder bei 50 mm. Gleichen Abstand erneut messen [mm]")
    check(abs(a - b) <= 0.1, "Abweichung %.2f mm nach 10 Fahrten" % abs(a - b))
    return "vorher %.2f, nachher %.2f mm (Differenz %.2f)" % (a, b, abs(a - b))


@test("T22", "3", "Unerwarteter Endschalter während der Fahrt -> MOTOR_FAULT (Hand)")
def t22(c):
    d = c.d
    d.ready()
    d.move(60)
    if not c.args.sim:
        c.prompt("Gleich fährt die Achse langsam von 60 nach 200 mm. Dann den VORDEREN Endschalter kurz von Hand drücken")
        if c.args.no_manual: raise Skip("Handprüfung übersprungen")
    d.ok("MOVE", pos=200, speed=5)
    if c.args.sim:
        time.sleep(1.0); d.hook(press_fwd=1); time.sleep(0.2); d.hook(press_fwd=0)
    p = d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 40, "MOTOR_FAULT", allow_fault=True)
    check(p["err"] == 4, "Fehler %s (erwartet MOTOR_FAULT)" % SYS_ERR[p["err"]])
    stopped = p["motion"]["pos"]
    time.sleep(1.0)
    check(abs(d.proc()["motion"]["pos"] - stopped) < 1.0, "Achse fährt weiter")
    d.ok("ACK")
    p = d.wait(lambda q: q["state"] == "SAFE_START", 2, "SAFE_START nach ACK")
    check(not p["motion"]["homed"], "Referenz nach MOTOR_FAULT noch gültig")
    return "MOTOR_FAULT bei %.1f mm, Achse steht, nach ACK INIT nötig" % stopped


@test("T23", "4", "Not-Aus in Fahrt -> ESTOP, INIT nötig; Not-Aus + cooling_flow false -> FAULT ESTOP (Hand)")
def t23(c):
    d = c.d
    d.ready(hv=True)
    if not c.args.sim:
        c.prompt("Gleich startet eine Profilfahrt. Sobald sie läuft, NOT-AUS drücken und gedrückt halten")
        if c.args.no_manual: raise Skip("Handprüfung übersprungen")
    d.ok("START_TRAVERSE", start=80, end=240, speed=5)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["shutter"] == "open", 60, "Profilfahrt läuft")
    if c.args.sim:
        d.hook(estop=1)
    p = d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 60, "Not-Aus", allow_fault=True)
    check(p["err"] == 1 and p["safety"]["estop"], "Fehler %s (erwartet ESTOP)" % SYS_ERR[p["err"]])
    check(not p["safety"]["hv_on"] and p["safety"]["shutter"] == "closed", "HV/Shutter nicht aus")
    if c.ask("Dauerton des Summers etwa 4 s?") is False: raise Fail("kein Not-Aus-Signal")
    d.ok("SET", key="cooling_flow", value=False)
    p = d.proc()
    check(p["err"] == 1, "mit cooling_flow false: Fehler %s (erwartet ESTOP)" % SYS_ERR[p["err"]])
    d.expect(-6, "ACK")
    if c.args.sim:
        d.hook(estop=0)
    else:
        c.prompt("Not-Aus jetzt LÖSEN")
    d.wait(lambda q: not q["safety"]["estop"], 30, "Not-Aus gelöst", allow_fault=True)
    d.ok("SET", key="cooling_flow", value=True)
    d.ok("ACK")
    p = d.wait(lambda q: q["state"] in ("SAFE_START", "READY"), 2, "nach ACK")
    check(p["state"] == "SAFE_START" and not p["motion"]["homed"], "nach Not-Aus in Fahrt %s, homed %s" % (
        p["state"], p["motion"]["homed"]))
    return "ESTOP in Fahrt, HV/Shutter aus; mit cooling_flow false weiter ESTOP; ACK bei gedrücktem Not-Aus -> PRECONDITION; danach SAFE_START, INIT nötig"


def stress(c, name, *extra, timeout=900):
    cmd = [sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "stress_api.py"),
           "--host", c.args.host, name, *extra]
    print("    $ " + " ".join(cmd[1:]), flush=True)
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    tail = [l for l in (r.stdout + r.stderr).splitlines() if l.strip()][-3:]
    check(r.returncode == 0, "stress_api %s: %s" % (name, " | ".join(tail)))
    return " | ".join(tail)


@test("T24", "4", "STOP per API während Profilfahrt: Antwort < 150 ms, Achse steht < 1 s")
def t24(c):
    c.d.ready()
    r = stress(c, "latency", "--n", "10", "--prepare")
    c.d.ready()
    return r


@test("T25", "4", "Queue-Flut (200 SET): nur OK/BUSY, alle CRC gültig")
def t25(c):
    c.d.ready()
    return stress(c, "flood", "--n", "200")


@test("T26", "4", "Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort")
def t26(c):
    c.d.ready()
    return stress(c, "parallel", "--n", "400", "--threads", "8")


@test("T27", "4", "Zeitverhalten 10 min mit dauernd geladener Weboberfläche (soak --web)")
def t27(c):
    if c.args.skip_soak:
        raise Skip("--skip-soak")
    c.d.ready()
    return stress(c, "soak", "--minutes", str(c.args.soak_minutes), "--web",
                  timeout=int(c.args.soak_minutes * 60 + 300))


ORDER = ["T01", "T02", "T03", "T04", "T05", "T06", "T07", "T09", "T10", "T11", "T12", "T13", "T14", "T15",
         "T16", "T17", "T18", "T19", "T20", "T21", "T22", "T23", "T24", "T25", "T26", "T27"]


def recover(d):
    """After a failed check: bring the device into a defined state (best effort)."""
    d.hb = True
    for f in (lambda: d.hook(estop=0, press_fwd=0, press_bwd=0) if d.sim else None,
              lambda: d.api("STOP"), lambda: d.api("HV_OFF"),
              lambda: d.api("SET", key="interlock", value=True), lambda: d.api("SET", key="cooling_flow", value=True),
              lambda: d.api("SET", key="temp", value=18.0)):
        try: f()
        except Exception: pass
    try:
        if d.proc()["state"] == "WAIT_ACK": d.api("ACK")
    except Exception:
        pass


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="192.168.4.1")
    ap.add_argument("--only", help="z. B. T05,T18")
    ap.add_argument("--no-manual", action="store_true", help="Handprüfungen überspringen")
    ap.add_argument("--sim", action="store_true", help="gegen scripts/sim_device.py (Test-Hooks)")
    ap.add_argument("--yes", action="store_true", help="ohne Sicherheitsabfrage starten")
    ap.add_argument("--skip-soak", action="store_true", help="T27 (10-min-Dauertest) auslassen")
    ap.add_argument("--soak-minutes", type=float, default=10)
    ap.add_argument("--out", help="Protokolldatei (Markdown)")
    args = ap.parse_args()
    ids = [t.strip().upper() for t in args.only.split(",")] if args.only else ORDER
    tests = {t[0]: t for t in TESTS}
    unknown = [i for i in ids if i not in tests]
    if unknown:
        raise SystemExit("unbekannte Prüfung: %s" % unknown)

    print("Abnahme A1 gegen http://%s/ – %d Prüfungen" % (args.host, len(ids)))
    print("ACHTUNG: Die Achse fährt über den ganzen Verfahrweg, die HV wird geschaltet, die Anlage startet neu.")
    print("Fahrbereich frei halten, Not-Aus in Reichweite. Abbruch jederzeit mit Strg+C (danach STOP).")
    if not args.yes and input("Starten? Mit 'ja' bestätigen: ").strip().lower() != "ja":
        raise SystemExit("abgebrochen")

    d = Device(args.host, args.sim)
    c = Ctx(d, args)
    start = datetime.datetime.now()
    try:
        boot = [l for l in d.log() if "boot: ready" in l]
    except Exception as e:
        raise SystemExit("Anlage nicht erreichbar: %s" % e)
    results = []
    try:
        for tid in ids:
            _, block, title, f = tests[tid]
            print("\n[%s] Block %s: %s" % (tid, block, title), flush=True)
            t0 = time.time()
            c.notes = []
            try:
                detail = f(c) or ""
                status = "PASS"
            except Skip as e:
                status, detail = "SKIP", str(e)
                recover(d)
            except Fail as e:
                status, detail = "FAIL", str(e)
                recover(d)
            except Exception as e:
                status, detail = "FAIL", "%s: %s" % (type(e).__name__, e)
                recover(d)
            if c.notes:
                detail += " [" + "; ".join(c.notes) + "]"
            print("  -> %s  %s  (%.0f s)" % (status, detail, time.time() - t0), flush=True)
            results.append((tid, block, title, status, detail))
    except KeyboardInterrupt:
        print("\nabgebrochen – sende STOP und HV_OFF")
        results.append(("–", "–", "Abbruch durch den Prüfer", "FAIL", "Strg+C"))
    finally:
        recover(d)
        try:
            d.ok("PARK") if d.proc()["motion"]["homed"] else None
            d.idle(60)
        except Exception:
            pass
    # T28: no unexpected restart during the whole run
    results.append(("T28", "1", "Kein unerwarteter Neustart während der Abnahme", "PASS" if d.resets == 0 else "FAIL",
                    "Laufzeit %.0f min, unerwartete Neustarts: %d" % ((datetime.datetime.now() - start).seconds / 60, d.resets)))
    # T29: timing since boot (covers every load of this run, incl. the buzzer signals of T09/T16/T23)
    timing, st = "", "FAIL"
    try:
        d.ok("GET_STATE")
        r = d.last_reply
        timing = "cycle_max_us %s, period_max_us %s, prio_lat_max_us %s, overruns %s (Info)" % (
            r["cycle_max_us"], r["period_max_us"], r["prio_lat_max_us"], r["overruns"])
        st = "PASS" if all(r[k] <= 3000 for k in ("cycle_max_us", "period_max_us", "prio_lat_max_us")) else "FAIL"
    except Exception as e:
        timing = "GET_STATE: %s" % e
    results.append(("T29", "4", "Zeitwerte seit dem letzten Start ≤ 3000 µs (inkl. Summer-Signale)", st,
                    timing + ("" if not d.sim else " – Simulator: Zeitwerte nicht simuliert")))
    d.run = False

    n = {s: sum(1 for r in results if r[3] == s) for s in ("PASS", "FAIL", "SKIP")}
    out = args.out or "abnahme_a1_%s.md" % start.strftime("%Y%m%d_%H%M")
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("# Abnahme A1 – automatischer Teil\n\n")
        fh.write("| | |\n|---|---|\n")
        fh.write("| Datum | %s |\n| Gerät | http://%s/ %s|\n| Firmware | %s |\n| Ergebnis | **%d bestanden, %d nicht bestanden, %d übersprungen** |\n"
                 % (start.strftime("%d.%m.%Y %H:%M"), args.host, "(Simulator) " if args.sim else "",
                    boot[-1] if boot else "–", n["PASS"], n["FAIL"], n["SKIP"]))
        fh.write("\n")
        fh.write("| ID | Block | Prüfung | Ergebnis | Details |\n|---|---|---|---|---|\n")
        for tid, block, title, status, detail in results:
            mark = {"PASS": "✅ bestanden", "FAIL": "❌ **nicht bestanden**", "SKIP": "⏭ übersprungen"}[status]
            fh.write("| %s | %s | %s | %s | %s |\n" % (tid, block, title, mark, detail.replace("|", "/")))
    print("\n%d bestanden, %d nicht bestanden, %d übersprungen – Protokoll: %s" % (n["PASS"], n["FAIL"], n["SKIP"], out))
    sys.exit(1 if n["FAIL"] else 0)


if __name__ == "__main__":
    main()
