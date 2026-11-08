# Abnahme A1 – Fehlerbericht

| | |
|---|---|
| Prüfer | Norbert Walter |
| Beginn / Ende | 08.11.2026 15:19 / 15:27 |
| Gerät | http://192.168.1.120/ |
| Firmware | – |
| Repository | c22b0b3 Abnahmegrenzen an den optischen Sensor angepasst (Pflichtenheft V1.7.16) |
| Aufruf | `--host 192.168.1.120 --only M06,M11,T18K,T18,T19,T26 --tag wiederholung` |
| Ergebnis gesamt | **12 ✅ bestanden (davon 2 mit ⚠️ Achtung) · 3 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen** |
| Ergebnis Block 1 – Sicherheit | 7 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 3:24 min |
| Ergebnis Block 2 – weitere Handprüfungen | 2 ✅ bestanden (davon 0 mit ⚠️ Achtung) · 0 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 1:58 min |
| Ergebnis Block 3 – automatische Prüfungen | 3 ✅ bestanden (davon 2 mit ⚠️ Achtung) · 3 ❌ nicht bestanden · 0 ⛔ Problem · 0 ⏭ übersprungen · Dauer 2:55 min |

### Symbole

| Symbol | Bedeutung |
|---|---|
| ✅ | bestanden |
| ❌ | nicht bestanden |
| ⏭ | übersprungen / nicht geprüft |
| ⛔ | Problem: eine Voraussetzung oder Randbedingung stimmt nicht, die Prüfung konnte nicht sinnvoll zu Ende geführt werden |
| ⚠️ | Achtung: Messwert nah am Grenzwert (mehr als 80 % der Toleranz ausgenutzt bzw. im äußeren Zehntel eines Bereichs); kann zusammen mit ✅ auftreten |
| – | Information, kein Sollwert |

## Block 3 – automatische Prüfungen

| ID | Block | Prüfung | Ergebnis | Bemerkung | Dauer |
|---|---|---|---|---|---|
| T18K | 8 | Kalibrierung unter Messbedingungen vor der Messung: 6 Stufen, HV an, Shutter durchgehend offen, gespeichert | ❌ **nicht bestanden** | Stufe 115 mm: CALIB point -> NOT_STABLE | 17 s |
| T19 | 8 | Profilfahrt hin und zurück: Kanten erkannt und aufgezeichnet, Stufen ±1 mm, Shutter durchgehend offen | ❌ **nicht bestanden** | hin: Kante Messgut ab nicht erkannt (81.1); hin: Kante Messgut bis nicht erkannt (238.9); zurück: Kante Messgut ab nicht erkannt (91.4); zurück: Kante Messgut bis nicht erkannt (239.6) | 78 s |
| T26 | 4 | Parallele Anfragen (8 Threads, 400 Befehle): keine vertauschte Antwort | ❌ **nicht bestanden** | stress_api parallel:     raise URLError(err) / urllib.error.URLError: <urlopen error timed out> | 12 s |

## Messwerte

Nur die Werte außerhalb des Solls.

| | ID | Größe | Wert | Einheit | Soll |
|---|---|---|---|---|---|
| ❌ | T19 | hin: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 81.1 | mm | 105 ± 10 |
| ❌ | T19 | hin: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 238.9 | mm | 225 ± 10 |
| ❌ | T19 | zurück: Kante Messgut ab erkannt (Aufzeichnung, grob 105 ± 10) | 91.4 | mm | 105 ± 10 |
| ❌ | T19 | zurück: Kante Messgut bis erkannt (Aufzeichnung, grob 225 ± 10) | 239.6 | mm | 225 ± 10 |

## Anhang

### GET_STATE am Ende

```json
{"status": "ok", "err": 0, "cmd": "GET_STATE", "state": "READY", "sys_err": 0, "local": false, "cycle_max_us": 2882, "period_max_us": 3008, "overruns": 3382, "prio_lat_max_us": 1744, "crc": "BBE8B3E9"}
```

### /log am Ende

```
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
net: http 605 ms
nvs: save deferred until axis idle and HV off
comm: /api gap 4146 ms
comm: /api gap 2070 ms
comm: /api gap 2093 ms
comm: timeout, last /api packet 5002 ms ago, longest gap while armed 4146 ms
nvs: saved
http: idle connection closed after 500 ms (1 so far)
ctrl: cycle 464 us, period 2514 us (adc 151, tick 211, cmd 20, out 9, pub 73), 1 events
ctrl: cycle 529 us, period 2654 us (adc 202, tick 221, cmd 21, out 9, pub 76), 1 events
```

### T26 – stress_api.py parallel --n 400 --threads 8

```
15:27:21 192.168.1.120: parallel: 400 mixed commands from 8 threads, every reply checked (about 10 ... 30 s)
  limits: cycle_max_us, period_max_us, prio_lat_max_us <= 3100; no COMM_TIMEOUT; cycles over 1 ms (overruns) = info
  slowest /api round trip: 7317.0 ms (HV_OFF)
  device log (ctrl/net/comm/http/sta):
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
    net: http 605 ms
    comm: /api gap 4146 ms
    comm: /api gap 2070 ms
    comm: /api gap 2093 ms
    comm: timeout, last /api packet 5002 ms ago, longest gap while armed 4146 ms
    http: idle connection closed after 500 ms (1 so far)
    ctrl: cycle 464 us, period 2514 us (adc 151, tick 211, cmd 20, out 9, pub 73), 1 events
    ctrl: cycle 529 us, period 2654 us (adc 202, tick 221, cmd 21, out 9, pub 76), 1 events
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

