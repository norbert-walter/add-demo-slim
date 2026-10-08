# ADD Dashboard Style Guide v1.0 – excerpt for Sprint S4

> Generated from ADD_Style_Guide_v1_0.md (CC BY 4.0, Norbert Walter): sections 2–7 (without instruments, pictogram panel, protocol log, slider, dropdown, checkbox/radio) and Appendix A, images removed.

## 2. Design Principles

### 2.1 Clarity Over Decoration

Every visual element must earn its place by carrying information. This principle derives from Edward Tufte's *data-ink ratio*: the proportion of pixels that directly represent data should be maximized. Decorative gradients, drop shadows, and animations that do not encode data are avoided.

**Practical rule:** The B&W theme is the reference check. If a dashboard element disappears or becomes meaningless in B&W, it was decorative and should be redesigned or removed.

### 2.2 Information Hierarchy

Operators scan dashboards in a predictable order: large values first, then context, then detail. The Style Guide enforces a three-level hierarchy:

- **Primary values** — the measurement that defines the device's purpose: large, high contrast, immediately readable from across the room
- **Secondary values** — environmental context and derived measurements: medium size, readable at a glance
- **System values** — device health, connectivity, firmware: small, available on demand

Primary measurements are always positioned top-left. Instruments precede data panels. System information appears at the bottom.

### 2.3 Theme Consistency

All four standard themes must be semantically equivalent — the same information is present in all themes, only the visual encoding changes. No theme may hide or add data. Color encodes meaning (accent = active/ok, danger = fault) but is never the *only* encoding — a label, icon, or shape must carry the same information for colorblind operators and E-Ink displays.

### 2.4 Accessibility and E-Ink Compatibility

The B&W theme is designed for monochrome E-Ink displays. Requirements:

- Exactly 4 grayscale values: `#ffffff`, `#aaaaaa`, `#444444`, `#000000`
- No gradients, no transparency, no animations
- Minimum touch target: 44×44 px for all interactive controls
- Minimum readable font size: 10px
- All controls must function without color distinction

### 2.5 ISA-101 Alignment

For industrial applications, the Style Guide aligns with ISA-101 (Human Machine Interfaces for Process Automation Systems):

- Neutral gray as default background — reduces eye strain over long operating periods
- Color used sparingly: green = normal, yellow = caution, red = alarm
- Critical values are never hidden behind interactions (no tooltips for primary data)
- Alarm states use both color and a distinct symbol or label

### 2.6 Night Watch Compatibility

The Night theme is designed for low-light environments where the operator's dark-adapted vision must be preserved. Requirements:

- No blue, green, or white light
- All colors in the red/amber spectrum (600–620 nm)
- Pure black background
- No animations that cause bright flashes

---


## 3. Grid System

### 3.1 Unit Definition

The Grid System is based on **1U = 160px**. All panels and instruments are sized in multiples of 1U. A fixed 10px gap separates all grid cells.

| Size | Pixels | Typical use |
|---|---|---|
| 0.5U | 80px | Compact pictogram panel |
| 1U | 160px | Data panel, pictogram, small control |
| 2U | 340px | Instrument (steelseries gauge), medium chart |
| 3U | 500px | Large chart, map panel |
| 4U | 660px | Full-width section, video panel |

### 3.2 Column Layouts

Each dashboard section defines its own column grid. Mix sections freely to create balanced layouts.

**1-column** — full width
```
[ ████████████████████████████████████ ]
```
Use for: NMEA log, full-width map, wide chart, validation strip.

**2-column** — two equal halves
```
[ ████████████████ ]  [ ████████████████ ]
```
Use for: paired instruments, two equal data groups.

**3-column** — instrument + two panels
```
[ ████ 2U inst ████ ]  [ 1U data ]  [ 1U data ]
```
Use for: one primary instrument + scalar context values. Most common for single-axis sensors.

**4-column** — two instruments + two panels
```
[ 2U inst ]  [ 2U hist ]  [ 1U data ]  [ 1U data ]
```
Use for: live instrument + history plot + scalar context values.

**Automatic layout rule for AI agents:**

| MeasuringValues count | Recommended layout |
|---|---|
| 1 | 2-col: 1 instrument + 1 data panel |
| 2–4 | 3-col: 1 instrument + data panels |
| 5–8 | 4-col: 2 instruments + data panels |
| 9+ | Multiple sections |

### 3.3 Responsive Breakpoints

| Viewport | Behavior |
|---|---|
| ≥ 1300px | Full designed layout |
| 960–1299px | 4-col → 4×1fr flexible |
| 520–959px | All → 2-col |
| < 520px | All → 1-col stacked |

Instruments are fixed at 340×340px and do not scale below their natural size.

### 3.4 Standard Section Order

1. Primary measurement section (instrument + data panels)
2. Environmental / context section
3. System health section
4. Protocol / raw data section (NMEA, Modbus, etc.)
5. Validation strip
6. Footer

---


## 4. Themes

The dashboard supports four built-in themes, selectable at runtime. All themes share identical CSS variable names — switching themes requires only changing the `class` attribute on `<body>`.

```javascript
function setTheme(name) {
  document.body.className = name;
  try { localStorage.setItem('add-dashboard-theme', name); } catch(e) {}
}
```

### 4.1 Dark (Default)

For low-light environments: below deck, control rooms, nighttime workstations.

```css
body.theme-dark {
  --bg-color:    rgb(32,32,32);
  --bg-image:    linear-gradient(45deg,black 25%,transparent 25%,transparent 75%,black 75%,black),
                 linear-gradient(45deg,black 25%,transparent 25%,transparent 75%,black 75%,black),
                 linear-gradient(to bottom,rgb(8,8,8),rgb(32,32,32));
  --bg-size:     10px 10px,10px 10px,10px 5px;
  --bg-pos:      0px 0px,5px 5px,0px 0px;
  --panel:       #1e1e1e;
  --border:      #2e2e2e;
  --text:        #c8c8c8;
  --text-strong: #ffffff;
  --muted:       #5a7060;
  --label:       #8ab2d0;
  --accent:      #00c8a0;
  --accent2:     #0084ff;
  --danger:      #e84040;
  --warn:        #f5a623;
  --ok:          #00c8a0;
  --nmea-bg:     #0a0a0a;
  --nmea-ts:     #444444;
  --instr-bg:    #202020;
  --instr-brd:   #333333;
  --v-big-color: #ffffff;
  --v-med-color: #ffffff;
  --input-bg:    #161616;
  --input-brd:   #3a3a3a;
  --pulse-anim:  pulse 2s infinite;
}
```

### 4.2 Light

For well-lit environments: on deck in daylight, bright offices, tablet use in sunlight.

```css
body.theme-light {
  --bg-color:    #f0f2f5;
  --bg-image:    none;
  --bg-size:     auto;
  --bg-pos:      0 0;
  --panel:       #ffffff;
  --border:      #dde2ea;
  --text:        #3a4050;
  --text-strong: #111620;
  --muted:       #8a95a8;
  --label:       #5a80b0;
  --accent:      #00a882;
  --accent2:     #0066cc;
  --danger:      #cc2020;
  --warn:        #d4870a;
  --ok:          #00a882;
  --nmea-bg:     #f8f9fb;
  --nmea-ts:     #aaaaaa;
  --instr-bg:    #e8eaee;
  --instr-brd:   #c8cdd8;
  --v-big-color: #111620;
  --v-med-color: #111620;
  --input-bg:    #f5f7fa;
  --input-brd:   #c8cdd8;
  --pulse-anim:  pulse 2s infinite;
}
```

### 4.3 Night

For night watch and dark-adapted vision preservation. All colors in the red/amber spectrum.

```css
body.theme-night {
  --bg-color:    #000000;
  --bg-image:    none;
  --bg-size:     auto;
  --bg-pos:      0 0;
  --panel:       #0d0d0d;
  --border:      #1a1100;
  --text:        #cc6600;
  --text-strong: #ff9900;
  --muted:       #664400;
  --label:       #995500;
  --accent:      #cc5500;
  --accent2:     #bb4400;
  --danger:      #aa0000;
  --warn:        #cc7700;
  --ok:          #886600;
  --nmea-bg:     #050200;
  --nmea-ts:     #442200;
  --instr-bg:    #0a0500;
  --instr-brd:   #221100;
  --v-big-color: #ff9900;
  --v-med-color: #ff9900;
  --input-bg:    #0a0500;
  --input-brd:   #331a00;
  --pulse-anim:  pulse 2s infinite;
}
```

### 4.4 B&W (E-Ink)

For monochrome E-Ink displays. Strictly 4 grayscale values, no animation.

```css
body.theme-bw {
  --bg-color:    #ffffff;
  --bg-image:    none;
  --bg-size:     auto;
  --bg-pos:      0 0;
  --panel:       #aaaaaa;
  --border:      #444444;
  --text:        #444444;
  --text-strong: #000000;
  --muted:       #444444;
  --label:       #444444;
  --accent:      #000000;
  --accent2:     #000000;
  --danger:      #000000;
  --warn:        #000000;
  --ok:          #000000;
  --nmea-bg:     #aaaaaa;
  --nmea-ts:     #444444;
  --instr-bg:    #aaaaaa;
  --instr-brd:   #444444;
  --v-big-color: #000000;
  --v-med-color: #000000;
  --input-bg:    #ffffff;
  --input-brd:   #444444;
  --pulse-anim:  none;
}
```

### 4.5 Custom Themes

Define a custom theme by declaring a new body class with the complete variable set. All variables listed in Appendix A are required. Declare the theme option in the selector:

```html
<option value="theme-custom">My Brand</option>
```

---


## 5. Typography

### 5.1 Font Selection

The ADD Dashboard Style Guide uses the **Ubuntu** type family, served from [bunny.net](https://fonts.bunny.net) — a GDPR-compliant open font CDN with no tracking.

```css
@import url('https://fonts.bunny.net/css2?family=ubuntu:wght@400;500;700&family=ubuntu-mono:wght@400;700&display=swap');

:root {
  --sans: 'Ubuntu', sans-serif;
  --mono: 'Ubuntu Mono', monospace;
}
```

**Ubuntu** (sans-serif) — labels, section headers, UI text, button labels.
**Ubuntu Mono** (monospace) — all numeric values, protocol strings, technical identifiers, code.

Monospace is mandatory for numeric values because digits must occupy a fixed width. A value changing from `1013.2` to `999.8` must not cause layout shift.

### 5.2 Size Hierarchy

| Class | Size | Weight | Font | Color | Use |
|---|---|---|---|---|---|
| `.v-big` | 2.1rem | 700 | Mono | `--v-big-color` | Primary measurement value |
| `.v-big .u` | 0.9rem | 400 | Mono | `--label` | Unit of primary value |
| `.v-med` | 1.5rem | 700 | Mono | `--v-med-color` | Secondary measurement value |
| `.v-med .u` | 0.8rem | 400 | Mono | `--label` | Unit of secondary value |
| `.v-sm` | 1.0rem | 600 | Mono | `--accent2` | System/detail value |
| `.v-sm .u` | 0.7rem | 400 | Mono | `--muted` | Unit of system value |
| `.lbl` | 0.62rem | 700 | Sans | `--muted` | Field label (uppercase) |
| `.sec` | 0.60rem | 700 | Sans | `--muted` | Section header (uppercase) |
| `.sub` | 0.68rem | 400 | Mono | `--muted` | Sub-value or context text |

### 5.3 Formatting Rules

**Units** — always inline, immediately after the value, in a `.u` span. Never omit.

```html
<div class="v-big">1013.2<span class="u"> mbar</span></div>
```

**Decimal places by measurement type:**

| Type | Places | Example |
|---|---|---|
| Direction / angle | 1 | `269.2°` |
| Speed | 2 | `12.34 kn` |
| Temperature | 1 | `27.1°C` |
| Pressure | 1 | `1013.2 mbar` |
| Humidity / percentage | 1 | `79.2%` |
| Integer values | 0 | `80 MHz` |
| Large integers | 0, localized | `32,456 Byte` |

**Sub-text** — use `.sub` for derived or context values below the primary value:

```html
<div class="v-big">269.2<span class="u">°</span></div>
<div class="sub">W — moderate breeze Bft 4</div>
```

---


## 6. Display Components

### 6.5 Data Panel



The most common component. Displays a single measured value with label, value, unit, and optional sub-text rows.

```html
<!-- Simple -->
<div class="panel">
  <div class="lbl">Air Pressure</div>
  <div class="v-med">1013.2<span class="u"> mbar</span></div>
  <div class="sub">Altitude: 53 m</div>
</div>

<!-- With multiple sub-rows -->
<div class="panel">
  <div class="lbl">Wind Direction</div>
  <div class="v-big">269.2<span class="u">°</span></div>
  <div class="sub">W — moderate breeze Bft 4</div>
  <div class="mt">
    <div class="lbl">Resolution</div>
    <div class="v-sm">0.35<span class="u"> °</span></div>
  </div>
  <div class="mt">
    <div class="lbl">Wind Type</div>
    <div class="v-sm">Relative (R)</div>
  </div>
</div>
```

```css
.panel {
  background: var(--panel);
  border: 1px solid var(--border);
  border-radius: 10px;
  padding: 16px;
}
.lbl {
  font-size: .62rem; font-weight: 700;
  letter-spacing: .1em; text-transform: uppercase;
  color: var(--muted); margin-bottom: 7px;
}
.mt  { margin-top: 12px; }
.sub { font-size: .68rem; color: var(--muted); font-family: var(--mono); margin-top: 5px; }
```

---

### 6.7 Sparkline Panel



A compact time-series line chart embedded in a data panel. Shows the recent trend of a measurement without requiring a full chart section.

```html
<div class="panel">
  <div class="lbl">Temperature</div>
  <div class="v-med" id="tempVal">27.1<span class="u">°C</span></div>
  <canvas id="sparkCanvas" width="260" height="40"
          style="margin-top:8px; width:100%; height:40px"></canvas>
</div>
```

```javascript
var sparkData = [];
var SPARK_MAX = 60;

function pushSpark(value) {
  sparkData.push(value);
  if (sparkData.length > SPARK_MAX) sparkData.shift();
  drawSparkline('sparkCanvas', sparkData);
}

function drawSparkline(canvasId, data) {
  var canvas = document.getElementById(canvasId);
  var ctx = canvas.getContext('2d');
  var W = canvas.width, H = canvas.height;
  ctx.clearRect(0, 0, W, H);
  if (data.length < 2) return;

  var min = Math.min.apply(null, data);
  var max = Math.max.apply(null, data);
  var range = max - min || 1;
  var style = getComputedStyle(document.body);
  var color = style.getPropertyValue('--accent2').trim() || '#0084ff';

  ctx.beginPath();
  data.forEach(function(v, i) {
    var x = (i / (data.length - 1)) * W;
    var y = H - ((v - min) / range) * (H - 4) - 2;
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  });
  ctx.strokeStyle = color;
  ctx.lineWidth   = 1.5;
  ctx.stroke();
}
```

---

### 6.8 Status Pill



The online/offline connectivity indicator shown in the header.

```html
<div class="status-pill" id="statusPill">
  <span class="dot" id="statusDot"></span>
  <span id="statusText">Connecting…</span>
</div>
```

```css
.status-pill {
  display: flex; align-items: center; gap: 7px;
  padding: 5px 13px; border-radius: 100px;
  font-size: .7rem; font-weight: 600; font-family: var(--mono);
  background: rgba(0,200,160,.1); border: 1px solid rgba(0,200,160,.3);
  color: var(--accent);
}
.status-pill.offline {
  background: rgba(200,0,0,.1); border-color: rgba(200,0,0,.3);
  color: var(--danger);
}
.dot {
  width: 7px; height: 7px; border-radius: 50%;
  background: var(--accent); animation: var(--pulse-anim);
}
.dot.off { background: var(--danger); animation: none; }
@keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.3} }
```

```javascript
function setOnline(online) {
  document.getElementById('statusDot').className  = 'dot' + (online?'':' off');
  document.getElementById('statusPill').className = 'status-pill' + (online?'':' offline');
  document.getElementById('statusText').textContent = online ? 'Online' : 'Offline';
}
```

---

### 6.10 Validation Strip



A strip of status badges showing the ADD validation result. Required on every ADD dashboard.

```html
<div class="val-strip">
  <span class="val-lbl">ADD v1.0:</span>
  <span class="badge">✓ structure</span>
  <span class="badge">✓ comprehensibility</span>
  <span class="badge">✓ functional</span>
  <span class="badge">✓ rules_compliance</span>
  <span class="badge">✓ security</span>
  <span class="badge">✓ discovery</span>
  <span class="badge">✓ timing_compliance</span>
  <span class="badge info">PASSED · model-name · YYYY-MM-DD</span>
</div>
```

```css
.val-strip {
  background: var(--panel); border: 1px solid var(--border);
  border-radius: 10px; padding: 14px 16px;
  display: flex; flex-wrap: wrap; gap: 6px; align-items: center;
}
.badge {
  padding: 3px 9px; border-radius: 4px;
  font-size: .62rem; font-family: var(--mono); font-weight: 600;
  background: rgba(0,200,160,.1); border: 1px solid rgba(0,200,160,.25);
  color: var(--accent);
}
.badge.warn {
  background: rgba(245,166,35,.1); border-color: rgba(245,166,35,.25);
  color: var(--warn);
}
.badge.fail {
  background: rgba(232,64,64,.1); border-color: rgba(232,64,64,.25);
  color: var(--danger);
}
.badge.info {
  background: rgba(0,132,255,.1); border-color: rgba(0,132,255,.25);
  color: var(--accent2);
}
```

---


## 7. Input Controls

Input controls are used to trigger ADD device actions. Every control must clearly show its current state and must require explicit user interaction — no control may trigger an action automatically.

Controls that trigger `requires_confirmation: true` actions must display a confirmation dialog before executing.

### 7.1 Button

```html
<!-- Primary action -->
<button class="btn btn-primary" onclick="executeAction('action_id')">
  Start Measurement
</button>

<!-- Secondary / neutral -->
<button class="btn btn-secondary" onclick="fetchData()">↻ Refresh</button>

<!-- Danger — destructive or irreversible action -->
<button class="btn btn-danger" onclick="confirmReset()">Reset Device</button>
```

```css
.btn {
  padding: 6px 16px;
  border-radius: 6px;
  font-family: var(--mono);
  font-size: .72rem; font-weight: 600;
  cursor: pointer; border: 1px solid;
  transition: background .15s;
}
.btn-primary {
  background: rgba(0,200,160,.12); border-color: rgba(0,200,160,.3);
  color: var(--accent);
}
.btn-primary:hover { background: rgba(0,200,160,.22); }

.btn-secondary {
  background: rgba(0,132,255,.1); border-color: rgba(0,132,255,.3);
  color: var(--accent2);
}
.btn-secondary:hover { background: rgba(0,132,255,.22); }

.btn-danger {
  background: rgba(232,64,64,.1); border-color: rgba(232,64,64,.3);
  color: var(--danger);
}
.btn-danger:hover { background: rgba(232,64,64,.22); }
```

---

### 7.2 Toggle Switch

For binary on/off actions (enable/disable a device feature).

```html
<label class="toggle-wrap">
  <span class="lbl">Auto-Average</span>
  <label class="toggle">
    <input type="checkbox" id="autoAverage" onchange="setAutoAverage(this.checked)">
    <span class="toggle-slider"></span>
  </label>
</label>
```

```css
.toggle-wrap {
  display: flex; align-items: center; justify-content: space-between;
  gap: 12px; padding: 8px 0;
}
.toggle { position: relative; display: inline-block; width: 40px; height: 22px; }
.toggle input { opacity: 0; width: 0; height: 0; }
.toggle-slider {
  position: absolute; inset: 0;
  background: var(--input-brd); border-radius: 22px; cursor: pointer;
  transition: background .2s;
}
.toggle-slider::before {
  content: '';
  position: absolute; width: 16px; height: 16px;
  left: 3px; top: 3px; border-radius: 50%;
  background: var(--text-strong); transition: transform .2s;
}
.toggle input:checked + .toggle-slider { background: var(--accent); }
.toggle input:checked + .toggle-slider::before { transform: translateX(18px); }
```

---

### 7.5 Text Input

For free-text configuration values (device name, API key, SSID).

```html
<div class="input-wrap">
  <label class="lbl" for="deviceName">Device Name</label>
  <input type="text" class="text-input" id="deviceName"
         placeholder="Enter device name"
         value=""
         onchange="setDeviceName(this.value)">
</div>
```

```css
.input-wrap { display: flex; flex-direction: column; gap: 5px; }
.text-input {
  padding: 6px 10px;
  background: var(--input-bg); border: 1px solid var(--input-brd);
  border-radius: 6px; color: var(--text-strong);
  font-family: var(--mono); font-size: .8rem; outline: none;
  transition: border-color .15s;
}
.text-input:focus { border-color: var(--accent); }
.text-input::placeholder { color: var(--muted); }
```

---

### 7.8 Confirmation Dialog

Required for any ADD action with `requires_confirmation: true`. Must display the action name, expected effect, and require explicit confirmation before sending the request to the device.

```html
<div class="dialog-overlay" id="confirmOverlay" style="display:none">
  <div class="dialog">
    <div class="dialog-title" id="confirmTitle">Confirm Action</div>
    <div class="dialog-body"  id="confirmBody">Are you sure?</div>
    <div class="dialog-btns">
      <button class="btn btn-secondary" onclick="closeConfirm()">Cancel</button>
      <button class="btn btn-danger"    id="confirmOk">Execute</button>
    </div>
  </div>
</div>
```

```css
.dialog-overlay {
  position: fixed; inset: 0;
  background: rgba(0,0,0,.6); display: flex;
  align-items: center; justify-content: center; z-index: 999;
}
.dialog {
  background: var(--panel); border: 1px solid var(--border);
  border-radius: 10px; padding: 24px; max-width: 360px; width: 90%;
}
.dialog-title { font-size: 1rem; font-weight: 700; color: var(--text-strong); margin-bottom: 12px; }
.dialog-body  { font-size: .8rem; color: var(--text); margin-bottom: 20px; font-family: var(--mono); }
.dialog-btns  { display: flex; justify-content: flex-end; gap: 10px; }
```

```javascript
var pendingAction = null;

function showConfirm(title, body, actionFn) {
  document.getElementById('confirmTitle').textContent = title;
  document.getElementById('confirmBody').textContent  = body;
  document.getElementById('confirmOk').onclick = function() {
    closeConfirm(); actionFn();
  };
  document.getElementById('confirmOverlay').style.display = 'flex';
}
function closeConfirm() {
  document.getElementById('confirmOverlay').style.display = 'none';
}
```

---


## Appendix A — CSS Variable Reference

| Variable | Purpose | Dark | Light | Night | B&W |
|---|---|---|---|---|---|
| `--bg-color` | Body background base color | `rgb(32,32,32)` | `#f0f2f5` | `#000000` | `#ffffff` |
| `--bg-image` | Body background pattern | checkered gradient | `none` | `none` | `none` |
| `--panel` | Panel background | `#1e1e1e` | `#ffffff` | `#0d0d0d` | `#aaaaaa` |
| `--border` | Panel border | `#2e2e2e` | `#dde2ea` | `#1a1100` | `#444444` |
| `--text` | Body text | `#c8c8c8` | `#3a4050` | `#cc6600` | `#444444` |
| `--text-strong` | Headings, primary values | `#ffffff` | `#111620` | `#ff9900` | `#000000` |
| `--muted` | Labels, secondary text | `#5a7060` | `#8a95a8` | `#664400` | `#444444` |
| `--label` | Unit labels | `#8ab2d0` | `#5a80b0` | `#995500` | `#444444` |
| `--accent` | Primary accent (ok/active) | `#00c8a0` | `#00a882` | `#cc5500` | `#000000` |
| `--accent2` | Secondary accent (values/controls) | `#0084ff` | `#0066cc` | `#bb4400` | `#000000` |
| `--danger` | Error / alarm | `#e84040` | `#cc2020` | `#aa0000` | `#000000` |
| `--warn` | Warning / caution | `#f5a623` | `#d4870a` | `#cc7700` | `#000000` |
| `--ok` | Normal / confirmed ok | `#00c8a0` | `#00a882` | `#886600` | `#000000` |
| `--nmea-bg` | Protocol log background | `#0a0a0a` | `#f8f9fb` | `#050200` | `#aaaaaa` |
| `--nmea-ts` | Protocol log timestamp | `#444444` | `#aaaaaa` | `#442200` | `#444444` |
| `--instr-bg` | Instrument panel background | `#202020` | `#e8eaee` | `#0a0500` | `#aaaaaa` |
| `--instr-brd` | Instrument panel border | `#333333` | `#c8cdd8` | `#221100` | `#444444` |
| `--v-big-color` | Primary value color | `#ffffff` | `#111620` | `#ff9900` | `#000000` |
| `--v-med-color` | Secondary value color | `#ffffff` | `#111620` | `#ff9900` | `#000000` |
| `--input-bg` | Input field background | `#161616` | `#f5f7fa` | `#0a0500` | `#ffffff` |
| `--input-brd` | Input field border | `#3a3a3a` | `#c8cdd8` | `#331a00` | `#444444` |
| `--pulse-anim` | Status dot animation | `pulse 2s infinite` | `pulse 2s infinite` | `pulse 2s infinite` | `none` |

---

