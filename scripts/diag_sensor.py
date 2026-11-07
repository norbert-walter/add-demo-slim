#!/usr/bin/env python3
"""diag_sensor.py – does the distance reading depend on the operating state? (after K1, 08.11.2026)

The A1 run after K1 showed: the raw values at calibration were about 80 mV (sensor voltage) above the
smoketest values, and during the stationary measurement (HV on, shutter open) another 25 ... 58 mV.
This script stands on ONE step and measures the distance (/proc measure.dist, 40 samples each, 50 ms
apart) in this order:

    A  HV off, axis idle                      (condition of the calibration)
    B  HV on, shutter closed
    C  measuring: START_MEASURE (shutter open, as in T18)
    D  STOP, HV off again
    E  like A after a pause (default 60 s)    (drift over time?)

Prints mean and sigma per condition and the difference to A. Moves the axis to the step and
switches HV and shutter (simulated) – Not-Aus within reach.

    python3 scripts/diag_sensor.py --host 192.168.1.120 [--pos 115] [--pause 60]
"""
import argparse, json, statistics, sys, time, urllib.request, zlib


def frame(obj):
    b = json.dumps(obj, separators=(",", ":"))
    return b[:-1] + ',"crc":"%08X"}' % (zlib.crc32(b.encode()) & 0xFFFFFFFF)


class Dev:
    def __init__(self, host):
        self.base = "http://" + host

    def api(self, cmd, **params):
        req = urllib.request.Request(self.base + "/api", data=frame({"cmd": cmd, "params": params}).encode(),
                                     method="POST", headers={"Content-Type": "text/plain"})
        r = json.loads(urllib.request.urlopen(req, timeout=4).read())
        if r.get("err") != 0:
            sys.exit("%s %s -> err %s" % (cmd, params, r.get("err")))
        return r

    def proc(self):
        return json.loads(urllib.request.urlopen(self.base + "/proc?t=%d" % time.time(), timeout=3).read())

    def wait(self, cond, timeout, what):
        t0 = time.time()
        while time.time() - t0 < timeout:
            p = self.proc()
            if cond(p):
                return p
            self.api("GET_STATE")                       # heartbeat while armed
            time.sleep(0.2)
        sys.exit("timeout: " + what)

    def sample(self, n=40):
        v = []
        while len(v) < n:
            p = self.proc()
            if p["measure"]["in_range"]:
                v.append(p["measure"]["dist"])
            if len(v) % 10 == 0:
                self.api("GET_STATE")
            time.sleep(0.05)
        return v


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--host", required=True)
    ap.add_argument("--pos", type=float, default=115.0, help="step centre (115 ... 215), default 115")
    ap.add_argument("--pause", type=float, default=60.0, help="s before condition E, default 60")
    a = ap.parse_args()
    d = Dev(a.host)
    p = d.proc()
    if p["state"] not in ("READY", "RUNNING") or not p["motion"]["homed"]:
        sys.exit("first INIT (state %s)" % p["state"])
    if p["safety"]["hv_on"]:
        d.api("HV_OFF")
    d.api("MOVE", pos=a.pos)
    d.wait(lambda q: q["motion"]["state"] == "idle" and abs(q["motion"]["pos"] - a.pos) < 0.5, 60, "MOVE")
    res = {}

    def rest(sec):                                       # wait with heartbeat (watchdog 5 s while armed)
        t0 = time.time()
        while time.time() - t0 < sec:
            d.api("GET_STATE")
            time.sleep(min(1.0, max(0.0, sec - (time.time() - t0))))

    def cond(name, text, settle=3.0):
        rest(settle)
        v = d.sample()
        res[name] = v
        print("%s  %-36s mean %7.2f mm  sigma %.2f" % (name, text, statistics.mean(v), statistics.pstdev(v)), flush=True)

    cond("A", "HV aus, Achse steht")
    d.api("HV_ON")
    d.wait(lambda q: q["safety"]["hv_on"], 3, "HV_ON")
    cond("B", "HV an, Shutter zu")
    d.api("START_MEASURE", pos=a.pos)
    d.wait(lambda q: q["safety"]["shutter"] == "open", 10, "Shutter offen")
    cond("C", "Messen (HV an, Shutter offen)")
    d.api("STOP")
    d.api("HV_OFF")
    cond("D", "STOP, HV aus")
    print("Pause %.0f s ..." % a.pause, flush=True)
    rest(a.pause)
    cond("E", "wie A nach der Pause", settle=0.5)
    ma = statistics.mean(res["A"])
    print("Differenz zu A:  " + "  ".join("%s %+.2f mm" % (k, statistics.mean(v) - ma) for k, v in res.items() if k != "A"))


if __name__ == "__main__":
    main()
