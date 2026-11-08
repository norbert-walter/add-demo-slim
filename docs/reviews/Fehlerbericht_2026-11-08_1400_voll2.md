# Abnahme A1 – Fehlerbericht

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 14:00 / 14:34 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | f17c52d A1.md: Prüfungsnummern S/M nachgezogen |
| Aufruf | `--host 192.168.1.120 --tag voll2` |
| Ergebnis gesamt | **31 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 8 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 7 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 3:42 min |
| Ergebnis Block 2 – weitere Handprüfungen | 6 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 1 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 13:19 min |
| Ergebnis Block 3 – automatische Prüfungen | 18 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 7 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 16:34 min |

### Symbole

| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |

## Block 2 – weitere Handprüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| M06 | 2 | Abstand neben dem Messgut plausibel (Maßband, ±5 %) | ❌ **nicht bestanden** | Abweichung -53.3 mm (zulässig ±6.2 mm) | 73 s |

## Block 3 – automatische Prüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ❌ **nicht bestanden** | Stufe 115 mm: CALIB point -> NOT_STABLE | 15 s |
| T18 | 2, 8 | Stehende Messung je Stufe: Dicke und Abstand ±1 mm, 3σ ≤ 1 mm (Mittelwert, Sigma) | ❌ **nicht bestanden** | außerhalb der Toleranz: 115 mm, 135 mm | 45 s |
| T19 | 8 | Profilfahrt hin und zurück: Kanten in beiden Richtungen, Stufen ±1 mm, Shutter durchgehend offen | ❌ **nicht bestanden** | hin 115 mm; hin 135 mm; zurück 115 mm; Kante Messgut ab (hin 80.9, zurück 80.2); Kante Messgut bis (hin 239.7, zurück 239.6) | 78 s |
| T20 | 8 | Kalibriertabelle (6 Stufen) gespeichert und wirksam: Abweichung je Stufe ±1 mm (Mittelwert, Sigma) | ❌ **nicht bestanden** | Kalibrierung nicht wirksam an: 115 mm, 135 mm | 0 s |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort | ❌ **nicht bestanden** | stress_api parallel:     raise URLError(err) / urllib.error.URLError: <urlopen error timed out> | 12 s |
| T27 | 4 | Zeitverhalten 10 min mit dauernd geladener Weboberfläche (soak --web) | ❌ **nicht bestanden** | stress_api soak:     net: http 206 ms / RESULT: FAIL | 602 s |
| T29 | 4 | Zeitwerte ≤ 3000 µs (inkl. Summer-Signale aus Block 1 und 2) | ❌ **nicht bestanden** | seit dem letzten Neustart | – |

## Messwerte

Nur die Werte außerhalb des Solls.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ❌ | M06 | Abweichung Anlage – Maßband | -53.3 | mm | 0 ± 6.2 (5 %) |
| ❌ | T18 | 115 mm: Abstand Mittelwert | 62.79 | mm | 65 ± 1 |
| ❌ | T18 | 115 mm: Dicke Mittelwert | 7.21 | mm | 5 ± 1 |
| ❌ | T18 | 115 mm: Dicke Mittelwert der Anlage (avg) | 7.1 | mm | 5 ± 1 |
| ❌ | T18 | 115 mm: Dicke Sigma | 0.615 | mm | ≤ 0.33 (3σ ≤ 1) |
| ❌ | T18 | 135 mm: Abstand Mittelwert | 58.977 | mm | 60 ± 1 |
| ❌ | T18 | 135 mm: Dicke Mittelwert | 11.022 | mm | 10 ± 1 |
| ❌ | T19 | hin: Stufe 115 mm: Dicke (Median) | 7.1 | mm | 5 ± 1 |
| ❌ | T19 | hin: Stufe 135 mm: Dicke (Median) | 11.2 | mm | 10 ± 1 |
| ❌ | T19 | zurück: Stufe 115 mm: Dicke (Median) | 7.2 | mm | 5 ± 1 |
| ❌ | T19 | Kante Messgut ab: Mittel hin/zurück | 80.55 | mm | 105 ± 3 |
| ❌ | T19 | Kante Messgut bis: Mittel hin/zurück | 239.65 | mm | 225 ± 3 |
| ❌ | T20 | Stufe 115 mm: Abweichung Abstand (Mittelwert – 65 mm) | -2.21 | mm | 0 ± 1 |
| ❌ | T20 | Stufe 135 mm: Abweichung Abstand (Mittelwert – 60 mm) | -1.023 | mm | 0 ± 1 |
| ❌ | T29 | period_max_us | 3008 | µs | ≤ 3000 |

## Anhang

### GET_STATE am Ende

```json
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2882, "period_max_us": 3008, "overruns": 2768, "prio_lat_max_us": 1744, "crc": "0D866056"}
```

### /log am Ende

```
net: http 204 ms
net: http 201 ms
net: http 705 ms
net: http 258 ms
net: http 210 ms
ctrl: cycle 2058 us, period 1654 us (adc 680, tick 1231, cmd 27, out 10, pub 110), 1 events
net: http 726 ms
net: http 236 ms
net: http 209 ms
net: http 207 ms
net: http 705 ms
net: http 255 ms
net: http 207 ms
net: http 731 ms
net: http 238 ms
net: http 706 ms
net: http 261 ms
net: http 729 ms
net: http 232 ms
net: http 206 ms
```

### T18 – Einzelwerte Stufe 115 mm (Soll Abstand 65.0, Dicke 5.0)

```
Abstand [mm]: 64.3 63.2 63.5 62.7 63.4 62.3 63.0 62.2 62.7 63.1 62.2 62.8 62.0 62.1 61.8 61.1 62.5 62.6 63.4 63.0 63.7 63.4 63.5 63.7 62.8 63.0 62.6 63.4 62.9 63.0 63.1 62.3 62.7 62.1 62.9 63.1 62.5 62.4 62.0 62.6
Dicke [mm]:   5.7 6.8 6.5 7.3 6.6 7.7 7.0 7.8 7.3 6.9 7.8 7.2 8.0 7.9 8.2 8.9 7.5 7.4 6.6 7.0 6.3 6.6 6.5 6.3 7.2 7.0 7.4 6.6 7.1 7.0 6.9 7.7 7.3 7.9 7.1 6.9 7.5 7.6 8.0 7.4
```

### T18 – Einzelwerte Stufe 135 mm (Soll Abstand 60.0, Dicke 10.0)

```
Abstand [mm]: 59.5 59.5 59.3 58.9 59.2 58.9 59.2 59.2 59.3 59.1 58.6 58.9 58.5 58.9 58.5 58.5 58.9 58.6 58.7 58.4 59.0 59.3 58.8 59.3 59.1 59.3 59.0 59.0 59.3 59.3 59.3 58.9 59.0 58.6 58.8 59.0 59.0 59.0 58.8 58.7
Dicke [mm]:   10.5 10.5 10.7 11.1 10.8 11.1 10.8 10.8 10.7 10.9 11.4 11.1 11.5 11.1 11.5 11.5 11.1 11.4 11.3 11.6 11.0 10.7 11.2 10.7 10.9 10.7 11.0 11.0 10.7 10.7 10.7 11.1 11.0 11.4 11.2 11.0 11.0 11.0 11.2 11.3
```

### T18 – Einzelwerte Stufe 155 mm (Soll Abstand 55.0, Dicke 15.0)

```
Abstand [mm]: 54.9 54.7 54.5 54.7 54.6 54.7 54.7 54.8 54.8 54.6 54.6 54.2 54.4 54.4 54.0 54.3 54.2 54.3 54.4 54.3 54.4 54.0 54.2 54.6 54.7 54.6 54.5 54.6 54.7 54.7 54.6 54.6 54.7 54.6 54.5 54.3 54.4 54.4 54.2 54.3
Dicke [mm]:   15.1 15.3 15.5 15.3 15.4 15.3 15.3 15.2 15.2 15.4 15.4 15.8 15.6 15.6 16.0 15.7 15.8 15.7 15.6 15.7 15.6 16.0 15.8 15.4 15.3 15.4 15.5 15.4 15.3 15.3 15.4 15.4 15.3 15.4 15.5 15.7 15.6 15.6 15.8 15.7
```

### T18 – Einzelwerte Stufe 175 mm (Soll Abstand 50.0, Dicke 20.0)

```
Abstand [mm]: 49.9 49.8 49.9 49.9 49.9 49.9 49.9 49.8 49.8 49.9 49.8 49.9 49.7 49.6 49.7 49.6 49.7 49.6 49.6 49.6 49.7 49.7 49.6 49.8 49.8 49.8 49.8 49.9 50.0 50.0 49.9 49.9 50.0 49.9 49.9 49.8 49.7 49.6 49.5 49.6
Dicke [mm]:   20.1 20.2 20.1 20.1 20.1 20.1 20.1 20.2 20.2 20.1 20.2 20.1 20.3 20.4 20.3 20.4 20.3 20.4 20.4 20.4 20.3 20.3 20.4 20.2 20.2 20.2 20.2 20.1 20.0 20.0 20.1 20.1 20.0 20.1 20.1 20.2 20.3 20.4 20.5 20.4
```

### T18 – Einzelwerte Stufe 195 mm (Soll Abstand 45.0, Dicke 25.0)

```
Abstand [mm]: 44.4 44.4 44.4 44.5 44.5 44.6 44.5 44.6 44.6 44.5 44.6 44.6 44.5 44.5 44.3 44.2 44.2 44.3 44.3 44.3 44.3 44.2 44.3 44.3 44.3 44.4 44.5 44.4 44.5 44.5 44.5 44.5 44.5 44.5 44.5 44.6 44.6 44.5 44.4 44.2
Dicke [mm]:   25.6 25.6 25.6 25.5 25.5 25.4 25.5 25.4 25.4 25.5 25.4 25.4 25.5 25.5 25.7 25.8 25.8 25.7 25.7 25.7 25.7 25.8 25.7 25.7 25.7 25.6 25.5 25.6 25.5 25.5 25.5 25.5 25.5 25.5 25.5 25.4 25.4 25.5 25.6 25.8
```

### T18 – Einzelwerte Stufe 215 mm (Soll Abstand 40.0, Dicke 30.0)

```
Abstand [mm]: 39.7 39.6 39.5 39.6 39.6 39.7 39.7 39.8 39.8 39.8 39.8 39.8 39.8 39.8 39.7 39.7 39.7 39.6 39.6 39.6 39.6 39.6 39.6 39.6 39.6 39.5 39.5 39.6 39.7 39.7 39.8 39.8 39.8 39.7 39.7 39.8 39.8 39.7 39.6 39.6
Dicke [mm]:   30.3 30.4 30.5 30.4 30.4 30.3 30.3 30.2 30.2 30.2 30.2 30.2 30.2 30.2 30.3 30.3 30.3 30.4 30.4 30.4 30.4 30.4 30.4 30.4 30.4 30.5 30.5 30.4 30.3 30.3 30.2 30.2 30.2 30.3 30.3 30.2 30.2 30.3 30.4 30.4
```

### T26 – stress_api.py parallel --n 400 --threads 8

```
14:23:50 192.168.1.120: parallel: 400 mixed commands from 8 threads, every reply checked (about 10 ... 30 s)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3000; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  slowest /api round trip: 7377.5 ms (HV_OFF)
  device log (ctrl/net/comm/http/sta):
    sta: attempt 1/3
    sta: connected 192.168.1.120
    net: http 211 ms
    comm: timeout, last /api packet 4994 ms ago, longest gap while armed 97 ms
    ctrl: cycle 452 us, period 2715 us (adc 167, tick 159, cmd 32, out 5, pub 89), 1 events
Traceback (most recent call last):
  File "/usr/lib/python3.10/urllib/request.py", line 1348, in do_open
    h.request(req.get_method(), req.selector, req.data, headers,
  File "/usr/lib/python3.10/http/client.py", line 1312, in request
    self._send_request(method, url, body, headers, encode_chunked)
  File "/usr/lib/python3.10/http/client.py", line 1358, in _send_request
    self.endheaders(body, encode_chunked=encode_chunked)
  File "/usr/lib/python3.10/http/client.py", line 1307, in endheaders
    self._send_output(message_body, encode_chunked=encode_chunked)
  File "/usr/lib/python3.10/http/client.py", line 1067, in _send_output
    self.send(msg)
  File "/usr/lib/python3.10/http/client.py", line 1005, in send
    self.connect()
  File "/usr/lib/python3.10/http/client.py", line 971, in connect
    self.sock = self._create_connection(
  File "/usr/lib/python3.10/socket.py", line 845, in create_connection
    raise err
  File "/usr/lib/python3.10/socket.py", line 833, in create_connection
    sock.connect(sa)
TimeoutError: timed out

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 390, in <module>
    main()
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 378, in main
    ok = t_parallel(a.host, a.n or 400, a.threads)
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 243, in t_parallel
    for cmd, exp, r, dt in ex.map(one, jobs):
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 621, in result_iterator
    yield _result_or_cancel(fs.pop())
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 319, in _result_or_cancel
    return fut.result(timeout)
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 458, in result
    return self.__get_result()
  File "/usr/lib/python3.10/concurrent/futures/_base.py", line 403, in __get_result
    raise self._exception
  File "/usr/lib/python3.10/concurrent/futures/thread.py", line 58, in run
    result = self.fn(*self.args, **self.kwargs)
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 238, in one
    r, dt = api(host, cmd, params, timeout=10.0)
  File "/home/norbert/add-demo-slim/scripts/stress_api.py", line 68, in api
    txt = http(host, "/api", build(cmd, params or {}), timeout=timeout)
  File "/home/norbert/add-demo-slim/scripts/add_client.py", line 66, in http
    with urllib.request.urlopen(req, timeout=timeout) as r:
  File "/usr/lib/python3.10/urllib/request.py", line 216, in urlopen
    return opener.open(url, data, timeout)
  File "/usr/lib/python3.10/urllib/request.py", line 519, in open
    response = self._open(req, data)
  File "/usr/lib/python3.10/urllib/request.py", line 536, in _open
    result = self._call_chain(self.handle_open, protocol, protocol +
  File "/usr/lib/python3.10/urllib/request.py", line 496, in _call_chain
    result = func(*args)
  File "/usr/lib/python3.10/urllib/request.py", line 1377, in http_open
    return self.do_open(http.client.HTTPConnection, req)
  File "/usr/lib/python3.10/urllib/request.py", line 1351, in do_open
    raise URLError(err)
urllib.error.URLError: <urlopen error timed out>
```

### T27 – stress_api.py soak --minutes 10 --web

```
14:24:01 192.168.1.120: soak: GET_STATE every second for 10 min, meanwhile the web UI files are loaded without pause; a line every minute, a dot every 10 s; Ctrl+C aborts
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3000; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
.....
  1 min: cycles over 1 ms +228, cycle_max_us=1937, period_max_us=2715, web files 221 ok / 0 failed
.....
  2 min: cycles over 1 ms +473, cycle_max_us=1991, period_max_us=2715, web files 441 ok / 0 failed
.....
  3 min: cycles over 1 ms +704, cycle_max_us=2148, period_max_us=2715, web files 659 ok / 0 failed
....
  over the limit: period_max_us=3008 after 228 s
.
  4 min: cycles over 1 ms +942, cycle_max_us=2882, period_max_us=3008, web files 883 ok / 0 failed
.....
  5 min: cycles over 1 ms +1191, cycle_max_us=2882, period_max_us=3008, web files 1103 ok / 0 failed
.....
  6 min: cycles over 1 ms +1410, cycle_max_us=2882, period_max_us=3008, web files 1323 ok / 0 failed
.....
  7 min: cycles over 1 ms +1631, cycle_max_us=2882, period_max_us=3008, web files 1541 ok / 0 failed
.....
  8 min: cycles over 1 ms +1861, cycle_max_us=2882, period_max_us=3008, web files 1761 ok / 0 failed
.....
  9 min: cycles over 1 ms +2088, cycle_max_us=2882, period_max_us=3008, web files 1981 ok / 0 failed
.....
  10 min: cycles over 1 ms +2325, cycle_max_us=2882, period_max_us=3008, web files 2199 ok / 0 failed

  472 heartbeats, web load: 2204 files (79313 kB) ok, 0 failed
  cycles over 1 ms +2329 (info; since boot ready 2757) cycle_max_us=2882 period_max_us=3008 (!) prio_lat_max_us=1744 -> FAIL
  slowest /api round trip: 778.2 ms (GET_STATE)
  device log (ctrl/net/comm/http/sta):
    net: http 204 ms
    net: http 201 ms
    net: http 705 ms
    net: http 258 ms
    net: http 210 ms
    ctrl: cycle 2058 us, period 1654 us (adc 680, tick 1231, cmd 27, out 10, pub 110), 1 events
    net: http 726 ms
    net: http 236 ms
    net: http 209 ms
    net: http 207 ms
    net: http 705 ms
    net: http 255 ms
    net: http 207 ms
    net: http 731 ms
    net: http 238 ms
    net: http 706 ms
    net: http 261 ms
    net: http 729 ms
    net: http 232 ms
    net: http 206 ms
RESULT: FAIL
```

