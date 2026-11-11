#!/usr/bin/env python3
"""
add_traverse_scan.py - kontinuierliche Profilmessung in beiden Richtungen
für die Demoanlage ADD (Slim), Firmware 1.0.0 / CONTRACT V1.6.4

Geschrieben von Claude (claude-opus-5-5) nach den Regeln in /add. Gestartet wird das Skript vom
Bediener; der Start ersetzt die Bestätigung je Aktion für den geplanten Ablauf.

Ablauf
  1. /proc lesen, Ausgangszustand prüfen, Ablauf anzeigen, einmal bestätigen lassen
     ("ja" = Achse frei, Ablauf freigegeben)
  2. INIT, falls nicht referenziert; HV_ON, falls HV aus
  3. Messfahrten hin und zurück (START_TRAVERSE start->end, dann end->start, ...).
     Die nächste Fahrt wird sofort nach dem Ende der vorigen gesendet, also innerhalb von
     shutter_hold: Der Shutter bleibt über die ganze Messung offen (CONTRACT §6.4).
     Ein Zyklus = eine Fahrt hin und eine zurück.
  4. Am Ende STOP und HV_OFF.

Unterbrechungen (Not-Aus, anderer Fehler, HV-Ausfall, Bediener am Display)
  Das Skript erkennt sie selbst in /proc (alle 0,2 s), sendet STOP und HV_OFF und führt durch den
  Wiederanlauf:
    - Not-Aus entriegeln (das Skript wartet, bis safety.estop false ist)
    - Fehler quittieren: Das Skript nennt die Ursache und sendet ACK erst, wenn du bestätigst, dass
      die Ursache beseitigt und die Anlage sicher ist. Quittieren am Display wird ebenfalls erkannt.
    - HV-Freigabe prüfen (Interlock, Kühlwasserfluss, Temperatur - simuliert)
    - Wiederanlauf bestätigen: Nach einem Not-Aus sind HV und Shutter aus und die Referenz ist
      meist verloren. Das Skript zeigt, was es senden wird (INIT, HV_ON, Messfahrt) und startet
      erst nach "ja". Ein automatischer Wiederanlauf ohne Rückfrage ist bewusst nicht vorgesehen:
      Die Firmware startet nach ACK nicht von selbst (CONTRACT §6.2), das Ethical Framework
      verlangt bei geänderten Bedingungen eine neue Bestätigung, und nach einem Not-Aus kann
      jemand an der Achse sein.
  Die unterbrochene Fahrt wird danach wiederholt; der Zykluszähler zählt nur vollständige Fahrten.

Beenden
  Strg+C oder SIGTERM: STOP und HV_OFF, dann Ende.
  Hartes Beenden (kill -9, Absturz, Netzwerkausfall): Das Skript kann nichts mehr senden. Der
  Kommunikations-Watchdog des Geräts stoppt nach 5 s ohne /api-Paket die Achse, schaltet HV aus
  und meldet COMM_TIMEOUT (Quittierung nötig).

Sicherheit
  - GET_STATE alle 1 s in einem eigenen Thread (ADD verlangt <= 2 s, solange comm_armed).
  - Alle Anfragen laufen nacheinander (das Gerät bedient nur eine Verbindung zur Zeit).
  - Jede Antwort und jedes /proc wird über die CRC geprüft; ungültige Daten werden verworfen,
    dreimal hintereinander ungültig -> STOP, HV_OFF, Abbruch.
  - Gesendet werden nur INIT, HV_ON, START_TRAVERSE, STOP, HV_OFF, ACK, GET_STATE, GET_CONFIG.
    Kein SET, kein CALIB. Keine Zugangsdaten: nur für den Betrieb im LAN.

Aufruf
  python3 add_traverse_scan.py --cycles 5 --speed 10
  python3 add_traverse_scan.py --cycles 0 --speed 5 --start 100 --end 230     # 0 = bis Strg+C
  python3 add_traverse_scan.py --host 192.168.4.1 --cycles 3

Ausgabe
  scan_<Zeit>.csv    - alle Messpunkte mit offenem Shutter (Fahrt, Richtung, Position, Dicke ...)
  scan_<Zeit>.jsonl  - jede Anfrage und Antwort mit Zeitstempel und Dauer

Nur Python-Standardbibliothek.
"""
import argparse
import csv
import json
import signal
import sys
import threading
import time
import urllib.error
import urllib.request
import zlib

TAG = ',"crc":"'
ERR = {0: "OK", -1: "BAD_CRC", -2: "BAD_JSON", -3: "UNKNOWN_CMD", -4: "BAD_PARAM", -5: "STATE",
       -6: "PRECONDITION", -7: "BUSY", -8: "NOT_STABLE", -9: "CALIB_INVALID", -10: "LOCAL_CONTROL"}
SYS_ERR = {0: "NONE", 1: "ESTOP - Not-Aus gedrückt", 2: "TEMP_HIGH - simulierte Kühlwassertemperatur > 32 °C",
           3: "COMM_TIMEOUT - > 5 s kein /api-Paket", 4: "MOTOR_FAULT - Endschalter unerwartet angefahren",
           5: "HOMING_FAILED - Referenzfahrt gescheitert"}
STEP_CENTRES = [115, 135, 155, 175, 195, 215]
STEP_THICK = [5, 10, 15, 20, 25, 30]
HB_PERIOD = 1.0
POLL = 0.2


class Abort(Exception):
    """Messung beenden; finally sendet STOP und HV_OFF."""


class Interrupted(Exception):
    """Messfahrt unterbrochen (Not-Aus, Fehler, HV-Ausfall, Bediener am Display)."""


# ------------------------------------------------------------------ CRC-Rahmen (ADD interfaces[0])

def frame(body):
    return body[:-1] + TAG + "%08X" % (zlib.crc32(body.encode()) & 0xFFFFFFFF) + '"}'


def verify(text):
    t = text.rstrip(" \t\r\n")
    i = t.rfind(TAG)
    if i < 0 or not t.endswith('"}') or len(t) != i + len(TAG) + 10:
        return False
    try:
        expected = int(t[i + len(TAG): i + len(TAG) + 8], 16)
    except ValueError:
        return False
    return (zlib.crc32((t[:i] + "}").encode()) & 0xFFFFFFFF) == expected


def build(cmd, params):
    return frame(json.dumps({"cmd": cmd, "params": params}, separators=(",", ":")))


def selftest():
    assert build("HV_ON", {}) == '{"cmd":"HV_ON","params":{},"crc":"88F7C802"}'
    assert frame('{"cmd":"SET","params":{"key":"temp","value":35.0}}').endswith('"C93F2D33"}')
    assert frame('{"cmd":"SET","params":{"key":"temp","value":35}}').endswith('"F2B0BAC5"}')


# ------------------------------------------------------------------ Gerät

class Device:
    def __init__(self, host, logfile):
        self.base = "http://" + host
        self.io = threading.Lock()
        self.log = open(logfile, "w", encoding="utf-8")
        self.hb_run = False
        self.hb_sent = self.hb_ok = 0
        self.hb_max_gap = 0.0
        self.hb_last = time.time()
        self.bad_crc = 0

    def _http(self, path, data=None, timeout=3.0):
        req = urllib.request.Request(self.base + path, data=data.encode() if data else None,
                                     headers={"Content-Type": "text/plain"} if data else {})
        with self.io:
            t0 = time.time()
            try:
                with urllib.request.urlopen(req, timeout=timeout) as r:
                    text = r.read().decode()
            except urllib.error.HTTPError as e:
                text = e.read().decode()
            ms = (time.time() - t0) * 1000
            self.log.write(json.dumps({"t": round(t0, 3), "path": path, "req": data, "ms": round(ms, 1),
                                       "resp": text.strip(), "crc_ok": verify(text)}, ensure_ascii=False) + "\n")
            self.log.flush()
        return text

    def note(self, **kw):
        with self.io:
            self.log.write(json.dumps({"t": round(time.time(), 3), **kw}, ensure_ascii=False) + "\n")
            self.log.flush()

    def proc(self):
        text = self._http("/proc?t=%d" % int(time.time()))
        if not verify(text):
            self.bad_crc += 1
            if self.bad_crc >= 3:
                raise Abort("/proc dreimal hintereinander mit ungültiger CRC")
            return None
        self.bad_crc = 0
        return json.loads(text)

    def proc_valid(self):
        while True:
            p = self.proc()
            if p:
                return p
            time.sleep(POLL)

    def api(self, cmd, params=None):
        text = self._http("/api", build(cmd, params or {}))
        if not verify(text):
            raise Abort("Antwort auf %s mit ungültiger CRC - keine weiteren Schreibbefehle" % cmd)
        return json.loads(text)

    def _heartbeat(self):
        while self.hb_run:
            time.sleep(HB_PERIOD)
            if not self.hb_run:
                break
            self.hb_sent += 1
            try:
                self.api("GET_STATE")
                now = time.time()
                self.hb_ok += 1
                self.hb_max_gap = max(self.hb_max_gap, now - self.hb_last)
                self.hb_last = now
            except Exception as e:  # noqa: BLE001 - Lücke wird in hb_max_gap sichtbar
                print("\n!! Heartbeat fehlgeschlagen: %s" % e, flush=True)

    def start_heartbeat(self):
        self.hb_run = True
        self.hb_last = time.time()
        threading.Thread(target=self._heartbeat, daemon=True).start()

    def stop_heartbeat(self):
        self.hb_run = False


# ------------------------------------------------------------------ Hilfen

def line(p):
    m, q, s = p["motion"], p["measure"], p["safety"]
    return ("%-10s err=%d | pos=%6.1f %s/%s homed=%d | thick=%5.1f valid=%d | hv=%d shutter=%s estop=%d"
            % (p["state"], p["err"], m["pos"], m["state"], m["mode"], m["homed"], q["thick"], q["valid"],
               s["hv_on"], s["shutter"], s["estop"]))


def interruption(p, expect_running):
    """Grund für eine Unterbrechung oder None."""
    s = p["safety"]
    if s["estop"] or p["err"] == 1:
        return "Not-Aus"
    if p["err"] != 0 or p["state"] in ("FAULT", "WAIT_ACK", "RECOVER"):
        return "Fehler %d %s" % (p["err"], SYS_ERR.get(p["err"], "?"))
    if p["local"]:
        return "Ein Bediener arbeitet am Touchdisplay (local)"
    if expect_running and (p["state"] != "RUNNING" or not s["hv_on"]):
        return ("Hochspannung abgefallen (simuliert: interlock=%s, cooling_flow=%s, temp=%.1f °C)"
                % (s["interlock"], s["cooling_flow"], s["temp"]))
    return None


def missing_hv(p):
    s = p["safety"]
    m = []
    if not s["interlock"]:
        m.append("Interlock offen (simuliert)")
    if not s["cooling_flow"]:
        m.append("kein Kühlwasserfluss (simuliert)")
    if not s["temp_ok"]:
        m.append("Temperatur %.1f °C > 30 °C (simuliert)" % s["temp"])
    if s["estop"]:
        m.append("Not-Aus aktiv")
    return ", ".join(m) or "unbekannt"


def ask(prompt):
    return input(prompt).strip().lower()


def send(dev, cmd, params=None, effect=None):
    """Befehl senden. err != 0 bricht ab; BUSY: /proc lesen, nur ohne sichtbare Wirkung einmal wiederholen."""
    r = dev.api(cmd, params)
    e = r.get("err")
    if e == -7 and effect:
        time.sleep(0.5)
        p = dev.proc_valid()
        if effect(p):
            return r
        r = dev.api(cmd, params)
        e = r.get("err")
    if e != 0:
        raise Abort("%s abgelehnt: %s %s %s" % (cmd, e, ERR.get(e, "?"), r.get("msg", "")))
    print("    %s %s: OK" % (cmd, json.dumps(params) if params else ""))
    return r


def wait_for(dev, done, timeout, what, expect_running):
    t_end = time.time() + timeout
    while time.time() < t_end:
        p = dev.proc()
        if p:
            why = interruption(p, expect_running)
            if why:
                raise Interrupted(why)
            if done(p):
                return p
        time.sleep(POLL)
    raise Abort("Zeitüberschreitung: %s nicht innerhalb %.0f s" % (what, timeout))


def safe_stop(dev):
    for cmd in ("STOP", "HV_OFF"):
        try:
            r = dev.api(cmd)
            print("    %s: %s" % (cmd, ERR.get(r.get("err"), r.get("err"))))
        except Exception as e:  # noqa: BLE001
            print("!!! %s nicht bestätigt (%s). Not-Aus drücken, notfalls Netzteil trennen!" % (cmd, e))


# ------------------------------------------------------------------ Ablaufschritte

def prepare(dev):
    """Referenz und HV herstellen (bereits bestätigt)."""
    p = dev.proc_valid()
    why = interruption(p, False)
    if why:
        raise Interrupted(why)
    if not p["motion"]["homed"]:
        print("  Referenzfahrt")
        send(dev, "INIT", effect=lambda q: q["motion"]["state"] != "idle" or q["motion"]["homed"])
        wait_for(dev, lambda q: q["motion"]["homed"] and q["motion"]["state"] == "idle" and q["state"] == "READY",
                 80, "Referenzfahrt", False)
    p = dev.proc_valid()
    if not p["safety"]["hv_on"]:
        if not p["safety"]["hv_ok"]:
            raise Interrupted("HV-Freigabe fehlt: " + missing_hv(p))
        print("  Hochspannung (simuliert)")
        send(dev, "HV_ON", effect=lambda q: q["safety"]["hv_on"])
        wait_for(dev, lambda q: q["state"] == "RUNNING" and q["safety"]["hv_on"], 5, "HV_ON", False)


def traverse(dev, run_no, start, end, speed, writer):
    """Eine Messfahrt; Rückgabe: Messpunkte mit offenem Shutter."""
    direction = "hin" if end > start else "zurück"
    params = {"start": start, "end": end, "speed": speed}
    t0 = time.time()
    send(dev, "START_TRAVERSE", params,
         effect=lambda q: q["motion"]["mode"] == "traverse" and q["motion"]["state"] != "idle")
    p0 = dev.proc_valid()
    timeout = (abs(p0["motion"]["pos"] - start) / 10.0) + abs(end - start) / speed + 15.0
    started = False
    samples = []
    while True:
        if time.time() - t0 > timeout:
            raise Abort("Zeitüberschreitung in Fahrt %d (%.0f s)" % (run_no, timeout))
        p = dev.proc()
        if p:
            why = interruption(p, True)
            if why:
                raise Interrupted(why)
            m, q = p["motion"], p["measure"]
            if m["mode"] == "traverse":
                started = True
                if p["safety"]["shutter"] == "open":
                    samples.append(p)
                    writer.writerow([run_no, direction, p["ts"], m["pos"], q["dist"], q["thick"],
                                     int(q["on_material"]), int(q["valid"])])
            if (started or time.time() - t0 > 2.0) and m["state"] == "idle" and m["mode"] == "none" \
                    and abs(m["pos"] - end) < 1.0:
                break
        time.sleep(POLL)
    return direction, samples


def summary(run_no, direction, samples, start, end):
    parts = []
    for c, soll in zip(STEP_CENTRES, STEP_THICK):
        if min(start, end) <= c <= max(start, end):
            v = [s["measure"]["thick"] for s in samples if abs(s["motion"]["pos"] - c) <= 5 and s["measure"]["valid"]]
            parts.append("%d:%s" % (soll, ("%.1f" % (sum(v) / len(v))) if v else "-"))
    print("  Fahrt %d %-6s %5.0f -> %5.0f mm  %3d Punkte  Stufen soll:ist  %s"
          % (run_no, direction, start, end, len(samples), "  ".join(parts)), flush=True)


def recover(dev, reason):
    """Nach einer Unterbrechung sicher anhalten und durch den Wiederanlauf führen.
    Rückgabe erst, wenn der Bediener den Wiederanlauf bestätigt hat."""
    print("\n!!! UNTERBRECHUNG: %s" % reason, flush=True)
    dev.note(interruption=reason)
    safe_stop(dev)
    while True:
        p = dev.proc_valid()
        print("    " + line(p))
        s = p["safety"]
        if s["estop"]:
            print("    Not-Aus ist gedrückt. Bitte entriegeln - ich warte ...", flush=True)
            while dev.proc_valid()["safety"]["estop"]:
                time.sleep(0.5)
            print("    Not-Aus entriegelt.")
            continue
        if p["state"] in ("FAULT", "WAIT_ACK"):
            print("    Fehler liegt an: %d %s" % (p["err"], SYS_ERR.get(p["err"], "?")))
            a = ask('    Ursache beseitigt und Anlage sicher? "ack" = ACK senden, '
                    'Enter = neu prüfen (z. B. am Display quittiert), "q" = beenden: ')
            if a == "q":
                raise Abort("Bediener beendet nach Unterbrechung")
            if a == "ack":
                r = dev.api("ACK")
                e = r.get("err")
                if e == 0:
                    print("    ACK: OK")
                    dev.note(ack="ok")
                elif e == -6:
                    print("    ACK abgelehnt (-6 PRECONDITION): Die Ursache besteht noch. Nicht wiederholt.")
                else:
                    print("    ACK abgelehnt: %s %s" % (e, ERR.get(e, "?")))
            time.sleep(0.5)
            continue
        if p["state"] == "RECOVER":
            time.sleep(0.2)
            continue
        if p["local"]:
            print("    Ein Bediener arbeitet am Display. Ich warte, bis local=false ...", flush=True)
            while dev.proc_valid()["local"]:
                time.sleep(1.0)
            continue
        if not s["hv_ok"]:
            print("    HV-Freigabe fehlt: " + missing_hv(p))
            a = ask('    Enter = neu prüfen, "q" = beenden: ')
            if a == "q":
                raise Abort("Bediener beendet: HV-Freigabe fehlt")
            continue
        break
    plan = []
    if not p["motion"]["homed"]:
        plan.append("INIT (Referenz verloren)")
    plan += ["HV_ON (simuliert)", "Messfahrt wiederholen und Messung fortsetzen"]
    print("\n>>> Wiederanlauf: " + ", ".join(plan))
    print(">>> Die Achse bewegt sich. Ist niemand in Reichweite der Achse?")
    a = ask('>>> Wiederanlauf freigeben? "ja" = fortsetzen, alles andere beendet: ')
    if a != "ja":
        raise Abort("Wiederanlauf vom Bediener nicht freigegeben")
    dev.note(resume="confirmed")


# ------------------------------------------------------------------ Hauptablauf

def run(a):
    stamp = time.strftime("%Y%m%d_%H%M%S")
    csvname, logname = "scan_%s.csv" % stamp, "scan_%s.jsonl" % stamp
    dev = Device(a.host, logname)
    f = open(csvname, "w", newline="", encoding="utf-8")
    writer = csv.writer(f)
    writer.writerow(["run", "direction", "ts", "pos", "dist", "thick", "on_material", "valid"])
    runs_done = 0
    interruptions = 0
    armed = False
    try:
        p = dev.proc_valid()
        print("Gerät http://%s\n  %s" % (a.host, line(p)))
        why = interruption(p, False)
        if why:
            raise Abort("Ausgangszustand nicht in Ordnung: " + why + " - erst beheben, dann neu starten")
        dev.start_heartbeat()
        cfg = dev.api("GET_CONFIG")
        if cfg.get("err") == 0:
            c = cfg["config"]
            print("  Konfiguration: shutter_hold %.1f s, edge_threshold %.1f mm, base_dist %.1f mm, Kalibrierung %s"
                  % (c["shutter_hold"], c["edge_threshold"], c["base_dist"], "gültig" if c["calib_valid"] else "UNGÜLTIG"))
            if c["shutter_hold"] <= 0:
                print("  Hinweis: shutter_hold = 0, der Shutter schließt nach jeder Fahrt.")
        plan = []
        if not p["motion"]["homed"]:
            plan.append("INIT")
        if not p["safety"]["hv_on"]:
            plan.append("HV_ON")
        cyc = "endlos (bis Strg+C)" if a.cycles == 0 else "%d Zyklen = %d Fahrten" % (a.cycles, 2 * a.cycles)
        plan.append("START_TRAVERSE %g <-> %g mm mit %g mm/s, %s" % (a.start, a.end, a.speed, cyc))
        plan.append("am Ende STOP, HV_OFF")
        print("\n>>> Ablauf: " + "; ".join(plan))
        print(">>> Die Achse bewegt sich. Ist niemand in Reichweite der Achse?")
        if ask('>>> Starten? "ja" eingeben, alles andere bricht ab: ') != "ja":
            raise Abort("Start nicht bestätigt")
        armed = True
        dev.note(start={"cycles": a.cycles, "speed": a.speed, "start": a.start, "end": a.end})
        total = 2 * a.cycles
        while total == 0 or runs_done < total:
            try:
                prepare(dev)
                while total == 0 or runs_done < total:
                    s, e = (a.start, a.end) if runs_done % 2 == 0 else (a.end, a.start)
                    direction, samples = traverse(dev, runs_done + 1, s, e, a.speed, writer)
                    f.flush()
                    summary(runs_done + 1, direction, samples, s, e)
                    runs_done += 1
            except Interrupted as ex:
                interruptions += 1
                recover(dev, str(ex))
        print("\nAlle %d Fahrten abgeschlossen." % runs_done)
    except KeyboardInterrupt:
        print("\n!!! Strg+C / SIGTERM - Messung wird beendet")
        dev.note(abort="Strg+C/SIGTERM")
    except Abort as ex:
        print("\n!!! ABBRUCH: %s" % ex)
        dev.note(abort=str(ex))
    except Exception as ex:  # noqa: BLE001 - jeder unerwartete Fehler endet sicher
        print("\n!!! UNERWARTETER FEHLER: %r" % ex)
        dev.note(abort=repr(ex))
    finally:
        if armed:
            print("Sicher anhalten:")
            safe_stop(dev)
            try:
                p = dev.proc()
                if p:
                    print("  Endzustand: " + line(p))
            except Exception:  # noqa: BLE001
                pass
        dev.stop_heartbeat()
        f.close()
        print("\nFahrten: %d   Unterbrechungen: %d   Heartbeat: %d/%d beantwortet, längste Lücke %.2f s"
              % (runs_done, interruptions, dev.hb_ok, dev.hb_sent, dev.hb_max_gap))
        dev.note(summary={"runs": runs_done, "interruptions": interruptions, "hb_sent": dev.hb_sent,
                          "hb_ok": dev.hb_ok, "hb_max_gap_s": round(dev.hb_max_gap, 2)})
        dev.log.close()
        print("Dateien: %s, %s" % (csvname, logname))


def main():
    ap = argparse.ArgumentParser(description="Kontinuierliche Profilmessung hin und zurück (Demoanlage ADD Slim)")
    ap.add_argument("--host", default="192.168.1.120", help="IP-Adresse im LAN (Standard 192.168.1.120)")
    ap.add_argument("--cycles", type=int, default=1, help="Zyklen (hin + zurück); 0 = endlos bis Strg+C")
    ap.add_argument("--speed", type=float, default=10.0, help="Messgeschwindigkeit 1 ... 20 mm/s (Standard 10)")
    ap.add_argument("--start", type=float, default=80.0, help="Startposition > 60 ... 242 mm (Standard 80)")
    ap.add_argument("--end", type=float, default=240.0, help="Endposition > 60 ... 242 mm (Standard 240)")
    a = ap.parse_args()
    if a.cycles < 0:
        ap.error("--cycles muss >= 0 sein")
    if not 1 <= a.speed <= 20:
        ap.error("--speed muss 1 ... 20 mm/s sein")
    for n, v in (("--start", a.start), ("--end", a.end)):
        if not 60 < v <= 242:
            ap.error("%s muss > 60 und <= 242 mm sein (Messzone)" % n)
    if abs(a.end - a.start) < 1:
        ap.error("--start und --end müssen mindestens 1 mm auseinanderliegen")
    selftest()

    def on_term(signum, frame_):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, on_term)
    run(a)


if __name__ == "__main__":
    sys.exit(main())
