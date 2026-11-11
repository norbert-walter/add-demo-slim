#!/usr/bin/env python3
"""
add_traverse_scan.py - Profilmessung mit Kantenverfolgung für die Demoanlage ADD (Slim),
Firmware 1.0.0 / CONTRACT V1.6.5

Geschrieben von Claude (claude-opus-5-5) nach den Regeln in /add. Gestartet wird das Skript vom
Bediener; der Start ersetzt die Bestätigung je Aktion für den geplanten Ablauf.

Ablauf
  1. /proc lesen, Ausgangszustand prüfen, Ablauf anzeigen, einmal bestätigen lassen
     ("ja" = Achse frei, Ablauf freigegeben)
  2. INIT, falls nicht referenziert; HV_ON, falls HV aus
  3. Messfahrten mit Kantenverfolgung innerhalb des Messbereichs --start ... --end:
     - Das Skript wertet die Dicke measure.thick jeder Probe mit dem Schwellwert --threshold aus
       (Hysterese HYST_MM): Material ist Dicke >= Schwellwert + 3 mm, kein Material ist Dicke <
       Schwellwert; dazwischen bleibt der vorige Zustand. Jede Fahrt sucht zuerst Material: Erst
       wenn es MAT_MIN_MM mm am Stück anliegt, gilt das Messgut als gefunden, und erst danach wird
       die Kante gesucht (kein Material, entprellt: EDGE_N Proben und EDGE_MM mm am Stück). Dann
       wechselt die Richtung.
     - Nach dem Öffnen des Shutters werden die Proben der ersten SETTLE_MS ms nicht ausgewertet:
       Das Sensorsignal springt beim Öffnen (LED am GPIO, siehe Hardware-Dokumentation) und
       braucht kurz, bis es steht.
     - Erste Fahrt: bis an den Rand des Messbereichs; an der Kante STOP und zurück (der Shutter
       schließt dabei einmal kurz).
     - Jede weitere Fahrt fährt nur bis zur erwarteten Kante plus Überlauf (MARGIN_MM, mindestens
       der Weg bis zur Entprellung). Das nächste START_TRAVERSE folgt sofort, innerhalb von
       shutter_hold: Der Shutter bleibt offen. Liegt die Kante weiter außen als erwartet, wird
       in derselben Richtung bis höchstens an den Rand des Messbereichs weitergefahren.
     - Ein Zyklus = eine Fahrt hin und eine zurück.
  4. Am Ende STOP und HV_OFF.

Gestoppt mit Meldung (kein Wiederanlauf)
  - Kein Material im Messbereich: Eine Fahrt erreicht den Rand des Messbereichs, ohne Material
    gesehen zu haben.
  - Keine Kante im Messbereich: Material gesehen, aber es reicht bis an den Rand des Messbereichs
    (Messgut breiter als der Messbereich oder zu nah am Rand).
  - Endlagenschalter: limit_fwd oder limit_bwd außerhalb der Referenzfahrt, oder Fehler
    MOTOR_FAULT.

Unterbrechungen mit Wiederanlauf
  Ein eigener Thread liest /proc ununterbrochen (alle POLL s), auch während das Skript auf eine
  Eingabe wartet; wichtige Änderungen werden sofort angezeigt. Erkannt werden:
    - STOP am Display (oder von einem anderen Client): Die Fahrt endet vor ihrem Ziel, ohne dass
      das Skript STOP gesendet hat
    - ein anderer Ablauf am Display (stehende Messung, Fahrt, Park- oder Serviceposition)
    - Not-Aus, andere Fehler, Abfall der Hochspannung, Bediener mit Vorrang (local)
  Das Skript sendet STOP und HV_OFF und führt durch den Wiederanlauf:
    - Not-Aus entriegeln (das Skript wartet, bis safety.estop false ist)
    - Fehler quittieren: ACK erst, wenn du bestätigst, dass die Ursache beseitigt und die Anlage
      sicher ist. Quittieren am Display wird ebenfalls erkannt.
    - HV-Freigabe prüfen (Interlock, Kühlwasserfluss, Temperatur - simuliert)
    - Wiederanlauf bestätigen ("ja"). Nach der Eingabe wird der Zustand neu geprüft; hat sich in
      der Zwischenzeit etwas geändert, beginnt die Prüfung von vorn.
  Die unterbrochene Fahrt wird wiederholt; gezählt werden nur vollständige Fahrten.

Beenden
  Strg+C oder SIGTERM: STOP und HV_OFF, dann Ende.
  Hartes Beenden (kill -9, Absturz, Netzwerkausfall): Der Kommunikations-Watchdog des Geräts
  stoppt nach 5 s ohne /api-Paket die Achse, schaltet HV aus und meldet COMM_TIMEOUT.

Sicherheit
  - GET_STATE alle 1 s in einem eigenen Thread (ADD verlangt <= 2 s, solange comm_armed).
  - Alle Anfragen laufen nacheinander (das Gerät bedient nur eine Verbindung zur Zeit).
  - Jede Antwort und jedes /proc wird über die CRC geprüft; ungültige Daten werden verworfen,
    dreimal hintereinander ungültig -> STOP, HV_OFF, Abbruch.
  - Gesendet werden nur INIT, HV_ON, START_TRAVERSE, STOP, HV_OFF, ACK, GET_STATE, GET_CONFIG.
    Kein SET, kein CALIB. Keine Zugangsdaten: nur für den Betrieb im LAN.

Aufruf
  python3 add_traverse_scan.py --cycles 5 --speed 10
  python3 add_traverse_scan.py --cycles 5 --speed 10 --threshold 2      # Material ab 5 mm Dicke
  python3 add_traverse_scan.py --cycles 0 --speed 5 --start 80 --end 240     # 0 = bis Strg+C
  python3 add_traverse_scan.py --host 192.168.4.1 --cycles 3

Ausgabe
  scan_<Zeit>.csv    - alle Messpunkte mit offenem Shutter (Fahrt, Richtung, Position, Dicke ...)
  scan_<Zeit>.jsonl  - jede Anfrage und Antwort, Kanten und Ereignisse mit Zeitstempel

Nur Python-Standardbibliothek.
"""
import argparse
import collections
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
POLL = 0.1          # /proc-Takt des Überwachungs-Threads [s]
EDGE_N = 3          # Entprellung der Kante: so viele Proben am Stück ...
EDGE_MM = 3.0       # ... und mindestens so viele mm am Stück ohne Material
HYST_MM = 3.0       # Material: Dicke >= Schwellwert + HYST_MM; kein Material: Dicke < Schwellwert
MAT_MIN_MM = 10.0   # Material gilt erst als gefunden, wenn on_material so viele mm am Stück anliegt
SETTLE_MS = 1000    # nach dem Öffnen des Shutters so lange keine Auswertung
MARGIN_MM = 5.0     # Überlauf hinter der erwarteten Kante
LEAD_SLACK_MM = 10.0  # zusätzlicher Überlauf, wenn die Kante nur als vordere Kante bekannt ist (Sensor
                      # und Filter laufen nach: die vordere Kante wird einige mm zu spät erkannt)


class Abort(Exception):
    """Messung beenden; finally sendet STOP und HV_OFF."""


class NoEdge(Abort):
    """Keine Kante im Messbereich."""


class LimitHit(Abort):
    """Endlagenschalter ausgelöst."""


class Interrupted(Exception):
    """Messfahrt unterbrochen; Wiederanlauf nach Bestätigung möglich."""


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
        self.run = True
        self.hb_run = False
        self.hb_sent = self.hb_ok = 0
        self.hb_max_gap = 0.0
        self.hb_last = time.time()
        self.bad_crc = 0
        self.fatal = None                    # Abbruchgrund aus dem Überwachungs-Thread
        self.cv = threading.Condition()
        self.latest = None                   # letztes gültiges /proc
        self.seq = 0                         # zählt gültige /proc
        self.samples = collections.deque(maxlen=2000)
        self.prompting = False               # Hauptthread wartet auf eine Eingabe
        self.open_ts = None                  # Gerätezeit (ts), seit der der Shutter offen ist
        self._shown = None

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
            if path.startswith("/api") or not verify(text):     # /proc nur bei Auffälligkeit protokollieren
                self.log.write(json.dumps({"t": round(t0, 3), "path": path, "req": data, "ms": round(ms, 1),
                                           "resp": text.strip(), "crc_ok": verify(text)}, ensure_ascii=False) + "\n")
                self.log.flush()
        return text

    def note(self, **kw):
        with self.io:
            self.log.write(json.dumps({"t": round(time.time(), 3), **kw}, ensure_ascii=False) + "\n")
            self.log.flush()

    def api(self, cmd, params=None):
        text = self._http("/api", build(cmd, params or {}))
        if not verify(text):
            raise Abort("Antwort auf %s mit ungültiger CRC - keine weiteren Schreibbefehle" % cmd)
        return json.loads(text)

    # --- Überwachung: /proc ununterbrochen -----------------------------------------------------

    def _monitor(self):
        while self.run:
            t0 = time.time()
            try:
                text = self._http("/proc?t=%d" % int(t0 * 1000))
                if verify(text):
                    p = json.loads(text)
                    self.bad_crc = 0
                    if p["safety"]["shutter"] == "open":
                        if self.open_ts is None:
                            self.open_ts = p["ts"]
                    else:
                        self.open_ts = None
                    with self.cv:
                        self.latest = p
                        self.seq += 1
                        self.samples.append(p)
                        self.cv.notify_all()
                    if self.prompting:
                        self._show_change(p)
                else:
                    self.bad_crc += 1
                    if self.bad_crc >= 3:
                        self.fatal = "/proc dreimal hintereinander mit ungültiger CRC"
            except Exception as e:  # noqa: BLE001 - Netzwerkfehler: weiter versuchen, sichtbar machen
                self.bad_crc += 1
                if self.bad_crc >= 3:
                    self.fatal = "/proc nicht erreichbar: %s" % e
            time.sleep(max(0.0, POLL - (time.time() - t0)))

    def _show_change(self, p):
        """Während einer Eingabe: wichtige Zustandsänderungen sofort anzeigen."""
        s = p["safety"]
        key = (p["state"], p["err"], s["estop"], s["hv_on"], s["hv_ok"], p["local"], p["motion"]["state"])
        if key != self._shown:
            if self._shown is not None:
                print("\n    [Zustand] " + line(p), flush=True)
            self._shown = key

    def start(self):
        threading.Thread(target=self._monitor, daemon=True).start()
        with self.cv:
            self.cv.wait_for(lambda: self.latest is not None or self.fatal, timeout=5)
        if self.fatal or self.latest is None:
            raise Abort(self.fatal or "kein gültiges /proc in 5 s")

    def now(self):
        """Letztes gültiges /proc (höchstens POLL alt)."""
        if self.fatal:
            raise Abort(self.fatal)
        return self.latest

    def fresh(self, timeout=2.0):
        """Ein /proc, das nach diesem Aufruf gelesen wurde."""
        with self.cv:
            s0 = self.seq
            self.cv.wait_for(lambda: self.seq > s0 or self.fatal, timeout=timeout)
        if self.fatal:
            raise Abort(self.fatal)
        if self.seq == s0:
            raise Abort("kein neues /proc in %.0f s" % timeout)
        return self.latest

    def take(self):
        """Alle neuen Proben seit dem letzten Aufruf (wartet höchstens 1 s)."""
        with self.cv:
            if not self.samples:
                self.cv.wait(timeout=1.0)
            out = list(self.samples)
            self.samples.clear()
        if self.fatal:
            raise Abort(self.fatal)
        return out

    # --- Herzschlag --------------------------------------------------------------------------

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

    def stop(self):
        self.hb_run = False
        self.run = False


# ------------------------------------------------------------------ Hilfen

def line(p):
    m, q, s = p["motion"], p["measure"], p["safety"]
    return ("%-10s err=%d | pos=%6.1f %s/%s homed=%d lim=%d%d | thick=%5.1f mat=%d valid=%d | hv=%d shutter=%s estop=%d local=%d"
            % (p["state"], p["err"], m["pos"], m["state"], m["mode"], m["homed"], m["limit_bwd"], m["limit_fwd"],
               q["thick"], q["on_material"], q["valid"], s["hv_on"], s["shutter"], s["estop"], p["local"]))


def check_limits(p):
    """Endlagenschalter außerhalb der Referenzfahrt -> Abbruch."""
    m = p["motion"]
    if p["err"] == 4:
        raise LimitHit("Endlagenschalter: Fehler MOTOR_FAULT (Endschalter unerwartet angefahren) bei %.1f mm" % m["pos"])
    if m["state"] != "homing" and (m["limit_fwd"] or m["limit_bwd"]):
        which = "vorne (limit_fwd)" if m["limit_fwd"] else "hinten (limit_bwd)"
        raise LimitHit("Endlagenschalter %s ausgelöst bei %.1f mm" % (which, m["pos"]))


def interruption(p, expect_running):
    """Grund für eine Unterbrechung oder None (Endlagen vorher mit check_limits)."""
    s = p["safety"]
    if s["estop"] or p["err"] == 1:
        return "Not-Aus"
    if p["err"] != 0 or p["state"] in ("FAULT", "WAIT_ACK", "RECOVER"):
        return "Fehler %d %s" % (p["err"], SYS_ERR.get(p["err"], "?"))
    if p["local"]:
        return "Ein Bediener arbeitet am Touchdisplay (local)"
    if expect_running and (p["state"] != "RUNNING" or not s["hv_on"]):
        return ("Hochspannung abgefallen (HV_OFF am Display oder simuliert: interlock=%s, cooling_flow=%s, temp=%.1f °C)"
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


def ask(dev, prompt):
    dev._shown = None
    dev.prompting = True
    try:
        return input(prompt).strip().lower()
    finally:
        dev.prompting = False


def send(dev, cmd, params=None, effect=None):
    """Befehl senden. err != 0 bricht ab; BUSY: /proc lesen, nur ohne sichtbare Wirkung einmal wiederholen."""
    r = dev.api(cmd, params)
    e = r.get("err")
    if e == -7 and effect:
        time.sleep(0.5)
        if effect(dev.fresh()):
            return r
        r = dev.api(cmd, params)
        e = r.get("err")
    if e == -10:
        raise Interrupted("%s abgelehnt: Bediener am Display hat Vorrang (LOCAL_CONTROL)" % cmd)
    if e != 0:
        raise Abort("%s abgelehnt: %s %s %s" % (cmd, e, ERR.get(e, "?"), r.get("msg", "")))
    return r


def wait_for(dev, done, timeout, what, expect_running):
    t_end = time.time() + timeout
    while time.time() < t_end:
        p = dev.fresh()
        check_limits(p)
        why = interruption(p, expect_running)
        if why:
            raise Interrupted(why)
        if done(p):
            return p
    raise Abort("Zeitüberschreitung: %s nicht innerhalb %.0f s" % (what, timeout))


def safe_stop(dev):
    for cmd in ("STOP", "HV_OFF"):
        try:
            r = dev.api(cmd)
            print("    %s: %s" % (cmd, ERR.get(r.get("err"), r.get("err"))))
        except Exception as e:  # noqa: BLE001
            print("!!! %s nicht bestätigt (%s). Not-Aus drücken, notfalls Netzteil trennen!" % (cmd, e))


# ------------------------------------------------------------------ Kantenerkennung

class EdgeTracker:
    """Kantensuche über die Proben einer Fahrt (nur gültige Messwerte nach der Beruhigungszeit).

    1. Material suchen: on_material muss MAT_MIN_MM mm (und mindestens EDGE_N Proben) am Stück
       anliegen. Kürzere Stellen mit on_material (Ausreißer) werden übergangen.
    2. Erst danach die Kante suchen: kein Material für EDGE_MM mm und EDGE_N Proben am Stück.
    material = Position, ab der das Material sicher anlag; leading = Übergang kein Material ->
    Material, wenn die Fahrt daneben begann; trailing = Kante für den Richtungswechsel.
    """

    def __init__(self, threshold):
        self.off_below = threshold              # kein Material: Dicke < Schwellwert
        self.on_from = threshold + HYST_MM      # Material: Dicke >= Schwellwert + 3 mm
        self.seen = False            # Material in dieser Fahrt sicher gefunden
        self.run_val = None          # Wert der laufenden Folge gleicher Proben
        self.run = []                # deren Positionen
        self.before = None           # letzte Position vor der laufenden Folge
        self.material = None
        self.leading = None
        self.trailing = None

    def feed(self, pos, thick):
        if thick >= self.on_from:
            on = True
        elif thick < self.off_below:
            on = False
        elif self.run_val is None:
            return                   # im Hysterese-Band, noch kein Zustand
        else:
            on = self.run_val        # im Hysterese-Band: voriger Zustand
        if on != self.run_val:
            if self.run:
                self.before = self.run[-1]
            self.run_val, self.run = on, []
        self.run.append(pos)
        span = abs(self.run[-1] - self.run[0])
        edge = (self.run[0] + self.before) / 2.0 if self.before is not None else self.run[0]
        if not self.seen:
            if on and len(self.run) >= EDGE_N and span >= MAT_MIN_MM:
                self.seen = True
                self.material = self.run[0]
                self.leading = edge if self.before is not None else None
            return
        if self.trailing is None and not on and len(self.run) >= EDGE_N and span >= EDGE_MM:
            self.trailing = edge


# ------------------------------------------------------------------ Ablaufschritte

def prepare(dev):
    """Referenz und HV herstellen (bereits bestätigt)."""
    p = dev.fresh()
    check_limits(p)
    why = interruption(p, False)
    if why:
        raise Interrupted(why)
    if not p["motion"]["homed"]:
        print("  Referenzfahrt")
        send(dev, "INIT", effect=lambda q: q["motion"]["state"] != "idle" or q["motion"]["homed"])
        wait_for(dev, lambda q: q["motion"]["homed"] and q["motion"]["state"] == "idle" and q["state"] == "READY",
                 80, "Referenzfahrt", False)
    p = dev.fresh()
    if not p["safety"]["hv_on"]:
        if not p["safety"]["hv_ok"]:
            raise Interrupted("HV-Freigabe fehlt: " + missing_hv(p))
        print("  Hochspannung (simuliert)")
        send(dev, "HV_ON", effect=lambda q: q["safety"]["hv_on"])
        wait_for(dev, lambda q: q["state"] == "RUNNING" and q["safety"]["hv_on"], 5, "HV_ON", False)


def segment(dev, start, end, speed):
    """START_TRAVERSE start -> end senden (start/end in der Messzone, >= 1 mm auseinander)."""
    dev.take()                                           # alte Proben verwerfen
    send(dev, "START_TRAVERSE", {"start": round(start, 1), "end": round(end, 1), "speed": speed},
         effect=lambda q: q["motion"]["mode"] == "traverse" and q["motion"]["state"] != "idle")


def traverse(dev, run_no, frm, bound, expect, slack, speed, threshold, lo, hi, writer):
    """Eine Fahrt mit Kantenverfolgung von frm in Richtung bound (Rand des Messbereichs).

    expect = erwartete Kante oder None, slack = zusätzlicher Überlauf dafür. Rückgabe (Richtung, Proben,
    vordere Kante, hintere Kante, Shutter offen geblieben). Bricht ab mit NoEdge, LimitHit,
    Interrupted.
    """
    sign = 1 if bound > frm else -1
    direction = "hin" if sign > 0 else "zurück"
    margin = max(MARGIN_MM, EDGE_MM + (EDGE_N + 2) * speed * POLL)
    target = bound
    if expect is not None:
        target = expect + sign * (margin + slack)
        if (target - bound) * sign >= 0 or abs(target - frm) < 1.0:
            target = bound
    edges = EdgeTracker(threshold)
    samples = []
    own_stop = False
    stopped_at_edge = False
    segment(dev, frm, target, speed)
    t_seg = time.time()
    timeout = abs(dev.now()["motion"]["pos"] - frm) / 10.0 + abs(target - frm) / speed + 15.0
    started = False
    while True:
        if time.time() - t_seg > timeout:
            raise Abort("Zeitüberschreitung in Fahrt %d (%.0f s)" % (run_no, timeout))
        for p in dev.take():
            check_limits(p)
            why = interruption(p, True)
            if why:
                raise Interrupted(why)
            m, q = p["motion"], p["measure"]
            if m["mode"] == "measure":
                raise Interrupted("Ein anderer Ablauf wurde gestartet (stehende Messung, vermutlich am Display)")
            if m["mode"] == "traverse":
                started = True
                if p["safety"]["shutter"] == "open":
                    samples.append(p)
                    writer.writerow([run_no, direction, p["ts"], m["pos"], q["dist"], q["thick"],
                                     int(q["on_material"]), int(q["valid"])])
                    settled = dev.open_ts is not None and p["ts"] - dev.open_ts >= SETTLE_MS
                    if q["valid"] and q["in_range"] and settled:
                        edges.feed(m["pos"], q["thick"])
                # Kante gefunden, Ziel noch weit weg (erste Fahrt oder Kante weiter innen): anhalten
                if edges.trailing is not None and not own_stop and abs(target - m["pos"]) > margin:
                    own_stop = stopped_at_edge = True
                    send(dev, "STOP")
                    dev.note(event="STOP an der Kante", run=run_no, edge=edges.trailing, pos=m["pos"])
                continue
            if not started and time.time() - t_seg < 2.0:
                continue                                  # Befehl noch nicht wirksam
            if m["state"] != "idle":
                if m["state"] == "moving" and m["mode"] == "none" and started:
                    raise Interrupted("Ein anderer Ablauf wurde gestartet (Fahrt, Park- oder Serviceposition, vermutlich am Display)")
                continue
            # Achse steht, keine Profilfahrt mehr
            if own_stop:
                return direction, samples, edges.leading, edges.trailing, False
            if abs(m["pos"] - target) >= 1.0:
                raise Interrupted("Fahrt bei %.1f mm vor dem Ziel %.1f mm gestoppt (STOP am Display oder von einem "
                                  "anderen Client)" % (m["pos"], target))
            if edges.trailing is not None:
                return direction, samples, edges.leading, edges.trailing, True
            if target != bound and abs(bound - m["pos"]) >= 1.0:   # Kante weiter außen als erwartet: bis zum Rand
                dev.note(event="Kante nicht am erwarteten Ort, weiter bis zum Rand", run=run_no, expect=expect)
                frm, target = target, bound
                segment(dev, frm, target, speed)
                t_seg = time.time()
                timeout = abs(target - frm) / speed + 15.0
                started = False
                break
            if not edges.seen:
                raise NoEdge("Kein Material im Messbereich %.0f ... %.0f mm gesehen (Fahrt %d %s bis %.0f mm)"
                             % (lo, hi, run_no, direction, bound))
            raise NoEdge("Keine Kante im Messbereich %.0f ... %.0f mm gefunden (Fahrt %d %s: Material ab %.1f mm "
                         "reicht bis an den Rand %.0f mm)" % (lo, hi, run_no, direction, edges.material, bound))


def summary(run_no, direction, samples, trail, width, stayed_open):
    parts = []
    for c, soll in zip(STEP_CENTRES, STEP_THICK):
        v = [s["measure"]["thick"] for s in samples if abs(s["motion"]["pos"] - c) <= 5 and s["measure"]["valid"]]
        if v:
            parts.append("%d:%.1f" % (soll, sum(v) / len(v)))
    width = ("Breite %.1f mm" % width) if width is not None else "Breite -"
    print("  Fahrt %d %-6s Kante %6.1f mm  %-14s %3d Punkte  Stufen soll:ist %s%s"
          % (run_no, direction, trail, width, len(samples),
             "  ".join(parts) or "-", "" if stayed_open else "  (STOP an der Kante, Shutter kurz zu)"), flush=True)


def recover(dev, reason):
    """Nach einer Unterbrechung sicher anhalten und durch den Wiederanlauf führen.
    Rückgabe erst, wenn der Bediener den Wiederanlauf bestätigt hat und der Zustand danach noch passt."""
    print("\n!!! UNTERBRECHUNG: %s" % reason, flush=True)
    dev.note(interruption=reason)
    safe_stop(dev)
    while True:
        p = dev.fresh()
        check_limits(p)
        print("    " + line(p))
        s = p["safety"]
        if s["estop"]:
            print("    Not-Aus ist gedrückt. Bitte entriegeln - ich warte ...", flush=True)
            while dev.fresh()["safety"]["estop"]:
                pass
            print("    Not-Aus entriegelt.")
            continue
        if p["state"] in ("FAULT", "WAIT_ACK"):
            print("    Fehler liegt an: %d %s" % (p["err"], SYS_ERR.get(p["err"], "?")))
            a = ask(dev, '    Ursache beseitigt und Anlage sicher? "ack" = ACK senden, '
                         'Enter = neu prüfen (z. B. am Display quittiert), "q" = beenden: ')
            if a == "q":
                raise Abort("Bediener beendet nach Unterbrechung")
            if a == "ack" and dev.fresh()["state"] == "WAIT_ACK":
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
            while dev.fresh()["local"]:
                time.sleep(0.5)
            continue
        if p["motion"]["state"] != "idle":
            print("    Die Achse fährt noch (Ablauf am Display?). Ich warte, bis sie steht ...", flush=True)
            while dev.fresh()["motion"]["state"] != "idle":
                pass
            continue
        if not s["hv_ok"]:
            print("    HV-Freigabe fehlt: " + missing_hv(p))
            a = ask(dev, '    Enter = neu prüfen, "q" = beenden: ')
            if a == "q":
                raise Abort("Bediener beendet: HV-Freigabe fehlt")
            continue
        plan = []
        if not p["motion"]["homed"]:
            plan.append("INIT (Referenz verloren)")
        plan += ["HV_ON (simuliert)", "Messfahrt wiederholen und Messung fortsetzen"]
        print("\n>>> Wiederanlauf: " + ", ".join(plan))
        print(">>> Die Achse bewegt sich. Ist niemand in Reichweite der Achse?")
        a = ask(dev, '>>> Wiederanlauf freigeben? "ja" = fortsetzen, alles andere beendet: ')
        if a != "ja":
            raise Abort("Wiederanlauf vom Bediener nicht freigegeben")
        q = dev.fresh()                                    # hat sich während der Eingabe etwas geändert?
        if (interruption(q, False) or q["motion"]["state"] != "idle" or q["motion"]["homed"] != p["motion"]["homed"]
                or not q["safety"]["hv_ok"]):
            print("    Der Zustand hat sich während der Eingabe geändert - neue Prüfung.")
            continue
        dev.note(resume="confirmed")
        return


# ------------------------------------------------------------------ Hauptablauf

def run(a):
    stamp = time.strftime("%Y%m%d_%H%M%S")
    csvname, logname = "scan_%s.csv" % stamp, "scan_%s.jsonl" % stamp
    dev = Device(a.host, logname)
    f = open(csvname, "w", newline="", encoding="utf-8")
    writer = csv.writer(f)
    writer.writerow(["run", "direction", "ts", "pos", "dist", "thick", "on_material", "valid"])
    lo, hi = min(a.start, a.end), max(a.start, a.end)
    runs_done = 0
    interruptions = 0
    armed = False
    widths = []
    try:
        dev.start()
        p = dev.now()
        print("Gerät http://%s\n  %s" % (a.host, line(p)))
        why = interruption(p, False)                       # z. B. noch nicht quittierter Fehler
        if why:
            raise Abort("Ausgangszustand nicht in Ordnung: " + why + " - erst beheben bzw. quittieren, dann neu starten")
        check_limits(p)
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
        plan.append("Profilfahrten mit Kantenverfolgung im Messbereich %g ... %g mm mit %g mm/s, %s" % (lo, hi, a.speed, cyc))
        plan.append("Kante: Dicke < %g mm, Material: Dicke >= %g mm" % (a.threshold, a.threshold + HYST_MM))
        plan.append("am Ende STOP, HV_OFF")
        print("\n>>> Ablauf: " + "; ".join(plan))
        print(">>> Die Achse bewegt sich. Ist niemand in Reichweite der Achse?")
        if ask(dev, '>>> Starten? "ja" eingeben, alles andere bricht ab: ') != "ja":
            raise Abort("Start nicht bestätigt")
        armed = True
        dev.note(start={"cycles": a.cycles, "speed": a.speed, "lo": lo, "hi": hi})
        total = 2 * a.cycles
        frm, bound, expect, slack = a.start, a.end, None, 0.0   # erste Fahrt: von --start Richtung --end
        edge_at = {}                                       # Rand des Messbereichs -> zuletzt gefundene Kante dort
        while total == 0 or runs_done < total:
            try:
                prepare(dev)
                while total == 0 or runs_done < total:
                    direction, samples, lead, trail, stayed_open = traverse(
                        dev, runs_done + 1, frm, bound, expect, slack, a.speed, a.threshold, lo, hi, writer)
                    f.flush()
                    runs_done += 1
                    edge_at[bound] = trail
                    width = abs(edge_at[lo] - edge_at[hi]) if lo in edge_at and hi in edge_at else None
                    if width is not None:
                        widths.append(width)
                    dev.note(run=runs_done, direction=direction, leading=lead, trailing=trail, width=width)
                    summary(runs_done, direction, samples, trail, width, stayed_open)
                    # Richtungswechsel: von der Stelle, an der die Achse steht, zum anderen Rand.
                    # Erwartete Kante dort: die zuletzt dort gefundene (Richtungswechsel-)Kante, sonst
                    # die vordere Kante dieser Fahrt mit zusätzlichem Überlauf.
                    pos = dev.fresh()["motion"]["pos"]
                    frm = min(max(pos, lo), hi)
                    bound = lo if bound == hi else hi
                    if bound in edge_at:
                        expect, slack = edge_at[bound], 0.0
                    else:
                        expect, slack = lead, LEAD_SLACK_MM
            except Interrupted as ex:
                interruptions += 1
                recover(dev, str(ex))
                # unterbrochene Fahrt wiederholen: vom nächsten Rand aus, ohne erwartete Kante
                pos = dev.fresh()["motion"]["pos"]
                frm = min(max(pos, lo), hi)
                if abs(frm - bound) < 1.0:
                    bound = lo if bound == hi else hi
                expect, slack = edge_at.get(bound), 0.0
        print("\nAlle %d Fahrten abgeschlossen." % runs_done)
    except KeyboardInterrupt:
        print("\n!!! Strg+C / SIGTERM - Messung wird beendet")
        dev.note(abort="Strg+C/SIGTERM")
    except LimitHit as ex:
        print("\n!!! ENDLAGE: %s - Messung gestoppt" % ex)
        dev.note(abort=str(ex))
    except NoEdge as ex:
        print("\n!!! %s - Messung gestoppt" % ex)
        dev.note(abort=str(ex))
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
                time.sleep(2 * POLL)
                p = dev.latest
                if p:
                    print("  Endzustand: " + line(p))
            except Exception:  # noqa: BLE001
                pass
        dev.stop()
        f.close()
        w = (" Breite Mittel %.1f mm (%d Fahrten)" % (sum(widths) / len(widths), len(widths))) if widths else ""
        print("\nFahrten: %d   Unterbrechungen: %d   Heartbeat: %d/%d beantwortet, längste Lücke %.2f s%s"
              % (runs_done, interruptions, dev.hb_ok, dev.hb_sent, dev.hb_max_gap, w))
        dev.note(summary={"runs": runs_done, "interruptions": interruptions, "hb_sent": dev.hb_sent,
                          "hb_ok": dev.hb_ok, "hb_max_gap_s": round(dev.hb_max_gap, 2)})
        dev.log.close()
        print("Dateien: %s, %s" % (csvname, logname))


def main():
    ap = argparse.ArgumentParser(description="Profilmessung mit Kantenverfolgung (Demoanlage ADD Slim)")
    ap.add_argument("--host", default="192.168.1.120", help="IP-Adresse im LAN (Standard 192.168.1.120)")
    ap.add_argument("--cycles", type=int, default=1, help="Zyklen (hin + zurück); 0 = endlos bis Strg+C")
    ap.add_argument("--speed", type=float, default=10.0, help="Messgeschwindigkeit 1 ... 20 mm/s (Standard 10)")
    ap.add_argument("--start", type=float, default=80.0, help="Messbereich, Anfang > 60 ... 242 mm (Standard 80)")
    ap.add_argument("--end", type=float, default=240.0, help="Messbereich, Ende > 60 ... 242 mm (Standard 240)")
    ap.add_argument("--threshold", type=float, default=1.0,
                    help="Schwellwert der Kante in mm Dicke: kein Material unter dem Schwellwert, Material ab "
                         "Schwellwert + 3 mm (Standard 1.0 -> Material ab 4 mm, die dünnste Stufe hat 5 mm)")
    a = ap.parse_args()
    if not 0 < a.threshold <= 50:
        ap.error("--threshold muss > 0 und <= 50 mm sein")
    if a.cycles < 0:
        ap.error("--cycles muss >= 0 sein")
    if not 1 <= a.speed <= 20:
        ap.error("--speed muss 1 ... 20 mm/s sein")
    for n, v in (("--start", a.start), ("--end", a.end)):
        if not 60 < v <= 242:
            ap.error("%s muss > 60 und <= 242 mm sein (Messzone)" % n)
    if abs(a.end - a.start) < 10:
        ap.error("Der Messbereich --start ... --end muss mindestens 10 mm breit sein")
    selftest()

    def on_term(signum, frame_):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, on_term)
    run(a)


if __name__ == "__main__":
    sys.exit(main())
