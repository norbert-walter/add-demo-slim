# ADD Style Guide – IMS Operator UI

**AI Device Description — style guide for external operator pages**
Version 1.0 · 2026-11-07 · Norbert Walter
Reference implementation: `data/www/index.html` (IMS operator UI, CONTRACT V1.5.26 §5.5)

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Design Principles](#2-design-principles)
3. [Layout](#3-layout)
4. [Colours](#4-colours)
5. [Typography](#5-typography)
6. [Components](#6-components)
7. [Page Patterns](#7-page-patterns)
8. [Device Communication](#8-device-communication)
9. [Starter Template](#9-starter-template)
10. [Checklist for a New Page](#10-checklist-for-a-new-page)
- [Appendix A — CSS Token Block](#appendix-a--css-token-block)
- [Appendix B — Changelog](#appendix-b--changelog)

---

## 1. Introduction

### 1.1 Purpose

This guide describes the look and the behaviour of the **IMS operator UI** of the ADD demo plant,
so that new web pages can be built in the same style: additional operator pages, service tools,
dashboards for presentations, or pages that an AI agent generates from an ADD document.

A page that follows this guide

- looks like the IMS operator UI (same layout, colours, fonts and components),
- talks to the device in the same way (`/proc`, `/api`, CRC32 frame),
- follows the same safety rules (STOP always available, confirmations, no automatic repetition),
- works **as a local file** (opened from disk, connection dialog) and, if desired, **served by the
  device** (same file, no dialog).

### 1.2 Relationship to other documents

| Document | Role |
|---|---|
| `docs/reference/ADD_Style_Guide_v1_0.md` | ADD Dashboard Style Guide v1.0 – generic, brand-neutral dashboards with themes and instruments. Still valid for generic ADD dashboards |
| **this guide** | IMS industrial operator style: one dark colour scheme, navigation rail, alarm header. Use it for pages that belong to the demo plant or to IMS-style devices |
| `docs/CONTRACT.md` §4, §5 | binding definition of the CRC32 frame, `/proc`, `/api`, error codes |
| `data/www/app.js` (ADDAPP) | protocol and rule functions; external pages embed a copy |
| `data/www/index.html` | reference implementation of everything in this guide |

Where this guide and the CONTRACT disagree on protocol details, the **CONTRACT wins**.

### 1.3 How AI agents use this document

An agent that builds a page from an ADD document:

1. takes the shell (§3), the token block (Appendix A) and the components (§6) unchanged,
2. reads the device's `/add` document to decide which values and commands the page shows,
3. maps live values to the material bar, panels and charts (§7),
4. maps commands to buttons and applies the rules of §8 (confirmation, STOP, heartbeat),
5. checks the result against the checklist in §10.

---

## 2. Design Principles

1. **Operator first.** The page serves a person running a machine. State, alarms and the STOP
   button are always visible; decoration is not.
2. **Colour means state.** Green, yellow, red and cyan are reserved for state (§4.2). Everything
   else uses the blue scale of the scheme. Never use a status colour for decoration.
3. **One colour scheme.** Dark blue, no themes. The page sets `color-scheme: dark` and ignores the
   browser's light preference (a control room is dimmed, and a stable appearance is part of safe
   operation).
4. **Numbers are stable.** All values use tabular figures (`font-variant-numeric: tabular-nums`),
   fixed decimals per quantity and a unit. A value that is not valid is shown as `–`, never as an
   old number.
5. **Dense but calm.** Base font size 13 px, compact panels with 3 px radius, 1 px borders. Space
   is spent on values, not on margins.
6. **No internet required.** No CDN, no external fonts or libraries. The device may run as its own
   access point without internet.
7. **Language.** The operator-facing text is in the language of the operators (the demo plant uses
   German). Identifiers, commands, state names (`READY`, `HV_ON`) and code comments stay in English.

---

## 3. Layout

### 3.1 Shell

```
┌──────┬──────────────────────────────────────────────────────────────────────┐
│ LOGO │ [⚠ Alarm title / detail line          ] [Status box][Status box] ID   │ header 50 px
│ 60px ├──────────────────────────────────────────────────────────────────────┤
│      │ ☆ Page title  DEMO                         ● Verbunden   12:44:05     │ sub-bar 38 px
│ nav  ├──────────────────────────────────────────────────────────────────────┤
│ rail │                                                                      │
│      │                         page content                                 │
│ 60px │                     (padding 12px 16px, scrolls)                     │
│      │                                                                      │
│ ──── │                                                                      │
│ Log  │                                                                      │
│ Conn │                                                                      │
└──────┴──────────────────────────────────────────────────────────────────────┘
```

| Part | Class | Size | Content |
|---|---|---|---|
| navigation rail | `.nav-rail` | 60 px wide, full height | logo (60 × 60), page buttons, bottom group (log, connection) |
| header | `.topbar` | 50 px high | alarm box (flexible), status boxes, device id, brand mark |
| sub-bar | `.subbar` | 38 px high | page title, DEMO badge, connection indicator, clock |
| content | `.page-content` | rest | one `div.page-content` per page, only the `.active` one is shown |

The body does not scroll; only `.page-content` scrolls. All pages live in **one HTML file**; the
navigation switches pages and writes the page name into the URL hash (`#process`, `#settings`), so
a reload or a bookmark opens the same page.

### 3.2 Grids

| Grid | Definition | Used for |
|---|---|---|
| `.process-grid` | `grid-template-columns: 250px 1fr; gap: 10px` | left column of panels and buttons, right column of charts |
| `.material-bar` | `repeat(auto-fit, minmax(120px, 1fr))`, spans all columns | row of key values on top of the main page |
| `.settings-grid` | `repeat(auto-fit, minmax(320px, 1fr)); gap: 12px` | settings and status panels |
| `.calib-grid` | `1fr 1fr; gap: 12px` | calibration steps beside table and chart |

### 3.3 Responsive behaviour

One breakpoint at **700 px**:

- all grids become one column,
- the status boxes and the brand mark in the header are hidden,
- the labels under the navigation icons are hidden (tooltips remain),
- the material bar shows two columns.

The layout is designed for 1280 × 800 (tablet landscape, laptop) and stays usable down to a phone
in portrait.

---

## 4. Colours

### 4.1 Tokens

All colours are CSS custom properties on `:root` (full block in Appendix A). Pages use the tokens,
never raw hex values in markup. Canvas drawings, which cannot read CSS variables cheaply, use the
hex values from this table.

| Token | Value | Use |
|---|---|---|
| `--ims-blue` | `#005A9A` | brand blue (logo) |
| `--ims-orange` | `#F39200` | brand orange (logo, DEMO badge, highlight points in charts) |
| `--ims-silver` | `#D0D0D0` | brand silver (rarely used) |
| `--nav-bg` | `#07151c` | navigation rail, scrollbar track |
| `--nav-active` | `#005F8F` | background of the active navigation item |
| `--nav-icon` | `#CDE6F9` | navigation icons |
| `--nav-icon-active` | `#00B6F8` | active icon and its 3 px left marker |
| `--nav-icon-hover` | `#FFA104` | hover colour of icons and header icons |
| `--header-bg` | `#004578` | header, logo cell |
| `--header-fg` | `#CDE6F9` | header text |
| `--page-bg` | `#003759` | page and sub-bar background |
| `--panel-bg` | `#004578` | panels, modal boxes |
| `--panel-border` | `#3B6481` | panel borders, table lines, dividers |
| `--panel-header-bg` | `#002847` | panel headers, table headers |
| `--fg` | `#CDE6F9` | normal text and values |
| `--fg-dim` | `#6184A0` | labels, units, secondary text, chart axes |
| `--fg-muted` | `#4A7CA0` | inactive indicators |
| `--input-bg` | `#003256` | input fields |
| `--input-border` | `#8099AB` | input borders |
| `--input-fg` | `#F5FBFF` | input text |
| `--btn-bg` | `#337BAE` | buttons |
| `--btn-fg` | `#CDE6F9` | button text |
| `--btn-bg-disabled` | `#005694` | disabled button |
| `--btn-fg-disabled` | `#6184A0` | disabled button text |
| `--btn-hover-border` | `#FFA104` | hover border of buttons and inputs |
| `--ok` | `#03CA15` | state good / safe |
| `--warn` | `#F0F000` | attention, transitional state, limit line |
| `--err` | `#FF1010` | fault, danger, hazard active |
| `--info` | `#00B6F8` | primary measured value, focus, progress |
| `--table-even` | `#003452` | even table rows, step items, log area |
| `--chart-grid` | `#1F5E8E` | chart grid lines, gauge track |

### 4.2 State colours

| Colour | Meaning | Examples in the reference page |
|---|---|---|
| `--ok` green | safe, ready, running normally | state `READY`/`RUNNING`, HV off, shutter closed, connection online |
| `--warn` yellow | attention, waiting, not yet ready | `SAFE_START`, `HOMING`, shutter open, warning badges |
| `--err` red | fault or active hazard | `FAULT`, `WAIT_ACK`, HV on, no connection |
| `--info` cyan | the primary measured value | actual thickness, gauge arc, focus ring |

The state colour of the safety state comes from `ADDAPP.stateClass(state)`
(`ok` / `warn` / `danger`), never from a page-local table.

**Hazard indicators are inverted on purpose:** a hazard that is *active* (high voltage on, shutter
open) is shown red or yellow with a badge; the safe condition is green. This matches the convention
of radiation equipment: "red = radiation possible".

### 4.3 Button colours

| Variant | Class | Colours | Use |
|---|---|---|---|
| normal | `.btn` | `--btn-bg` / `--btn-fg` | all ordinary commands |
| secondary | `.btn.btn-secondary` | `--panel-header-bg`, border `--panel-border` | cancel, reload, demo mode |
| danger | `.btn.btn-danger` | `#6B1010`, border `--err` | confirm a dangerous action, reboot, reset |
| STOP | `.btn.btn-stop` | `#8B2000`, border `#CC4400`, text `#FFCCAA` | STOP only |
| hazard on | `.btn.btn-hv-on` | `#6B1010`, border `--err`, text `#FFD0D0` | button that *switches a hazard on* |
| hazard off | `.btn.btn-hv-off` | `#0B5A1E`, border `--ok`, text `#D8FFD8` | the same button while the hazard is on (it now switches it off) |

### 4.4 Chart colours

| Series | Colour |
|---|---|
| primary measured value (thickness) | `#00B6F8` (`--info`) |
| secondary value (distance) | `#F39200` (`--ims-orange`) |
| limit / threshold line (dashed) | `#F0F000` (`--warn`) |
| reference line (dashed) | `#6184A0` (`--fg-dim`) |
| grid lines | `#1F5E8E` (`--chart-grid`) |
| axis labels | `#6184A0`, 10 px |
| invalid model (e.g. calibration not valid) | `#FF1010` (`--err`) |

---

## 5. Typography

### 5.1 Fonts

| Role | Font | Fallback |
|---|---|---|
| text and values | **Ubuntu** 400 / 700 | `system-ui, sans-serif` |
| logs, raw values, code | **Ubuntu Mono** 400 / 700 | `ui-monospace, monospace` |

The fonts are served by the device (`/font-u400.woff2`, `/font-u700.woff2`, `/font-m400.woff2`,
`/font-m700.woff2`, licence `LICENCE-ubuntu-font.txt`). Declare them with absolute device paths:

```css
@font-face { font-family: 'Ubuntu'; font-weight: 400; font-display: swap; src: url('/font-u400.woff2') format('woff2'); }
@font-face { font-family: 'Ubuntu'; font-weight: 700; font-display: swap; src: url('/font-u700.woff2') format('woff2'); }
@font-face { font-family: 'Ubuntu Mono'; font-weight: 400; font-display: swap; src: url('/font-m400.woff2') format('woff2'); }
@font-face { font-family: 'Ubuntu Mono'; font-weight: 700; font-display: swap; src: url('/font-m700.woff2') format('woff2'); }
```

Opened as a local file these URLs do not resolve and the fallback font is used. This is intended;
never load fonts from a CDN.

### 5.2 Size scale

| Size | Weight | Use |
|---|---|---|
| 28 px | 700 | big gauge value |
| 15 px | 600 | modal title, highlighted actual value |
| 14 px | 600 | material-bar values, device id |
| 13 px | 400 | body text, inputs, alarm title (600) |
| 12 px | 400–500 | buttons, table cells, status rows, chart legend |
| 11 px | 600 | panel headers, form labels, table headers (upper case, letter spacing 0.05–0.06 em) |
| 10 px | 400–700 | material-bar labels (upper case), log tags, chart axes |
| 9 px | 500 | navigation labels (upper case) |

### 5.3 Formatting rules

- Values: `ADDAPP.fmt(x, digits)` → fixed decimals; positions and speeds 1 decimal, distances and
  thicknesses 2 decimals (the reference page uses 1 decimal in the material bar and 2 in the
  measurement panel and gauge).
- Value and unit are separated by a space: `61.6 mm`, `0.0 mm`, `23.0 °C`.
- Invalid or unknown: `–` (en dash) in the operator page; ADDAPP returns `---` for the same case
  and the page may show it as it is.
- Time: local format of the operator language (`toLocaleTimeString('de-DE')` in the demo).
- Labels of the material bar, panel headers and table headers are upper case via CSS
  (`text-transform`), not in the source text.

---

## 6. Components

All component CSS is in the reference page (`data/www/index.html`, style block). Copy it as a
whole; the markup below shows how the classes are combined.

### 6.1 Navigation rail

```html
<nav class="nav-rail" aria-label="Hauptnavigation">
  <div class="nav-logo"><!-- 36 px wide brand mark (SVG) --></div>
  <div class="nav-items">
    <button class="nav-item active" data-page="process" data-tip="Prozess">
      <svg viewBox="0 0 24 24"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg><span>Prozess</span>
    </button>
    <!-- more pages -->
  </div>
  <div class="nav-bottom"><!-- log, connection --></div>
</nav>
```

- Item 60 × 56 px, icon 22 px line icon (`stroke: currentColor; stroke-width: 1.8; fill: none`),
  label 9 px upper case. Use simple inline SVG line icons; no icon font.
- Active item: background `--nav-active`, icon `--nav-icon-active`, 3 px marker on the left.
- `data-tip` shows a tooltip on hover (needed below 700 px where labels are hidden).
- The connection item (`#navConnect`) is only shown when the page runs as a local file.

### 6.2 Header

```html
<header class="topbar">
  <div class="topbar-alarm warn" id="alarmBox">
    <div class="alarm-icon"><svg viewBox="0 0 24 24"><!-- warning triangle --></svg></div>
    <div class="alarm-text">
      <div class="alarm-title" id="alarmTitle">Keine Verbindung</div>
      <div class="alarm-detail" id="alarmDetail">Host eingeben und verbinden</div>
    </div>
  </div>
  <div class="topbar-status-boxes">
    <div class="status-box" id="sboxHv">Hochspannung<div class="badge">!</div></div>
  </div>
  <div class="topbar-meta"><div class="topbar-id" id="deviceId">–</div><div class="topbar-sub">Device name</div></div>
</header>
```

- **Alarm box** (`.topbar-alarm` + `ok` / `warn` / `err`): the one-line summary of the machine.
  Title: `<STATE>[ – <error name>] · <main hazard state>`; detail: mode, position, axis state, time.
  Without data: class `err`, title "Keine Verbindung".
- **Status boxes** (`.status-box` + `on` = green / `warn` = yellow / none = red): one box per hazard
  (high voltage, shutter). The red badge (`.badge`) is visible only while the hazard is active
  (`.badge.hidden` otherwise). At most three boxes.
- **Meta**: device id or host (14 px, tabular), device type below (11 px dim).
- Brand mark at the right end (`.ims-logo-sm`); use the owner's brand assets only with permission.

### 6.3 Sub-bar

```html
<div class="subbar">
  <span class="star">☆</span><span class="page-title" id="pageTitle">Prozess</span>
  <span class="demo-badge hidden" id="demoBadge">DEMO</span>
  <div style="margin-left:auto;display:flex;gap:12px;align-items:center;">
    <div class="conn-indicator"><div class="conn-dot offline" id="connDot"></div><span id="connLabel">Nicht verbunden</span></div>
    <div class="subbar-right" id="clockDisplay">–</div>
  </div>
</div>
```

Connection indicator: `.conn-dot.online` (green with glow) / `.conn-dot.offline` (red). Labels:
"Verbunden", "CRC-Fehler", "Keine Verbindung" (device known but not answering), "Nicht verbunden"
(local file, no host yet), "Demo-Modus". The DEMO badge (orange) is shown whenever simulated data
is displayed.

### 6.4 Panel

```html
<div class="panel">
  <div class="panel-header">Messung <button class="btn btn-small">Aktualisieren</button></div>
  <div class="panel-body">…</div>
</div>
```

Header 11 px upper case on `--panel-header-bg`; optional small button right-aligned. Body padding
10 px 12 px. Chart panels use `.chart-panel` with a legend (`.chart-legend`, `.legend-item`,
`.legend-dot`) in the header and `.chart-canvas-wrap` (220 px high) as body.

### 6.5 Material bar

```html
<div class="material-bar">
  <div class="mat-cell"><div class="label">Zustand</div><div class="value ok" id="matState">READY</div></div>
  <div class="mat-cell"><div class="label">Position</div><div class="value" id="matPos">61.6 mm</div></div>
</div>
```

The key values of the machine in one row (4–6 cells). The value may carry `ok` / `warn` / `err`.

### 6.6 Measurement table and status rows

```html
<table class="meas-table">
  <tr><td class="lbl">Max</td><td class="val" id="mMax">–</td><td class="unit">mm</td></tr>
  <tr class="pos-row"><td class="lbl">Pos</td><td class="val" id="mPos">–</td><td class="unit">mm</td></tr>
</table>

<div class="status-rows">
  <div class="status-row"><span class="s-label">Zykluszeit max.</span><span class="s-val s-ok" id="ssCycle">1.2 ms</span></div>
</div>
```

`.meas-table`: compact label / value / unit rows; `.pos-row` highlights the position in `--info`.
`.status-row`: label left, value right; colour the value with `.s-ok`, `.s-warn`, `.s-err`,
`.s-info`.

### 6.7 Buttons

```html
<div class="ctrl-buttons grid2">
  <button class="btn" id="btnInit">INIT</button>
  <button class="btn btn-hv-on" id="btnHv">HV EIN</button>
  <button class="btn wide" id="btnAck">ACK</button>
  <button class="btn btn-stop wide" id="btnStop">STOP</button>
</div>
```

- Full width inside their column; `.ctrl-buttons.grid2` arranges them in two columns, `.wide`
  spans both. Minimum height **40 px** in the control column (touch use on a tablet).
- `.btn-small` for buttons in panel headers and step lists.
- Hover shows the orange border; disabled buttons use the disabled colours and `cursor: not-allowed`.
- Labels are the command names in upper case (`INIT`, `ACK`, `STOP`) or short operator words
  (`HV EIN`, `MESSEN`).
- Below the buttons a hint line (`.ctrl-hint`, 11 px yellow) explains why buttons are disabled
  ("Zuerst INIT (Referenzfahrt)", "Bedienung am Display hat Vorrang").
- **STOP is always the last button, always enabled while the device is reachable, and never behind
  a dialog.**

### 6.8 Forms

```html
<div class="form-group">
  <label for="f_measure_pos">Messposition</label>
  <div class="form-row"><input class="form-control" id="f_measure_pos" type="number" step="0.1"><span class="unit-label">mm</span></div>
</div>
<div class="form-hint">61 … 242 mm, im Messbereich</div>
<div class="checkbox-row"><input type="checkbox" id="f_interlock"><label for="f_interlock">Sicherheitskreis geschlossen (SIM)</label></div>
<hr class="section-divider">
<div class="save-bar"><button class="btn btn-secondary">Neu laden</button><button class="btn">Speichern</button></div>
```

- Label 11 px upper case dim; input `--input-bg`, focus border `--info`, hover border orange.
- Unit as a separate `.unit-label` box beside the input.
- Range hint below (`.form-hint`; `.form-hint.warn` for a warning such as wiring changes).
- Invalid fields get `.form-control.bad` (red border and ring); nothing is sent while a field is bad.
- Password fields stay empty; the placeholder says whether a value is stored ("gespeichert").

### 6.9 Tables

`.calib-table` and `.log-table`: header row 11 px upper case on `--panel-header-bg`, 1 px row
lines, even rows `--table-even`, log rows highlight on hover. Log levels as tags:

```html
<span class="log-tag info">INFO</span> <span class="log-tag ok">OK</span>
<span class="log-tag warn">WARN</span> <span class="log-tag error">ERROR</span>
```

The raw device log is shown in `<pre class="devlog">` (Ubuntu Mono 11 px, max. 260 px high, scrolls).

### 6.10 Step list and progress

```html
<div class="steps-panel">
  <div class="step-item">
    <div class="step-nr">1</div><div class="step-text">Nahes Referenzteil auflegen, anfahren</div>
    <button class="btn">Anfahren</button><span class="step-status ok">✓</span>
  </div>
</div>
<div class="progress-wrap"><div class="progress-fill" style="width:40%"></div></div>
```

For guided procedures (calibration). Each step: number circle, text, one button, status
(`.step-status.ok` / `.err` / `.dim`).

### 6.11 Gauge

A half-circle gauge drawn on a canvas (`drawGauge(value, max)` in the reference page): track 10 px
in `#1F5E8E`, value arc 10 px in `#00B6F8` with round cap, needle and hub in `#CDE6F9`, scale labels
0 / max/2 / max in 9 px `#6184A0`. The number below in `.gauge-value-big` (28 px) with
`.gauge-unit`. A value that is not valid shows `–` and the needle at 0.

### 6.12 Line chart

Canvas line chart (`drawLine(id, values, color, ref, refColor)` in the reference page):

- device-pixel-ratio aware (`fitCanvas`), redrawn after each new sample and on resize,
- 5 horizontal grid lines with labels on the left (10 px, 1 decimal),
- automatic range with 10 % margin, minimum span 2 units,
- optional dashed reference line (threshold, reference plane),
- at most `SERIES_MAX` = 100 points, only valid samples,
- fewer than 2 points: centred text "noch keine Messwerte".

No chart library is needed; if one is used it must be embedded in the file (no CDN).

### 6.13 Dialogs

```html
<div class="modal-overlay" id="confirmModal" hidden>
  <div class="modal-box">
    <h3 id="confirmTitle">Hochspannung einschalten?</h3>
    <p id="confirmBody">Die Röntgenquelle wird eingeschaltet. Der Shutter öffnet erst beim Messen.</p>
    <div class="save-bar">
      <button class="btn btn-secondary" id="confirmCancel">Abbrechen</button>
      <button class="btn btn-danger" id="confirmOk">Ausführen</button>
    </div>
  </div>
</div>
```

- Overlay black 70 % with 3 px blur; box 380 px, panel colours, 24 px padding.
- Title is a question naming the action; the text says what will happen and what the operator
  must make sure of.
- Cancel left (secondary), execute right (danger).
- The **connection dialog** uses the same box with host and port fields and the buttons
  "Verbinden" and "Demo-Modus".

### 6.14 Toast

```html
<div id="toast"></div>
```

Bottom right, fades in and out. Every `/api` reply is shown as a toast: `<CMD>: OK` for 2.5 s, or
`<CMD>: <ERROR> - <msg>` with class `err` (red) for 5 s (`ADDAPP.replyText`). The same text goes
into the page's event log.

### 6.15 Validation panel

On the status page a panel "ADD-Validierung (/add)" lists the badges from
`ADDAPP.validationBadges(add)`: `✓ key` (normal), `! key` (`warn`), `✗ key` (`fail`), and a summary
line (`info`). It shows the operator that the device description has been validated by an AI.

---

## 7. Page Patterns

| Page | Hash | Content |
|---|---|---|
| **Prozess** (main page) | `#process` | material bar (state, position, speed, distance, thickness, temperature) · left: measurement panel (max/avg/min/actual/n/pos), gauge, control buttons with hint · right: trend chart of the primary value with threshold, chart of the secondary value with reference plane |
| **Status** | `#status` | panels with status rows: system (host, uptime, WLAN, cycle times), axis, safety inputs, I/O; ADD validation |
| **Einstellungen** | `#settings` | one panel per topic (positions, profile, measurement, safety simulation, network, system); save bar |
| **Kalibrierung** | `#calibration` | live values, step list (move – capture – save), reference table, characteristic chart |
| **Log** | `#log` | device log (`/log`, button "Laden") and the page's own event table |

Rules:

- The process page is the start page and always reachable in one click.
- Settings are loaded with `GET_CONFIG` when the page is opened; "Neu laden" discards edits.
- Pages that do not need a value do not request it (status and log load on opening, not on every
  poll).

---

## 8. Device Communication

The interface is defined in `docs/CONTRACT.md` §4 and §5. This section lists what a page must do.

### 8.1 Where the page runs

```js
const ON_DEVICE = location.protocol === 'http:' || location.protocol === 'https:';
function baseUrl() { return HOST ? 'http://' + HOST : ''; }   // '' = own origin on the device
```

| Opened | Behaviour |
|---|---|
| served by the device | relative URLs (`/proc`, `/api`), no connection dialog, connection item hidden |
| as a local file (`file://`) | connection dialog: host/IP (`192.168.4.1` in the device's own access point) and port, stored in `localStorage`; or demo mode with simulated data |

The device answers every request with `Access-Control-Allow-Origin: *`, so a local file may talk
to it.

### 8.2 Protocol

- **Embed ADDAPP.** Copy `data/www/app.js` into the page (first inline `<script>`, escape any
  `</script>` as `<\/script>`). Use its functions; do not write a second CRC or frame
  implementation.
- **Read:** `GET /proc?t=<now>` every **500 ms**, the next request only after the previous one has
  finished, timeout 2.5 s. `ADDAPP.parseFramed(text)` → `{ok, data}`.
  - CRC or JSON error → connection state "CRC-Fehler", show no value as valid, only STOP enabled.
  - network error → "Keine Verbindung", all values `–`.
- **Write:** `POST /api` with body `ADDAPP.apiBody(cmd, params)` and
  `Content-Type: text/plain` (a "simple" request, no CORS preflight), timeout 4 s. The reply is
  framed as well; HTTP 400 replies (bad CRC/JSON) are framed too and must be parsed.
- **Heartbeat:** while `safety.comm_armed` is true, the device stops the machine after 5 s without a
  valid `/api` packet. Send `GET_STATE` when no command was sent for **1 s**.
- **Never repeat a command automatically**, also not after `BUSY`; the next `/proc` shows whether
  it took effect.
- One device serves one HTTP connection at a time. Keep the page small (the reference page is
  about 105 kB), do not open parallel requests, and do not poll faster than 500 ms.

### 8.3 Safety rules for the operator

| Rule | Implementation |
|---|---|
| STOP always possible | STOP button enabled while the device is reachable, sent immediately without a dialog |
| confirm dangerous commands | dialog for `HV_ON`, `ACK`, `REBOOT`, `CALIB_RESET`, wiring changes (`estop_nc`) |
| enable rules from the device state | `ADDAPP.buttons(p)`; never enable a command the state does not allow |
| local operator has priority | while `p.local` only STOP and HV off are enabled, hint "Bedienung am Display hat Vorrang" |
| no stale values | invalid or missing data → `–`; charts keep only valid samples |
| hazard state always visible | status boxes in the header on every page |
| physically meaningful display | example: thickness is 0 while the high voltage is off (no radiation, no measurement), `–` while the shutter is closed with HV on |

### 8.4 Demo mode

A page opened without a device may offer a demo mode: a small simulated device in JavaScript with
the same `proc()` / `api()` interface. The DEMO badge must be visible the whole time, and the
device id shows `DEMO`.

---

## 9. Starter Template

A complete, minimal page in this style: shell, one process page with material bar, measurement
panel and STOP, connection dialog, polling and heartbeat. Copy it, paste the CSS of the reference
page and `data/www/app.js` where marked, and add pages and components from §6.

```html
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ADD Demoanlage – Neue Seite</title>
<style>
/* 1. Paste the complete <style> block of data/www/index.html here (tokens, fonts, components). */
</style>
</head>
<body>
<div class="modal-overlay" id="urlModal" hidden>
  <div class="modal-box">
    <h3>Verbindung zur Demoanlage</h3>
    <p>IP-Adresse oder Hostname der Anlage (eigener Access Point: 192.168.4.1).</p>
    <div class="form-group"><label for="hostInput">Host / IP-Adresse</label><input class="form-control" id="hostInput"></div>
    <div class="save-bar"><button class="btn" id="btnConnect">Verbinden</button></div>
  </div>
</div>

<div class="shell">
  <nav class="nav-rail" aria-label="Hauptnavigation">
    <div class="nav-logo"></div>
    <div class="nav-items">
      <button class="nav-item active" data-page="process" data-tip="Prozess">
        <svg viewBox="0 0 24 24"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg><span>Prozess</span>
      </button>
    </div>
    <div class="nav-bottom">
      <button class="nav-item" id="navConnect" data-tip="Verbindung">
        <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="2"/><path d="M16.24 7.76a6 6 0 0 1 0 8.49M7.76 7.76a6 6 0 0 0 0 8.49"/></svg><span>Verbind.</span>
      </button>
    </div>
  </nav>

  <div class="main-area">
    <header class="topbar">
      <div class="topbar-alarm err" id="alarmBox">
        <div class="alarm-icon"><svg viewBox="0 0 24 24"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg></div>
        <div class="alarm-text"><div class="alarm-title" id="alarmTitle">Nicht verbunden</div><div class="alarm-detail" id="alarmDetail">–</div></div>
      </div>
      <div class="topbar-status-boxes">
        <div class="status-box" id="sboxHv">Hochspannung<div class="badge hidden" id="sboxHvBadge">!</div></div>
      </div>
      <div class="topbar-meta"><div class="topbar-id" id="deviceId">–</div><div class="topbar-sub">ADD Demoanlage Slim</div></div>
    </header>

    <div class="subbar">
      <span class="page-title">Prozess</span>
      <div style="margin-left:auto" class="conn-indicator"><div class="conn-dot offline" id="connDot"></div><span id="connLabel">Nicht verbunden</span></div>
    </div>

    <div class="page-content active" id="page-process">
      <div class="process-grid">
        <div class="material-bar">
          <div class="mat-cell"><div class="label">Zustand</div><div class="value" id="matState">–</div></div>
          <div class="mat-cell"><div class="label">Position</div><div class="value" id="matPos">–</div></div>
          <div class="mat-cell"><div class="label">Dicke</div><div class="value" id="matThick">–</div></div>
        </div>
        <div class="left-panels">
          <div class="panel">
            <div class="panel-header">Bedienung</div>
            <div class="panel-body ctrl-buttons">
              <button class="btn btn-stop" id="btnStop">STOP</button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</div>
<div id="toast"></div>

<script>
/* 2. Paste data/www/app.js here unchanged (it defines window.ADDAPP). */
</script>
<script>
'use strict';
const A = window.ADDAPP;
const POLL_MS = 500, HEARTBEAT_MS = 1000;
const $ = (id) => document.getElementById(id);
const ON_DEVICE = location.protocol === 'http:' || location.protocol === 'https:';
let HOST = '', conn = 'offline', last = null, lastApi = 0;
function baseUrl() { return HOST ? 'http://' + HOST : ''; }

function toast(msg, isErr) {
  const t = $('toast'); t.textContent = msg; t.className = 'show' + (isErr ? ' err' : '');
  setTimeout(() => { t.className = ''; }, isErr ? 5000 : 2500);
}

async function api(cmd, params, quiet) {
  lastApi = Date.now();
  let reply = null;
  try {
    const r = await fetch(baseUrl() + '/api', { method: 'POST', headers: { 'Content-Type': 'text/plain' },
      body: A.apiBody(cmd, params), cache: 'no-store', signal: AbortSignal.timeout(4000) });
    const f = A.parseFramed(await r.text());
    if (f.ok) reply = f.data;
  } catch (e) { /* no reply */ }
  if (!quiet) toast(A.replyText(reply), !reply || reply.err !== 0);
  return reply;
}

async function poll() {
  try {
    const r = await fetch(baseUrl() + '/proc?t=' + Date.now(), { cache: 'no-store', signal: AbortSignal.timeout(2500) });
    const f = A.parseFramed(await r.text());
    conn = f.ok ? 'ok' : 'crc'; last = f.ok ? f.data : null;
  } catch (e) { conn = 'offline'; last = null; }
  render(last);
  if (last && last.safety.comm_armed && Date.now() - lastApi >= HEARTBEAT_MS) api('GET_STATE', null, true);
  setTimeout(poll, POLL_MS);
}

function render(p) {
  $('connDot').className = 'conn-dot ' + (conn === 'ok' ? 'online' : 'offline');
  $('connLabel').textContent = conn === 'ok' ? 'Verbunden' : conn === 'crc' ? 'CRC-Fehler' : 'Keine Verbindung';
  $('deviceId').textContent = HOST || location.host || '–';
  $('btnStop').disabled = conn === 'offline';
  if (!p) {
    $('alarmBox').className = 'topbar-alarm err';
    $('alarmTitle').textContent = conn === 'crc' ? 'CRC-Fehler' : 'Keine Verbindung';
    ['matState', 'matPos', 'matThick'].forEach((id) => { $(id).textContent = '–'; $(id).className = 'value'; });
    return;
  }
  const sc = A.stateClass(p.state), c = sc === 'ok' ? 'ok' : sc === 'danger' ? 'err' : 'warn';
  const hv = p.safety.hv_on, m = p.measure;
  $('alarmBox').className = 'topbar-alarm ' + c;
  $('alarmTitle').textContent = p.state + (hv ? ' · Hochspannung EIN' : ' · Hochspannung AUS');
  $('alarmDetail').textContent = 'Pos ' + A.fmt(p.motion.pos, 1) + ' mm | ' + new Date().toLocaleTimeString('de-DE');
  $('sboxHv').className = 'status-box' + (hv ? '' : ' on');
  $('sboxHvBadge').classList.toggle('hidden', !hv);
  $('matState').textContent = p.state; $('matState').className = 'value ' + c;
  $('matPos').textContent = A.fmt(p.motion.pos, 1) + ' mm';
  const tv = !hv ? 0 : !m.valid ? null : m.on_material ? m.thick : 0;
  $('matThick').textContent = tv === null ? '–' : A.fmt(tv, 1) + ' mm';
}

$('btnStop').onclick = () => api('STOP');                 // never behind a dialog
$('btnConnect').onclick = () => {
  HOST = $('hostInput').value.trim().replace(/^https?:\/\//, '').replace(/\/.*$/, '');
  try { localStorage.setItem('ims_host', HOST); } catch (e) {}
  $('urlModal').hidden = true; poll();
};
$('navConnect').onclick = () => { $('urlModal').hidden = false; };
if (ON_DEVICE) { $('navConnect').hidden = true; poll(); }
else {
  try { $('hostInput').value = localStorage.getItem('ims_host') || ''; } catch (e) {}
  $('urlModal').hidden = false;
}
</script>
</body>
</html>
```

Test a new page without hardware against the simulated device:

```
python3 scripts/fake_device.py 8767
```

Then open the page as a file and connect to `localhost:8767`.

---

## 10. Checklist for a New Page

**Look**
- [ ] Shell with navigation rail, header (alarm box, hazard status boxes) and sub-bar (§3)
- [ ] Only tokens from Appendix A; status colours only for state (§4.2)
- [ ] Ubuntu / Ubuntu Mono from the device paths, no CDN, no external resource of any kind
- [ ] Values tabular, fixed decimals, unit, `–` when invalid
- [ ] Usable at 1280 × 800 and at 700 px width; control buttons at least 40 px high

**Behaviour**
- [ ] ADDAPP embedded unchanged; no own CRC or frame code
- [ ] `/proc` every 500 ms, sequential; CRC error and offline handled
- [ ] Commands as `text/plain`; heartbeat `GET_STATE` after 1 s without a command while `comm_armed`
- [ ] STOP always enabled while reachable, no dialog; confirmation for dangerous commands
- [ ] Buttons follow `ADDAPP.buttons(p)`; LOCAL priority respected
- [ ] No automatic repetition of commands
- [ ] Every reply shown as a toast
- [ ] Works as a local file (connection dialog) and, if intended, served by the device
- [ ] Demo mode, if offered, clearly marked

**Check**
- [ ] Tested against `scripts/fake_device.py`: connect, commands, offline (stop the simulator)
- [ ] Browser console free of errors

---

## Appendix A — CSS Token Block

```css
:root {
  /* brand */
  --ims-blue:        #005A9A;
  --ims-orange:      #F39200;
  --ims-silver:      #D0D0D0;
  /* navigation */
  --nav-bg:          #07151c;
  --nav-active:      #005F8F;
  --nav-icon:        #CDE6F9;
  --nav-icon-active: #00B6F8;
  --nav-icon-hover:  #FFA104;
  /* header, page, panels */
  --header-bg:       #004578;
  --header-fg:       #CDE6F9;
  --page-bg:         #003759;
  --panel-bg:        #004578;
  --panel-border:    #3B6481;
  --panel-header-bg: #002847;
  /* text */
  --fg:              #CDE6F9;
  --fg-dim:          #6184A0;
  --fg-muted:        #4A7CA0;
  /* inputs and buttons */
  --input-bg:        #003256;
  --input-border:    #8099AB;
  --input-fg:        #F5FBFF;
  --btn-bg:          #337BAE;
  --btn-fg:          #CDE6F9;
  --btn-bg-disabled: #005694;
  --btn-fg-disabled: #6184A0;
  --btn-hover-border:#FFA104;
  /* state */
  --ok:              #03CA15;
  --warn:            #F0F000;
  --err:             #FF1010;
  --info:            #00B6F8;
  /* tables and charts */
  --table-even:      #003452;
  --chart-grid:      #1F5E8E;

  color-scheme: dark;
}

html { height: 100%; }
body {
  margin: 0;
  font-family: 'Ubuntu', system-ui, sans-serif;
  font-size: 13px;
  background: var(--page-bg);
  color: var(--fg);
  height: 100%;
  overflow: hidden;
}
```

---

## Appendix B — Changelog

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-11-07 | First version, derived from the IMS operator UI of the ADD demo plant (CONTRACT V1.5.26) |
