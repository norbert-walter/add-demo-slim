#!/usr/bin/env python3
"""
serial_cmd.py - send one command to the smoketest (or firmware console) and
print the reply lines. Non-interactive: exits after the final JSON line
(containing "done", "error" or any single JSON line for simple commands) or
after --timeout seconds. Safe to use from Cline / scripts.

  python scripts/serial_cmd.py "ver"
  python scripts/serial_cmd.py "adc 20" --timeout 10
  python scripts/serial_cmd.py --port /dev/ttyUSB0 "in"

Requires: pip install pyserial
"""
import argparse
import glob
import json
import sys
import time

import serial  # pyserial

MULTI_LINE = ("watch", "adc", "scan", "move", "home", "touch", "part", "help", "beep", "stop")


def guess_port():
    ports = sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*") + glob.glob("/dev/cu.usbserial*"))
    if not ports:
        sys.exit("no serial port found - use --port")
    return ports[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("command", help='e.g. "ver" or "move 10 300"')
    ap.add_argument("--port", default=None)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=5.0, help="seconds without final line")
    a = ap.parse_args()

    port = a.port or guess_port()
    # dsrdtr/rtscts off and DTR/RTS low: do not reset the ESP32 on open
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, a.baud, 0.1
    s.dtr = False
    s.rts = False
    s.open()
    time.sleep(0.1)
    s.reset_input_buffer()
    s.write((a.command.strip() + "\n").encode())
    s.flush()

    multi = a.command.split()[0] in MULTI_LINE
    deadline = time.time() + a.timeout
    rc = 1
    while time.time() < deadline:
        raw = s.readline()
        if not raw:
            continue
        txt = raw.decode(errors="replace").rstrip()
        print(txt)
        deadline = time.time() + a.timeout          # progress extends the timeout
        if not txt.startswith("{"):
            continue
        try:
            obj = json.loads(txt)
        except ValueError:
            continue
        if "error" in obj:
            rc = 2
            break
        if not multi or obj.get("done"):
            rc = 0
            break
    else:
        print('{"error":"timeout waiting for reply"}')
    s.close()
    sys.exit(rc)


if __name__ == "__main__":
    main()
