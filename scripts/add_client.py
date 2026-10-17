#!/usr/bin/env python3
"""
add_client.py - reference client for the ADD Demo System REST API (CONTRACT §4/§5).

Implements the CRC frame rule exactly like the firmware:
  body without crc -> CRC32 (zlib) -> insert ,"crc":"XXXXXXXX" before final '}'
and verifies every response and /proc frame.

  python scripts/add_client.py --host 192.168.4.1 proc
  python scripts/add_client.py --host 192.168.4.1 cmd HV_ON
  python scripts/add_client.py --host 192.168.4.1 cmd SET key=temp value=35.0
  python scripts/add_client.py selftest            # offline check of the frame rule
  python scripts/add_client.py --host 192.168.4.1 shell    # interactive console (V1.5.22)

shell: type API commands with key=value parameters, e.g.
  SET key=interlock value=true      INIT      HV_ON      START_TRAVERSE start=100 end=200 speed=5
plus: proc [full] | watch [seconds] | hb on|off | log | get <path> | help | quit
After an accepted INIT, MOVE, PARK, SERVICE, START_*, or HV_ON the shell sends GET_STATE every second
(heartbeat) until /proc reports comm_armed=false, so the 5 s watchdog does not fire while you type.
"hb off" stops it (to test the watchdog).

Only the Python standard library is used.
"""
import argparse
import json
import sys
import threading
import time
import urllib.request
import zlib

TAG = ',"crc":"'


def frame(body: str) -> str:
    """Append the CRC frame to a compact JSON object text."""
    if not (body.startswith("{") and body.endswith("}")) or body == "{}":
        raise ValueError("body must be a non-empty JSON object")
    crc = zlib.crc32(body.encode()) & 0xFFFFFFFF
    return body[:-1] + TAG + f"{crc:08X}" + '"}'


def verify(text: str) -> bool:
    """Check a framed JSON text (trailing whitespace allowed)."""
    t = text.rstrip(" \t\r\n")
    i = t.rfind(TAG)
    if i < 0 or len(t) != i + len(TAG) + 10:
        return False
    hexpart = t[i + len(TAG): i + len(TAG) + 8]
    if t[-2:] != '"}':
        return False
    try:
        expected = int(hexpart, 16)
    except ValueError:
        return False
    return (zlib.crc32((t[:i] + "}").encode()) & 0xFFFFFFFF) == expected


def build(cmd: str, params: dict) -> str:
    return frame(json.dumps({"cmd": cmd, "params": params}, separators=(",", ":")))


def http(host: str, path: str, data: str = None, timeout: float = 3.0) -> str:
    req = urllib.request.Request(f"http://{host}{path}", data=data.encode() if data else None,
                                 headers={"Content-Type": "application/json"} if data else {})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read().decode()


def parse_value(v: str):
    if v in ("true", "false"):
        return v == "true"
    try:
        return float(v) if any(c in v for c in ".eE") else int(v)
    except ValueError:
        return v


ERR = {0: "OK", -1: "BAD_CRC", -2: "BAD_JSON", -3: "UNKNOWN_CMD", -4: "BAD_PARAM", -5: "STATE",
       -6: "PRECONDITION", -7: "BUSY", -8: "NOT_STABLE", -9: "CALIB_INVALID", -10: "LOCAL_CONTROL"}
SYS_ERR = ["NONE", "ESTOP", "TEMP_HIGH", "COMM_TIMEOUT", "MOTOR_FAULT", "HOMING_FAILED"]


def summary(p: dict) -> str:
    """One line of a /proc object."""
    m, q, s = p["motion"], p["measure"], p["safety"]
    err = SYS_ERR[p["err"]] if 0 <= p["err"] < len(SYS_ERR) else p["err"]
    return (f'{p["state"]:<10} err={err} local={int(p["local"])} | pos={m["pos"]:6.1f} vel={m["vel"]:4.1f} '
            f'{m["state"]}/{m["mode"]} homed={int(m["homed"])} | dist={q["dist"]:5.1f} thick={q["thick"]:5.1f} '
            f'valid={int(q["valid"])} avg={q["avg"]:5.1f} n={q["n"]} | hv={int(s["hv_on"])} hv_ok={int(s["hv_ok"])} '
            f'shutter={s["shutter"]} estop={int(s["estop"])} il={int(s["interlock"])} cf={int(s["cooling_flow"])} '
            f'temp={s["temp"]:.1f} armed={int(s["comm_armed"])}')


class Shell:
    HELP = """API commands (CONTRACT §5.2), parameters as key=value:
  INIT  HV_ON  HV_OFF  STOP  ACK  PARK  SERVICE  SHUTTER_OPEN  SHUTTER_CLOSE  REBOOT
  START_MEASURE [pos=155]   START_TRAVERSE [start=80 end=240 speed=10]   MOVE pos=100 [speed=10]
  SET key=<name> value=<v>  (interlock, cooling_flow, temp, measure_pos, maint_pos, traverse_*,
      shutter_hold, edge_threshold, base_dist, calib_near_ref, calib_far_ref, estop_nc, wifi_ssid, wifi_pass)
  CALIB action=near|far|base|save|reset     GET_STATE     GET_CONFIG
shell: proc [full] | watch [seconds, default 30, Ctrl+C stops] | hb on|off | log | get <path> | help | quit
Heartbeat: after an accepted INIT, MOVE, PARK, SERVICE, START_MEASURE, START_TRAVERSE or HV_ON the
shell sends GET_STATE every second until /proc reports comm_armed=false (axis idle and HV off)."""
    ARMING = {"INIT", "MOVE", "PARK", "SERVICE", "START_MEASURE", "START_TRAVERSE", "HV_ON"}
    PERIOD = 1.0      # s between two heartbeats
    WARN = 2.5        # s without an answer -> warning (the device fires at 5 s)

    def __init__(self, host):
        self.host = host
        self.enabled = True          # "hb on|off"
        self.active = False          # heartbeat running (watchdog armed)
        self.last_tx = 0.0           # last /api request sent (any command counts for the device)
        self.last_ok = time.time()   # last /api answer received
        self.io = threading.Lock()   # one /api request at a time
        self.run = True

    def say(self, text):
        print("\n" + text, flush=True)

    def api(self, cmd, params, timeout=3.0):
        with self.io:
            self.last_tx = time.time()
            r = http(self.host, "/api", build(cmd, params), timeout=timeout)
            self.last_ok = time.time()
            return r

    def armed(self):
        t = http(self.host, "/proc", timeout=2.0)
        return verify(t) and json.loads(t)["safety"]["comm_armed"]

    def start(self, why):
        if not self.active:
            self.active = True
            self.last_ok = time.time()
            self.say(f"[watchdog armed ({why}) - heartbeat every {self.PERIOD:.0f} s]")

    def heartbeat(self):
        while self.run:
            time.sleep(0.1)
            if not (self.enabled and self.active) or time.time() - self.last_tx < self.PERIOD:
                continue
            try:
                self.api("GET_STATE", {}, timeout=1.5)
            except Exception:  # noqa: BLE001 - retry at once, then warn
                try:
                    self.api("GET_STATE", {}, timeout=1.5)
                except Exception as e:  # noqa: BLE001
                    age = time.time() - self.last_ok
                    if age > self.WARN:
                        self.say(f"!! no answer for {age:.1f} s - the watchdog fires at 5 s ({e})")
                    continue
            try:
                if not self.armed():
                    self.active = False
                    self.say("[watchdog off - heartbeat stopped]")
            except Exception:  # noqa: BLE001 - /proc missed once: keep pinging
                pass

    def line(self, ln):
        w = ln.split()
        if not w:
            return
        op = w[0]
        if op in ("quit", "exit"):
            raise EOFError
        if op == "help":
            print(self.HELP)
        elif op == "proc":
            t = http(self.host, "/proc")
            ok = verify(t)
            print(t.strip() if len(w) > 1 else summary(json.loads(t)), "" if ok else "  <-- CRC INVALID")
        elif op == "watch":
            end = time.time() + (float(w[1]) if len(w) > 1 else 30.0)
            try:
                while time.time() < end:
                    t = http(self.host, "/proc")
                    print(time.strftime("%H:%M:%S"), summary(json.loads(t)), "" if verify(t) else "CRC!", flush=True)
                    time.sleep(0.5)
            except KeyboardInterrupt:
                print()
        elif op == "hb":
            self.enabled = len(w) < 2 or w[1] != "off"
            print("heartbeat", "on" if self.enabled else "OFF (the watchdog will fire while armed)")
        elif op == "log":
            print(http(self.host, "/log"))
        elif op == "get":
            print(http(self.host, w[1] if len(w) > 1 else "/"))
        else:
            params = {}
            for kv in w[1:]:
                k, _, v = kv.partition("=")
                params[k] = parse_value(v)
            cmd = op.upper()
            t = time.time()
            r = self.api(cmd, params)
            ms = (time.time() - t) * 1000
            ok = verify(r)
            e = json.loads(r).get("err") if ok else None
            print(r.strip(), f"  [{ERR.get(e, e)}, {ms:.0f} ms]" if ok else "  <-- CRC INVALID")
            if e == 0 and cmd in self.ARMING:
                self.start(cmd)

    def loop(self):
        try:
            import readline  # noqa: F401 - history and line editing where available
        except ImportError:
            pass
        print(f"ADD console for http://{self.host}  (help, quit)")
        try:
            if self.armed():
                self.start("already armed")
        except Exception as e:  # noqa: BLE001
            print("device not reachable:", e)
        threading.Thread(target=self.heartbeat, daemon=True).start()
        while True:
            try:
                ln = input("add> ")
            except (EOFError, KeyboardInterrupt):
                print()
                break
            try:
                self.line(ln)
            except EOFError:
                break
            except Exception as e:  # noqa: BLE001 - show and continue
                print("error:", e)
        self.run = False


def selftest():
    assert f"{zlib.crc32(b'123456789'):08X}" == "CBF43926"
    f = frame('{"cmd":"HV_ON","params":{}}')
    assert f == '{"cmd":"HV_ON","params":{},"crc":"88F7C802"}', f
    assert verify(f) and verify(f + "\r\n")
    assert not verify(f.replace("HV_ON", "HV_OF"))
    assert build("SET", {"key": "temp", "value": 35.0}) == \
        '{"cmd":"SET","params":{"key":"temp","value":35.0},"crc":"C93F2D33"}'
    print('{"selftest":"ok"}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="192.168.4.1")
    sub = ap.add_subparsers(dest="op", required=True)
    sub.add_parser("proc")
    sub.add_parser("selftest")
    sub.add_parser("shell")
    c = sub.add_parser("cmd")
    c.add_argument("name")
    c.add_argument("params", nargs="*", help="key=value pairs")
    a = ap.parse_args()

    if a.op == "selftest":
        selftest()
        return
    if a.op == "shell":
        Shell(a.host).loop()
        return
    if a.op == "proc":
        txt = http(a.host, "/proc")
        print(txt)
        sys.exit(0 if verify(txt) else 3)
    params = {}
    for kv in a.params:
        k, _, v = kv.partition("=")
        params[k] = parse_value(v)
    txt = http(a.host, "/api", build(a.name, params))
    print(txt)
    if not verify(txt):
        print('{"error":"response CRC invalid"}')
        sys.exit(3)
    sys.exit(0 if json.loads(txt).get("err") == 0 else 1)


if __name__ == "__main__":
    main()
