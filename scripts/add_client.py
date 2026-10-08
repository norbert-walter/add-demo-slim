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

Only the Python standard library is used.
"""
import argparse
import json
import sys
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
    c = sub.add_parser("cmd")
    c.add_argument("name")
    c.add_argument("params", nargs="*", help="key=value pairs")
    a = ap.parse_args()

    if a.op == "selftest":
        selftest()
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
