#!/usr/bin/env python3
"""
stress_api.py - acceptance and stress tests for the REST API (Contract V1.1).

  python scripts/stress_api.py --host 192.168.4.1 latency  [--n 10]
  python scripts/stress_api.py --host 192.168.4.1 flood    [--n 200]
  python scripts/stress_api.py --host 192.168.4.1 soak     [--minutes 10]
  python scripts/stress_api.py --host 192.168.4.1 local

SAFETY: "latency" MOVES THE AXIS (START_TRAVERSE / STOP). Run it only with the
user's explicit approval, E-stop in reach, axis free. The other tests do not
start motion. Every test aborts if the device reports a fault.

Acceptance criteria (Pflichtenheft V1.5 §7):
  latency : STOP reply < 150 ms (HTTP round trip incl. WLAN), axis idle in /proc < 1 s
  flood   : every response CRC-valid, no response other than OK/BUSY,
            afterwards cycle_max_us < 500 and overruns == 0
  soak    : 10 min GET_STATE every 1 s: cycle_max_us < 500, overruns == 0,
            no COMM_TIMEOUT, all frames valid
  local   : (manual) press any display button, then API HV_ON -> err -10
            LOCAL_CONTROL, API STOP -> OK; after 30 s idle API INIT -> no -10

Only the Python standard library is used (plus scripts/add_client.py).
"""
import argparse
import json
import statistics
import sys
import time
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from add_client import build, http, verify  # noqa: E402

LOCAL_CONTROL = -10
BUSY = -7


def api(host, cmd, params=None):
    t0 = time.perf_counter()
    txt = http(host, "/api", build(cmd, params or {}))
    dt = (time.perf_counter() - t0) * 1000.0
    if not verify(txt):
        raise SystemExit(f"FAIL: invalid CRC frame in reply to {cmd}: {txt}")
    return json.loads(txt), dt


def proc(host):
    txt = http(host, "/proc")
    if not verify(txt):
        raise SystemExit(f"FAIL: invalid CRC frame in /proc: {txt}")
    return json.loads(txt)


def fail_on_fault(p):
    if p["state"] in ("FAULT", "WAIT_ACK"):
        raise SystemExit(f"ABORT: device in {p['state']}, err={p['err']}")


def cycle_ok(host):
    r, _ = api(host, "GET_STATE")
    mx, ov = r.get("cycle_max_us"), r.get("overruns")
    ok = mx is not None and mx < 500 and ov == 0
    print(f"  cycle_max_us={mx} overruns={ov} -> {'OK' if ok else 'FAIL'}")
    return ok


def t_latency(host, n):
    p = proc(host)
    fail_on_fault(p)
    if p["state"] != "RUNNING":
        raise SystemExit("precondition: state RUNNING (INIT, set flags, HV_ON) - not done automatically")
    replies, idles = [], []
    for i in range(n):
        r, _ = api(host, "START_TRAVERSE", {"start": 20, "end": 220, "speed": 3000})
        if r["err"] != 0:
            raise SystemExit(f"START_TRAVERSE failed: {r}")
        time.sleep(0.8 + 0.37 * (i % 3))      # stop at different phases
        r, dt = api(host, "STOP")
        t0 = time.perf_counter()
        if r["err"] != 0:
            raise SystemExit(f"STOP rejected: {r}")
        while True:
            p = proc(host)
            fail_on_fault(p)
            if p["motion"]["state"] == "idle":
                break
            if time.perf_counter() - t0 > 3.0:
                raise SystemExit("FAIL: axis not idle 3 s after STOP")
        idle = (time.perf_counter() - t0) * 1000.0 + dt
        replies.append(dt)
        idles.append(idle)
        print(f"  #{i + 1}: STOP reply {dt:6.1f} ms, idle in /proc after {idle:6.1f} ms")
        time.sleep(0.5)
    ok = max(replies) < 150 and max(idles) < 1000
    print(f"reply  min/median/max: {min(replies):.1f} / {statistics.median(replies):.1f} / {max(replies):.1f} ms")
    print(f"idle   min/median/max: {min(idles):.1f} / {statistics.median(idles):.1f} / {max(idles):.1f} ms")
    return ok and cycle_ok(host)


def t_flood(host, n):
    fail_on_fault(proc(host))
    counts = {}
    worst = 0.0
    for i in range(n):
        r, dt = api(host, "SET", {"key": "temp", "value": 18.0})
        counts[r["err"]] = counts.get(r["err"], 0) + 1
        worst = max(worst, dt)
    print(f"  results by err: {counts}, worst round trip {worst:.1f} ms")
    ok = set(counts) <= {0, BUSY}
    return ok and cycle_ok(host)


def t_soak(host, minutes):
    end = time.time() + minutes * 60
    worst = 0
    k = 0
    while time.time() < end:
        r, _ = api(host, "GET_STATE")
        k += 1
        worst = max(worst, r.get("cycle_max_us", 0))
        if r.get("sys_err") == 3:
            raise SystemExit("FAIL: COMM_TIMEOUT during soak")
        if r.get("overruns", 0) != 0:
            raise SystemExit(f"FAIL: overruns={r['overruns']} after {k} s")
        time.sleep(1.0)
    print(f"  {k} heartbeats, cycle_max_us={worst}")
    return cycle_ok(host)


def t_local(host):
    input("Press any button on the display (e.g. STOP), then press Enter here ... ")
    r, _ = api(host, "HV_ON")
    a = r["err"] == LOCAL_CONTROL
    print(f"  API HV_ON  -> err {r['err']} (expected {LOCAL_CONTROL}) {'OK' if a else 'FAIL'}")
    r, _ = api(host, "STOP")
    b = r["err"] == 0
    print(f"  API STOP   -> err {r['err']} (expected 0) {'OK' if b else 'FAIL'}")
    p = proc(host)
    c = p.get("local") is True
    print(f"  /proc local = {p.get('local')} {'OK' if c else 'FAIL'}")
    print("  waiting 32 s (do not touch the display) ...")
    time.sleep(32)
    p = proc(host)
    d = p.get("local") is False
    print(f"  /proc local = {p.get('local')} after hold time {'OK' if d else 'FAIL (HV on or motion running?)'}")
    return a and b and c and d


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="192.168.4.1")
    ap.add_argument("test", choices=["latency", "flood", "soak", "local"])
    ap.add_argument("--n", type=int, default=0)
    ap.add_argument("--minutes", type=float, default=10)
    a = ap.parse_args()
    if a.test == "latency":
        ok = t_latency(a.host, a.n or 10)
    elif a.test == "flood":
        ok = t_flood(a.host, a.n or 200)
    elif a.test == "soak":
        ok = t_soak(a.host, a.minutes)
    else:
        ok = t_local(a.host)
    print("RESULT:", "PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
