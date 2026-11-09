#!/usr/bin/env python3
"""
add_validation_run.py - Validierungslauf der Demoanlage ADD (Slim) nach Software-Doku §13.9

Geschrieben von Claude (claude-opus-5-5) nach den Regeln in /add. Gestartet wird das Skript
vom Bediener; der Start und jede Rückfrage ersetzen die Bestätigung je Aktion (ADD-Regel
"Never start a script ... on your own").

Ablauf (jeder Schritt mit Bewegung oder HV wird vorher einzeln bestätigt):
  0. Selbsttest der CRC-Rahmenregel (Testvektoren aus dem ADD)
  1. /proc lesen und Ausgangszustand prüfen (kein Fehler, kein Bediener am Display, kein Not-Aus)
  2. GET_CONFIG lesen und ausgeben (keine Bestätigung nötig, keine physische Wirkung)
  3. INIT        - Referenzfahrt                         [Bestätigung + Achse frei]
  4. HV_ON       - simulierte Hochspannung ein           [Bestätigung]
  5. START_TRAVERSE start=80 end=240 speed=10, /proc alle 0,2 s aufzeichnen
                                                         [Bestätigung + Achse frei]
  6. STOP, HV_OFF - sichere Richtung, ohne Rückfrage
  7. PARK        - Parkposition 2 mm                     [Bestätigung + Achse frei]
  8. Profil auswerten: Materialkanten und Dicke je Stufe gegen 5 ... 30 mm

Sicherheit:
  - GET_STATE alle 1 s in einem eigenen Thread (Heartbeat, ADD verlangt <= 2 s).
  - Alle HTTP-Anfragen laufen nacheinander über eine Sperre: das Gerät bedient nur eine
    Verbindung zur Zeit (Software-Doku §15); parallele Anfragen führten zu > 20 s Wartezeit.
  - Bei Strg+C, Fehlercode, ungültiger CRC (3x hintereinander), Bediener am Display oder
    unerwartetem Zustand: sofort STOP und HV_OFF, dann Abbruch.
  - Keine Zugangsdaten: nur für den Betrieb im LAN (http://<IP>, Gerät ohne Anmeldung).
  - SET estop_nc / wifi_ssid / wifi_pass werden nie gesendet; überhaupt kein SET oder CALIB.

Ausgabe:
  validation_<Zeit>.jsonl  - jede Anfrage und Antwort mit Zeitstempel und Dauer (für die Auswertung)
  profile_<Zeit>.csv       - Profil-Samples (pos, dist, thick, on_material, valid)

Aufruf:
  python3 add_validation_run.py                 # Gerät unter 192.168.1.120
  python3 add_validation_run.py --host 192.168.4.1
  python3 add_validation_run.py --selftest      # nur CRC-Selbsttest, kein Netz

Nur Python-Standardbibliothek.
"""
import argparse
import csv
import json
import sys
import threading
import time
import urllib.request
import zlib

TAG = ',"crc":"'
ERR = {0: "OK", -1: "BAD_CRC", -2: "BAD_JSON", -3: "UNKNOWN_CMD", -4: "BAD_PARAM", -5: "STATE",
       -6: "PRECONDITION", -7: "BUSY", -8: "NOT_STABLE", -9: "CALIB_INVALID", -10: "LOCAL_CONTROL"}
ERR_TEXT = {-4: "Parameter fehlt, falscher Typ oder außerhalb des Bereichs",
            -5: "Befehl im aktuellen Sicherheitszustand nicht erlaubt",
            -6: "Vorbedingung nicht erfüllt (HV-Freigabe, Zone, nicht referenziert, Fehlerursache besteht)",
            -10: "Ein Bediener am Touchdisplay hat Vorrang"}
SYS_ERR = {0: "NONE", 1: "ESTOP (Not-Aus gedrückt)", 2: "TEMP_HIGH (simulierte Temperatur > 32 °C)",
           3: "COMM_TIMEOUT (> 5 s kein /api-Paket)", 4: "MOTOR_FAULT (Endschalter unerwartet)",
           5: "HOMING_FAILED (Referenzfahrt gescheitert)"}

TRAVERSE = {"start": 80, "end": 240, "speed": 10}
STEP_CENTRES = [115, 135, 155, 175, 195, 215]      # mm, Stufe 1 ... 6
STEP_THICK = [5, 10, 15, 20, 25, 30]               # mm, Sollwerte (base_dist 70 - ref 65 ... 40)
HB_PERIOD = 1.0                                    # s


class Abort(Exception):
    """Ablauf abbrechen; finally sendet STOP und HV_OFF."""


# ---------------------------------------------------------------- CRC-Rahmen (ADD interfaces[0])

def frame(body: str) -> str:
    crc = zlib.crc32(body.encode()) & 0xFFFFFFFF
    return body[:-1] + TAG + f"{crc:08X}" + '"}'


def verify(text: str) -> bool:
    t = text.rstrip(" \t\r\n")
    i = t.rfind(TAG)
    if i < 0 or not t.endswith('"}') or len(t) != i + len(TAG) + 10:
        return False
    try:
        expected = int(t[i + len(TAG): i + len(TAG) + 8], 16)
    except ValueError:
        return False
    return (zlib.crc32((t[:i] + "}").encode()) & 0xFFFFFFFF) == expected


def build(cmd: str, params: dict) -> str:
    return frame(json.dumps({"cmd": cmd, "params": params}, separators=(",", ":")))


def selftest():
    assert build("HV_ON", {}) == '{"cmd":"HV_ON","params":{},"crc":"88F7C802"}'
    assert frame('{"cmd":"SET","params":{"key":"temp","value":35.0}}').endswith('"C93F2D33"}')
    assert frame('{"cmd":"SET","params":{"key":"temp","value":35}}').endswith('"F2B0BAC5"}')
    assert verify(build("STOP", {})) and not verify(build("STOP", {}).replace("STOP", "STOQ"))
    print("CRC-Selbsttest: ok (alle drei Testvektoren aus dem ADD)")


# ---------------------------------------------------------------- Gerät

class Device:
    def __init__(self, host, logfile):
        self.base = f"http://{host}"
        self.io = threading.Lock()          # nie zwei Anfragen gleichzeitig
        self.log = open(logfile, "w", encoding="utf-8")
        self.hb_run = False
        self.hb_sent = self.hb_ok = 0
        self.hb_max_gap = 0.0
        self.hb_last_ok = time.time()
        self.req_max_ms = 0.0
        self.crc_bad_in_row = 0

    def _http(self, path, data=None, timeout=3.0):
        req = urllib.request.Request(self.base + path, data=data.encode() if data else None,
                                     headers={"Content-Type": "text/plain"} if data else {})
        with self.io:
            t0 = time.time()
            try:
                with urllib.request.urlopen(req, timeout=timeout) as r:
                    text = r.read().decode()
            except urllib.error.HTTPError as e:      # 400 bei BAD_CRC / BAD_JSON, Antwort lesen
                text = e.read().decode()
            ms = (time.time() - t0) * 1000
        self.req_max_ms = max(self.req_max_ms, ms)
        self.log.write(json.dumps({"t": round(t0, 3), "path": path, "req": data, "ms": round(ms, 1),
                                   "resp": text.strip(), "crc_ok": verify(text)}, ensure_ascii=False) + "\n")
        self.log.flush()
        return text

    def proc(self):
        """/proc lesen; None bei ungültiger CRC (Daten nicht verwenden)."""
        text = self._http(f"/proc?t={int(time.time())}")
        if not verify(text):
            self.crc_bad_in_row += 1
            if self.crc_bad_in_row >= 3:
                raise Abort("/proc dreimal hintereinander mit ungültiger CRC")
            return None
        self.crc_bad_in_row = 0
        return json.loads(text)

    def api(self, cmd, params=None):
        text = self._http("/api", build(cmd, params or {}))
        if not verify(text):
            raise Abort(f"Antwort auf {cmd} mit ungültiger CRC - keine weiteren Schreibbefehle")
        return json.loads(text)

    # Heartbeat: GET_STATE alle 1 s, solange die Sitzung läuft (ohne physische Wirkung)
    def _heartbeat(self):
        while self.hb_run:
            time.sleep(HB_PERIOD)
            if not self.hb_run:
                break
            try:
                self.hb_sent += 1
                self.api("GET_STATE")
                now = time.time()
                self.hb_ok += 1
                self.hb_max_gap = max(self.hb_max_gap, now - self.hb_last_ok)
                self.hb_last_ok = now
            except Exception as e:  # noqa: BLE001 - Lücke wird über hb_max_gap sichtbar
                print(f"\n!! Heartbeat fehlgeschlagen: {e}", flush=True)

    def start_heartbeat(self):
        self.hb_run = True
        self.hb_last_ok = time.time()
        threading.Thread(target=self._heartbeat, daemon=True).start()

    def stop_heartbeat(self):
        self.hb_run = False


# ---------------------------------------------------------------- Hilfen

def line(p):
    m, q, s = p["motion"], p["measure"], p["safety"]
    return (f'{p["state"]:<10} err={p["err"]} | pos={m["pos"]:6.1f} {m["state"]}/{m["mode"]} '
            f'homed={int(m["homed"])} | thick={q["thick"]:5.1f} valid={int(q["valid"])} | '
            f'hv={int(s["hv_on"])} shutter={s["shutter"]} armed={int(s["comm_armed"])}')


def check_proc(p, allowed_states):
    """Abbruchgründe nach den ADD-Regeln."""
    if p["err"] != 0:
        raise Abort(f"Systemfehler {p['err']} {SYS_ERR.get(p['err'], '?')}")
    if p["local"]:
        raise Abort("Ein Bediener arbeitet am Touchdisplay (local=true) - nicht wiederholen, warten")
    if p["safety"]["estop"]:
        raise Abort("Not-Aus aktiv")
    if p["state"] not in allowed_states:
        raise Abort(f"Unerwarteter Zustand {p['state']} (erwartet {allowed_states})")


def confirm(cmd, params, moves):
    print()
    print(f">>> Nächste Aktion: {cmd} {json.dumps(params) if params else ''}")
    if moves:
        print(">>> Die Achse bewegt sich. Ist niemand in Reichweite der Achse?")
    a = input('>>> Ausführen? "ja" eingeben, alles andere bricht ab: ').strip().lower()
    if a != "ja":
        raise Abort(f"{cmd} vom Bediener nicht bestätigt")


def send(dev, cmd, params=None, effect=None):
    """Befehl senden; err != 0 bricht ab. BUSY: /proc lesen, nur ohne sichtbare Wirkung einmal wiederholen."""
    r = dev.api(cmd, params)
    e = r.get("err")
    if e == -7 and effect:
        time.sleep(1.0)
        p = dev.proc()
        if p and effect(p):
            print(f"    {cmd}: BUSY, Wirkung in /proc sichtbar - nicht wiederholt")
            return
        r = dev.api(cmd, params)
        e = r.get("err")
    if e != 0:
        raise Abort(f"{cmd} abgelehnt: {e} {ERR.get(e, '?')} - {ERR_TEXT.get(e, r.get('msg', ''))}")
    print(f"    {cmd}: OK (angenommen)")


def wait_for(dev, done, allowed, timeout, what, samples=None):
    """/proc alle 0,2 s lesen, bis done(p) gilt; jede Probe wird auf Abbruchgründe geprüft."""
    t_end = time.time() + timeout
    last = 0.0
    while time.time() < t_end:
        p = dev.proc()
        if p:
            check_proc(p, allowed)
            if samples is not None:
                samples.append(p)
            if time.time() - last > 2.0:
                print("    " + line(p), flush=True)
                last = time.time()
            if done(p):
                print("    " + line(p))
                return p
        time.sleep(0.2)
    raise Abort(f"Zeitüberschreitung: {what} nicht innerhalb {timeout} s abgeschlossen")


def evaluate(samples, csvfile):
    tr = [p for p in samples if p["motion"]["mode"] == "traverse"]
    with open(csvfile, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["ts", "pos", "dist", "thick", "on_material", "valid", "in_range", "shutter"])
        for p in tr:
            m, q = p["motion"], p["measure"]
            w.writerow([p["ts"], m["pos"], q["dist"], q["thick"], int(q["on_material"]),
                        int(q["valid"]), int(q["in_range"]), p["safety"]["shutter"]])
    print(f"\n=== Profilauswertung ({len(tr)} Samples, gespeichert in {csvfile})")
    if not tr:
        print("    Keine Samples mit mode=traverse - Auswertung nicht möglich")
        return
    edges = [(a["motion"]["pos"] + b["motion"]["pos"]) / 2 for a, b in zip(tr, tr[1:])
             if a["measure"]["on_material"] != b["measure"]["on_material"]]
    print("    Materialkanten (on_material wechselt) bei: " +
          (", ".join(f"{e:.1f} mm" for e in edges) if edges else "keine gefunden") + "  (Soll ≈ 105 und 225 mm)")
    print("    Stufe | Mitte  | Soll  | Mittel ±5 mm | Abweichung | n")
    for i, (c, soll) in enumerate(zip(STEP_CENTRES, STEP_THICK), 1):
        v = [p["measure"]["thick"] for p in tr if abs(p["motion"]["pos"] - c) <= 5 and p["measure"]["valid"]]
        if v:
            mean = sum(v) / len(v)
            print(f"      {i}   | {c} mm | {soll:2d} mm | {mean:6.2f} mm    | {mean - soll:+5.2f} mm  | {len(v)}")
        else:
            print(f"      {i}   | {c} mm | {soll:2d} mm |   keine gültigen Werte")


# ---------------------------------------------------------------- Ablauf

def run(host):
    stamp = time.strftime("%Y%m%d_%H%M%S")
    logfile, csvfile = f"validation_{stamp}.jsonl", f"profile_{stamp}.csv"
    dev = Device(host, logfile)
    sent_motion = False
    samples = []
    print(f"Gerät: http://{host}   Protokoll: {logfile}")
    try:
        # 1. Ausgangszustand
        p = dev.proc()
        if p is None:
            raise Abort("/proc mit ungültiger CRC")
        print("\n[1] Ausgangszustand\n    " + line(p))
        check_proc(p, ["SAFE_START", "READY"])
        if p["safety"]["hv_on"]:
            raise Abort("HV ist bereits an - Ausgangszustand nicht wie erwartet")

        dev.start_heartbeat()

        # 2. Konfiguration
        print("\n[2] GET_CONFIG")
        cfg = dev.api("GET_CONFIG")
        if cfg.get("err") != 0:
            raise Abort(f"GET_CONFIG: {cfg.get('err')}")
        print("    " + json.dumps(cfg.get("config"), ensure_ascii=False))

        # 3. Referenzfahrt
        print("\n[3] Referenzfahrt")
        confirm("INIT", {}, moves=True)
        sent_motion = True
        send(dev, "INIT", effect=lambda q: q["motion"]["state"] != "idle" or q["motion"]["homed"])
        wait_for(dev, lambda q: q["motion"]["homed"] and q["motion"]["state"] == "idle" and q["state"] == "READY",
                 ["SAFE_START", "READY"], 80, "Referenzfahrt")

        # 4. Hochspannung
        print("\n[4] Hochspannung (simuliert)")
        p = wait_for(dev, lambda q: True, ["READY"], 5, "Lesen")
        s = p["safety"]
        if not s["hv_ok"]:
            raise Abort(f"HV-Freigabe fehlt: interlock={s['interlock']} cooling_flow={s['cooling_flow']} "
                        f"temp={s['temp']} °C estop={s['estop']} (alle simuliert außer Not-Aus)")
        confirm("HV_ON", {}, moves=False)
        send(dev, "HV_ON", effect=lambda q: q["safety"]["hv_on"])
        wait_for(dev, lambda q: q["state"] == "RUNNING" and q["safety"]["hv_on"], ["READY", "RUNNING"], 5, "HV_ON")

        # 5. Profilfahrt
        print("\n[5] Profilfahrt")
        confirm("START_TRAVERSE", TRAVERSE, moves=True)
        send(dev, "START_TRAVERSE", TRAVERSE, effect=lambda q: q["motion"]["mode"] == "traverse")
        wait_for(dev, lambda q: q["motion"]["mode"] == "traverse", ["RUNNING"], 30, "Start der Messfahrt", samples)
        # Ende: Achse steht am Endpunkt (mode wird je nach Firmware none oder bleibt traverse)
        wait_for(dev, lambda q: q["motion"]["state"] == "idle" and abs(q["motion"]["pos"] - TRAVERSE["end"]) < 1.0,
                 ["RUNNING"], 60, "Messfahrt", samples)

        # 6. Messprogramm beenden (sichere Richtung, ohne Rückfrage)
        print("\n[6] STOP und HV_OFF")
        send(dev, "STOP")
        send(dev, "HV_OFF")
        wait_for(dev, lambda q: not q["safety"]["hv_on"] and q["safety"]["shutter"] == "closed",
                 ["READY", "RUNNING"], 5, "HV aus")

        # 7. Parken
        print("\n[7] Parkposition")
        confirm("PARK", {}, moves=True)
        send(dev, "PARK", effect=lambda q: q["motion"]["state"] != "idle" or q["motion"]["pos"] < 3)
        p = wait_for(dev, lambda q: q["motion"]["state"] == "idle" and q["motion"]["pos"] < 3,
                     ["READY"], 40, "Parken")
        print(f"    Watchdog nach dem Parken: comm_armed={p['safety']['comm_armed']} (erwartet false: Achse steht, HV aus)")
        sent_motion = False
        print("\nAblauf vollständig.")
    except (Abort, KeyboardInterrupt, Exception) as e:  # noqa: BLE001 - jeder Fehler endet sicher
        reason = "Strg+C" if isinstance(e, KeyboardInterrupt) else str(e)
        print(f"\n!!! ABBRUCH: {reason}")
        dev.log.write(json.dumps({"t": round(time.time(), 3), "abort": reason}, ensure_ascii=False) + "\n")
    finally:
        if sent_motion:
            for cmd in ("STOP", "HV_OFF"):
                try:
                    r = dev.api(cmd)
                    print(f"    Sicherheitsbefehl {cmd}: {ERR.get(r.get('err'), r.get('err'))}")
                except Exception as e:  # noqa: BLE001
                    print(f"!!! {cmd} nicht bestätigt ({e}). Not-Aus drücken, notfalls Netzteil trennen!")
            try:
                p = dev.proc()
                if p:
                    print("    Endzustand: " + line(p))
            except Exception:  # noqa: BLE001
                pass
        dev.stop_heartbeat()
        time.sleep(0.2)
        evaluate(samples, csvfile)
        print(f"\n=== Kommunikation: Heartbeat gesendet {dev.hb_sent}, beantwortet {dev.hb_ok}, "
              f"längste Lücke {dev.hb_max_gap:.2f} s; längste Anfrage {dev.req_max_ms:.0f} ms")
        dev.log.write(json.dumps({"summary": {"hb_sent": dev.hb_sent, "hb_ok": dev.hb_ok,
                                              "hb_max_gap_s": round(dev.hb_max_gap, 2),
                                              "req_max_ms": round(dev.req_max_ms)}}) + "\n")
        dev.log.close()
        print(f"Bitte {logfile} und {csvfile} an Claude zurückgeben.")


def main():
    ap = argparse.ArgumentParser(description="Validierungslauf Demoanlage ADD (Slim)")
    ap.add_argument("--host", default="192.168.1.120", help="IP-Adresse im LAN (Standard 192.168.1.120)")
    ap.add_argument("--selftest", action="store_true", help="nur CRC-Selbsttest")
    a = ap.parse_args()
    selftest()
    if a.selftest:
        return
    run(a.host)


if __name__ == "__main__":
    sys.exit(main())
