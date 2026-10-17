#!/usr/bin/env python3
"""
serial_cmd.py - send one command to the smoketest (or firmware console) and
print the reply lines. Non-interactive: exits after the final JSON line
(containing "done", "error" or any single JSON line for simple commands) or
after --timeout seconds. Safe to use from Cline / scripts.

  python scripts/serial_cmd.py "ver"
  python scripts/serial_cmd.py "adc 20" --timeout 10
  python scripts/serial_cmd.py --port /dev/ttyUSB0 "in"

Firmware console (V1.5.23): help, ver, status, reboot, wifi <ssid> <password>, wifi off
  python scripts/serial_cmd.py "status"
Opening the port resets the board on most USB-serial adapters (DTR/RTS). The script therefore
waits for "boot: ready" (or for 1 s of silence if the board did not reset) before it sends.

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
    ap.add_argument("--boot-wait", type=float, default=10.0, help="max. seconds to wait for 'boot: ready' after a reset")
    a = ap.parse_args()

    port = a.port or guess_port()
    # dsrdtr/rtscts off and DTR/RTS low: do not reset the ESP32 on open
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, a.baud, 0.1
    s.dtr = False
    s.rts = False
    s.open()
    # V1.5.23: wait for the end of a boot caused by opening the port
    t0 = time.time()
    last_rx = t0
    booting = False
    while time.time() - t0 < a.boot_wait:
        raw = s.readline()
        if raw:
            last_rx = time.time()
            booting = True
            txt = raw.decode(errors="replace").rstrip()
            if "boot: ready" in txt or "SMOKETEST" in txt.upper():
                time.sleep(0.2)
                break
            continue
        if not booting and time.time() - last_rx > 1.0:
            break                                   # no reset happened: the board is idle
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
