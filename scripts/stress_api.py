#!/usr/bin/env python3
"""
stress_api.py - acceptance and stress tests for the REST API (Contract V1.1).

  python scripts/stress_api.py --host 192.168.4.1 latency  [--n 10] [--prepare]
  python scripts/stress_api.py --host 192.168.4.1 flood    [--n 200]
  python scripts/stress_api.py --host 192.168.4.1 parallel [--n 400] [--threads 4]
  python scripts/stress_api.py --host 192.168.4.1 soak     [--minutes 10]
  python scripts/stress_api.py --host 192.168.4.1 local

SAFETY: "latency" MOVES THE AXIS (START_TRAVERSE / STOP; with --prepare also
INIT = homing and HV_ON). Run it only with the user's explicit approval, E-stop
in reach, axis free. The other tests do not start motion. Every test aborts if
the device reports a fault.

V1.5.25 (device test I1 part 2c): the device runs the Arduino WebServer, which
serves ONE connection at a time with a listen backlog of 4. "parallel" therefore
uses 4 threads and a 10 s timeout per request (a refused connection is retried
by the OS after 1 s and 3 s). The control cycle is 1 ms (CONTROL_PERIOD_MS);
the cycle statistics count from "boot: ready" on, and every test judges the
overruns that happened DURING the test (before/after), not since boot.

Acceptance criteria (Pflichtenheft V1.5 §7):
  latency : STOP reply < 150 ms (HTTP round trip incl. WLAN), axis idle in /proc < 1 s
  flood   : every response CRC-valid, no response other than OK/BUSY,
            afterwards timing ok (see below)
  parallel: 8 threads send mixed commands with a KNOWN expected result at the
            same time; every reply must be CRC-valid, carry the right "cmd" and
            either the expected err or BUSY (-7) -> no reply is ever mixed up
  timing  : (GET_STATE after every test) cycle_max_us < 800 (1 ms cycle),
            no new overruns during the test, period_max_us < 1500 (jitter),
            prio_lat_max_us <= 2000
            (priority command: enqueue -> outputs applied, measured in the device)
  soak    : 10 min GET_STATE every 1 s: no new overruns, no COMM_TIMEOUT,
            all frames valid, then timing as above
  local   : only with LOCAL_HOLD_MS > 0 in config.h (default 0 = no LOCAL
            priority): press any display button, then API HV_ON -> err -10
            LOCAL_CONTROL, API STOP -> OK; after the hold time API INIT -> no -10

Only the Python standard library is used (plus scripts/add_client.py).
"""
import argparse
import concurrent.futures
import json
import random
import statistics
import sys
import time
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from add_client import build, http, verify  # noqa: E402

LOCAL_CONTROL = -10
BUSY = -7


def api(host, cmd, params=None, timeout=3.0):
    t0 = time.perf_counter()
    txt = http(host, "/api", build(cmd, params or {}), timeout=timeout)
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


LIMITS = {"cycle_max_us": 800, "period_max_us": 1500, "prio_lat_max_us": 2001}


def overruns(host):
    r, _ = api(host, "GET_STATE")
    return r.get("overruns", 0)


def cycle_ok(host, base):
    """Device-side timing (CycleStats, CONTRACT §10). base = overruns before the test.
    prio_lat_max_us stays 0 until the first priority command (STOP, HV_OFF,
    SHUTTER_CLOSE) was executed."""
    r, _ = api(host, "GET_STATE")
    new = r.get("overruns", 0) - base
    ok = new == 0
    parts = [f"overruns +{new} (since boot ready {r.get('overruns')})"]
    for k, lim in LIMITS.items():
        v = r.get(k)
        good = v is not None and v < lim
        ok = ok and good
        parts.append(f"{k}={v}{'' if good else ' (!)'}")
    print("  " + " ".join(parts) + f" -> {'OK' if ok else 'FAIL'}")
    return ok


def wait_idle(host, what, limit=60.0):
    """Poll with GET_STATE (keeps the watchdog fed) until the axis is idle in /proc."""
    t0 = time.time()
    while True:
        r, _ = api(host, "GET_STATE")
        if r.get("state") in ("FAULT", "WAIT_ACK"):
            raise SystemExit(f"ABORT: {r.get('state')} during {what}, sys_err={r.get('sys_err')}")
        p = proc(host)
        if p["motion"]["state"] == "idle" and r.get("state") != "SAFE_START":
            return p
        if time.time() - t0 > limit:
            raise SystemExit(f"FAIL: axis not idle {limit:.0f} s after {what}")
        time.sleep(0.3)


def prepare(host):
    """--prepare: INIT (homing) if not homed, wait until the axis is idle, then HV_ON."""
    p = proc(host)
    fail_on_fault(p)
    if not p["motion"]["homed"] or p["state"] == "SAFE_START":
        print("  prepare: INIT (homing) ...")
        r, _ = api(host, "INIT")
        if r["err"] != 0:
            raise SystemExit(f"INIT failed: {r}")
    p = wait_idle(host, "INIT")
    if not p["safety"]["hv_on"]:
        r, _ = api(host, "HV_ON")
        if r["err"] != 0:
            raise SystemExit(f"HV_ON failed: {r} (interlock, cooling_flow, temp?)")
    print("  prepare: homed, axis idle, HV on")


def t_latency(host, n, do_prepare):
    try:
        return latency_run(host, n, do_prepare)
    finally:
        if do_prepare:
            try:
                api(host, "STOP")
                api(host, "HV_OFF")           # leave the device safe: the watchdog disarms (axis idle, HV off)
                print("  STOP, HV_OFF sent")
            except Exception as e:  # noqa: BLE001
                print("  WARNING: STOP/HV_OFF at the end failed:", e)


def latency_run(host, n, do_prepare):
    if do_prepare:
        prepare(host)
    p = proc(host)
    fail_on_fault(p)
    if p["state"] != "RUNNING":
        raise SystemExit("precondition: state RUNNING (INIT, HV_ON) - or start with --prepare")
    # V1.5.25: traverse inside the measuring zone (> 60 ... 242 mm, CONTRACT §2), at most 20 mm/s
    cfg, _ = api(host, "GET_CONFIG")
    c = cfg["config"]
    trav = {"start": c["traverse_start"], "end": c["traverse_end"], "speed": min(20.0, max(10.0, c["traverse_speed"]))}
    print(f"  traverse {trav['start']} -> {trav['end']} mm at {trav['speed']} mm/s")
    base = overruns(host)
    replies, idles = [], []
    for i in range(n):
        wait_idle(host, "STOP" if i else "start")
        r, _ = api(host, "START_TRAVERSE", trav)
        if r["err"] != 0:
            raise SystemExit(f"START_TRAVERSE failed: {r}")
        t_end = time.time() + 0.8 + 0.37 * (i % 3)      # stop at different phases, heartbeat meanwhile
        while time.time() < t_end:
            api(host, "GET_STATE")
            time.sleep(0.2)
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
    ok = max(replies) < 150 and max(idles) < 1000
    print(f"reply  min/median/max: {min(replies):.1f} / {statistics.median(replies):.1f} / {max(replies):.1f} ms")
    print(f"idle   min/median/max: {min(idles):.1f} / {statistics.median(idles):.1f} / {max(idles):.1f} ms")
    print("  (HTTP round trip incl. WLAN; the device-internal STOP latency is prio_lat_max_us below)")
    return cycle_ok(host, base) and ok


def t_flood(host, n):
    fail_on_fault(proc(host))
    base = overruns(host)
    counts = {}
    worst = 0.0
    for i in range(n):
        r, dt = api(host, "SET", {"key": "temp", "value": 18.0})
        counts[r["err"]] = counts.get(r["err"], 0) + 1
        worst = max(worst, dt)
    print(f"  results by err: {counts}, worst round trip {worst:.1f} ms")
    ok = set(counts) <= {0, BUSY}
    return cycle_ok(host, base) and ok


def t_parallel(host, n, threads):
    p = proc(host)
    fail_on_fault(p)
    if p["state"] not in ("SAFE_START", "READY") or p.get("local"):
        raise SystemExit("precondition: state SAFE_START or READY, HV off, local false")
    base = overruns(host)
    # (cmd, params, expected err) - all without motion, deterministic in SAFE_START/READY
    cases = [
        ("SET", {"key": "temp", "value": 18.0}, 0),
        ("SET", {"key": "temp", "value": 500.0}, -4),   # out of range
        ("SHUTTER_OPEN", {}, -5),                       # only in RUNNING
        ("ACK", {}, -5),                                # only in WAIT_ACK
        ("SHUTTER_CLOSE", {}, 0),                       # priority queue
        ("HV_OFF", {}, 0),                              # priority queue
        ("GET_STATE", {}, 0),                           # answered by the HTTP task
    ]
    jobs = [random.choice(cases) for _ in range(n)]

    def one(job):
        cmd, params, exp = job
        r, dt = api(host, cmd, params, timeout=10.0)
        return cmd, exp, r, dt

    bad, busy, worst = [], 0, 0.0
    with concurrent.futures.ThreadPoolExecutor(max_workers=threads) as ex:
        for cmd, exp, r, dt in ex.map(one, jobs):
            worst = max(worst, dt)
            if r.get("err") == BUSY:
                busy += 1
            elif r.get("cmd") != cmd or r.get("err") != exp:
                bad.append((cmd, exp, r))
    print(f"  {n} requests, {threads} threads: {len(bad)} mismatches, {busy} BUSY, worst {worst:.1f} ms")
    for b in bad[:10]:
        print("   mismatch:", b)
    return cycle_ok(host, base) and not bad


def t_soak(host, minutes):
    base = overruns(host)
    end = time.time() + minutes * 60
    worst = 0
    k = 0
    while time.time() < end:
        r, _ = api(host, "GET_STATE")
        k += 1
        worst = max(worst, r.get("cycle_max_us", 0))
        if r.get("sys_err") == 3:
            raise SystemExit("FAIL: COMM_TIMEOUT during soak")
        if r.get("overruns", 0) != base:
            raise SystemExit(f"FAIL: overruns +{r['overruns'] - base} after {k} s")
        if r.get("period_max_us", 0) >= LIMITS["period_max_us"]:
            print(f"  WARNING: period_max_us={r['period_max_us']} after {k} s")
        time.sleep(1.0)
    print(f"  {k} heartbeats, cycle_max_us={worst}")
    return cycle_ok(host, base)


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
    ap.add_argument("test", choices=["latency", "flood", "parallel", "soak", "local"])
    ap.add_argument("--threads", type=int, default=4)
    ap.add_argument("--prepare", action="store_true", help="latency: INIT, wait for READY, HV_ON first; HV_OFF at the end")
    ap.add_argument("--n", type=int, default=0)
    ap.add_argument("--minutes", type=float, default=10)
    a = ap.parse_args()
    if a.test == "latency":
        ok = t_latency(a.host, a.n or 10, a.prepare)
    elif a.test == "flood":
        ok = t_flood(a.host, a.n or 200)
    elif a.test == "parallel":
        ok = t_parallel(a.host, a.n or 400, a.threads)
    elif a.test == "soak":
        ok = t_soak(a.host, a.minutes)
    else:
        ok = t_local(a.host)
    print("RESULT:", "PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
