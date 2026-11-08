#!/usr/bin/env python3
"""acceptance_a1.py – Abnahme Sprint A1 (Pflichtenheft Kapitel 10).

Ablauf in drei Blöcken (Entscheidung 08.11.2026):
  Block 1 – Sicherheit, immer zuerst und Pflicht: S01 Not-Aus im Stillstand, S02 Shutter, S03 Fahrtrichtung,
           S04/S05 Endlagen, S06 Not-Aus in Fahrt, S07 Not-Aus ≤ 2 ms (Oszilloskop). Beim ersten Fehler
           (auch eine unbeantwortete Frage) bricht die Abnahme ab: erst beheben, dann neu starten.
  Block 2 – weitere Handprüfungen (Prüfer an der Anlage): Summer, LEDs, Maßband, Messschieber, Display,
           Weboberfläche.
  Block 3 – automatische Prüfungen (ca. 40 min, ohne Prüfer): REST-API, Sicherheitslogik, Shutter, Messung,
           Last- und Dauertests. Vorher alle anderen Clients (Browser, add_client shell) schließen.
Ergebnis: Abnahme_A1_<JJJJ-MM-TT>_<HHMM>[_<tag>].md mit Ergebnis und Dauer je Block und allen Messwerten;
bei Fehlern zusätzlich Fehlerbericht_<JJJJ-MM-TT>_<HHMM>[_<tag>].md (gleicher Aufbau, nur die Fehler).
Standardverzeichnis docs/reviews (falls vorhanden). Antworten: j = ja, n = nein, ü = nicht geprüft.

ACHTUNG: Die Achse fährt über den ganzen Verfahrweg, die (simulierte) HV wird geschaltet, die Anlage
startet zweimal neu. Fahrbereich frei, Not-Aus in Reichweite. Abbruch mit Strg+C (sendet STOP).

    python3 scripts/acceptance_a1.py --host 192.168.4.1
    python3 scripts/acceptance_a1.py --host 192.168.4.1 --only T18,T19 --tag wiederholung   (Block 1 läuft vorher)
    python3 scripts/acceptance_a1.py --host 192.168.4.1 --no-manual          (Block 1 und 3, ohne Block 2)
    python3 scripts/acceptance_a1.py --host localhost:8090 --sim --yes       (gegen sim_device.py)

--sim: gegen scripts/sim_device.py; Not-Aus und Endschalter über die Test-Hooks, Fragen bleiben
offen, Display- und Messwert-Eingaben werden übersprungen (im Simulator sperrt ein ⏭ nicht).
"""
import argparse, datetime, json, os, random, subprocess, sys, threading, time, urllib.error, urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from add_client import build, frame, verify   # noqa: E402

ERR = {0: "OK", -1: "BAD_CRC", -2: "BAD_JSON", -3: "UNKNOWN_CMD", -4: "BAD_PARAM", -5: "STATE",
       -6: "PRECONDITION", -7: "BUSY", -8: "NOT_STABLE", -9: "CALIB_INVALID", -10: "LOCAL_CONTROL"}
SYS_ERR = ["NONE", "ESTOP", "TEMP_HIGH", "COMM_TIMEOUT", "MOTOR_FAULT", "HOMING_FAILED"]
# Measuring accuracy (decisions 08.11.2026, Norbert Walter; Pflichtenheft V1.7.16): the optical sensor
# reaches ±2 mm for single values after the calibration (3σ ≤ 2 mm); the MEAN per step stays at ±1 mm,
# so that a wrong calibration (up to 2.4 mm off) is still found. Values beside the material (lower boom,
# far outside the table) are recorded only; the edges in T19 are recorded, only "found" is checked.
TOL_MM = 1.0                # mean per step (T18, T20, step medians T19)
SINGLE_MM = 2.0             # single values: 3σ ≤ SINGLE_MM
SIGMA_MAX = round(SINGLE_MM / 3, 2)
RAW_PLAUS_MM = 7.0          # T18K: uncalibrated value against the reference (sensor / stair body plausible)
EDGE_FOUND_MM = 10.0        # T19: edge must be found within 105 / 225 ± 10 mm in each direction
TIMING_MAX_US = 3100        # T29 (decision 08.11.2026: demo, not critical)

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


class Problem(Exception):
    """A precondition or boundary condition is not met: the check cannot be completed meaningfully."""
    pass


SYMBOL = {"ok": "✅", "warn": "✅ ⚠️", "fail": "❌", "skip": "⏭", "problem": "⛔", "info": "–"}
LEGEND = """| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |
"""


def num(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def fmt(v):
    return ("%g" % v) if num(v) else str(v)


def check(cond, msg):
    if not cond:
        raise Fail(msg)


def say(text):
    print(text, flush=True)


def now_banner(text):
    say("\n  " + "=" * 64 + "\n  >>> JETZT: %s <<<\n  " % text + "=" * 64 + "\n")


# --------------------------------------------------------------------------------------- device
class Device:
    def __init__(self, host, sim):
        self.host, self.sim = host, sim
        self.io = threading.Lock()
        self.last_api = 0.0
        self.last_reply = {}
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

    @staticmethod
    def still(p):
        """No sequence active. motion.state is "moving" for every active sequence (also while a stationary
        measurement holds its position) and "error" (latched) after a fault until the next start."""
        return p["motion"]["state"] in ("idle", "error") and p["motion"]["mode"] == "none"

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

    def idle(self, timeout=90, what="Achse steht"):
        return self.wait(self.still, timeout, what)

    def move(self, pos, speed=10):
        self.ok("MOVE", pos=pos, speed=speed)
        p = self.idle(timeout=abs(pos - self.proc()["motion"]["pos"]) / speed + 15, what="MOVE %s" % pos)
        check(abs(p["motion"]["pos"] - pos) <= 0.1, "MOVE %s: Position %.1f" % (pos, p["motion"]["pos"]))
        return p

    def reboot(self):
        """REBOOT and wait until the device is back (ts restarts)."""
        p = self.proc()
        if p["safety"]["hv_on"]: self.ok("HV_OFF")
        if not self.still(p): self.ok("STOP"); self.idle(30)
        ts = self.proc()["ts"]
        self.expect_reboot = True
        try:
            self.ok("REBOOT")
            t0 = time.time()
            while True:
                time.sleep(0.5)
                try:
                    if self.proc()["ts"] < ts: break
                except Exception:
                    pass
                check(time.time() - t0 < 40, "Anlage nach 40 s nicht wieder erreichbar")
            time.sleep(1.0)
            self.last_ts = self.proc()["ts"]
        finally:
            self.expect_reboot = False

    def ready(self, hv=False):
        """READY (or RUNNING with hv=True), axis homed and idle, shutter closed, signals at their defaults."""
        p = self.proc()
        if p["safety"]["estop"]:
            raise Problem("Voraussetzung: Not-Aus ist gedrückt")
        if p["state"] in ("FAULT", "WAIT_ACK"):
            raise Problem("Voraussetzung: Anlage im Fehler %s (nicht quittierbar)" % SYS_ERR[p["err"]])
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
            say("  Referenzfahrt (INIT) läuft – bitte warten, nichts drücken ...")
            self.ok("INIT")
            try:
                self.wait(lambda q: q["state"] == "READY" and q["motion"]["homed"] and self.still(q), 120, "INIT")
            except Fail as e:
                raise Problem("Voraussetzung: Referenzfahrt gescheitert – %s" % e)
        if hv and not self.proc()["safety"]["hv_on"]:
            self.ok("HV_ON")
        return self.proc()


# ------------------------------------------------------------------------------------ context
class Ctx:
    def __init__(self, dev, args):
        self.d, self.args = dev, args
        self.notes = []
        self.values = []          # (id, name, value, unit, target)
        self.raw_out = []         # (title, text) for the appendix
        self.cache = {}           # shared data between checks (e.g. step measurements for T18 and T20)
        self.tid = ""

    @property
    def interactive(self):
        return not self.args.sim               # --no-manual only leaves out block 2; the safety block always asks

    def val(self, name, value, unit="", eq=None, nom=None, tol=None, lo=None, hi=None, mx=None, mn=None,
            target=None, warn=True, status=None):
        """Records a measured value and evaluates it against its target:
        eq (equal) · nom ± tol · lo … hi · ≤ mx · ≥ mn. Achtung (warn): > 80 % of the tolerance used,
        within the outer tenth of a range, > 80 % of a maximum or < 120 % of a minimum."""
        st, tgt = "info", ""
        if status:
            st = status
        elif eq is not None:
            tgt, st = fmt(eq), ("ok" if value == eq else "fail")
        elif num(value) and nom is not None and tol is not None:
            dev = abs(value - nom)
            tgt = "%s ± %s" % (fmt(nom), fmt(tol))
            st = "fail" if dev > tol + 1e-9 else ("warn" if warn and dev > 0.8 * tol else "ok")
        elif num(value) and lo is not None and hi is not None:
            tgt = "%s … %s" % (fmt(lo), fmt(hi))
            edge = min(value - lo, hi - value)
            st = "fail" if edge < -1e-9 else ("warn" if warn and edge < 0.1 * (hi - lo) else "ok")
        elif num(value) and mx is not None:
            tgt = "≤ %s" % fmt(mx)
            st = "fail" if value > mx + 1e-9 else ("warn" if warn and value > 0.8 * mx else "ok")
        elif num(value) and mn is not None:
            tgt = "≥ %s" % fmt(mn)
            st = "fail" if value < mn - 1e-9 else ("warn" if warn and mn > 0 and value < 1.2 * mn else "ok")
        elif any(x is not None for x in (eq, nom, lo, mx, mn)):
            st = "fail"                                   # no numeric value where one was expected
        if target is not None:
            tgt = target
        if isinstance(value, float):
            value = round(value, 3)
        self.values.append((self.tid, st, name, value, unit, tgt))
        return st

    def ask(self, question):
        """Yes/no question. -> True / False / None (not checked). Recorded in the protocol."""
        if self.interactive:
            while True:
                a = input("\n  ??? %s [j/n/ü] " % question).strip().lower()
                if a in ("j", "ja", "y"):
                    self.val(question, "ja", target="ja", status="ok"); return True
                if a in ("n", "nein"):
                    self.val(question, "nein", target="ja", status="fail"); return False
                if a in ("ü", "u", "s"): break
        self.val(question, "nicht geprüft", target="ja", status="skip")
        return None

    def asks(self, questions):
        """Several questions; Fail if one is answered with no."""
        bad = [q for q in questions if self.ask(q) is False]
        check(not bad, "nein: " + "; ".join(bad))

    def prompt(self, text):
        if self.interactive:
            input("\n  ... %s – weiter mit Enter " % text)

    def number(self, question, unit):
        if not self.interactive:
            raise Skip("Eingabe übersprungen")
        while True:
            a = input("\n  ### %s [%s] (leer = überspringen) " % (question, unit)).strip().replace(",", ".")
            if not a: raise Skip("vom Prüfer übersprungen")
            try: return float(a)
            except ValueError: pass

    def need_hand(self):
        pass                                   # kept for the call sites; block 2 is left out as a whole


TESTS = []


def test(tid, part, block, title):
    def deco(f):
        TESTS.append((tid, part, block, title, f))
        return f
    return deco


# ====================================================== Block 1 – Sicherheit (zuerst, Pflicht)
# Decision 08.11.2026 (Norbert Walter): the safety checks come first, in this order: emergency stop,
# shutter, direction of travel, end positions – then the emergency stop in motion. Without a passed
# safety block no further check runs; an error there has to be fixed first.

@test("S01", 0, "2, 4, 6", "Not-Aus im Stillstand: ESTOP, HV aus, Shutter zu, Dauerton, ACK erst nach Lösen am Display (1 s)")
def s01(c):
    d = c.d
    p = d.proc()
    if p["safety"]["estop"]:
        raise Problem("Voraussetzung: Not-Aus ist schon gedrückt – lösen und neu starten")
    if p["state"] in ("FAULT", "WAIT_ACK"):
        d.api("ACK")
        p = d.wait(lambda q: q["state"] not in ("FAULT", "WAIT_ACK"), 5, "Fehler quittiert", allow_fault=True)
    if not d.still(p):
        d.ok("STOP"); d.idle(30, "STOP")
    c.prompt("Die Achse steht und bewegt sich in diesem Test nicht. Gleich bei 'JETZT' den NOT-AUS drücken "
             "und gedrückt halten")
    if c.args.sim:
        d.hook(estop=1)
    else:
        now_banner("NOT-AUS drücken und gedrückt halten")
    p = d.wait(lambda q: q["safety"]["estop"] and q["state"] in ("FAULT", "WAIT_ACK"), 60, "Not-Aus", allow_fault=True)
    c.val("Fehler", SYS_ERR[p["err"]], eq="ESTOP")
    c.val("HV / Shutter", "%s / %s" % ("an" if p["safety"]["hv_on"] else "aus", p["safety"]["shutter"]),
          eq="aus / closed")
    e = d.api("ACK")
    c.val("ACK per API bei gedrücktem Not-Aus", ERR.get(e, e), eq="PRECONDITION")
    c.asks(["Dauerton des Summers etwa 4 s?",
            "Display: alle Tasten ausgeblendet, nur die ACK-Fläche, grau solange Not-Aus gedrückt?"])
    if c.args.sim:
        d.hook(estop=0)
        d.wait(lambda q: not q["safety"]["estop"], 10, "Not-Aus gelöst", allow_fault=True)
        d.ok("ACK")
    else:
        c.prompt("Not-Aus jetzt LÖSEN")
        d.wait(lambda q: not q["safety"]["estop"], 60, "Not-Aus gelöst", allow_fault=True)
        c.prompt("Finger auf die Display-Taste STOP legen und liegen lassen, dann Enter")
        c.asks(["Quittiert der liegende Finger NICHT (ACK-Fläche bleibt)?"])
        now_banner("ACK-Fläche auf dem Display drücken und 1 s halten")
        t0 = time.time()
        d.wait(lambda q: q["state"] not in ("FAULT", "WAIT_ACK"), 60, "Quittierung am Display", allow_fault=True)
        c.val("Zeit bis Quittierung am Display", time.time() - t0, "s", mn=1.0, warn=False)
    p = d.wait(lambda q: q["state"] in ("SAFE_START", "READY"), 3, "nach ACK")
    check(p["err"] == 0, "nach ACK Fehler %s" % SYS_ERR[p["err"]])
    return "ESTOP, HV aus, Shutter zu, ACK über API abgewiesen, Quittierung am Display"


@test("S02", 0, "4", "Shutter im Stillstand: zu, rote LED aus, Öffnen ohne HV abgewiesen")
def s02(c):
    d = c.d
    p = d.proc()
    if p["safety"]["hv_on"]:
        d.ok("HV_OFF")
        p = d.proc()
    c.val("Shutter laut /proc", p["safety"]["shutter"], eq="closed")
    e = d.api("SHUTTER_OPEN")
    c.val("SHUTTER_OPEN ohne HV", ERR.get(e, e), status="ok" if e != 0 else "fail", target="abgewiesen")
    if e == 0:
        d.api("SHUTTER_CLOSE")
    c.asks(["Shutter mechanisch zu und rote LED aus?"])
    check(p["safety"]["shutter"] == "closed" and e != 0, "Shutter offen oder Öffnen ohne HV angenommen")
    return "Shutter zu, Öffnen ohne HV abgewiesen (%s)" % ERR.get(e, e)


@test("S03", 0, "2, 3", "Fahrtrichtung: Referenzfahrt zum hinteren Endschalter (Parkseite), +mm weg davon, Summer 1 Piep")
def s03(c):
    d = c.d
    p = d.proc()
    c.prompt("Gleich startet die Referenzfahrt (INIT). Der Schlitten muss zur PARKSEITE (hinterer Endschalter) "
             "fahren. Fährt er in die andere Richtung: SOFORT NOT-AUS. Hand am Not-Aus")
    t0 = time.time()
    if p["state"] == "READY" and p["motion"]["homed"]:
        d.ok("INIT")                                       # re-home on purpose: the direction is watched
    else:
        say("  Referenzfahrt (INIT) läuft – Hand am Not-Aus ...")
        d.ok("INIT")
    fwd = [False]

    def done(q):
        fwd[0] = fwd[0] or q["motion"]["limit_fwd"]
        return q["state"] == "READY" and q["motion"]["homed"] and d.still(q)
    d.wait(done, 120, "Referenzfahrt")
    c.val("Dauer INIT", time.time() - t0, "s")
    c.val("vorderer Endschalter während INIT", "nein" if not fwd[0] else "ja", eq="nein")
    p = d.proc()
    c.val("Position nach INIT", p["motion"]["pos"], "mm", nom=0.0, tol=0.05, warn=False)
    c.asks(["Ist der Schlitten zur Parkseite (hinterer Endschalter) gefahren?",
            "Hat der Summer am Ende der Referenzfahrt einmal kurz gepiept?"])
    c.prompt("Gleich fährt die Achse langsam auf 100 mm. Sie muss sich von der Parkseite WEG bewegen")
    d.move(100, speed=10)
    c.asks(["Ist der Schlitten von der Parkseite weg gefahren?"])
    check(not fwd[0], "vorderer Endschalter während der Referenzfahrt – Fahrtrichtung falsch?")
    return "INIT zur Parkseite, +mm weg davon"


@test("S04", 0, "2", "Endlagen: Endschalter vorn/hinten in /proc, kein Fehler bei stehender Achse")
def m03(c):
    d = c.d
    c.need_hand()
    d.ready()
    for name, key, hook in (("VORDEREN", "limit_fwd", "press_fwd"), ("HINTEREN", "limit_bwd", "press_bwd")):
        if c.args.sim:
            d.hook(**{hook: 1})
        else:
            now_banner("%s Endschalter von Hand drücken und halten" % name)
        d.wait(lambda q: q["motion"][key], 30, "%s Endschalter gedrückt" % name)
        if c.args.sim:
            d.hook(**{hook: 0})
        else:
            say("  erkannt – jetzt loslassen")
        p = d.wait(lambda q: not q["motion"][key], 30, "%s Endschalter losgelassen" % name)
        check(p["state"] not in ("FAULT", "WAIT_ACK"), "Endschalter bei stehender Achse löst Fehler aus")
        c.val("%s gedrückt / losgelassen erkannt" % key, "ja", eq="ja")
    return "beide Endschalter erkannt, kein Fehler"


@test("S05", 0, "3", "Endlagen: unerwarteter Endschalter während der Fahrt -> MOTOR_FAULT, Achse steht")
def m08(c):
    d = c.d
    c.need_hand()
    d.ready()
    d.move(60)
    c.prompt("Gleich fährt die Achse langsam von 60 nach 200 mm. Erst bei 'JETZT' den VORDEREN Endschalter kurz drücken")
    d.ok("MOVE", pos=200, speed=5)
    d.wait(lambda q: q["motion"]["state"] == "moving", 3, "Fahrt läuft")
    if c.args.sim:
        time.sleep(1.0); d.hook(press_fwd=1); time.sleep(0.2); d.hook(press_fwd=0)
    else:
        now_banner("VORDEREN Endschalter kurz drücken")
    p = d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 40, "MOTOR_FAULT", allow_fault=True)
    c.val("Fehler", SYS_ERR[p["err"]], eq="MOTOR_FAULT")
    c.val("Halteposition", p["motion"]["pos"], "mm")
    check(p["err"] == 4, "Fehler %s (erwartet MOTOR_FAULT)" % SYS_ERR[p["err"]])
    stopped = p["motion"]["pos"]
    time.sleep(1.0)
    check(abs(d.proc()["motion"]["pos"] - stopped) < 1.0, "Achse fährt weiter")
    d.ok("ACK")
    p = d.wait(lambda q: q["state"] == "SAFE_START", 3, "SAFE_START nach ACK")
    check(not p["motion"]["homed"], "Referenz nach MOTOR_FAULT noch gültig")
    return "MOTOR_FAULT bei %.1f mm, nach ACK INIT nötig" % stopped


@test("S06", 0, "2, 4, 6", "Not-Aus in Fahrt (HV an, Shutter offen): ESTOP, HV aus, Shutter zu, Referenz verloren")
def s06(c):
    d = c.d
    for attempt in range(1, 4):
        d.ready(hv=True)
        c.prompt("Gleich startet eine Profilfahrt (80 -> 240 mm, 5 mm/s, Shutter öffnet). NICHT vorher drücken: "
                 "erst wenn 'JETZT' erscheint (nach etwa 5 – 10 s) den NOT-AUS drücken und gedrückt halten")
        d.ok("START_TRAVERSE", start=80, end=240, speed=5)
        p = d.wait(lambda q: (q["motion"]["mode"] == "traverse" and q["safety"]["shutter"] == "open")
                   or q["state"] in ("FAULT", "WAIT_ACK"), 60, "Profilfahrt läuft", allow_fault=True)
        if p["state"] not in ("FAULT", "WAIT_ACK"):
            break
        say("  Not-Aus zu früh gedrückt (Profilfahrt lief noch nicht) – Versuch %d von 3" % attempt)
        c.notes.append("Versuch %d: zu früh gedrückt" % attempt)
        if not c.args.sim:
            c.prompt("Not-Aus LÖSEN")
        d.wait(lambda q: not q["safety"]["estop"], 60, "Not-Aus gelöst", allow_fault=True)
        d.api("ACK")
        if attempt == 3:
            raise Problem("Not-Aus dreimal vor dem Start der Profilfahrt gedrückt")
    if c.args.sim:
        d.hook(estop=1)
    else:
        now_banner("NOT-AUS drücken und gedrückt halten")
    p = d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 60, "Not-Aus", allow_fault=True)
    c.val("Fehler", SYS_ERR[p["err"]], eq="ESTOP")
    c.val("Halteposition", p["motion"]["pos"], "mm")
    c.val("HV / Shutter", "%s / %s" % ("an" if p["safety"]["hv_on"] else "aus", p["safety"]["shutter"]),
          eq="aus / closed")
    check(p["err"] == 1 and p["safety"]["estop"], "Fehler %s (erwartet ESTOP)" % SYS_ERR[p["err"]])
    check(not p["safety"]["hv_on"] and p["safety"]["shutter"] == "closed", "HV/Shutter nicht aus")
    d.ok("SET", key="cooling_flow", value=False)
    p2 = d.proc()
    c.val("Fehler mit cooling_flow = false", SYS_ERR[p2["err"]], eq="ESTOP")
    check(p2["err"] == 1, "mit cooling_flow false: Fehler %s" % SYS_ERR[p2["err"]])
    d.expect(-6, "ACK")
    d.ok("SET", key="cooling_flow", value=True)
    if c.args.sim:
        d.hook(estop=0)
        d.wait(lambda q: not q["safety"]["estop"], 10, "Not-Aus gelöst", allow_fault=True)
        d.ok("ACK")
    else:
        c.prompt("Not-Aus jetzt LÖSEN")
        d.wait(lambda q: not q["safety"]["estop"], 60, "Not-Aus gelöst", allow_fault=True)
        now_banner("ACK-Fläche auf dem Display drücken und 1 s halten")
        d.wait(lambda q: q["state"] != "WAIT_ACK", 60, "Quittierung am Display", allow_fault=True)
    p = d.wait(lambda q: q["state"] in ("SAFE_START", "READY"), 3, "nach ACK")
    c.val("Zustand nach ACK", p["state"], eq="SAFE_START")
    check(p["state"] == "SAFE_START" and not p["motion"]["homed"], "nach Not-Aus in Fahrt %s, homed %s" % (
        p["state"], p["motion"]["homed"]))
    return "ESTOP in Fahrt, HV aus, Shutter zu; mit cooling_flow false weiter ESTOP; danach INIT nötig"


@test("S07", 0, "4", "Not-Aus stoppt die Bewegung ≤ 2 ms (Oszilloskop)")
def m12(c):
    d = c.d
    c.need_hand()
    if not c.interactive:
        raise Skip("Oszilloskop nur von Hand")
    d.ready()
    d.move(2)
    c.prompt("Oszilloskop: Kanal 1 an PROBE (GPIO 22), Kanal 2 an STEP von Treiber X, Trigger Kanal 1 fallende Flanke, "
             "Single, 1 ms/div. Gleich fährt die Achse mit 20 mm/s nach 240 mm")
    d.ok("MOVE", pos=240, speed=20)
    d.wait(lambda q: q["motion"]["vel"] >= 19.0, 10, "20 mm/s erreicht")
    now_banner("NOT-AUS drücken")
    d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 30, "Not-Aus", allow_fault=True)
    ms = c.number("Zeit von der Flanke (Kanal 1) bis zum letzten Schrittpuls (Kanal 2)", "ms")
    c.val("Not-Aus -> letzter Schritt", ms, "ms", mx=2.0)
    c.prompt("Bildschirmfoto als docs/reviews/A1_estop_oszi.png speichern. Not-Aus LÖSEN")
    d.wait(lambda q: not q["safety"]["estop"], 60, "Not-Aus gelöst", allow_fault=True)
    d.ok("ACK")
    check(ms <= 2.0, "%.2f ms" % ms)
    return "%.2f ms" % ms


# ================================================ Block 2 – weitere Handprüfungen
@test("M01", 1, "1, 2", "Neustart: Summer lang–kurz, Startsignal kurz–lang, SAFE_START, blaue LED 0,5 Hz")
def m01(c):
    d = c.d
    c.prompt("Gleich startet die Anlage neu. Auf Summer und Display achten")
    d.reboot()
    p = d.proc()
    check(p["state"] == "SAFE_START", "nach Neustart %s" % p["state"])
    check(d.count("boot: ready") >= 1, "keine Zeile 'boot: ready' im Log")
    c.val("Zustand nach Neustart", p["state"], eq="SAFE_START")
    c.asks(["Vor dem Neustart Summer lang–kurz?", "Nach dem Neustart Summer kurz–lang?",
            "Display zeigt SAFE_START?", "Blaue LED blinkt gleichmäßig 1 s an / 1 s aus (10 Perioden ≈ 20 s)?"])
    return "SAFE_START, boot: ready im Log"


@test("M04", 1, "2", "LED grün folgt HV, LED rot folgt Shutter")
def m04(c):
    d = c.d
    d.ready()
    d.move(40)
    d.ok("HV_ON")
    c.asks(["HV ein: leuchtet die grüne LED?"])
    d.ok("SHUTTER_OPEN")
    c.asks(["Shutter offen: leuchtet die rote LED?"])
    d.ok("SHUTTER_CLOSE")
    c.asks(["Shutter zu: ist die rote LED aus?"])
    d.ok("HV_OFF")
    c.asks(["HV aus: ist die grüne LED aus?"])
    return "HV und Shutter geschaltet"


@test("M05", 1, "2", "Fehlersignal: Summer 3 Pieps (Übertemperatur)")
def m05(c):
    d = c.d
    d.ready(hv=True)
    c.prompt("Gleich wird Übertemperatur simuliert. Auf den Summer achten")
    d.ok("SET", key="temp", value=35.0)
    p = d.wait(lambda q: q["state"] == "WAIT_ACK", 3, "TEMP_HIGH", allow_fault=True)
    c.val("Fehler", SYS_ERR[p["err"]], eq="TEMP_HIGH")
    c.asks(["Hat der Summer 3-mal gepiept?", "Zeigt das Display den Fehler TEMP_HIGH mit ACK-Fläche?"])
    d.ok("SET", key="temp", value=18.0)
    d.ok("ACK")
    d.wait(lambda q: q["state"] == "READY", 3, "READY nach ACK")
    check(SYS_ERR[p["err"]] == "TEMP_HIGH", "Fehler %s" % SYS_ERR[p["err"]])
    return "TEMP_HIGH, quittiert"


@test("M06", 1, "2", "Abstand neben dem Messgut aufzeichnen (Maßband, ohne Bewertung)")
def m06(c):
    d = c.d
    c.prompt("M06: Die Achse fährt auf 242 mm (ganz vorn, neben dem Stufenkörper), der Sensor schaut auf den "
             "unteren Ausleger. Danach misst du mit dem Maßband den Abstand von der Vorderkante des Sensorfensters "
             "bis zur Oberfläche des unteren Auslegers und gibst ihn in mm ein (keinen Prozentwert)")
    d.ready(hv=True)
    d.ok("START_MEASURE", pos=242)
    d.wait(lambda q: q["motion"]["mode"] == "measure" and abs(q["motion"]["pos"] - 242) <= 0.1 and q["motion"]["vel"] == 0,
           60, "Messung bei 242 mm")
    time.sleep(2.0)
    dist = d.proc()["measure"]["dist"]
    d.ok("STOP"); d.ok("HV_OFF")
    c.val("Abstand laut Anlage bei 242 mm", dist, "mm")
    tape = c.number("Abstand Sensorfenster – unterer Ausleger, mit dem Maßband gemessen", "mm")
    check(tape > 0, "Maßband-Wert muss größer 0 sein")
    c.val("Abstand Maßband", tape, "mm")
    c.val("Abweichung Anlage – Maßband (Info)", dist - tape, "mm")     # outside the table, noisy: no rating
    return "aufgezeichnet: Anlage %.1f mm, Maßband %.1f mm (ohne Bewertung)" % (dist, tape)


@test("M07", 1, "3", "MOVE ±0,1 mm, keine Schrittverluste nach 10 Fahrten (Messschieber)")
def m07(c):
    d = c.d
    c.need_hand()
    c.prompt("M07 prüft, ob der Antrieb Schritte verliert. Ablauf: (1) die Achse fährt auf 50 mm, du misst mit "
             "dem Messschieber den Abstand Schlitten – fester Bezugspunkt (z. B. Rahmen) und gibst ihn ein; "
             "(2) die Achse macht 10 Fahrten zu zufälligen Zielen zwischen 61 und 240 mm mit 5 … 20 mm/s "
             "(ca. 2 – 3 min, Fahrbereich frei halten); (3) sie fährt zurück auf 50 mm, du misst denselben Abstand "
             "noch einmal. Bestanden, wenn beide Werte höchstens 0,1 mm auseinander liegen")
    d.ready()
    d.move(50)
    a = c.number("Achse bei 50 mm. Abstand Schlitten – fester Bezugspunkt mit dem Messschieber", "mm")
    say("  10 Fahrten laufen (ca. 2 – 3 min) ...")
    rnd = random.Random(1)
    targets = []
    for _ in range(10):
        t = round(rnd.uniform(61, 240), 1); v = rnd.choice([5, 10, 15, 20])
        targets.append("%.1f@%d" % (t, v))
        d.move(t, speed=v)
    d.move(50)
    b = c.number("Wieder bei 50 mm. Gleichen Abstand erneut messen", "mm")
    c.val("Messschieber vorher", a, "mm"); c.val("Messschieber nachher", b, "mm")
    c.val("Differenz", abs(a - b), "mm", mx=0.1)
    c.val("Fahrten (Ziel mm @ mm/s)", " ".join(targets))
    check(abs(a - b) <= 0.1, "Abweichung %.2f mm nach 10 Fahrten" % abs(a - b))
    return "Differenz %.2f mm" % abs(a - b)


DISPLAY_KEYS = [
    ("INIT", lambda q: q["state"] == "READY" and q["motion"]["homed"] and Device.still(q), 120, "Referenzfahrt fertig"),
    ("HV", lambda q: q["safety"]["hv_on"], 20, "HV ein"),
    ("MEAS", lambda q: q["motion"]["mode"] == "measure", 20, "Messung gestartet"),
    ("STOP", lambda q: Device.still(q), 20, "Messung gestoppt"),
    ("TRAV", lambda q: q["motion"]["mode"] == "traverse", 20, "Profilfahrt gestartet"),
    ("STOP", lambda q: Device.still(q), 20, "Profilfahrt gestoppt"),
    ("HV", lambda q: not q["safety"]["hv_on"], 20, "HV aus"),
    ("PARK", lambda q: Device.still(q) and abs(q["motion"]["pos"] - 2.0) <= 0.1, 90, "Parkposition 2 mm"),
    ("SERV", lambda q: Device.still(q) and abs(q["motion"]["pos"] - 40.0) <= 0.1, 90, "Serviceposition 40 mm"),
]


@test("M10", 1, "6", "Display: 7 Tasten wirken, Anzeige korrekt, Positionsbalken synchron")
def m10(c):
    d = c.d
    if not c.interactive:
        raise Skip("Display-Bedienung nur von Hand")
    p = d.proc()
    if p["safety"]["hv_on"]: d.ok("HV_OFF")
    c.prompt("Display-Tasten der Reihe nach drücken, wenn das Skript es sagt (WLAN wird dafür nicht gebraucht)")
    done = []
    for key, cond, timeout, what in DISPLAY_KEYS:
        now_banner("Display-Taste %s drücken" % key)
        t0 = time.time()
        d.wait(cond, timeout, "Taste %s: %s" % (key, what))
        c.val("Taste %s -> %s" % (key, what), round(time.time() - t0, 1), "s", status="ok", target="Wirkung in /proc")
        done.append(key)
        if key == "TRAV":
            c.asks(["Läuft der Positionsbalken synchron mit der Achse?"])
    c.asks(["Hat jede Taste mit einem kurzen Klick-Ton quittiert?",
            "Werte, Zustand und Anzeigen korrekt, kein Flackern?"])
    return "Tasten " + ", ".join(done)


@test("M11", 1, "7", "Weboberfläche (Tablet im AP, lokale Datei, Hilfe, Einstellungen, Kalibrierseite, Router)")
def m11(c):
    if not c.interactive:
        raise Skip("Weboberfläche nur von Hand")
    c.prompt("Tablet mit WLAN 'ADD-Demo' (Passwort adddemo1) verbinden und http://192.168.4.1/ öffnen. "
             "Für die lokale Datei am PC: web-ui/ims-demo-ui.html aus dem AKTUELLEN Repository öffnen (nach git pull; "
             "eine alte Kopie aus Downloads o. Ä. passt nicht zur Firmware), Adresse %s eingeben" % c.d.host)
    c.asks(["Seite lädt ohne Internet, Werte aktualisieren sich mindestens zweimal pro Sekunde?",
            "Verlauf auf der Prozessseite zeigt bis zu 100 Punkte?",
            "Hilfe-Symbol öffnet /doc in einem neuen Tab mit Beschreibung und Linktabelle?",
            "Einstellungen: Wert ändern, Speichern, Neu laden -> Wert bleibt?",
            "Einstellungen: ungültiger Wert wird rot markiert und nicht gesendet?",
            "web-ui/ims-demo-ui.html am PC lokal geöffnet: Verbindungsdialog, Verbindung klappt?"])
    if c.ask("Router-Test durchführen (SSID/Passwort setzen, Neustart, Router aus/an – ca. 10 min)?"):
        c.asks(["Anlage im Heimnetz erreichbar (IP im Display), AP ADD-Demo bleibt erreichbar?",
                "Router aus: Log zeigt sta: attempt 1/3 … 3/3, dann sta: backoff 120 s?",
                "Router wieder an: Verbindung kommt von selbst zurück (sta: connected)?"])
    return "Weboberfläche geprüft"


# ============================================================ Teil 2 – automatische Prüfungen
@test("T01", 2, "5", "Alle Endpunkte antworten in < 1 s")
def t01(c):
    d, slow = c.d, []
    paths = ["/", "/settings", "/doc", "/proc", "/log", "/llms.txt", "/app.js", "/font-u400.woff2",
             "/font-u700.woff2", "/font-m400.woff2", "/font-m700.woff2", "/add"]
    for path in paths:
        t0 = time.perf_counter(); code, _ = d.raw(path, timeout=5); dt = time.perf_counter() - t0
        c.val("GET %s (HTTP %d)" % (path, code), dt * 1000, "ms", mx=1000)
        if path == "/add" and code == 404:
            continue                          # until Sprint D1
        check(code == 200, "%s -> HTTP %d" % (path, code))
        if dt >= 1.0: slow.append(path)
    t0 = time.perf_counter(); d.ok("GET_STATE")
    c.val("POST /api GET_STATE", (time.perf_counter() - t0) * 1000, "ms", mx=1000)
    check(not slow, "langsamer als 1 s: %s" % slow)
    return "alle < 1 s (/add 404 bis Sprint D1)"


@test("T02", 2, "5", "/proc vollständig, CRC gültig, ≤ 512 Byte")
def t02(c):
    code, txt = c.d.raw("/proc")
    check(code == 200 and verify(txt), "CRC ungültig")
    n = len(txt.encode())
    c.val("Länge /proc", n, "Byte", mx=512, warn=False)
    check(n <= 512, "länger als 512 Byte (%d)" % n)
    p = json.loads(txt)
    check(set(p) - {"crc"} == PROC_KEYS, "Felder: %s" % sorted(p))
    check(set(p["motion"]) == MOTION_KEYS and set(p["measure"]) == MEASURE_KEYS and set(p["safety"]) == SAFETY_KEYS,
          "Unterfelder unvollständig")
    return "alle Felder vorhanden"


@test("T03", 2, "5", "Fehlerantworten: falsche CRC, kaputtes JSON, unbekannter Befehl, Parameter, Zustand")
def t03(c):
    d = c.d
    cases = [('{"cmd":"GET_STATE","params":{},"crc":"00000000"}', 400, -1),
             (frame('{"cmd":}'), 400, -2), (build("FOO", {}), 200, -3), (build("MOVE", {}), 200, -4)]
    for body, code, err in cases:
        hc, txt = d.raw("/api", body)
        check(verify(txt), "Antwort ohne CRC bei %s" % ERR[err])
        r = json.loads(txt)
        c.val("Antwort auf %s-Fall" % ERR[err], "HTTP %d, err %d" % (hc, r["err"]), eq="HTTP %d, err %d" % (code, err))
        check(hc == code and r["err"] == err, "%s: HTTP %d err %s" % (ERR[err], hc, r["err"]))
    if d.proc()["state"] != "WAIT_ACK":
        d.expect(-5, "ACK")
        c.val("ACK außerhalb WAIT_ACK", "err -5", eq="err -5")
    return "alle Fehlerantworten wie erwartet"


@test("T04", 2, "1", "Parameter nach Neustart wiederhergestellt, Sicherheitsflags zurückgesetzt")
def t04(c):
    d = c.d
    orig = d.config()["measure_pos"]
    new = 150.5 if orig != 150.5 else 151.5
    d.ok("SET", key="measure_pos", value=new)
    d.ok("SET", key="interlock", value=False)
    d.ok("SET", key="temp", value=25.0)
    time.sleep(1.0)
    d.reboot()
    cfg = d.config(); p = d.proc()
    c.val("measure_pos nach Neustart", cfg["measure_pos"], "mm", nom=new, tol=0.05, warn=False)
    c.val("interlock / cooling_flow / temp nach Neustart", "%s / %s / %s" % (cfg["interlock"], cfg["cooling_flow"], cfg["temp"]),
          eq="True / True / 18.0")
    try:
        check(abs(cfg["measure_pos"] - new) < 0.05, "measure_pos nach Neustart %s" % cfg["measure_pos"])
        check(cfg["interlock"] is True and cfg["cooling_flow"] is True and cfg["temp"] == 18.0, "Sicherheitsflags nicht zurückgesetzt")
        check(p["state"] == "SAFE_START" and not p["motion"]["homed"], "nach Neustart %s" % p["state"])
    finally:
        d.ok("SET", key="measure_pos", value=orig)
    time.sleep(1.0)
    return "gespeicherter Wert da, Sicherheitsflags Standard, SAFE_START"


@test("T05", 2, "3", "INIT referenziert (hinterer Endschalter, 2 mm Rückzug, Position 0)")
def t05(c):
    d = c.d
    p = d.proc()
    if p["safety"]["hv_on"]: d.ok("HV_OFF")
    if p["motion"]["homed"]:
        d.move(30)
    d.ok("INIT")
    t0 = time.time()
    p = d.wait(lambda q: q["state"] == "READY" and q["motion"]["homed"] and Device.still(q), 90, "INIT")
    c.val("Dauer INIT", time.time() - t0, "s")
    c.val("Position nach INIT", p["motion"]["pos"], "mm", nom=0.0, tol=0.05, warn=False)
    check(abs(p["motion"]["pos"]) <= 0.05, "Position nach INIT %.2f" % p["motion"]["pos"])
    check(not p["motion"]["limit_bwd"], "hinterer Endschalter nach INIT noch betätigt (kein Rückzug)")
    return "READY, Endschalter frei"


@test("T07", 2, "4", "HV_ON nur mit allen Bedingungen; jede fehlende -> PRECONDITION")
def t07(c):
    d = c.d
    d.ready()
    for key, bad, good in (("interlock", False, True), ("cooling_flow", False, True), ("temp", 31.0, 18.0)):
        d.ok("SET", key=key, value=bad)
        check(not d.proc()["safety"]["hv_ok"], "hv_ok trotz %s=%s" % (key, bad))
        e = d.api("HV_ON")
        c.val("HV_ON mit %s = %s" % (key, bad), ERR.get(e, e), eq="PRECONDITION")
        check(e == -6 and not d.proc()["safety"]["hv_on"], "HV_ON mit %s=%s -> %s" % (key, bad, ERR.get(e, e)))
        d.ok("SET", key=key, value=good)
    d.ok("HV_ON")
    st = d.proc()["state"]
    c.val("HV_ON mit allen Bedingungen", st, eq="RUNNING")
    check(st == "RUNNING", "-> %s" % st)
    d.ok("HV_OFF")
    return "jede fehlende Bedingung -> PRECONDITION (Not-Aus: S01, S06)"


@test("T09", 2, "4", "Temperatur 31 °C -> HV aus ohne Fehler; 35 °C -> TEMP_HIGH, ACK erst nach Absenken")
def t09(c):
    d = c.d
    d.ready(hv=True)
    d.ok("SET", key="temp", value=31.0)
    p = d.wait(lambda q: not q["safety"]["hv_on"], 2, "HV aus bei 31 °C")
    c.val("31 °C: Zustand / Fehler", "%s / %s" % (p["state"], SYS_ERR[p["err"]]), eq="READY / NONE")
    check(p["state"] == "READY" and p["err"] == 0, "31 °C: %s %s" % (p["state"], SYS_ERR[p["err"]]))
    d.ok("SET", key="temp", value=18.0)
    d.ok("HV_ON")
    d.ok("SET", key="temp", value=35.0)
    p = d.wait(lambda q: q["state"] == "WAIT_ACK", 2, "TEMP_HIGH", allow_fault=True)
    c.val("35 °C: Fehler", SYS_ERR[p["err"]], eq="TEMP_HIGH")
    check(p["err"] == 2 and not p["safety"]["hv_on"], "35 °C: %s" % SYS_ERR[p["err"]])
    e = d.api("ACK")
    c.val("ACK bei 35 °C", ERR.get(e, e), eq="PRECONDITION")
    check(e == -6, "ACK bei 35 °C -> %s" % ERR.get(e, e))
    d.ok("SET", key="temp", value=25.0)
    d.ok("ACK")
    p = d.wait(lambda q: q["state"] == "READY", 2, "READY nach ACK")
    c.val("nach ACK bei 25 °C: HV", p["safety"]["hv_on"], eq=False)
    check(not p["safety"]["hv_on"], "HV nach ACK wieder an")
    d.ok("SET", key="temp", value=18.0)
    return "wie erwartet"


@test("T10", 2, "4", "Shutter öffnet nie in der Parkzone (≤ 20 mm)")
def t10(c):
    d = c.d
    d.ready(hv=True)
    d.move(10)
    e1 = d.api("SHUTTER_OPEN"); e2 = d.api("START_MEASURE", pos=10)
    c.val("SHUTTER_OPEN bei 10 mm", ERR.get(e1, e1), eq="PRECONDITION")
    c.val("START_MEASURE bei 10 mm", ERR.get(e2, e2), eq="BAD_PARAM")
    check(e1 == -6 and e2 == -4, "SHUTTER_OPEN %s, START_MEASURE %s" % (ERR.get(e1), ERR.get(e2)))
    d.move(40)
    d.ok("SHUTTER_OPEN")
    d.ok("MOVE", pos=10, speed=5)
    bad, closed_at = [], None
    while True:
        p = d.proc()
        if p["safety"]["shutter"] == "open" and p["motion"]["pos"] < 19.95: bad.append(p["motion"]["pos"])  # /proc rounds to 0.1 mm
        if closed_at is None and p["safety"]["shutter"] == "closed": closed_at = p["motion"]["pos"]
        if Device.still(p): break
        time.sleep(0.03)
    c.val("manueller Shutter schließt bei", closed_at if closed_at is not None else "–", "mm", target="Info: erste Abfrage mit Shutter zu (Abtastung ≈ 0,5 mm)")
    c.val("Abfragen mit offenem Shutter in der Parkzone", len(bad), eq=0)
    check(not bad, "Shutter offen bei pos %s" % bad[:3])
    return "Parkzone immer zu"


@test("T11", 2, "4", "SHUTTER_OPEN manuell nur in der Servicezone; bei 145 mm -> PRECONDITION")
def t11(c):
    d = c.d
    d.ready(hv=True)
    d.move(40)
    e = d.api("SHUTTER_OPEN")
    c.val("SHUTTER_OPEN bei 40 mm", ERR.get(e, e), eq="OK")
    check(e == 0 and d.proc()["safety"]["shutter"] == "open", "Shutter bei 40 mm nicht offen")
    d.ok("SHUTTER_CLOSE")
    d.move(145)
    e = d.api("SHUTTER_OPEN")
    c.val("SHUTTER_OPEN bei 145 mm", ERR.get(e, e), eq="PRECONDITION")
    check(e == -6, "145 mm -> %s" % ERR.get(e, e))
    return "wie erwartet"


@test("T12", 2, "4", "Automatischer Shutter nie in der Servicezone (≤ 60 mm), schließt beim Einfahren")
def t12(c):
    d = c.d
    d.ready(hv=True)
    for cmd, kw in (("START_MEASURE", dict(pos=40)), ("START_MEASURE", dict(pos=60)),
                    ("START_TRAVERSE", dict(start=60, end=100, speed=10))):
        e = d.api(cmd, **kw)
        c.val("%s %s" % (cmd, kw), ERR.get(e, e), eq="BAD_PARAM")
        check(e == -4, "%s %s -> %s" % (cmd, kw, ERR.get(e, e)))
    d.ok("START_TRAVERSE", start=100, end=70, speed=10)
    bad, opened = [], False
    d.wait(lambda q: q["motion"]["mode"] == "traverse", 3, "Profilfahrt startet")
    while True:
        p = d.proc()
        if p["safety"]["shutter"] == "open":
            opened = True
            if p["motion"]["pos"] < 59.95: bad.append(p["motion"]["pos"])   # /proc rounds to 0.1 mm
        if p["motion"]["mode"] == "none": break
        time.sleep(0.03)
    check(opened, "Shutter hat sich bei der Profilfahrt 100 -> 70 nicht geöffnet")
    check(p["safety"]["shutter"] == "open", "Shutter direkt nach dem Lauf zu (Nachlaufzeit erwartet)")
    d.ok("MOVE", pos=40, speed=10)
    closed_at = None
    while True:
        p = d.proc()
        if p["safety"]["shutter"] == "open" and p["motion"]["pos"] < 59.95: bad.append(p["motion"]["pos"])
        if closed_at is None and p["safety"]["shutter"] == "closed": closed_at = p["motion"]["pos"]
        if Device.still(p): break
        time.sleep(0.03)
    c.val("automatischer Shutter schließt bei", closed_at if closed_at is not None else "–", "mm", target="Info: erste Abfrage mit Shutter zu (Abtastung ≈ 0,5 mm)")
    c.val("Abfragen mit offenem Shutter in der Servicezone", len(bad), eq=0)
    check(not bad, "Shutter offen bei pos %s" % bad[:3])
    return "Servicezone immer zu"


@test("T13", 2, "4", "Hin- und Rückfahrt innerhalb 3 s: Shutter bleibt offen; 3 s nach Ende zu; STOP schließt sofort")
def t13(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=100, end=200, speed=10)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["shutter"] == "open", 30, "Shutter offen im Lauf")
    d.wait(lambda q: q["motion"]["mode"] == "none", 30, "Ende Hinfahrt")
    t_gap = time.time()
    d.ok("START_TRAVERSE", start=200, end=100, speed=10)
    c.val("Zeit bis zum Folgebefehl", time.time() - t_gap, "s", mx=3.0)
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
    c.val("Shutter zu nach Laufende", dt, "s", lo=2.5, hi=4.0, target="3 (2,5 … 4,0)")
    check(2.5 <= dt <= 4.0, "Shutter nach %.1f s zu" % dt)
    d.ok("START_MEASURE", pos=155)
    d.wait(lambda q: q["motion"]["mode"] == "measure" and q["safety"]["shutter"] == "open", 30, "Messung, Shutter offen")
    d.ok("STOP")
    sh = d.proc()["safety"]["shutter"]
    c.val("Shutter direkt nach STOP", sh, eq="closed")
    check(sh == "closed", "Shutter nach STOP noch offen")
    return "wie erwartet"


@test("T14", 2, "4", "Fahrt auf 242 mm löst den vorderen Endschalter nicht aus")
def t14(c):
    d = c.d
    d.ready()
    p = d.move(242)
    c.val("Position", p["motion"]["pos"], "mm", nom=242.0, tol=0.1, warn=False)
    c.val("vorderer Endschalter", p["motion"]["limit_fwd"], eq=False)
    check(not p["motion"]["limit_fwd"] and p["state"] == "READY", "Endschalter %s, %s" % (p["motion"]["limit_fwd"], p["state"]))
    return "Endschalter frei"


@test("T15", 2, "4", "cooling_flow = false während Profilfahrt -> HV aus, Fahrt gestoppt, Shutter zu")
def t15(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=80, end=240, speed=10)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["shutter"] == "open" and q["motion"]["pos"] > 120,
           60, "Profilfahrt (Messlauf) bei 120 mm")
    t0 = time.time()
    d.ok("SET", key="cooling_flow", value=False)
    p = d.wait(lambda q: not q["safety"]["hv_on"] and q["safety"]["shutter"] == "closed" and q["motion"]["mode"] == "none",
               1.0, "HV aus, Shutter zu, Programm beendet")
    c.val("HV aus / Shutter zu nach", (time.time() - t0) * 1000, "ms", mx=1000)
    c.val("Zustand / Fehler", "%s / %s" % (p["state"], SYS_ERR[p["err"]]), eq="READY / NONE")
    check(p["state"] == "READY" and p["err"] == 0, "Zustand %s, Fehler %s" % (p["state"], SYS_ERR[p["err"]]))
    p = d.idle(2, "Achse steht")
    c.val("Halteposition", p["motion"]["pos"], "mm")
    d.ok("SET", key="cooling_flow", value=True)
    check(p["motion"]["homed"], "Referenz verloren (kontrollierter Halt erwartet)")
    return "READY ohne Fehler, Referenz bleibt"


@test("T16", 2, "4", "Kommunikations-Watchdog: COMM_TIMEOUT nach 5 s; nach ACK kein Wiederanlauf, INIT nötig")
def t16(c):
    d = c.d
    d.ready(hv=True)
    d.ok("START_TRAVERSE", start=240, end=80, speed=5)
    d.wait(lambda q: q["motion"]["mode"] == "traverse" and q["safety"]["comm_armed"], 40, "Profilfahrt läuft")
    d.hb = False
    with d.io:
        t_last = d.last_api
    try:
        try:
            p = d.wait(lambda q: q["state"] in ("FAULT", "WAIT_ACK"), 10, "COMM_TIMEOUT", allow_fault=True)
        except Fail as e:
            raise Fail(str(e) + " – sendet ein anderer Client (Browser mit der Weboberfläche, add_client shell) noch"
                       " GET_STATE? Für Teil 2 alle anderen Clients schließen")
        dt = time.time() - t_last
    finally:
        d.hb = True
    c.val("Fehler", SYS_ERR[p["err"]], eq="COMM_TIMEOUT")
    c.val("Zeit seit dem letzten /api-Paket", dt, "s", lo=4.8, hi=6.5, target="5 (4,8 … 6,5)")
    check(p["err"] == 3, "Fehler %s" % SYS_ERR[p["err"]])
    check(4.8 <= dt <= 6.5, "Fehler nach %.1f s" % dt)
    check(not p["safety"]["hv_on"] and p["safety"]["shutter"] == "closed", "HV/Shutter nach Fehler nicht aus")
    d.ok("ACK")
    time.sleep(2.0)
    p = d.proc()
    c.val("nach ACK: Zustand / homed / HV", "%s / %s / %s" % (p["state"], p["motion"]["homed"], p["safety"]["hv_on"]),
          eq="SAFE_START / False / False")
    check(not p["safety"]["hv_on"] and Device.still(p), "Wiederanlauf nach ACK")
    check(p["state"] == "SAFE_START" and not p["motion"]["homed"], "nach Fehler in Fahrt: %s" % p["state"])
    d.expect(-5, "HV_ON")
    return "COMM_TIMEOUT, kein Wiederanlauf, INIT nötig"


@test("T17", 2, "4", "SET maint_pos bei fahrender Achse: Speichern verschoben bis Stillstand mit HV aus")
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
    c.val("'save deferred' während der Fahrt", d.count("save deferred") > deferred0, eq=True)
    c.val("'nvs: saved' während der Fahrt", d.count("nvs: saved") > saved0, eq=False)
    check(d.count("save deferred") > deferred0, "keine neue Zeile 'nvs: save deferred' im Log")
    check(d.count("nvs: saved") == saved0, "während der Fahrt gespeichert")
    d.idle(40)
    time.sleep(1.0)
    c.val("'nvs: saved' nach dem Stillstand", d.count("nvs: saved") > saved0, eq=True)
    check(d.count("nvs: saved") > saved0, "nach dem Stillstand nicht gespeichert")
    check(d.config()["maint_pos"] == new, "maint_pos nicht übernommen")
    d.ok("SET", key="maint_pos", value=orig)
    time.sleep(1.0)
    return "Speichern erst im Stillstand"


def stats(v):
    """-> (mean, sigma (sample standard deviation), min, max, n)"""
    n = len(v)
    if not n:
        return float("nan"), float("nan"), float("nan"), float("nan"), 0
    m = sum(v) / n
    sd = (sum((x - m) ** 2 for x in v) / (n - 1)) ** 0.5 if n > 1 else 0.0
    return m, sd, min(v), max(v), n


def measure_at(c, pos):
    """Stationary measurement at pos with HV on and the shutter open; the shutter is meant to stay open
    over all steps (decision 08.11.2026: spare the shutter mechanics). From a running stationary
    measurement the next START_MEASURE is sent directly; a firmware that answers BUSY (before Contract
    V1.6.4) needs a STOP first, which closes the shutter – counted in c.cache["shutter_stops"]."""
    d = c.d
    p = d.proc()
    if p["motion"]["mode"] == "measure":
        e = d.api("START_MEASURE", pos=pos)
        if e == -7:                                              # BUSY: older firmware
            c.cache["shutter_stops"] = c.cache.get("shutter_stops", 0) + 1
            d.ok("STOP")
            d.idle(20, "STOP")
            d.ok("START_MEASURE", pos=pos)
        else:
            check(e == 0, "START_MEASURE %d -> %s" % (pos, ERR.get(e, e)))
    else:
        if p["state"] != "RUNNING" or not p["motion"]["homed"] or not d.still(p):
            d.ready(hv=True)
        d.ok("START_MEASURE", pos=pos)
    d.wait(lambda q: q["motion"]["mode"] == "measure" and abs(q["motion"]["pos"] - pos) <= 0.1
           and q["motion"]["vel"] == 0 and q["safety"]["shutter"] == "open", 40, "Messung bei %d mm" % pos)


def shutter_note(c):
    n = c.cache.get("shutter_stops", 0)
    c.val("Shutter zwischen den Stufen geschlossen (STOP nötig, Firmware vor V1.6.4)", n)
    return "" if not n else " – Shutter %d× geschlossen (Firmware vor V1.6.4)" % n


def measure_steps(c):
    """Stationary measurement at every step centre: 2 s settling, then 40 samples from /proc (every 50 ms).
    Cached for T18 and T20. -> {pos: {"dist": [...], "thick": [...], "avg": device mean, "n": device n}}"""
    if "steps" in c.cache:
        return c.cache["steps"]
    c.cache["shutter_stops"] = 0
    d = c.d
    res = {}
    for pos, dist, thick in STEPS:
        measure_at(c, pos)
        time.sleep(2.0)
        ds, ts = [], []
        for _ in range(40):
            q = d.proc()
            if q["measure"]["valid"] and q["measure"]["in_range"]:
                ds.append(q["measure"]["dist"]); ts.append(q["measure"]["thick"])
            time.sleep(0.05)
        m = d.proc()["measure"]                                  # no STOP: the shutter stays open
        res[pos] = {"dist": ds, "thick": ts, "avg": m["avg"], "n": m["n"]}
        c.raw_out.append(("%s – Einzelwerte Stufe %d mm (Soll Abstand %.1f, Dicke %.1f)" % (c.tid, pos, dist, thick),
                          "Abstand [mm]: " + " ".join("%.1f" % x for x in ds) +
                          "\nDicke [mm]:   " + " ".join("%.1f" % x for x in ts)))
    c.cache["steps"] = res
    return res


@test("T18K", 2, "8", "Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert")
def t18k(c):
    """Fresh calibration right before T18/T20 (decision 08.11.2026: the open shutter shifts the sensor
    signal, so the table is taken while measuring). Needs Contract V1.6.3 (CALIB during MEAS_HOLD)."""
    d = c.d
    before = d.config()
    if "calib_raw_1" not in before:
        raise Problem("Firmware ohne Kalibriertabelle (vor Contract V1.6.0) – erst K1 flashen")
    c.cache["shutter_stops"] = 0
    for i, (pos, dist, thick) in enumerate(STEPS, start=1):
        measure_at(c, pos)
        time.sleep(1.0)
        # the firmware takes a point only after 500 ms within ±0.5 mm; at the far step (σ ≈ 0.5 mm)
        # that needs patience: up to 30 attempts 0.5 s apart (≈ 15 s)
        for k in range(30):
            e = d.api("CALIB", action="point", index=i)
            if e != -8 or k == 29:                                  # NOT_STABLE: retry
                break
            time.sleep(0.5)
        c.val("Punkt %d (%d mm): Versuche bis stabil" % (i, pos), k + 1, warn=False)
        if e == -8:
            v = []
            for _ in range(40):
                v.append(d.proc()["measure"]["dist"]); time.sleep(0.05)
            c.val("Punkt %d (%d mm): Streuung Abstand (σ, Diagnose)" % (i, pos), stats(v)[1], "mm")
        if e == -7:
            d.ok("STOP")
            raise Problem("CALIB während der Messung abgelehnt (BUSY): Firmware vor Contract V1.6.3 (Sprint K2)")
        check(e == 0, "Stufe %d mm: CALIB point -> %s" % (pos, ERR.get(e, e)))
    d.ok("CALIB", action="save")
    after = d.config()
    for i, (pos, dist, thick) in enumerate(STEPS, start=1):
        st = c.val("Punkt %d (%d mm): Rohwert (unkalibriert) gegen Referenz %.0f" % (i, pos, after["calib_ref_%d" % i]),
                   after["calib_raw_%d" % i], "mm", nom=after["calib_ref_%d" % i], tol=RAW_PLAUS_MM, warn=False)
        if st == "fail":
            c.notes.append("Punkt %d unplausibel" % i)
        c.val("Punkt %d (%d mm): Änderung gegenüber der alten Tabelle" % (i, pos),
              round(after["calib_raw_%d" % i] - before["calib_raw_%d" % i], 2), "mm")
    c.val("calib_valid", after["calib_valid"], eq=True)
    check(after["calib_valid"], "Tabelle nach der Kalibrierung ungültig")
    check(not any(abs(after["calib_raw_%d" % i] - after["calib_ref_%d" % i]) > RAW_PLAUS_MM for i in range(1, 7)),
          "Rohwert mehr als %.0f mm von der Referenz: Sensor oder Stufenkörper prüfen" % RAW_PLAUS_MM)
    c.cache["calibrated"] = True
    return "6 Punkte mit HV an und offenem Shutter übernommen und gespeichert" + shutter_note(c)


@test("T18", 2, "2, 8", "Stehende Messung je Stufe: Mittelwert ±1 mm, Einzelwerte 3σ ≤ 2 mm")
def t18(c):
    c.cache.pop("steps", None)                                   # fresh values after T18K
    res = measure_steps(c)
    bad = []
    for pos, dist, thick in STEPS:
        r = res[pos]
        dm, dsd, dmin, dmax, n = stats(r["dist"])
        tm, tsd, tmin, tmax, _ = stats(r["thick"])
        sts = [c.val("%d mm: Abstand Mittelwert" % pos, dm, "mm", nom=dist, tol=TOL_MM),
               c.val("%d mm: Dicke Mittelwert" % pos, tm, "mm", nom=thick, tol=TOL_MM),
               c.val("%d mm: Dicke Mittelwert der Anlage (avg)" % pos, r["avg"], "mm", nom=thick, tol=TOL_MM),
               c.val("%d mm: Dicke Sigma" % pos, tsd, "mm", mx=SIGMA_MAX,
                     target="≤ %.2f (Einzelwerte 3σ ≤ %.0f)" % (SIGMA_MAX, SINGLE_MM)),
               c.val("%d mm: Anzahl Einzelwerte" % pos, n, mn=30, warn=False)]
        c.val("%d mm: Dicke Min / Max" % pos, "%.1f / %.1f" % (tmin, tmax), "mm")
        c.val("%d mm: Abstand Sigma" % pos, dsd, "mm")
        if "fail" in sts:
            bad.append("%d mm" % pos)
    check(not bad, "außerhalb der Toleranz: %s" % ", ".join(bad))
    return "alle Stufen in Toleranz (Einzelwerte im Anhang)" + shutter_note(c)


def material_span(samples, max_gap=3):
    """Edges of the material from one run, robust against single outliers beside the material (the sensor
    looks at the lower boom about 123 mm away and scatters strongly): the samples are taken in the order of
    travel, gaps of at most max_gap samples inside the material are bridged, and the LONGEST contiguous
    stretch of on_material is the material. -> (low edge, high edge, outliers outside the stretch)"""
    runs, cur, gap = [], None, 0
    for i, (pos, _, on) in enumerate(samples):
        if on:
            if cur is None: cur = [i, i]
            else: cur[1] = i
            gap = 0
        elif cur is not None:
            gap += 1
            if gap > max_gap:
                runs.append(cur); cur, gap = None, 0
    if cur is not None: runs.append(cur)
    if not runs:
        return None, None, 0
    a, b = max(runs, key=lambda r: r[1] - r[0])
    lo, hi = sorted((samples[a][0], samples[b][0]))
    outliers = sum(1 for j, s_ in enumerate(samples) if s_[2] and not a <= j <= b)
    return lo, hi, outliers


def traverse_run(c, start, end, speed):
    """One profile run start -> end with HV on. From a running stationary measurement or right after the
    previous run (within the shutter hold time) the shutter stays open; a firmware that answers BUSY to
    START_TRAVERSE during a stationary measurement (before V1.6.4) needs a STOP first (counted).
    -> (samples [(pos, thick, on_material)], closed samples in the material zone, device avg, n)"""
    d = c.d
    p = d.proc()
    if p["motion"]["mode"] == "measure":
        e = d.api("START_TRAVERSE", start=start, end=end, speed=speed)
        if e == -7:
            c.cache["shutter_stops"] = c.cache.get("shutter_stops", 0) + 1
            d.ok("STOP")
            d.idle(20, "STOP")
            d.ok("START_TRAVERSE", start=start, end=end, speed=speed)
        else:
            check(e == 0, "START_TRAVERSE -> %s" % ERR.get(e, e))
    else:
        if p["state"] != "RUNNING" or not p["motion"]["homed"]:
            d.ready(hv=True)
        d.ok("START_TRAVERSE", start=start, end=end, speed=speed)
    d.wait(lambda q: q["motion"]["mode"] == "traverse", 3, "Profilfahrt startet")
    samples, closed, running = [], 0, False
    while True:
        p = d.proc()
        if p["motion"]["mode"] == "none": break
        m = p["measure"]
        if m["valid"]: running = True                            # count only after the run has started
        if running and 100.0 <= p["motion"]["pos"] <= 230.0 and p["safety"]["shutter"] != "open":
            closed += 1
        if m["valid"]:
            samples.append((p["motion"]["pos"], m["thick"], m["on_material"]))
        time.sleep(0.05)
    return samples, closed, p["measure"]["avg"], p["measure"]["n"]


@test("T19", 2, "8", "Profilfahrt hin und zurück: Kanten erkannt und aufgezeichnet, Stufen ±1 mm, Shutter durchgehend offen")
def t19(c):
    """Decision 08.11.2026: the edges are measured in both directions and compared (the direction of travel
    shifts them – sensor spot and filter lag); the mean of both is checked against the stair body.
    Values beside the material (lower boom) are not assessed."""
    runs = {}
    c.cache["shutter_stops"] = 0
    for name, a, b in (("hin", 80, 240), ("zurück", 240, 80)):
        runs[name] = traverse_run(c, a, b, 5)
    bad, edges = [], {}
    for name, (samples, closed, avg, n) in runs.items():
        c.val("%s: Messwerte im Lauf" % name, len(samples), mn=201)
        check(len(samples) > 200, "%s: zu wenige Messwerte (%d)" % (name, len(samples)))
        lo, hi, outl = material_span(samples)
        check(lo is not None, "%s: kein Messgut erkannt" % name)
        edges[name] = (lo, hi)
        c.val("%s: einzelne Messgut-Ausreißer neben dem Stufenkörper (Info)" % name, outl)
        for label, v, nom in (("Messgut ab", lo, 105.0), ("Messgut bis", hi, 225.0)):
            st = c.val("%s: Kante %s erkannt (Aufzeichnung, grob %g ± %g)" % (name, label, nom, EDGE_FOUND_MM), v, "mm",
                       nom=nom, tol=EDGE_FOUND_MM, warn=False)
            if st == "fail": bad.append("%s: Kante %s nicht erkannt (%.1f)" % (name, label, v))
        c.val("%s: Shutter geschlossen im Messgutbereich (Messpunkte)" % name, closed, eq=0)
        if closed: bad.append("%s: Shutter zwischendurch zu" % name)
        for pos, _, thick in STEPS:
            v = sorted(s_[1] for s_ in samples if abs(s_[0] - pos) <= 5.0)
            med = v[len(v) // 2] if v else float("nan")
            st = c.val("%s: Stufe %d mm: Dicke (Median)" % (name, pos), med, "mm", nom=thick, tol=TOL_MM)
            c.val("%s: Stufe %d mm: Anzahl Werte" % (name, pos), len(v))
            if st == "fail": bad.append("%s %d mm" % (name, pos))
        c.val("%s: Mittelwert der Anlage" % name, avg, "mm", lo=5.0, hi=30.0, warn=False)
        c.val("%s: Anzahl Werte der Anlage" % name, n, mn=51)
        if not (5.0 <= avg <= 30.0 and n > 50): bad.append("%s: Statistik (Mittel %.2f, n %d)" % (name, avg, n))
    for i, label in enumerate(("Messgut ab", "Messgut bis")):
        h, z = edges["hin"][i], edges["zurück"][i]
        c.val("Kante %s: Mittel hin/zurück (Info)" % label, (h + z) / 2, "mm")
        c.val("Kante %s: Unterschied hin – zurück (Info)" % label, h - z, "mm")
    check(not bad, "; ".join(bad))
    return "Kanten in beiden Richtungen erkannt (Werte im Protokoll), Stufen in Toleranz" + shutter_note(c)


@test("T20", 2, "8", "Kalibriertabelle (6 Stufen) gespeichert und wirksam: Abweichung je Stufe ±1 mm (Mittelwert, Sigma)")
def t20(c):
    cfg = c.d.config()
    for k in sorted(k for k in cfg if k.startswith("calib_") and k != "calib_valid"):
        c.val("Parameter %s" % k, cfg[k], "mm")
    c.val("calib_valid", cfg["calib_valid"], eq=True)
    check(cfg["calib_valid"], "calib_valid = false")
    if "calib_raw_1" not in cfg:
        raise Problem("Firmware ohne Kalibriertabelle (vor Contract V1.6.0) – erst K1 flashen")
    if not c.cache.get("calibrated") and all(abs(cfg["calib_raw_%d" % i] - cfg["calib_ref_%d" % i]) < 0.01
                                             for i in range(1, 7)):
        raise Problem("Werkstabelle (Rohwert = Referenz): T18K (Kalibrierung) lief nicht oder scheiterte")
    res = measure_steps(c)
    bad = []
    for pos, dist, thick in STEPS:
        dm, dsd, _, _, n = stats(res[pos]["dist"])
        st = c.val("Stufe %d mm: Abweichung Abstand (Mittelwert – %.0f mm)" % (pos, dist), dm - dist, "mm", nom=0.0, tol=TOL_MM)
        c.val("Stufe %d mm: Sigma Abstand" % pos, dsd, "mm")
        if st == "fail":
            bad.append("%d mm" % pos)
    check(not bad, "Kalibrierung nicht wirksam an: %s" % ", ".join(bad))
    return "Tabelle gültig und kalibriert, Abweichung je Stufe ≤ %.0f mm" % TOL_MM


def stress(c, name, *extra, timeout=900):
    cmd = [sys.executable, os.path.join(HERE, "stress_api.py"), "--host", c.args.host, name, *extra]
    say("    $ " + " ".join(cmd[1:]))
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    out = (r.stdout + r.stderr).strip()
    c.raw_out.append(("%s – stress_api.py %s" % (c.tid, " ".join([name, *extra])), out))
    for l in out.splitlines():
        l = l.strip()
        if any(k in l for k in ("cycle_max_us", "slowest", "STOP", "latency", "max", "RESULT")) and len(l) < 200:
            c.val(name, l)
    tail = [l for l in out.splitlines() if l.strip()][-2:]
    check(r.returncode == 0, "stress_api %s: %s" % (name, " | ".join(tail)))
    return "RESULT: PASS (Ausgabe im Anhang)"


@test("T24", 2, "4", "STOP per API während Profilfahrt: Antwort < 150 ms, Achse steht < 1 s")
def t24(c):
    c.d.ready()
    r = stress(c, "latency", "--n", "10", "--prepare")
    c.d.ready()
    return r


@test("T25", 2, "4", "Queue-Flut (200 SET): nur OK/BUSY, alle CRC gültig")
def t25(c):
    c.d.ready()
    return stress(c, "flood", "--n", "200")


@test("T26", 2, "4", "Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort")
def t26(c):
    if c.cache.get("quiet", "ok") != "ok":
        c.notes.append("Ruhe-Check vor Block 3: " + c.cache["quiet"])
    c.d.ready()
    return stress(c, "parallel", "--n", "400", "--threads", "8")


@test("T27", 2, "4", "Zeitverhalten 10 min mit dauernd geladener Weboberfläche (soak --web)")
def t27(c):
    if c.args.skip_soak:
        raise Skip("--skip-soak")
    c.d.ready()
    return stress(c, "soak", "--minutes", str(c.args.soak_minutes), "--web", timeout=int(c.args.soak_minutes * 60 + 300))


def foreign_traffic(d, seconds=8.0):
    """Lines that another client causes in /log while this script only reads /proc: slow page loads
    ('net: http', 'net: gap') and /api gaps of a client that armed the watchdog."""
    before = d.log()
    time.sleep(seconds)
    after = d.log()
    k = 0
    for n in range(min(len(before), len(after)), 0, -1):          # overlap of the ring buffer
        if before[-n:] == after[:n]:
            k = n; break
    new = after[k:] if k else [l for l in after if l not in before]
    return [l for l in new if l.startswith("net: http") or l.startswith("comm: /api gap")]


def quiet_check(c, d):
    """Before block 3: no other client may load the web UI or poll /api (falsifies T16, T26, T27)."""
    for attempt in range(1, 4):
        lines = foreign_traffic(d)
        if len(lines) < 2:
            say("  Ruhe-Check: keine anderen Clients erkannt")
            c.cache["quiet"] = "ok"
            return
        say("  Ruhe-Check: ein anderer Client greift noch zu (%s)" % "; ".join(lines[:3]))
        if c.interactive and attempt < 3:
            c.prompt("Bitte ALLE Browser (Tablet, PC, Handy) mit der Anlage schließen, auch die lokale Datei")
    c.cache["quiet"] = "anderer Client aktiv: " + "; ".join(lines[:3])
    say("  WARNUNG: anderer Client weiterhin aktiv – T16, T26, T27 können dadurch scheitern")


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


def git_rev():
    try:
        return subprocess.run(["git", "-C", HERE, "log", "-1", "--format=%h %s"], capture_output=True, text=True,
                              timeout=5).stdout.strip() or "–"
    except Exception:
        return "–"


def md(s):
    return str(s).replace("|", "/").replace("\n", " ")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="192.168.4.1")
    ap.add_argument("--pruefer", help="Name des Prüfers (sonst Abfrage beim Start)")
    ap.add_argument("--only", help="z. B. M04,T18 – der Sicherheitsblock S01–S07 läuft immer vorher")
    ap.add_argument("--no-manual", action="store_true", help="Block 2 (weitere Handprüfungen) auslassen; "
                    "der Sicherheitsblock läuft immer")
    ap.add_argument("--sim", action="store_true", help="gegen scripts/sim_device.py (Test-Hooks)")
    ap.add_argument("--yes", action="store_true", help="ohne Sicherheitsabfrage starten")
    ap.add_argument("--skip-soak", action="store_true", help="T27 (10-min-Dauertest) auslassen")
    ap.add_argument("--soak-minutes", type=float, default=10)
    ap.add_argument("--out-dir", help="Verzeichnis des Protokolls (Standard: docs/reviews, sonst aktuelles)")
    ap.add_argument("--tag", help="Zusatz im Dateinamen, z. B. wiederholung")
    args = ap.parse_args()

    order = [t[0] for t in TESTS]
    ids = [t.strip().upper() for t in args.only.split(",")] if args.only else list(order)
    tests = {t[0]: t for t in TESTS}
    unknown = [i for i in ids if i not in tests]
    if unknown:
        raise SystemExit("unbekannte Prüfung: %s (vorhanden: %s)" % (unknown, ", ".join(order)))
    if args.no_manual:
        ids = [i for i in ids if tests[i][1] != 1]
    # decision 08.11.2026: without the safety checks no other check may run – block 1 always comes first
    ids = [t[0] for t in TESTS if t[1] == 0] + [i for i in ids if tests[i][1] != 0]
    ids = sorted(set(ids), key=order.index)

    say("Abnahme A1 gegen http://%s/ – %d Prüfungen" % (args.host, len(ids)))
    say("ACHTUNG: Die Achse fährt über den ganzen Verfahrweg, die HV wird geschaltet, die Anlage startet neu.")
    say("Fahrbereich frei halten, Not-Aus in Reichweite. Abbruch jederzeit mit Strg+C (danach STOP).")
    pruefer = args.pruefer or ("Simulator-Test" if args.sim else "")
    while not pruefer:
        pruefer = input("Name des Prüfers: ").strip()
    if not args.yes and input("Starten? Mit 'ja' bestätigen: ").strip().lower() != "ja":
        raise SystemExit("abgebrochen")

    d = Device(args.host, args.sim)
    c = Ctx(d, args)
    start = datetime.datetime.now()
    try:
        boot = [l for l in d.log() if "boot: ready" in l]
        cfg_start = d.config()
    except Exception as e:
        raise SystemExit("Anlage nicht erreichbar: %s" % e)
    results = []
    part_shown = -1
    block_time = {}                       # part -> [start, end]
    blocked = ""
    try:
        for tid in ids:
            _, part, block, title, f = tests[tid]
            if part != part_shown:
                if part_shown in block_time:
                    block_time[part_shown][1] = time.time()
                if part_shown == 0:
                    bad = [r for r in results if r[1] == 0 and (r[4] in ("FAIL", "PROBLEM") or
                                                                (r[4] == "SKIP" and not args.sim))]
                    if bad:
                        blocked = ", ".join(r[0] for r in bad)
                        say("\n" + "!" * 68 + "\n  SICHERHEITSPRÜFUNG NICHT BESTANDEN (%s).\n  Keine weiteren Prüfungen – "
                            "erst die Fehler beheben, dann neu starten.\n" % blocked + "!" * 68)
                        break
                part_shown = part
                block_time[part] = [time.time(), None]
                if part == 0:
                    say("\n### Block 1 – Sicherheit: Not-Aus, Shutter, Fahrtrichtung, Endlagen (bitte an der Anlage, "
                        "Hand am Not-Aus) ###")
                elif part == 1:
                    say("\n### Block 2 – weitere Handprüfungen: bitte an der Anlage bleiben ###")
                else:
                    say("\n### Block 3 – automatische Prüfungen (ca. 40 min) ###")
                    if c.interactive:
                        c.prompt("Handprüfungen fertig. Jetzt ALLE Browser-Tabs mit der Anlage (auch das Tablet aus "
                                 "M11!) und jede add_client shell schließen, Not-Aus gelöst, Fahrbereich frei. "
                                 "Danach läuft alles ohne dich")
                    quiet_check(c, d)
            say("\n[%s] Block %s: %s" % (tid, block, title))
            t0 = time.time()
            c.notes, c.tid = [], tid
            try:
                detail = f(c) or ""
                status = "PASS"
            except Skip as e:
                status, detail = "SKIP", str(e)
                recover(d)
            except Problem as e:
                status, detail = "PROBLEM", str(e)
                recover(d)
            except (urllib.error.URLError, ConnectionError, TimeoutError, OSError) as e:
                status, detail = "PROBLEM", "Anlage nicht erreichbar: %s" % e
                recover(d)
            except Fail as e:
                status, detail = "FAIL", str(e)
                recover(d)
            except Exception as e:
                status, detail = "FAIL", "%s: %s" % (type(e).__name__, e)
                recover(d)
            mine = [v for v in c.values if v[0] == tid]
            if status == "PASS" and any(v[1] == "fail" for v in mine):
                status = "FAIL"
                detail = "Messwert außerhalb Soll: " + ", ".join(v[2] for v in mine if v[1] == "fail")
            if status == "PASS" and any(v[1] in ("warn", "skip") for v in mine):
                if any(v[1] == "warn" for v in mine):
                    status = "WARN"
                    detail += " – Achtung nah am Grenzwert: " + ", ".join(v[2] for v in mine if v[1] == "warn")
                if part == 0 and not args.sim and any(v[1] == "skip" for v in mine):
                    status = "SKIP"                       # a safety question left open is not passed
                    detail += " – Frage nicht beantwortet (Sicherheit: zählt nicht als bestanden)"
            if c.notes:
                detail += " [" + "; ".join(c.notes) + "]"
            say("  -> %s  %s  (%.0f s)" % ({"PASS": "✅", "WARN": "✅ ⚠️", "FAIL": "❌", "SKIP": "⏭", "PROBLEM": "⛔"}[status],
                                         detail, time.time() - t0))
            results.append((tid, part, block, title, status, detail, time.time() - t0))
            if part == 0 and (status in ("FAIL", "PROBLEM") or (status == "SKIP" and not args.sim)):
                blocked = tid                            # stop at the first safety error: nothing may move on
                say("\n" + "!" * 68 + "\n  SICHERHEITSPRÜFUNG %s NICHT BESTANDEN.\n  Keine weiteren Prüfungen – erst den "
                    "Fehler beheben, dann neu starten.\n" % tid + "!" * 68)
                break
    except KeyboardInterrupt:
        say("\nabgebrochen – sende STOP und HV_OFF")
        results.append(("–", part_shown if part_shown >= 0 else 0, "–", "Abbruch durch den Prüfer", "FAIL", "Strg+C", 0))
    finally:
        if part_shown in block_time and block_time[part_shown][1] is None:
            block_time[part_shown][1] = time.time()
        recover(d)
        try:
            if d.proc()["motion"]["homed"]:
                d.ok("PARK"); d.idle(60)
        except Exception:
            pass
    t_end = time.time()
    if 2 in block_time:
        final_checks(c, d, args, results, start)
    try:
        cfg_end, log_end = d.config(), d.log()
    except Exception:
        cfg_end, log_end = {}, []
    state = c.cache.get("state_end", {})
    d.run = False
    write_reports(c, args, results, block_time, blocked, pruefer, start, boot, cfg_start, cfg_end, log_end, state)


def final_checks(c, d, args, results, start):
    c.tid = "T28"
    run_min = (datetime.datetime.now() - start).seconds / 60
    c.val("Laufzeit", run_min, "min"); c.val("unerwartete Neustarts", d.resets, eq=0)
    results.append(("T28", 2, "1", "Kein unerwarteter Neustart während der Abnahme", "PASS" if d.resets == 0 else "FAIL",
                    "Laufzeit %.0f min" % run_min, 0))
    c.tid = "T29"
    st, detail, state = "FAIL", "", {}
    try:
        d.ok("GET_STATE")
        state = dict(d.last_reply)
        for k in ("cycle_max_us", "period_max_us", "prio_lat_max_us"):
            c.val(k, state[k], "µs", mx=TIMING_MAX_US)
        c.val("overruns (Info)", state["overruns"])
        st = "PASS" if all(state[k] <= TIMING_MAX_US for k in ("cycle_max_us", "period_max_us", "prio_lat_max_us")) else "FAIL"
        if st == "PASS" and any(v[1] == "warn" for v in c.values if v[0] == "T29"):
            st = "WARN"
        detail = "seit dem letzten Neustart" + (" – Simulator: Zeitwerte nicht simuliert" if args.sim else "")
    except Exception as e:
        detail = "GET_STATE: %s" % e
    results.append(("T29", 2, "4", "Zeitwerte ≤ %d µs (inkl. Summer-Signale aus Block 1 und 2)" % TIMING_MAX_US, st, detail, 0))
    c.cache["state_end"] = state


PART_HEAD = {0: "Block 1 – Sicherheit", 1: "Block 2 – weitere Handprüfungen", 2: "Block 3 – automatische Prüfungen"}
MARK = {"PASS": "✅ bestanden", "WARN": "✅ ⚠️ bestanden, Achtung", "FAIL": "❌ **nicht bestanden**",
        "SKIP": "⏭ übersprungen", "PROBLEM": "⛔ **Problem**"}


def counts(rows):
    n = {s: sum(1 for r in rows if r[4] == s) for s in ("PASS", "WARN", "FAIL", "SKIP", "PROBLEM")}
    return n, "%d ✅ bestanden (davon %d mit ⚠️ Achtung) · %d ❌ nicht bestanden · %d ⛔ Problem · %d ⏭ übersprungen" % (
        n["PASS"] + n["WARN"], n["WARN"], n["FAIL"], n["PROBLEM"], n["SKIP"])


def mmss(sec):
    return "%d:%02d min" % (sec // 60, sec % 60)


def write_report(path, title, c, args, results, block_time, blocked, pruefer, start, boot, cfg_start, cfg_end,
                 log_end, state, errors_only):
    """Writes the protocol; errors_only: the same structure with only the failed checks (Fehlerbericht)."""
    bad_ids = {r[0] for r in results if r[4] in ("FAIL", "PROBLEM")}
    keep = (lambda r: r[0] in bad_ids) if errors_only else (lambda r: True)
    _, total = counts(results)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("# %s\n\n| | |\n|---|---|\n" % title)
        fh.write("| Prüfer | %s |\n" % md(pruefer))
        fh.write("| Beginn / Ende | %s / %s |\n" % (start.strftime("%d.%m.%Y %H:%M"), datetime.datetime.now().strftime("%H:%M")))
        fh.write("| Gerät | http://%s/%s |\n" % (args.host, " (Simulator)" if args.sim else ""))
        fh.write("| Firmware | %s |\n| Repository | %s |\n" % (md(boot[-1] if boot else "–"), md(git_rev())))
        fh.write("| Aufruf | `%s` |\n" % md(" ".join(sys.argv[1:])))
        fh.write("| Ergebnis gesamt | **%s** |\n" % total)
        for part in (0, 1, 2):
            rows = [r for r in results if r[1] == part]
            if part in block_time:
                t0, t1 = block_time[part]
                fh.write("| Ergebnis %s | %s · Dauer %s |\n" % (PART_HEAD[part], counts(rows)[1], mmss((t1 or t0) - t0)))
            else:
                fh.write("| Ergebnis %s | nicht ausgeführt%s |\n" % (
                    PART_HEAD[part], " – **gesperrt: Sicherheitsprüfung nicht bestanden (%s)**" % blocked if blocked
                    else " (Aufruf)"))
        fh.write("\n### Symbole\n\n" + LEGEND + "\n")
        if blocked:
            fh.write("> **Sicherheitsprüfung nicht bestanden (%s).** Alle weiteren Prüfungen wurden nicht ausgeführt. "
                     "Erst die Fehler beheben, dann die Abnahme neu starten.\n\n" % blocked)
        for part in (0, 1, 2):
            rows = [r for r in results if r[1] == part and keep(r)]
            if not rows: continue
            fh.write("## %s\n\n| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |\n|---|---|---|---|---|---|\n"
                     % PART_HEAD[part])
            for tid, _, block, ttl, status, detail, secs in rows:
                fh.write("| %s | %s | %s | %s | %s | %s |\n" % (tid, block, md(ttl), MARK[status], md(detail),
                                                              "%.0f s" % secs if secs else "–"))
            fh.write("\n")
        vals = [v for v in c.values if not errors_only or v[1] in ("fail", "problem")]
        if errors_only and not vals:
            vals = [(None, "info", "keine Messwerte außerhalb des Solls (Fehler siehe Bemerkung oben)", "", "", "")]
        fh.write("## Messwerte\n\n%s\n\n| | ID | Größe | Wert | Einheit | Soll |\n|---|---|---|---|---|---|\n" % (
            "Nur die Werte außerhalb des Solls." if errors_only else
            "Alle erhobenen Werte und Antworten – zum Vergleich zweier Abnahmen."))
        for tid, st, k, v, unit, target in vals:
            fh.write("| %s | %s | %s | %s | %s | %s |\n" % (SYMBOL[st], tid or "–", md(k), md(v), md(unit), md(target)))
        fh.write("\n## Anhang\n\n")
        if not errors_only:
            fh.write("### Konfiguration zu Beginn (GET_CONFIG)\n\n```json\n%s\n```\n\n"
                     % json.dumps(cfg_start, ensure_ascii=False, indent=1))
            fh.write("### Konfiguration am Ende\n\n```json\n%s\n```\n\n" % json.dumps(cfg_end, ensure_ascii=False, indent=1))
        fh.write("### GET_STATE am Ende\n\n```json\n%s\n```\n\n" % json.dumps(state, ensure_ascii=False))
        fh.write("### /log am Ende\n\n```\n%s\n```\n\n" % "\n".join(log_end))
        for ttl, text in c.raw_out:
            if errors_only and not any(ttl.startswith(i + " ") for i in bad_ids):
                continue
            fh.write("### %s\n\n```\n%s\n```\n\n" % (ttl, text))


def write_reports(c, args, results, block_time, blocked, pruefer, start, boot, cfg_start, cfg_end, log_end, state):
    n, total = counts(results)
    out_dir = args.out_dir or ("docs/reviews" if os.path.isdir("docs/reviews") else ".")
    os.makedirs(out_dir, exist_ok=True)
    suffix = "%s%s.md" % (start.strftime("%Y-%m-%d_%H%M"), "_" + args.tag if args.tag else "")
    out = os.path.join(out_dir, "Abnahme_A1_" + suffix)
    common = (c, args, results, block_time, blocked, pruefer, start, boot, cfg_start, cfg_end, log_end, state)
    write_report(out, "Abnahme A1 – Prüfprotokoll", *common, errors_only=False)
    say("\n%s\nProtokoll: %s" % (total, out))
    for part in (0, 1, 2):
        if part in block_time:
            t0, t1 = block_time[part]
            say("  %s: %s · %s" % (PART_HEAD[part], counts([r for r in results if r[1] == part])[1], mmss((t1 or t0) - t0)))
    if n["FAIL"] or n["PROBLEM"]:
        err = os.path.join(out_dir, "Fehlerbericht_" + suffix)
        write_report(err, "Abnahme A1 – Fehlerbericht", *common, errors_only=True)
        say("Fehlerbericht: %s" % err)
    if blocked:
        say("SICHERHEITSPRÜFUNG NICHT BESTANDEN (%s) – weitere Prüfungen gesperrt." % blocked)
    sys.exit(1 if n["FAIL"] or n["PROBLEM"] or blocked else 0)


if __name__ == "__main__":
    main()
