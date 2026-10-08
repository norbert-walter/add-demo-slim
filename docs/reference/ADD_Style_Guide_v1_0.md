# ADD Dashboard Style Guide v1.0

**AI Device Description — Dashboard Design Standard**
Version 1.0 | CC BY 4.0 © 2026 Norbert Walter / Open Boat Projects
https://github.com/norbert-walter/ai-device-description-add

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Design Principles](#2-design-principles)
3. [Grid System](#3-grid-system)
4. [Themes](#4-themes)
5. [Typography](#5-typography)
6. [Display Components](#6-display-components)
7. [Input Controls](#7-input-controls)
8. [Media Components](#8-media-components)
9. [Pictogram Library](#9-pictogram-library)
   - 9.1 Tabler Icons
   - 9.2 Phosphor Icons
   - 9.3 Standard Measurement Type Mapping
   - 9.4 Fallback Icon
   - 9.5 Icon Color Rules
10. [Complete Dashboard Example](#10-complete-dashboard-example)
- [Appendix A — CSS Variable Reference](#appendix-a--css-variable-reference)
- [Appendix B — SteelSeries Configuration Reference](#appendix-b--steelseries-configuration-reference)
- [Appendix D — Color Palette Guide](#appendix-d--color-palette-guide)
- [Appendix C — Changelog](#appendix-c--changelog)

---

## 1. Introduction

### 1.1 Why a Style Guide?

The AI Device Description (ADD) standard enables AI agents to autonomously discover, understand, and interact with IoT devices. When an AI agent reads an ADD document, it gains structured knowledge about a device's capabilities, data schema, and control actions.

A natural next step is for the AI agent to present that data to a human operator — as a live dashboard. Without guidance, every AI agent will invent its own visual language: different colors, different layouts, different levels of information density. The result is inconsistency across devices and manufacturers, and dashboards that feel foreign to operators working with multiple ADD-compatible devices.

The ADD Dashboard Style Guide solves this problem. It defines a visual language — a shared vocabulary of components, colors, typography, and layouts — that any AI agent can read and apply when generating a dashboard for any ADD-compatible device. The result is a family of dashboards that feel related, regardless of which AI generated them or which device they display.

This Style Guide is intentionally **industry-neutral**. It describes generic components — a pointer instrument with a degree scale, a linear gauge, a data panel — not device-specific widgets. A wind sensor, an industrial pressure transmitter, a laboratory scale, and a GPS tracker all produce numeric measurements. The same components serve all of them.

### 1.2 Relationship to ADD and the Developer Guide

The ADD ecosystem consists of three documents with distinct responsibilities:

| Document | Audience | Describes |
|---|---|---|
| ADD Specification | AI agents, developers | The device: sensors, actions, autonomy rules |
| ADD Developer Guide | Developers | How to implement ADD in firmware and software |
| ADD Dashboard Style Guide | AI agents, UI developers | How to present ADD data visually |

The Style Guide is referenced from the ADD document via a single optional `ui` block:

```json
"ui": {
  "style_guide_url": "https://norbert-walter.github.io/ai-device-description-add/style-guide/v1.0",
  "style_guide_version": "1.0",
  "dashboard_demo_url": "https://example.com/dashboard_demo.html",
  "theme_default": "dark"
}
```

The ADD document itself remains compact and device-focused. All visual design decisions live in the Style Guide. Manufacturers may publish their own Style Guide at any URL, extending or replacing this standard.

### 1.3 How AI Agents Use This Document

When an AI agent encounters a `ui.style_guide_url` field in an ADD document, it should:

1. Fetch the Style Guide from the given URL
2. Read the Component Reference to understand which components are available
3. Map the device's `MeasuringValues` to appropriate display components
4. Map the device's `Actions` to appropriate input controls
5. Apply the Grid System to compose a layout
6. Apply the active Theme for colors and typography
7. Embed any required libraries (SteelSeries, Leaflet, Tabler Icons)
8. Generate a self-contained HTML dashboard file

### 1.4 Versioning and Extensibility

The Style Guide is versioned independently from the ADD specification. A custom Style Guide must declare the version it extends:

```markdown
# MyBrand Dashboard Style Guide v1.0
Extends: ADD Dashboard Style Guide v1.0
```

---

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

### 6.1 Dual-Pointer Instrument (SteelSeries WindDirection2)

![Dual-Pointer Instrument](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMzAwIiB2aWV3Qm94PSIwIDAgNTAwIDMwMCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIzMDAiIGZpbGw9IiMwYjEyMjAiLz4KICA8IS0tIFBhbmVsIC0tPgogIDxyZWN0IHg9IjEwIiB5PSIxMCIgd2lkdGg9IjI4MCIgaGVpZ2h0PSIyODAiIHJ4PSIxMCIgZmlsbD0iIzIwMjAyMCIgc3Ryb2tlPSIjMzMzIiBzdHJva2Utd2lkdGg9IjEiLz4KICA8IS0tIE91dGVyIHJpbmcgLS0+CiAgPGNpcmNsZSBjeD0iMTUwIiBjeT0iMTUwIiByPSIxMjAiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzQ0NCIgc3Ryb2tlLXdpZHRoPSIxLjUiLz4KICA8IS0tIElubmVyIHJpbmcgLS0+CiAgPGNpcmNsZSBjeD0iMTUwIiBjeT0iMTUwIiByPSIxMDAiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzMzMyIgc3Ryb2tlLXdpZHRoPSIxIi8+CiAgPCEtLSBBTlRIUkFDSVRFIGZpbGwgLS0+CiAgPGNpcmNsZSBjeD0iMTUwIiBjeT0iMTUwIiByPSI5OSIgZmlsbD0iIzNjM2MzYyIvPgogIDwhLS0gQ29sb3JlZCBzZWN0b3JzIC0tPgogIDxwYXRoIGQ9Ik0xNTAgMTUwIEwxNTAgMzAgQTEyMCAxMjAgMCAwIDEgMjUzLjkgOTAgWiIgZmlsbD0icmdiYSgwLDI1NSwwLDAuMikiLz4KICA8cGF0aCBkPSJNMTUwIDE1MCBMMjUzLjkgMjEwIEExMjAgMTIwIDAgMCAxIDE1MCAyNzAgWiIgZmlsbD0icmdiYSgyNTUsMCwwLDAuMikiLz4KICA8IS0tIFRpY2sgbWFya3MgLS0+CiAgPGxpbmUgeDE9IjE1MC4wIiB5MT0iNTAuMCIgeDI9IjE1MC4wIiB5Mj0iMzAuMCIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjIiLz48bGluZSB4MT0iMTY3LjQiIHkxPSI1MS41IiB4Mj0iMTcwLjgiIHkyPSIzMS44IiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxODQuMiIgeTE9IjU2LjAiIHgyPSIxOTEuMCIgeTI9IjM3LjIiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIwMC4wIiB5MT0iNjMuNCIgeDI9IjIxMC4wIiB5Mj0iNDYuMSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjE0LjMiIHkxPSI3My40IiB4Mj0iMjI3LjEiIHkyPSI1OC4xIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyMjYuNiIgeTE9Ijg1LjciIHgyPSIyNDEuOSIgeTI9IjcyLjkiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIzNi42IiB5MT0iMTAwLjAiIHgyPSIyNTMuOSIgeTI9IjkwLjAiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjI0NC4wIiB5MT0iMTE1LjgiIHgyPSIyNjIuOCIgeTI9IjEwOS4wIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyNDguNSIgeTE9IjEzMi42IiB4Mj0iMjY4LjIiIHkyPSIxMjkuMiIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjUwLjAiIHkxPSIxNTAuMCIgeDI9IjI3MC4wIiB5Mj0iMTUwLjAiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIyIi8+PGxpbmUgeDE9IjI0OC41IiB5MT0iMTY3LjQiIHgyPSIyNjguMiIgeTI9IjE3MC44IiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyNDQuMCIgeTE9IjE4NC4yIiB4Mj0iMjYyLjgiIHkyPSIxOTEuMCIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjM2LjYiIHkxPSIyMDAuMCIgeDI9IjI1My45IiB5Mj0iMjEwLjAiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIyNi42IiB5MT0iMjE0LjMiIHgyPSIyNDEuOSIgeTI9IjIyNy4xIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyMTQuMyIgeTE9IjIyNi42IiB4Mj0iMjI3LjEiIHkyPSIyNDEuOSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjAwLjAiIHkxPSIyMzYuNiIgeDI9IjIxMC4wIiB5Mj0iMjUzLjkiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjE4NC4yIiB5MT0iMjQ0LjAiIHgyPSIxOTEuMCIgeTI9IjI2Mi44IiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxNjcuNCIgeTE9IjI0OC41IiB4Mj0iMTcwLjgiIHkyPSIyNjguMiIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTUwLjAiIHkxPSIyNTAuMCIgeDI9IjE1MC4wIiB5Mj0iMjcwLjAiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIyIi8+PGxpbmUgeDE9IjEzMi42IiB5MT0iMjQ4LjUiIHgyPSIxMjkuMiIgeTI9IjI2OC4yIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxMTUuOCIgeTE9IjI0NC4wIiB4Mj0iMTA5LjAiIHkyPSIyNjIuOCIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTAwLjAiIHkxPSIyMzYuNiIgeDI9IjkwLjAiIHkyPSIyNTMuOSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iODUuNyIgeTE9IjIyNi42IiB4Mj0iNzIuOSIgeTI9IjI0MS45IiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI3My40IiB5MT0iMjE0LjMiIHgyPSI1OC4xIiB5Mj0iMjI3LjEiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjYzLjQiIHkxPSIyMDAuMCIgeDI9IjQ2LjEiIHkyPSIyMTAuMCIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iNTYuMCIgeTE9IjE4NC4yIiB4Mj0iMzcuMiIgeTI9IjE5MS4wIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI1MS41IiB5MT0iMTY3LjQiIHgyPSIzMS44IiB5Mj0iMTcwLjgiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjUwLjAiIHkxPSIxNTAuMCIgeDI9IjMwLjAiIHkyPSIxNTAuMCIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjIiLz48bGluZSB4MT0iNTEuNSIgeTE9IjEzMi42IiB4Mj0iMzEuOCIgeTI9IjEyOS4yIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI1Ni4wIiB5MT0iMTE1LjgiIHgyPSIzNy4yIiB5Mj0iMTA5LjAiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjYzLjQiIHkxPSIxMDAuMCIgeDI9IjQ2LjEiIHkyPSI5MC4wIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI3My40IiB5MT0iODUuNyIgeDI9IjU4LjEiIHkyPSI3Mi45IiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI4NS43IiB5MT0iNzMuNCIgeDI9IjcyLjkiIHkyPSI1OC4xIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxMDAuMCIgeTE9IjYzLjQiIHgyPSI5MC4wIiB5Mj0iNDYuMSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTE1LjgiIHkxPSI1Ni4wIiB4Mj0iMTA5LjAiIHkyPSIzNy4yIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxMzIuNiIgeTE9IjUxLjUiIHgyPSIxMjkuMiIgeTI9IjMxLjgiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+CiAgPCEtLSBDYXJkaW5hbCBsYWJlbHMgLS0+CiAgPHRleHQgeD0iMTUwIiB5PSIyMiIgIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiMwMGM4YTAiPk48L3RleHQ+CiAgPHRleHQgeD0iMjc4IiB5PSIxNTUiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiM2NjYiPkU8L3RleHQ+CiAgPHRleHQgeD0iMTUwIiB5PSIyODIiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiM2NjYiPlM8L3RleHQ+CiAgPHRleHQgeD0iMjIiICB5PSIxNTUiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiM2NjYiPlc8L3RleHQ+CiAgPCEtLSBQb2ludGVyIChyZWQsIHBvaW50aW5nIH4yNzDCsCkgLS0+CiAgPGxpbmUgeDE9IjE1MCIgeTE9IjE1MCIgeDI9IjUwIiB5Mj0iMTUwIiBzdHJva2U9IiNlODQwNDAiIHN0cm9rZS13aWR0aD0iMyIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIi8+CiAgPHBvbHlnb24gcG9pbnRzPSI1MCwxNTAgNjIsMTQ0IDYyLDE1NiIgZmlsbD0iI2U4NDA0MCIvPgogIDwhLS0gQXZlcmFnZSBwb2ludGVyICh0aGlubmVyLCB+MjQwwrApIC0tPgogIDxsaW5lIHgxPSIxNTAiIHkxPSIxNTAiIHgyPSI5MCIgeTI9IjQ2IiBzdHJva2U9IiM4ODgiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIi8+CiAgPCEtLSBDZW50ZXIgZG90IC0tPgogIDxjaXJjbGUgY3g9IjE1MCIgY3k9IjE1MCIgcj0iNiIgZmlsbD0iI2ZmZmZmZiIvPgogIDwhLS0gTENEIHBhbmVsIC0tPgogIDxyZWN0IHg9IjkwIiB5PSIxODUiIHdpZHRoPSIxMjAiIGhlaWdodD0iNDQiIHJ4PSI0IiBmaWxsPSIjMTExIiBzdHJva2U9IiMzMzMiLz4KICA8dGV4dCB4PSIxNTAiIHk9IjIwMCIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNTU1Ij5EaXJlY3Rpb24gW8KwXTwvdGV4dD4KICA8dGV4dCB4PSIxNTAiIHk9IjIxNCIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMyIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiMwMGM4YTAiPjI3MC4wPC90ZXh0PgogIDx0ZXh0IHg9IjE1MCIgeT0iMjI2IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1NTUiPlNwZWVkIFtrbl08L3RleHQ+CiAgPCEtLSBDUSBsYWJlbCAtLT4KICA8dGV4dCB4PSIxNTAiIHk9IjI2MiIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjNWE3ODk4Ij5DUTogOTglPC90ZXh0PgogIDwhLS0gUmlnaHQgc2lkZTogZGF0YSBwYW5lbCAtLT4KICA8cmVjdCB4PSIzMDIiIHk9IjEwIiB3aWR0aD0iMTg4IiBoZWlnaHQ9IjEzMCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8dGV4dCB4PSIzMjAiIHk9IjMyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5XSU5EIERJUkVDVElPTjwvdGV4dD4KICA8dGV4dCB4PSIzMjAiIHk9IjcyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjM2IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iI2ZmZmZmZiI+MjcwPC90ZXh0PgogIDx0ZXh0IHg9IjQxMCIgeT0iNzIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTYiIGZpbGw9IiM1YTc4OTgiPsKwPC90ZXh0PgogIDx0ZXh0IHg9IjMyMCIgeT0iOTIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiM1YTc4OTgiPlc8L3RleHQ+CiAgPHRleHQgeD0iMzIwIiB5PSIxMTgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPlJFU09MVVRJT048L3RleHQ+CiAgPHRleHQgeD0iMzIwIiB5PSIxMzIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTIiIGZpbGw9IiMwMDg0ZmYiPjAuMzUgwrA8L3RleHQ+CiAgPCEtLSBTcGVlZCBwYW5lbCAtLT4KICA8cmVjdCB4PSIzMDIiIHk9IjE1MiIgd2lkdGg9IjE4OCIgaGVpZ2h0PSIxMzgiIHJ4PSIxMCIgZmlsbD0iIzFlMWUxZSIgc3Ryb2tlPSIjMmUyZTJlIi8+CiAgPHRleHQgeD0iMzIwIiB5PSIxNzQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPldJTkQgU1BFRUQ8L3RleHQ+CiAgPHRleHQgeD0iMzIwIiB5PSIyMTQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMzYiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjZmZmZmZmIj4xMi4zNDwvdGV4dD4KICA8dGV4dCB4PSI0MzAiIHk9IjIxNCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMyIgZmlsbD0iIzVhNzg5OCI+a248L3RleHQ+CiAgPHRleHQgeD0iMzIwIiB5PSIyMzQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiM1YTc4OTgiPkJmdCA0IOKAlCBtb2RlcmF0ZSBicmVlemU8L3RleHQ+CiAgPHRleHQgeD0iMzIwIiB5PSIyNTgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPlJPVEFUSU9OIFNQRUVEPC90ZXh0PgogIDx0ZXh0IHg9IjMyMCIgeT0iMjcyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEyIiBmaWxsPSIjMDA4NGZmIj4yNi45NiBycHM8L3RleHQ+Cjwvc3ZnPg==)


A circular instrument with two animated pointers, a degree scale, optional colored sectors, and an LCD display. Generic use: any measurement combining a **directional** value (pointer position on scale) with a **scalar** value (LCD display).

Examples: wind direction + speed, compass heading + boat speed, shaft angle + RPM.

**Required libraries** (loaded from device or CDN):
```html
<script src="{host}/tween-min.js"></script>
<script src="{host}/steelseries_micro.js"></script>
```

**Generic initialization:**
```javascript
var gauge = new steelseries.WindDirection2('canvasId', {
  size:                340,         // fixed — do not change
  section:             null,        // or array of steelseries.Section(from, to, color)
  pointSymbolsVisible: false,       // true = show N/E/S/W labels
  frameDesign:         steelseries.FrameDesign.BLACK_METAL,
  backgroundColor:     steelseries.BackgroundColor.ANTHRACITE,
  lcdVisible:          true,        // false = no LCD panel
  lcdColor:            steelseries.LcdColor.STANDARD,
  lcdTitleStrings:     ['Primary [unit]', 'Secondary [unit]'],
  pointerColor:        steelseries.ColorDef.RED,
  pointerTypeLatest:   steelseries.PointerType.TYPE6,
  pointerTypeAverage:  steelseries.PointerType.TYPE6,
  degreeScaleHalf:     true         // true = -180..+180, false = 0..360
});

gauge.setValueAnimatedLatest(primaryValue);   // moves pointer + LCD top
gauge.setValueAverage(secondaryValue);        // LCD bottom
```

**Colored sectors** — use to mark operating zones. Maximum 3 sectors recommended:
```javascript
section: [
  steelseries.Section(from1, to1, 'rgba(255,0,0,0.8)'),   // danger zone
  steelseries.Section(from2, to2, 'rgba(0,255,0,0.8)'),   // normal zone
  steelseries.Section(from3, to3, 'rgba(255,255,0,0.5)')  // caution zone
]
```

**Panel HTML:**
```html
<div class="instr-panel">
  <canvas id="gaugeCanvas" width="340" height="340"></canvas>
  <div class="instr-cq">Signal: <span id="signalQuality">—</span>%</div>
  <div class="instr-meta" id="gaugeMode"> </div>
</div>
```

**CSS:**
```css
.instr-panel {
  background: var(--instr-bg);
  border: 1px solid var(--instr-brd);
  border-radius: 10px;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 12px 8px 8px;
  gap: 6px;
}
.instr-panel canvas { display: block; }
.instr-cq  { font-family: var(--mono); font-size: .65rem; color: var(--accent); }
.instr-meta{ font-family: var(--mono); font-size: .65rem; color: var(--muted); }
```

See Appendix B for the complete SteelSeries parameter reference.

---

### 6.2 Single-Pointer Instrument (SteelSeries Radial)

![Single-Pointer Instrument](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjYwIiB2aWV3Qm94PSIwIDAgNTAwIDI2MCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyNjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSIyNDAiIGhlaWdodD0iMjQwIiByeD0iMTAiIGZpbGw9IiMyMDIwMjAiIHN0cm9rZT0iIzMzMyIvPgogIDwhLS0gQXJjIGJhY2tncm91bmQgLS0+CiAgPGNpcmNsZSBjeD0iMTMwIiBjeT0iMTQwIiByPSIxMDAiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzMzMyIgc3Ryb2tlLXdpZHRoPSIxIi8+CiAgPGNpcmNsZSBjeD0iMTMwIiBjeT0iMTQwIiByPSIxMDAiIGZpbGw9IiMzYzNjM2MiLz4KICA8IS0tIENvbG9yZWQgc2VjdG9yIGFyY3MgKGJvdHRvbSBoYWxmID0gZ2F1Z2UgYXJlYSkgLS0+CiAgPHBhdGggZD0iTTEzMCAxNDAgTDMwIDE0MCBBMTAwIDEwMCAwIDAgMSA4MCA1MyBaIiBmaWxsPSJyZ2JhKDAsMjAwLDAsMC4yNSkiLz4KICA8cGF0aCBkPSJNMTMwIDE0MCBMODAgNTMgQTEwMCAxMDAgMCAwIDEgMTgwIDUzIFoiIGZpbGw9InJnYmEoMjU1LDIwMCwwLDAuMjUpIi8+CiAgPHBhdGggZD0iTTEzMCAxNDAgTDE4MCA1MyBBMTAwIDEwMCAwIDAgMSAyMzAgMTQwIFoiIGZpbGw9InJnYmEoMjU1LDAsMCwwLjI1KSIvPgogIDwhLS0gVGljayBtYXJrcyBhbG9uZyBib3R0b20gYXJjIC0tPgogIDxsaW5lIHgxPSI0NS4wIiB5MT0iMTQwLjAiIHgyPSIzMC4wIiB5Mj0iMTQwLjAiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIyIi8+PGxpbmUgeDE9IjQ5LjIiIHkxPSIxMTMuNyIgeDI9IjM0LjkiIHkyPSIxMDkuMSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iNjEuMiIgeTE9IjkwLjAiIHgyPSI0OS4xIiB5Mj0iODEuMiIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjIiLz48bGluZSB4MT0iODAuMCIgeTE9IjcxLjIiIHgyPSI3MS4yIiB5Mj0iNTkuMSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTAzLjciIHkxPSI1OS4yIiB4Mj0iOTkuMSIgeTI9IjQ0LjkiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIyIi8+PGxpbmUgeDE9IjEzMC4wIiB5MT0iNTUuMCIgeDI9IjEzMC4wIiB5Mj0iNDAuMCIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTU2LjMiIHkxPSI1OS4yIiB4Mj0iMTYwLjkiIHkyPSI0NC45IiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMiIvPjxsaW5lIHgxPSIxODAuMCIgeTE9IjcxLjIiIHgyPSIxODguOCIgeTI9IjU5LjEiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjE5OC44IiB5MT0iOTAuMCIgeDI9IjIxMC45IiB5Mj0iODEuMiIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjIiLz48bGluZSB4MT0iMjEwLjgiIHkxPSIxMTMuNyIgeDI9IjIyNS4xIiB5Mj0iMTA5LjEiIHN0cm9rZT0iIzY2NiIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIxNS4wIiB5MT0iMTQwLjAiIHgyPSIyMzAuMCIgeTI9IjE0MC4wIiBzdHJva2U9IiM2NjYiIHN0cm9rZS13aWR0aD0iMiIvPgogIDwhLS0gU2NhbGUgbGFiZWxzIC0tPgogIDx0ZXh0IHg9IjI4IiAgeT0iMTUwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM2NjYiPjA8L3RleHQ+CiAgPHRleHQgeD0iMTEwIiB5PSI0OCIgIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzY2NiI+NTA8L3RleHQ+CiAgPHRleHQgeD0iMjE4IiB5PSIxNTAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzY2NiI+MTAwPC90ZXh0PgogIDwhLS0gUG9pbnRlciBhdCB+NzUlID0gMjI1wrAgZnJvbSBzdGFydCAtLT4KICA8bGluZSB4MT0iMTMwIiB5MT0iMTQwIiB4Mj0iMTkwLjEiIHkyPSI3OS45IiBzdHJva2U9IiNlODQwNDAiIHN0cm9rZS13aWR0aD0iMyIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIi8+CiAgPGNpcmNsZSBjeD0iMTMwIiBjeT0iMTQwIiByPSI2IiBmaWxsPSIjZmZmZmZmIi8+CiAgPCEtLSBMQ0QgLS0+CiAgPHJlY3QgeD0iODAiIHk9IjE1NSIgd2lkdGg9IjEwMCIgaGVpZ2h0PSIzNiIgcng9IjQiIGZpbGw9IiMxMTEiIHN0cm9rZT0iIzMzMyIvPgogIDx0ZXh0IHg9IjEzMCIgeT0iMTY4IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1NTUiPlByZXNzdXJlPC90ZXh0PgogIDx0ZXh0IHg9IjEzMCIgeT0iMTg0IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjE0IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iIzAwYzhhMCI+NzUuMyBtYmFyPC90ZXh0PgogIDwhLS0gVW5pdCBsYWJlbCAtLT4KICA8dGV4dCB4PSIxMzAiIHk9IjIyNSIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMCIgZmlsbD0iIzVhNzg5OCI+bWJhcjwvdGV4dD4KICA8IS0tIFJpZ2h0OiBkZXNjcmlwdGlvbiAtLT4KICA8cmVjdCB4PSIyNjUiIHk9IjEwIiB3aWR0aD0iMjI1IiBoZWlnaHQ9IjI0MCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8dGV4dCB4PSIyODMiIHk9IjM2IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5TSU5HTEUtUE9JTlRFUiBJTlNUUlVNRU5UPC90ZXh0PgogIDx0ZXh0IHg9IjI4MyIgeT0iNjAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IFNlbWljaXJjdWxhciBzY2FsZTwvdGV4dD4KICA8dGV4dCB4PSIyODMiIHk9IjgwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBNaW4gLyBNYXggLyBUaHJlc2hvbGQ8L3RleHQ+CiAgPHRleHQgeD0iMjgzIiB5PSIxMDAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IDMgY29sb3JlZCB6b25lczwvdGV4dD4KICA8dGV4dCB4PSIyODMiIHk9IjEyMCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgTENEIHZhbHVlIGRpc3BsYXk8L3RleHQ+CiAgPHRleHQgeD0iMjgzIiB5PSIxNDAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IEFuaW1hdGVkIHBvaW50ZXI8L3RleHQ+CiAgPHRleHQgeD0iMjgzIiB5PSIxNzUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPlVTRSBDQVNFUzwvdGV4dD4KICA8dGV4dCB4PSIyODMiIHk9IjE5NSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMCIgZmlsbD0iIzAwODRmZiI+VGVtcGVyYXR1cmUgwrcgUHJlc3N1cmU8L3RleHQ+CiAgPHRleHQgeD0iMjgzIiB5PSIyMTIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTAiIGZpbGw9IiMwMDg0ZmYiPlZvbHRhZ2UgwrcgUlBNIMK3IExldmVsPC90ZXh0Pgo8L3N2Zz4=)


A semicircular or full-circle gauge with one pointer and a numeric LCD. Generic use: any single scalar measurement with a defined min/max range.

Examples: temperature, pressure, battery voltage, RPM, fill level percentage.

```javascript
var gauge = new steelseries.Radial('canvasId', {
  size:           340,
  gaugeType:      steelseries.GaugeType.TYPE4,   // TYPE1=full, TYPE2=top-half, TYPE4=bottom-open
  minValue:       0,
  maxValue:       100,
  threshold:      80,
  section: [
    steelseries.Section(0,  60, 'rgba(0,200,0,0.3)'),
    steelseries.Section(60, 80, 'rgba(255,200,0,0.3)'),
    steelseries.Section(80,100, 'rgba(255,0,0,0.3)')
  ],
  unitString:     'mbar',
  titleString:    'Pressure',
  frameDesign:    steelseries.FrameDesign.BLACK_METAL,
  backgroundColor:steelseries.BackgroundColor.ANTHRACITE,
  lcdVisible:     true,
  lcdColor:       steelseries.LcdColor.STANDARD,
  pointerColor:   steelseries.ColorDef.RED,
  pointerType:    steelseries.PointerType.TYPE6
});

gauge.setValueAnimated(currentValue);
```

---

### 6.3 Linear Gauge (SteelSeries Linear)

![Linear Gauge](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjYwIiB2aWV3Qm94PSIwIDAgNTAwIDI2MCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyNjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8IS0tIFRocmVlIGxpbmVhciBnYXVnZXMgc2lkZSBieSBzaWRlIC0tPgogIDwhLS0gR2F1Z2UgMTogVGFuayA3MiUgLS0+CiAgPHJlY3QgeD0iMjAiIHk9IjIwIiB3aWR0aD0iODAiIGhlaWdodD0iMjIwIiByeD0iMTAiIGZpbGw9IiMyMDIwMjAiIHN0cm9rZT0iIzMzMyIvPgogIDx0ZXh0IHg9IjYwIiB5PSI0MiIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4Ij5UQU5LPC90ZXh0PgogIDxyZWN0IHg9IjM2IiB5PSI1NSIgd2lkdGg9IjQ4IiBoZWlnaHQ9IjE0MCIgcng9IjQiIGZpbGw9IiMxMTEiIHN0cm9rZT0iIzMzMyIvPgogIDwhLS0gZ3JlZW4gZmlsbCA3MiUgLS0+CiAgPHJlY3QgeD0iMzciIHk9Ijk0IiB3aWR0aD0iNDYiIGhlaWdodD0iMTAwIiByeD0iMyIgZmlsbD0iIzAwYzhhMCIgb3BhY2l0eT0iMC44Ii8+CiAgPCEtLSB0aHJlc2hvbGQgbGluZSAtLT4KICA8bGluZSB4MT0iMzQiIHkxPSI2OSIgeDI9Ijg2IiB5Mj0iNjkiIHN0cm9rZT0iI2U4NDA0MCIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1kYXNoYXJyYXk9IjMsMiIvPgogIDx0ZXh0IHg9IjYwIiB5PSIyMTMiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTMiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjZmZmZmZmIj43MiU8L3RleHQ+CiAgPHRleHQgeD0iNjAiIHk9IjIzMCIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4Ij43MiAvIDEwMCBMPC90ZXh0PgogIDwhLS0gR2F1Z2UgMjogQmF0dGVyeSA0NSUgLS0+CiAgPHJlY3QgeD0iMTE1IiB5PSIyMCIgd2lkdGg9IjgwIiBoZWlnaHQ9IjIyMCIgcng9IjEwIiBmaWxsPSIjMjAyMDIwIiBzdHJva2U9IiMzMzMiLz4KICA8dGV4dCB4PSIxNTUiIHk9IjQyIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiPkJBVFRFUlk8L3RleHQ+CiAgPHJlY3QgeD0iMTMxIiB5PSI1NSIgd2lkdGg9IjQ4IiBoZWlnaHQ9IjE0MCIgcng9IjQiIGZpbGw9IiMxMTEiIHN0cm9rZT0iIzMzMyIvPgogIDxyZWN0IHg9IjEzMiIgeT0iMTMyIiB3aWR0aD0iNDYiIGhlaWdodD0iNjMiIHJ4PSIzIiBmaWxsPSIjZjVhNjIzIiBvcGFjaXR5PSIwLjgiLz4KICA8bGluZSB4MT0iMTI5IiB5MT0iOTciIHgyPSIxODEiIHkyPSI5NyIgc3Ryb2tlPSIjZTg0MDQwIiBzdHJva2Utd2lkdGg9IjEuNSIgc3Ryb2tlLWRhc2hhcnJheT0iMywyIi8+CiAgPHRleHQgeD0iMTU1IiB5PSIyMTMiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTMiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjZmZmZmZmIj40NSU8L3RleHQ+CiAgPHRleHQgeD0iMTU1IiB5PSIyMzAiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCI+MTIuNiBWPC90ZXh0PgogIDwhLS0gR2F1Z2UgMzogU2lnbmFsIDg4JSAtLT4KICA8cmVjdCB4PSIyMTAiIHk9IjIwIiB3aWR0aD0iODAiIGhlaWdodD0iMjIwIiByeD0iMTAiIGZpbGw9IiMyMDIwMjAiIHN0cm9rZT0iIzMzMyIvPgogIDx0ZXh0IHg9IjI1MCIgeT0iNDIiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCI+U0lHTkFMPC90ZXh0PgogIDxyZWN0IHg9IjIyNiIgeT0iNTUiIHdpZHRoPSI0OCIgaGVpZ2h0PSIxNDAiIHJ4PSI0IiBmaWxsPSIjMTExIiBzdHJva2U9IiMzMzMiLz4KICA8cmVjdCB4PSIyMjciIHk9IjcxIiB3aWR0aD0iNDYiIGhlaWdodD0iMTIzIiByeD0iMyIgZmlsbD0iIzAwYzhhMCIgb3BhY2l0eT0iMC44Ii8+CiAgPHRleHQgeD0iMjUwIiB5PSIyMTMiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTMiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjZmZmZmZmIj44OCU8L3RleHQ+CiAgPHRleHQgeD0iMjUwIiB5PSIyMzAiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCI+LTUyIGRCbTwvdGV4dD4KICA8IS0tIERlc2NyaXB0aW9uIHBhbmVsIC0tPgogIDxyZWN0IHg9IjMwNSIgeT0iMjAiIHdpZHRoPSIxODUiIGhlaWdodD0iMjIwIiByeD0iMTAiIGZpbGw9IiMxZTFlMWUiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjMyMyIgeT0iNDQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPkxJTkVBUiBHQVVHRTwvdGV4dD4KICA8dGV4dCB4PSIzMjMiIHk9IjY4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBWZXJ0aWNhbCBiYXIgZGlzcGxheTwvdGV4dD4KICA8dGV4dCB4PSIzMjMiIHk9Ijg4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBQcm9wb3J0aW9uYWwgZmlsbDwvdGV4dD4KICA8dGV4dCB4PSIzMjMiIHk9IjEwOCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgQ29sb3Igem9uZXM8L3RleHQ+CiAgPHRleHQgeD0iMzIzIiB5PSIxMjgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IFRocmVzaG9sZCBtYXJrZXI8L3RleHQ+CiAgPHRleHQgeD0iMzIzIiB5PSIxNDgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IE51bWVyaWMgTENEPC90ZXh0PgogIDx0ZXh0IHg9IjMyMyIgeT0iMTc4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5VU0UgQ0FTRVM8L3RleHQ+CiAgPHRleHQgeD0iMzIzIiB5PSIxOTgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTAiIGZpbGw9IiMwMDg0ZmYiPlRhbmsgwrcgQmF0dGVyeSDCtyBTaWduYWw8L3RleHQ+CiAgPHRleHQgeD0iMzIzIiB5PSIyMTYiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTAiIGZpbGw9IiMwMDg0ZmYiPkZpbGwgbGV2ZWwgwrcgQ2hhcmdlPC90ZXh0Pgo8L3N2Zz4=)


A vertical or horizontal bar gauge. Generic use: fill levels, progress, linear measurements with clear min/max.

Examples: tank level, battery charge, signal strength bar, temperature in a narrow range.

```javascript
var gauge = new steelseries.Linear('canvasId', {
  width:          80,
  height:         280,
  minValue:       0,
  maxValue:       100,
  threshold:      90,
  unitString:     '%',
  titleString:    'Tank Level',
  frameDesign:    steelseries.FrameDesign.BLACK_METAL,
  backgroundColor:steelseries.BackgroundColor.ANTHRACITE,
  lcdVisible:     true,
  valueColor:     steelseries.ColorDef.GREEN
});

gauge.setValueAnimated(currentValue);
```

---

### 6.4 Circular History Plot

![Circular History Plot](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMzEwIiB2aWV3Qm94PSIwIDAgNTAwIDMxMCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIzMTAiIGZpbGw9IiMwYjEyMjAiLz4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSIyOTAiIGhlaWdodD0iMjkwIiByeD0iMTAiIGZpbGw9IiMyMDIwMjAiIHN0cm9rZT0iIzMzMyIvPgogIDwhLS0gRnJhbWUgcmluZyAtLT4KICA8Y2lyY2xlIGN4PSIxNTAiIGN5PSIxNTAiIHI9IjEyMCIgZmlsbD0ibm9uZSIgc3Ryb2tlPSIjNDQ0IiBzdHJva2Utd2lkdGg9IjIiLz4KICA8IS0tIENvdmVyIGlubmVyIC0tPgogIDxjaXJjbGUgY3g9IjE1MCIgY3k9IjE1MCIgcj0iMTAwIiBmaWxsPSIjM2MzYzNjIi8+CiAgPCEtLSBCYXJzIC0tPgogIDxwYXRoIGQ9Ik0xNDUuNywxNjEuMiBBMTIsMTIgMCAwLDEgMTQ0LjQsMTYwLjYgTDE0Ny4yLDE1NS4zIEE2LDYgMCAwLDAgMTQ3LjgsMTU1LjYgWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC4zNSIvPjxwYXRoIGQ9Ik0xMjUuMSwxNzAuMSBBMzIsMzIgMCAwLDEgMTIyLjksMTY3LjEgTDE0NC45LDE1My4yIEE2LDYgMCAwLDAgMTQ1LjMsMTUzLjggWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC41MiIvPjxwYXRoIGQ9Ik0xMTAuNiwxNzEuOCBBNDUsNDUgMCAwLDEgMTA4LjQsMTY3LjEgTDE0NC40LDE1Mi4zIEE2LDYgMCAwLDAgMTQ0LjgsMTUyLjkgWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC42MiIvPjxwYXRoIGQ9Ik0xMTMuMSwxNjIuNyBBMzksMzkgMCAwLDEgMTExLjksMTU4LjMgTDE0NC4xLDE1MS4zIEE2LDYgMCAwLDAgMTQ0LjMsMTUyLjAgWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC41NyIvPjxwYXRoIGQ9Ik02Ni4wLDE2My4zIEE4NSw4NSAwIDAsMSA2NS4xLDE1My40IEwxNDQuMCwxNTAuMiBBNiw2IDAgMCwwIDE0NC4xLDE1MC45IFoiIGZpbGw9IiMwMDg0ZmYiIG9wYWNpdHk9IjAuOTUiLz48cGF0aCBkPSJNNzguMCwxNDguNyBBNzIsNzIgMCAwLDEgNzguNywxNDAuMyBMMTQ0LjEsMTQ5LjIgQTYsNiAwIDAsMCAxNDQuMCwxNDkuOSBaIiBmaWxsPSIjMDA4NGZmIiBvcGFjaXR5PSIwLjg0Ii8+PHBhdGggZD0iTTU5LjcsMTMyLjQgQTkyLDkyIDAgMCwxIDYyLjQsMTIyLjAgTDE0NC4zLDE0OC4yIEE2LDYgMCAwLDAgMTQ0LjEsMTQ4LjkgWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMS4wMCIvPjxwYXRoIGQ9Ik0xMDguMCwxMzMuOSBBNDUsNDUgMCAwLDEgMTEwLjIsMTI5LjEgTDE0NC43LDE0Ny4yIEE2LDYgMCAwLDAgMTQ0LjQsMTQ3LjggWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC42MiIvPjxwYXRoIGQ9Ik0xMzMuNywxNDAuMiBBMTksMTkgMCAwLDEgMTM1LjAsMTM4LjQgTDE0NS4zLDE0Ni4zIEE2LDYgMCAwLDAgMTQ0LjksMTQ2LjkgWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC40MSIvPjxwYXRoIGQ9Ik0xNDIuNSwxNDAuNyBBMTIsMTIgMCAwLDEgMTQzLjYsMTM5LjkgTDE0Ni44LDE0NC45IEE2LDYgMCAwLDAgMTQ2LjIsMTQ1LjMgWiIgZmlsbD0iIzAwODRmZiIgb3BhY2l0eT0iMC4zNSIvPgogIDwhLS0gVGlja3MgLS0+CiAgPGxpbmUgeDE9IjE1MC4wIiB5MT0iNTAuMCIgeDI9IjE1MC4wIiB5Mj0iMzUuMCIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEuNSIvPjxsaW5lIHgxPSIxNjcuNCIgeTE9IjUxLjUiIHgyPSIxNzAuMCIgeTI9IjM2LjciIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjE4NC4yIiB5MT0iNTYuMCIgeDI9IjE4OS4zIiB5Mj0iNDEuOSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjAwLjAiIHkxPSI2My40IiB4Mj0iMjA3LjUiIHkyPSI1MC40IiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyMTQuMyIgeTE9IjczLjQiIHgyPSIyMjMuOSIgeTI9IjYxLjkiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIyNi42IiB5MT0iODUuNyIgeDI9IjIzOC4xIiB5Mj0iNzYuMSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjM2LjYiIHkxPSIxMDAuMCIgeDI9IjI0OS42IiB5Mj0iOTIuNSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjQ0LjAiIHkxPSIxMTUuOCIgeDI9IjI1OC4xIiB5Mj0iMTEwLjciIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjI0OC41IiB5MT0iMTMyLjYiIHgyPSIyNjMuMyIgeTI9IjEzMC4wIiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyNTAuMCIgeTE9IjE1MC4wIiB4Mj0iMjY1LjAiIHkyPSIxNTAuMCIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEuNSIvPjxsaW5lIHgxPSIyNDguNSIgeTE9IjE2Ny40IiB4Mj0iMjYzLjMiIHkyPSIxNzAuMCIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjQ0LjAiIHkxPSIxODQuMiIgeDI9IjI1OC4xIiB5Mj0iMTg5LjMiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIzNi42IiB5MT0iMjAwLjAiIHgyPSIyNDkuNiIgeTI9IjIwNy41IiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIyMjYuNiIgeTE9IjIxNC4zIiB4Mj0iMjM4LjEiIHkyPSIyMjMuOSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMjE0LjMiIHkxPSIyMjYuNiIgeDI9IjIyMy45IiB5Mj0iMjM4LjEiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjIwMC4wIiB5MT0iMjM2LjYiIHgyPSIyMDcuNSIgeTI9IjI0OS42IiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxODQuMiIgeTE9IjI0NC4wIiB4Mj0iMTg5LjMiIHkyPSIyNTguMSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTY3LjQiIHkxPSIyNDguNSIgeDI9IjE3MC4wIiB5Mj0iMjYzLjMiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjE1MC4wIiB5MT0iMjUwLjAiIHgyPSIxNTAuMCIgeTI9IjI2NS4wIiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMS41Ii8+PGxpbmUgeDE9IjEzMi42IiB5MT0iMjQ4LjUiIHgyPSIxMzAuMCIgeTI9IjI2My4zIiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxMTUuOCIgeTE9IjI0NC4wIiB4Mj0iMTEwLjciIHkyPSIyNTguMSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iMTAwLjAiIHkxPSIyMzYuNiIgeDI9IjkyLjUiIHkyPSIyNDkuNiIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iODUuNyIgeTE9IjIyNi42IiB4Mj0iNzYuMSIgeTI9IjIzOC4xIiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI3My40IiB5MT0iMjE0LjMiIHgyPSI2MS45IiB5Mj0iMjIzLjkiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjYzLjQiIHkxPSIyMDAuMCIgeDI9IjUwLjQiIHkyPSIyMDcuNSIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iNTYuMCIgeTE9IjE4NC4yIiB4Mj0iNDEuOSIgeTI9IjE4OS4zIiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSI1MS41IiB5MT0iMTY3LjQiIHgyPSIzNi43IiB5Mj0iMTcwLjAiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjUwLjAiIHkxPSIxNTAuMCIgeDI9IjM1LjAiIHkyPSIxNTAuMCIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEuNSIvPjxsaW5lIHgxPSI1MS41IiB5MT0iMTMyLjYiIHgyPSIzNi43IiB5Mj0iMTMwLjAiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjU2LjAiIHkxPSIxMTUuOCIgeDI9IjQxLjkiIHkyPSIxMTAuNyIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz48bGluZSB4MT0iNjMuNCIgeTE9IjEwMC4wIiB4Mj0iNTAuNCIgeTI9IjkyLjUiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjczLjQiIHkxPSI4NS43IiB4Mj0iNjEuOSIgeTI9Ijc2LjEiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9Ijg1LjciIHkxPSI3My40IiB4Mj0iNzYuMSIgeTI9IjYxLjkiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjEwMC4wIiB5MT0iNjMuNCIgeDI9IjkyLjUiIHkyPSI1MC40IiBzdHJva2U9IiM1NTUiIHN0cm9rZS13aWR0aD0iMSIvPjxsaW5lIHgxPSIxMTUuOCIgeTE9IjU2LjAiIHgyPSIxMTAuNyIgeTI9IjQxLjkiIHN0cm9rZT0iIzU1NSIgc3Ryb2tlLXdpZHRoPSIxIi8+PGxpbmUgeDE9IjEzMi42IiB5MT0iNTEuNSIgeDI9IjEzMC4wIiB5Mj0iMzYuNyIgc3Ryb2tlPSIjNTU1IiBzdHJva2Utd2lkdGg9IjEiLz4KICA8IS0tIExhYmVscyAtLT4KICA8dGV4dCB4PSIxNTAiIHk9IjMyIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiPjDCsDwvdGV4dD48dGV4dCB4PSIyNzEiIHk9IjE1NCIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjNWE3ODk4Ij45MMKwPC90ZXh0Pjx0ZXh0IHg9IjE1MCIgeT0iMjgwIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiPjE4MMKwPC90ZXh0Pjx0ZXh0IHg9IjI5IiB5PSIxNTQiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzVhNzg5OCI+MjcwwrA8L3RleHQ+CiAgPCEtLSBDZW50ZXIgZG90IC0tPgogIDxjaXJjbGUgY3g9IjE1MCIgY3k9IjE1MCIgcj0iNSIgZmlsbD0iIzVhNzg5OCIgb3BhY2l0eT0iMC42Ii8+CiAgPCEtLSBJbmZvIC0tPgogIDx0ZXh0IHg9IjE1MCIgeT0iMjc1IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiMwMDg0ZmYiPkhpc3Rvcnk6IDYwLzYwPC90ZXh0PgogIDx0ZXh0IHg9IjE1MCIgeT0iMjkwIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiPkZyZXF1ZW5jeSBkaXN0cmlidXRpb248L3RleHQ+CiAgPCEtLSBEZXNjcmlwdGlvbiAtLT4KICA8cmVjdCB4PSIzMTUiIHk9IjEwIiB3aWR0aD0iMTc1IiBoZWlnaHQ9IjI5MCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8dGV4dCB4PSIzMzMiIHk9IjM0IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5DSVJDVUxBUiBISVNUT1JZIFBMT1Q8L3RleHQ+CiAgPHRleHQgeD0iMzMzIiB5PSI1OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgMzYgc2VjdG9ycyDDlyAxMMKwPC90ZXh0PgogIDx0ZXh0IHg9IjMzMyIgeT0iNzgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IDYwIHNhbXBsZSBidWZmZXI8L3RleHQ+CiAgPHRleHQgeD0iMzMzIiB5PSI5OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgQmFyIGxlbmd0aCDiiJ0gZnJlcS48L3RleHQ+CiAgPHRleHQgeD0iMzMzIiB5PSIxMTgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IEFscGhhIOKInSBmcmVxdWVuY3k8L3RleHQ+CiAgPHRleHQgeD0iMzMzIiB5PSIxMzgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IEZ1bGwgMOKAkzM2MMKwIHNjYWxlPC90ZXh0PgogIDx0ZXh0IHg9IjMzMyIgeT0iMTcwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5VU0UgQ0FTRVM8L3RleHQ+CiAgPHRleHQgeD0iMzMzIiB5PSIxOTIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTAiIGZpbGw9IiMwMDg0ZmYiPldpbmQgZGlyZWN0aW9uPC90ZXh0PgogIDx0ZXh0IHg9IjMzMyIgeT0iMjEwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjMDA4NGZmIj5Db21wYXNzIGhlYWRpbmc8L3RleHQ+CiAgPHRleHQgeD0iMzMzIiB5PSIyMjgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTAiIGZpbGw9IiMwMDg0ZmYiPkFueSBjeWNsaWMgdmFsdWU8L3RleHQ+Cjwvc3ZnPg==)


A frequency distribution of the last N directional measurements, rendered as a polar bar chart. The instrument frame provides the degree scale; the inner area is overdrawn with the history bars.

Generic use: any directional or cyclic measurement where the distribution over time is more informative than the current value alone.

**Two-canvas overlay technique:**

```html
<div class="instr-wrap">
  <canvas id="histFrame"  width="340" height="340"></canvas>  <!-- SteelSeries frame -->
  <canvas id="histBars"   width="340" height="340"
          style="pointer-events:none"></canvas>               <!-- History bars -->
</div>
```

```css
.instr-wrap { position: relative; width: 340px; height: 340px; }
.instr-wrap canvas { position: absolute; top: 0; left: 0; display: block; }
```

**Frame initialization** — scale only, no pointer, no LCD:
```javascript
var histFrame = new steelseries.WindDirection2('histFrame', {
  size:                340,
  section:             null,
  pointSymbolsVisible: false,
  frameDesign:         steelseries.FrameDesign.BLACK_METAL,
  backgroundColor:     steelseries.BackgroundColor.ANTHRACITE,
  lcdVisible:          false,
  pointerColor:        steelseries.ColorDef.BLACK,   // invisible
  pointerTypeLatest:   steelseries.PointerType.TYPE1,
  degreeScaleHalf:     false,  // full 0–360°
  foregroundVisible:   false
});
```

**Geometry** (size = 340, re = 340):

| Element | Formula | Pixels |
|---|---|---|
| Tick outer edge | 0.38 × re | 129 px |
| Tick inner edge | 0.35 × re | 119 px |
| Number labels | 0.31 × re | 105 px |
| **Cover radius** | fixed | **100 px** |
| Bar max radius | cover − 3 | 97 px |
| Center dot | fixed | 6 px |

**Rendering algorithm:**
```javascript
var HIST_SIZE = 60; // configurable
var history   = [];

function pushHistory(deg) {
  history.push(deg);
  if (history.length > HIST_SIZE) history.shift();
  drawBars();
}

function drawBars() {
  var canvas = document.getElementById('histBars');
  var ctx = canvas.getContext('2d');
  var cx = 170, cy = 170;

  ctx.clearRect(0, 0, 340, 340);

  // 1. Cover inner area with instrument background color
  ctx.beginPath();
  ctx.arc(cx, cy, 100, 0, 2*Math.PI);
  ctx.fillStyle = '#3c3c3c'; // ANTHRACITE match
  ctx.fill();

  if (history.length === 0) return;

  // 2. Frequency bins (36 bins × 10°)
  var bins = new Array(36).fill(0);
  history.forEach(function(d) {
    bins[Math.floor(((d % 360) + 360) % 360 / 10) % 36]++;
  });
  var maxBin = Math.max.apply(null, bins);
  if (maxBin === 0) return;

  // 3. Bar color: accent2 normally, white in B&W theme
  var isBW   = document.body.classList.contains('theme-bw');
  var style  = getComputedStyle(document.body);
  var color  = isBW ? '#ffffff' : (style.getPropertyValue('--accent2').trim() || '#0084ff');
  var muted  = style.getPropertyValue('--muted').trim() || '#5a7060';

  // 4. Draw polar bars
  for (var s = 0; s < 36; s++) {
    if (bins[s] === 0) continue;
    var frac = bins[s] / maxBin;
    var len  = Math.round(frac * 91); // 97 - 6
    var a1   = (s * 10 - 90) * Math.PI / 180;
    var a2   = ((s + 1) * 10 - 90) * Math.PI / 180;
    var gap  = 0.018;
    ctx.beginPath();
    ctx.arc(cx, cy, 6 + len, a1 + gap, a2 - gap);
    ctx.arc(cx, cy, 6,       a2 - gap, a1 + gap, true);
    ctx.closePath();
    ctx.fillStyle   = color;
    ctx.globalAlpha = 0.3 + frac * 0.7;
    ctx.fill();
    ctx.globalAlpha = 1;
  }

  // 5. Center dot
  ctx.beginPath();
  ctx.arc(cx, cy, 6, 0, 2*Math.PI);
  ctx.fillStyle   = muted;
  ctx.globalAlpha = 0.6;
  ctx.fill();
  ctx.globalAlpha = 1;
}
```

---

### 6.5 Data Panel

![Data Panel](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjYwIiB2aWV3Qm94PSIwIDAgNTAwIDI2MCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyNjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8IS0tIFNpbXBsZSBwYW5lbCAtLT4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSIxNDgiIGhlaWdodD0iMTIwIiByeD0iMTAiIGZpbGw9IiMxZTFlMWUiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjI2IiB5PSIzMiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCIgbGV0dGVyLXNwYWNpbmc9IjEiPkFJUiBQUkVTU1VSRTwvdGV4dD4KICA8dGV4dCB4PSIyNiIgeT0iNzUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMzQiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjZmZmZmZmIj4xMDEzPC90ZXh0PgogIDx0ZXh0IHg9IjEyMCIgeT0iNzUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTQiIGZpbGw9IiM1YTc4OTgiPi4yPC90ZXh0PgogIDx0ZXh0IHg9IjE0MCIgeT0iNzUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTIiIGZpbGw9IiM1YTc4OTgiPm1iYXI8L3RleHQ+CiAgPHRleHQgeD0iMjYiIHk9Ijk2IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjNWE3ODk4Ij5BbHRpdHVkZTogNTMgbTwvdGV4dD4KICA8dGV4dCB4PSIyNiIgeT0iMTE0IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiPkJNRTI4MCBzZW5zb3I8L3RleHQ+CiAgPCEtLSBQYW5lbCB3aXRoIHN1Yi1yb3dzIC0tPgogIDxyZWN0IHg9IjE3MCIgeT0iMTAiIHdpZHRoPSIxNDgiIGhlaWdodD0iMjQwIiByeD0iMTAiIGZpbGw9IiMxZTFlMWUiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjE4NiIgeT0iMzIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiIGxldHRlci1zcGFjaW5nPSIxIj5XSU5EIERJUkVDVElPTjwvdGV4dD4KICA8dGV4dCB4PSIxODYiIHk9Ijc4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjM4IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iI2ZmZmZmZiI+MjY5PC90ZXh0PgogIDx0ZXh0IHg9IjI1NiIgeT0iNzgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTYiIGZpbGw9IiM1YTc4OTgiPi4ywrA8L3RleHQ+CiAgPHRleHQgeD0iMTg2IiB5PSI5OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iIzVhNzg5OCI+VyDigJQgbW9kZXJhdGUgYnJlZXplPC90ZXh0PgogIDxsaW5lIHgxPSIxODYiIHkxPSIxMTIiIHgyPSIzMDQiIHkyPSIxMTIiIHN0cm9rZT0iIzJlMmUyZSIgc3Ryb2tlLXdpZHRoPSIxIi8+CiAgPHRleHQgeD0iMTg2IiB5PSIxMzAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiIGxldHRlci1zcGFjaW5nPSIxIj5SRVNPTFVUSU9OPC90ZXh0PgogIDx0ZXh0IHg9IjE4NiIgeT0iMTQ4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjE0IiBmaWxsPSIjMDA4NGZmIj4wLjM1IMKwPC90ZXh0PgogIDxsaW5lIHgxPSIxODYiIHkxPSIxNjIiIHgyPSIzMDQiIHkyPSIxNjIiIHN0cm9rZT0iIzJlMmUyZSIgc3Ryb2tlLXdpZHRoPSIxIi8+CiAgPHRleHQgeD0iMTg2IiB5PSIxODAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiIGxldHRlci1zcGFjaW5nPSIxIj5XSU5EIFRZUEU8L3RleHQ+CiAgPHRleHQgeD0iMTg2IiB5PSIxOTgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTQiIGZpbGw9IiMwMDg0ZmYiPlJlbGF0aXZlIChSKTwvdGV4dD4KICA8bGluZSB4MT0iMTg2IiB5MT0iMjEyIiB4Mj0iMzA0IiB5Mj0iMjEyIiBzdHJva2U9IiMyZTJlMmUiIHN0cm9rZS13aWR0aD0iMSIvPgogIDx0ZXh0IHg9IjE4NiIgeT0iMjMwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIiBsZXR0ZXItc3BhY2luZz0iMSI+T0ZGU0VUPC90ZXh0PgogIDx0ZXh0IHg9IjE4NiIgeT0iMjQ4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjE0IiBmaWxsPSIjMDA4NGZmIj4wIMKwPC90ZXh0PgogIDwhLS0gRGVzY3JpcHRpb24gLS0+CiAgPHJlY3QgeD0iMzMwIiB5PSIxMCIgd2lkdGg9IjE2MCIgaGVpZ2h0PSIyNDAiIHJ4PSIxMCIgZmlsbD0iIzFlMWUxZSIgc3Ryb2tlPSIjMmUyZTJlIi8+CiAgPHRleHQgeD0iMzQ4IiB5PSIzNCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+REFUQSBQQU5FTDwvdGV4dD4KICA8dGV4dCB4PSIzNDgiIHk9IjYwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBMYWJlbCAodXBwZXJjYXNlKTwvdGV4dD4KICA8dGV4dCB4PSIzNDgiIHk9IjgwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBQcmltYXJ5IHZhbHVlPC90ZXh0PgogIDx0ZXh0IHg9IjM0OCIgeT0iMTAwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBVbml0IGlubGluZTwvdGV4dD4KICA8dGV4dCB4PSIzNDgiIHk9IjEyMCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgU3ViLXRleHQgcm93czwvdGV4dD4KICA8dGV4dCB4PSIzNDgiIHk9IjE0MCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgRGl2aWRlciBsaW5lczwvdGV4dD4KICA8dGV4dCB4PSIzNDgiIHk9IjE3MiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+U0laRSBDTEFTU0VTPC90ZXh0PgogIDx0ZXh0IHg9IjM0OCIgeT0iMTk0IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjMDBjOGEwIj52LWJpZzogMi4xcmVtPC90ZXh0PgogIDx0ZXh0IHg9IjM0OCIgeT0iMjEyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjMDBjOGEwIj52LW1lZDogMS41cmVtPC90ZXh0PgogIDx0ZXh0IHg9IjM0OCIgeT0iMjMwIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjMDA4NGZmIj52LXNtOiAgMS4wcmVtPC90ZXh0Pgo8L3N2Zz4=)


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

### 6.6 Pictogram Panel

![Pictogram Panel](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjYwIiB2aWV3Qm94PSIwIDAgNTAwIDI2MCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyNjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8IS0tIFBpY3RvZ3JhbSBwYW5lbDogSHVtaWRpdHkgLS0+CiAgPHJlY3QgeD0iMTAiIHk9IjEwIiB3aWR0aD0iMTkwIiBoZWlnaHQ9IjEwMCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8IS0tIERyb3BsZXQgaWNvbiAtLT4KICA8cGF0aCBkPSJNNTIgNjAgQzUyIDYwIDM2IDc2IDM2IDg1IEExNiAxNiAwIDAgMCA2OCA4NSBDNjggNzYgNTIgNjAgNTIgNjBaIiBzdHJva2U9IiMwMGM4YTAiIHN0cm9rZS13aWR0aD0iMS41IiBmaWxsPSIjMDBjOGEwIiBmaWxsLW9wYWNpdHk9IjAuMiIvPgogIDx0ZXh0IHg9IjgyIiB5PSI0NCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+SFVNSURJVFk8L3RleHQ+CiAgPHRleHQgeD0iODIiIHk9IjgyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjI4IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iI2ZmZmZmZiI+NzkuMjwvdGV4dD4KICA8dGV4dCB4PSIxNTIiIHk9IjgyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEzIiBmaWxsPSIjNWE3ODk4Ij4lPC90ZXh0PgogIDwhLS0gUGljdG9ncmFtIHBhbmVsOiBUZW1wZXJhdHVyZSAtLT4KICA8cmVjdCB4PSIxMCIgeT0iMTI1IiB3aWR0aD0iMTkwIiBoZWlnaHQ9IjEwMCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8IS0tIFRoZXJtb21ldGVyIGljb24gLS0+CiAgPHJlY3QgeD0iNDYiIHk9IjEzOCIgd2lkdGg9IjEwIiBoZWlnaHQ9IjM2IiByeD0iNSIgc3Ryb2tlPSIjMDBjOGEwIiBzdHJva2Utd2lkdGg9IjEuNSIgZmlsbD0iIzAwYzhhMCIgZmlsbC1vcGFjaXR5PSIwLjE1Ii8+CiAgPGNpcmNsZSBjeD0iNTEiIGN5PSIxODMiIHI9IjkiIHN0cm9rZT0iIzAwYzhhMCIgc3Ryb2tlLXdpZHRoPSIxLjUiIGZpbGw9IiMwMGM4YTAiIGZpbGwtb3BhY2l0eT0iMC4zIi8+CiAgPHJlY3QgeD0iNDgiIHk9IjE1NSIgd2lkdGg9IjYiIGhlaWdodD0iMjAiIGZpbGw9IiMwMGM4YTAiIG9wYWNpdHk9IjAuNyIvPgogIDx0ZXh0IHg9IjgyIiB5PSIxNTkiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPkFJUiBURU1QRVJBVFVSRTwvdGV4dD4KICA8dGV4dCB4PSI4MiIgeT0iMTk3IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjI4IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iI2ZmZmZmZiI+MjcuMTwvdGV4dD4KICA8dGV4dCB4PSIxNDgiIHk9IjE5NyIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMyIgZmlsbD0iIzVhNzg5OCI+wrBDPC90ZXh0PgogIDwhLS0gUGljdG9ncmFtIHBhbmVsOiBQcmVzc3VyZSAtLT4KICA8cmVjdCB4PSIyMTUiIHk9IjEwIiB3aWR0aD0iMTkwIiBoZWlnaHQ9IjEwMCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8IS0tIEdhdWdlIGljb24gc2ltcGxpZmllZCAtLT4KICA8cGF0aCBkPSJNMjQ1IDgwIEEyNiAyNiAwIDAgMSAyOTcgODAiIHN0cm9rZT0iIzAwYzhhMCIgc3Ryb2tlLXdpZHRoPSIxLjUiIGZpbGw9Im5vbmUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgogIDxsaW5lIHgxPSIyNzEiIHkxPSI4MCIgeDI9IjI3MSIgeTI9IjU4IiBzdHJva2U9IiMwMGM4YTAiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIi8+CiAgPGNpcmNsZSBjeD0iMjcxIiBjeT0iODAiIHI9IjMiIGZpbGw9IiMwMGM4YTAiLz4KICA8dGV4dCB4PSIzMjAiIHk9IjQ0IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5BSVIgUFJFU1NVUkU8L3RleHQ+CiAgPHRleHQgeD0iMzEwIiB5PSI4MiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIyMiIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiNmZmZmZmYiPjEwMTMuMjwvdGV4dD4KICA8dGV4dCB4PSIzODgiIHk9IjgyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjNWE3ODk4Ij5tYmFyPC90ZXh0PgogIDwhLS0gUGljdG9ncmFtIHBhbmVsOiBEZXdwb2ludCAtLT4KICA8cmVjdCB4PSIyMTUiIHk9IjEyNSIgd2lkdGg9IjE5MCIgaGVpZ2h0PSIxMDAiIHJ4PSIxMCIgZmlsbD0iIzFlMWUxZSIgc3Ryb2tlPSIjMmUyZTJlIi8+CiAgPCEtLSBEcm9wbGV0cyBpY29uIC0tPgogIDxwYXRoIGQ9Ik0yNDQgMTcwIEMyNDQgMTcwIDIzNCAxODAgMjM0IDE4NiBBMTAgMTAgMCAwIDAgMjU0IDE4NiBDMjU0IDE4MCAyNDQgMTcwIDI0NCAxNzBaIiBzdHJva2U9IiMwMGM4YTAiIHN0cm9rZS13aWR0aD0iMS4yIiBmaWxsPSIjMDBjOGEwIiBmaWxsLW9wYWNpdHk9IjAuMiIvPgogIDxwYXRoIGQ9Ik0yNjIgMTYyIEMyNjIgMTYyIDI1MiAxNzIgMjUyIDE3OCBBMTAgMTAgMCAwIDAgMjcyIDE3OCBDMjcyIDE3MiAyNjIgMTYyIDI2MiAxNjJaIiBzdHJva2U9IiMwMGM4YTAiIHN0cm9rZS13aWR0aD0iMS4yIiBmaWxsPSIjMDBjOGEwIiBmaWxsLW9wYWNpdHk9IjAuMiIvPgogIDx0ZXh0IHg9IjI4NSIgeT0iMTU5IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5ERVdQT0lOVDwvdGV4dD4KICA8dGV4dCB4PSIyODUiIHk9IjE5NyIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIyOCIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiNmZmZmZmYiPjE0LjE8L3RleHQ+CiAgPHRleHQgeD0iMzQ1IiB5PSIxOTciIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTMiIGZpbGw9IiM1YTc4OTgiPsKwQzwvdGV4dD4KICA8IS0tIERlc2NyaXB0aW9uIC0tPgogIDxyZWN0IHg9IjQyMCIgeT0iMTAiIHdpZHRoPSI3MCIgaGVpZ2h0PSIyMTUiIHJ4PSIxMCIgZmlsbD0iIzFlMWUxZSIgc3Ryb2tlPSIjMmUyZTJlIi8+CiAgPHRleHQgeD0iNDM4IiB5PSIzNiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+UElDVE88L3RleHQ+CiAgPHRleHQgeD0iNDM4IiB5PSI1MiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjYzhjOGM4Ij5JY29uICs8L3RleHQ+CiAgPHRleHQgeD0iNDM4IiB5PSI2OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjYzhjOGM4Ij5MYWJlbCArPC90ZXh0PgogIDx0ZXh0IHg9IjQzOCIgeT0iODQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iNyIgZmlsbD0iI2M4YzhjOCI+VmFsdWU8L3RleHQ+CiAgPHRleHQgeD0iNDM4IiB5PSIxMTAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iNyIgZmlsbD0iIzVhNzg5OCIgZm9udC13ZWlnaHQ9ImJvbGQiPklDT05TPC90ZXh0PgogIDx0ZXh0IHg9IjQzOCIgeT0iMTI2IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjciIGZpbGw9IiMwMDg0ZmYiPlRhYmxlcjwvdGV4dD4KICA8dGV4dCB4PSI0MzgiIHk9IjE0MiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjMDA4NGZmIj5JY29uczwvdGV4dD4KICA8dGV4dCB4PSI0MzgiIHk9IjE1OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjMDA4NGZmIj5NSVQ8L3RleHQ+Cjwvc3ZnPg==)


A compact panel combining a Tabler Icon with a value. Used in dense layouts where the icon provides immediate visual identification.

```html
<div class="panel picto-panel">
  <!-- Tabler icon (inline SVG, 36×36) -->
  <svg class="picto-icon" xmlns="http://www.w3.org/2000/svg"
       width="36" height="36" viewBox="0 0 24 24"
       fill="none" stroke="var(--accent)" stroke-width="1.5"
       stroke-linecap="round" stroke-linejoin="round">
    <!-- icon path from Tabler Icons -->
  </svg>
  <div>
    <div class="lbl">Humidity</div>
    <div class="v-med">79.2<span class="u">%</span></div>
  </div>
</div>
```

```css
.picto-panel {
  display: flex;
  align-items: center;
  gap: 14px;
}
.picto-icon {
  flex-shrink: 0;
  color: var(--accent);
}
```

See Section 9 for the standard icon assignments.

---

### 6.7 Sparkline Panel

![Sparkline Panel](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjIwIiB2aWV3Qm94PSIwIDAgNTAwIDIyMCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyMjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8IS0tIFNwYXJrbGluZSBwYW5lbCAtLT4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSIyNjAiIGhlaWdodD0iMjAwIiByeD0iMTAiIGZpbGw9IiMxZTFlMWUiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjI2IiB5PSIzNCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+VEVNUEVSQVRVUkU8L3RleHQ+CiAgPHRleHQgeD0iMjYiIHk9IjY4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjMwIiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iI2ZmZmZmZiI+MjcuMTwvdGV4dD4KICA8dGV4dCB4PSIxMDgiIHk9IjY4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEzIiBmaWxsPSIjNWE3ODk4Ij7CsEM8L3RleHQ+CiAgPCEtLSBTcGFya2xpbmUgYXJlYSAtLT4KICA8cmVjdCB4PSIyMCIgeT0iODIiIHdpZHRoPSIyMzAiIGhlaWdodD0iNTYiIHJ4PSI0IiBmaWxsPSIjMGEwYTE0Ii8+CiAgPCEtLSBHcmlkIGxpbmVzIC0tPgogIDxsaW5lIHgxPSIyMCIgeTE9IjEwMCIgeDI9IjI1MCIgeTI9IjEwMCIgc3Ryb2tlPSIjMjIyIiBzdHJva2Utd2lkdGg9IjEiLz4KICA8bGluZSB4MT0iMjAiIHkxPSIxMTgiIHgyPSIyNTAiIHkyPSIxMTgiIHN0cm9rZT0iIzIyMiIgc3Ryb2tlLXdpZHRoPSIxIi8+CiAgPCEtLSBTcGFya2xpbmUgLS0+CiAgPHBvbHlsaW5lIHBvaW50cz0iMTAuMCw3Ni4wIDEzLjQsNjguNiAxNi44LDc1LjcgMjAuMiw3Ni42IDIzLjYsODIuNSAyNi45LDc1LjYgMzAuMyw2Mi44IDMzLjcsNjkuNSAzNy4xLDYzLjYgNDAuNSw3MS4yIDQzLjksNjkuOCA0Ny4zLDcxLjggNTAuNyw4OS42IDU0LjEsNjUuMyA1Ny41LDY4LjcgNjAuOCw2OC44IDY0LjIsODkuOSA2Ny42LDkwLjQgNzEuMCw4Mi4yIDc0LjQsNzguMSA3Ny44LDcwLjYgODEuMiw3NC4wIDg0LjYsNjguNSA4OC4wLDc5LjggOTEuNCw3MC42IDk0LjcsNjkuOCA5OC4xLDc5LjkgMTAxLjUsNTcuMCAxMDQuOSw2OC4yIDEwOC4zLDYyLjAgMTExLjcsNzkuNiAxMTUuMSw4MC43IDExOC41LDc2LjkgMTIxLjksNzQuNiAxMjUuMyw2Ny41IDEyOC42LDcxLjIgMTMyLjAsNzcuOSAxMzUuNCw4Mi44IDEzOC44LDc4LjYgMTQyLjIsNjEuOCAxNDUuNiw4MS40IDE0OS4wLDcxLjIgMTUyLjQsNjkuNSAxNTUuOCw4Ny45IDE1OS4yLDczLjEgMTYyLjUsNjEuMCAxNjUuOSw5My4wIDE2OS4zLDc2LjcgMTcyLjcsNzQuNiAxNzYuMSw4MS41IDE3OS41LDY4LjggMTgyLjksNzQuMiAxODYuMyw4Ny43IDE4OS43LDY1LjYgMTkzLjEsNjcuMSAxOTYuNCw2NC40IDE5OS44LDU5LjcgMjAzLjIsNzAuMSAyMDYuNiw3Mi40IDIxMC4wLDg2LjEiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzAwODRmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lam9pbj0icm91bmQiLz4KICA8IS0tIE1pbi9tYXggbGFiZWxzIC0tPgogIDx0ZXh0IHg9IjIyIiB5PSI5MyIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjNWE3ODk4Ij4yOC41PC90ZXh0PgogIDx0ZXh0IHg9IjIyIiB5PSIxMzMiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iNyIgZmlsbD0iIzVhNzg5OCI+MjUuNTwvdGV4dD4KICA8IS0tIFRpbWUgbGFiZWwgLS0+CiAgPHRleHQgeD0iMTM1IiB5PSIxNTUiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCI+4oaQIDYwIHNhbXBsZXMg4oaSPC90ZXh0PgogIDx0ZXh0IHg9IjEzNSIgeT0iMTcwIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZpbGw9IiM1YTc4OTgiPjUgbWluIEAgNXMgaW50ZXJ2YWw8L3RleHQ+CiAgPCEtLSBEZXNjcmlwdGlvbiAtLT4KICA8cmVjdCB4PSIyODUiIHk9IjEwIiB3aWR0aD0iMjA1IiBoZWlnaHQ9IjIwMCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8dGV4dCB4PSIzMDMiIHk9IjM0IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiIGZvbnQtd2VpZ2h0PSJib2xkIj5TUEFSS0xJTkUgUEFORUw8L3RleHQ+CiAgPHRleHQgeD0iMzAzIiB5PSI1OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSIxMSIgZmlsbD0iI2M4YzhjOCI+wrcgQ29tcGFjdCB0cmVuZCBjaGFydDwvdGV4dD4KICA8dGV4dCB4PSIzMDMiIHk9Ijc4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBFbWJlZGRlZCBpbiBwYW5lbDwvdGV4dD4KICA8dGV4dCB4PSIzMDMiIHk9Ijk4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyA2MCBzYW1wbGUgYnVmZmVyPC90ZXh0PgogIDx0ZXh0IHg9IjMwMyIgeT0iMTE4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjExIiBmaWxsPSIjYzhjOGM4Ij7CtyBBdXRvIG1pbi9tYXggc2NhbGU8L3RleHQ+CiAgPHRleHQgeD0iMzAzIiB5PSIxMzgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTEiIGZpbGw9IiNjOGM4YzgiPsK3IENhbnZhcy1iYXNlZDwvdGV4dD4KICA8dGV4dCB4PSIzMDMiIHk9IjE2OCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+VVNFIENBU0VTPC90ZXh0PgogIDx0ZXh0IHg9IjMwMyIgeT0iMTg4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjMDA4NGZmIj5UZW1wIMK3IFByZXNzdXJlPC90ZXh0PgogIDx0ZXh0IHg9IjMwMyIgeT0iMjA2IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjEwIiBmaWxsPSIjMDA4NGZmIj5Wb2x0YWdlIMK3IEFueSB0cmVuZDwvdGV4dD4KPC9zdmc+)


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

![Status Pill](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjAwIiB2aWV3Qm94PSIwIDAgNTAwIDIwMCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyMDAiIGZpbGw9IiMwYjEyMjAiLz4KICA8IS0tIEhlYWRlciBiYXIgc2ltdWxhdGlvbiAtLT4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSI0ODAiIGhlaWdodD0iNzAiIHJ4PSIxMCIgZmlsbD0iIzFlMWUxZSIgc3Ryb2tlPSIjMmUyZTJlIi8+CiAgPCEtLSBMb2dvIHBsYWNlaG9sZGVyIC0tPgogIDxjaXJjbGUgY3g9IjQwIiBjeT0iNDUiIHI9IjE4IiBmaWxsPSJub25lIiBzdHJva2U9IiMwMGM4YTAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtZGFzaGFycmF5PSIzIDMiIG9wYWNpdHk9IjAuNSIvPgogIDxwb2x5Z29uIHBvaW50cz0iNDAsMjggNDMsNDUgNDAsNDMgMzcsNDUiIGZpbGw9IiMwMGM4YTAiLz4KICA8cG9seWdvbiBwb2ludHM9IjQwLDYyIDM3LDQ1IDQwLDQ3IDQzLDQ1IiBmaWxsPSIjNWE3ODk4Ii8+CiAgPCEtLSBEZXZpY2UgbmFtZSAtLT4KICA8dGV4dCB4PSI2OCIgeT0iNDAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTQiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjZmZmZmZmIj5EZXZpY2UgTmFtZTwvdGV4dD4KICA8dGV4dCB4PSI2OCIgeT0iNTgiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzVhNzg5OCI+RVNQMzIgwrcgMTkyLjE2OC4xLjQyIMK3IGRldmljZS5sb2NhbDwvdGV4dD4KICA8IS0tIE9ubGluZSBwaWxsIC0tPgogIDxyZWN0IHg9IjM2MCIgeT0iMzAiIHdpZHRoPSI4OCIgaGVpZ2h0PSIyNiIgcng9IjEzIiBmaWxsPSJyZ2JhKDAsMjAwLDE2MCwwLjEpIiBzdHJva2U9InJnYmEoMCwyMDAsMTYwLDAuMykiLz4KICA8Y2lyY2xlIGN4PSIzNzciIGN5PSI0MyIgcj0iNCIgZmlsbD0iIzAwYzhhMCI+PGFuaW1hdGUgYXR0cmlidXRlTmFtZT0ib3BhY2l0eSIgdmFsdWVzPSIxOzAuMzsxIiBkdXI9IjJzIiByZXBlYXRDb3VudD0iaW5kZWZpbml0ZSIvPjwvY2lyY2xlPgogIDx0ZXh0IHg9IjM4NiIgeT0iNDciIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iMTAiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjMDBjOGEwIj5PbmxpbmU8L3RleHQ+CiAgPCEtLSBPZmZsaW5lIHBpbGwgLS0+CiAgPHJlY3QgeD0iMTAiIHk9IjExMCIgd2lkdGg9IjIwMCIgaGVpZ2h0PSI2MCIgcng9IjEwIiBmaWxsPSIjMWUxZTFlIiBzdHJva2U9IiMyZTJlMmUiLz4KICA8dGV4dCB4PSIzMCIgeT0iMTMyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiM1YTc4OTgiPk9OTElORSBTVEFURTwvdGV4dD4KICA8cmVjdCB4PSIzMCIgeT0iMTQyIiB3aWR0aD0iODgiIGhlaWdodD0iMTgiIHJ4PSI5IiBmaWxsPSJyZ2JhKDAsMjAwLDE2MCwwLjEpIiBzdHJva2U9InJnYmEoMCwyMDAsMTYwLDAuMykiLz4KICA8Y2lyY2xlIGN4PSI0NCIgY3k9IjE1MSIgcj0iMyIgZmlsbD0iIzAwYzhhMCIvPgogIDx0ZXh0IHg9IjUyIiB5PSIxNTUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiMwMGM4YTAiPk9ubGluZTwvdGV4dD4KICA8IS0tIE9mZmxpbmUgc3RhdGUgLS0+CiAgPHJlY3QgeD0iMjMwIiB5PSIxMTAiIHdpZHRoPSIyMDAiIGhlaWdodD0iNjAiIHJ4PSIxMCIgZmlsbD0iIzFlMWUxZSIgc3Ryb2tlPSIjMmUyZTJlIi8+CiAgPHRleHQgeD0iMjUwIiB5PSIxMzIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzVhNzg5OCI+T0ZGTElORSBTVEFURTwvdGV4dD4KICA8cmVjdCB4PSIyNTAiIHk9IjE0MiIgd2lkdGg9Ijk0IiBoZWlnaHQ9IjE4IiByeD0iOSIgZmlsbD0icmdiYSgyMzIsNjQsNjQsMC4xKSIgc3Ryb2tlPSJyZ2JhKDIzMiw2NCw2NCwwLjMpIi8+CiAgPGNpcmNsZSBjeD0iMjY0IiBjeT0iMTUxIiByPSIzIiBmaWxsPSIjZTg0MDQwIi8+CiAgPHRleHQgeD0iMjcyIiB5PSIxNTUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiNlODQwNDAiPk9mZmxpbmU8L3RleHQ+Cjwvc3ZnPg==)


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

### 6.9 Protocol Log Panel

![Protocol Log Panel](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMjIwIiB2aWV3Qm94PSIwIDAgNTAwIDIyMCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIyMjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSI0ODAiIGhlaWdodD0iMjAwIiByeD0iMTAiIGZpbGw9IiMxZTFlMWUiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjI4IiB5PSIzNCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCIgbGV0dGVyLXNwYWNpbmc9IjEiPk5NRUEgMDE4MzwvdGV4dD4KICA8IS0tIExvZyBib3ggLS0+CiAgPHJlY3QgeD0iMjAiIHk9IjQ0IiB3aWR0aD0iNDYwIiBoZWlnaHQ9IjE1NiIgcng9IjYiIGZpbGw9IiMwNzBmMWMiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjMyIiB5PSI2NiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjM2E1ODc4Ij4wODoxNDoyMjwvdGV4dD4KICA8dGV4dCB4PSI4OCIgeT0iNjYiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzAwYzhhMCI+JFdJTVdWLDI2OS4yLFIsMTIuMzQsTixBKjRBPC90ZXh0PgogIDx0ZXh0IHg9IjMyIiB5PSI4NCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjM2E1ODc4Ij4wODoxNDoyMjwvdGV4dD4KICA8dGV4dCB4PSI4OCIgeT0iODQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzAwYzhhMCI+JFdJVldSLDA5MC44LFIsMTIuMzQsTiw2LjM1LE0sMjIuODUsSyoyQjwvdGV4dD4KICA8dGV4dCB4PSIzMiIgeT0iMTAyIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiMzYTU4NzgiPjA4OjE0OjIyPC90ZXh0PgogIDx0ZXh0IHg9Ijg4IiB5PSIxMDIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzAwYzhhMCI+JFdJVlBXLDAwMC4zOCxXLDAwMC41MixLKjJDPC90ZXh0PgogIDx0ZXh0IHg9IjMyIiB5PSIxMjAiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzNhNTg3OCI+MDg6MTQ6MjI8L3RleHQ+CiAgPHRleHQgeD0iODgiIHk9IjEyMCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjMDBjOGEwIj4kUFdXU1QsMjcuMTMqMUY8L3RleHQ+CiAgPHRleHQgeD0iMzIiIHk9IjEzOCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjM2E1ODc4Ij4wODoxNDoyMjwvdGV4dD4KICA8dGV4dCB4PSI4OCIgeT0iMTM4IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiMwMGM4YTAiPiRQV0lORixWMS4yMSw4MCwzMjQ1Niw5OCozRDwvdGV4dD4KICA8dGV4dCB4PSIzMiIgeT0iMTU2IiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZpbGw9IiMyYTNhNTAiPjA4OjE0OjE3PC90ZXh0PgogIDx0ZXh0IHg9Ijg4IiB5PSIxNTYiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzJhNjA1MCI+JFdJTVdWLDI2OC45LFIsMTIuMjEsTixBKjRCPC90ZXh0PgogIDx0ZXh0IHg9IjMyIiB5PSIxNzQiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzJhM2E1MCI+MDg6MTQ6MTc8L3RleHQ+CiAgPHRleHQgeD0iODgiIHk9IjE3NCIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjMmE2MDUwIj4kV0lWV1IsMDkxLjEsUiwxMi4yMSxOLDYuMjgsTSwyMi41OSxLKjJBPC90ZXh0PgogIDx0ZXh0IHg9IjMyIiB5PSIxOTIiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOSIgZmlsbD0iIzFhMmE0MCI+MDg6MTQ6MTI8L3RleHQ+CiAgPHRleHQgeD0iODgiIHk9IjE5MiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI5IiBmaWxsPSIjMWE1MDQwIj4kV0lNV1YsMjcwLjEsUiwxMi41NixOLEEqNDk8L3RleHQ+Cjwvc3ZnPg==)


A scrolling log of raw protocol strings (NMEA 0183, Modbus, MQTT, etc.). Provides protocol-level transparency for debugging and integration verification.

```html
<div class="panel">
  <div class="lbl">NMEA 0183</div>
  <div class="log-box" id="protocolLog">
    <span class="log-ts">Waiting for data…</span>
  </div>
</div>
```

```css
.log-box {
  background: var(--nmea-bg);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 10px 14px;
  font-family: var(--mono);
  font-size: .67rem;
  color: var(--accent);
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-height: 60px;
}
.log-ts { color: var(--nmea-ts); }
```

```javascript
var logHistory = [];
function updateLog(strings) {
  var ts    = new Date().toLocaleTimeString();
  var lines = Object.values(strings).map(function(s) {
    return '<span class="log-ts">' + ts + '</span> ' + s;
  });
  logHistory = lines.concat(logHistory).slice(0, 10);
  document.getElementById('protocolLog').innerHTML = logHistory.join('<br>');
}
```

---

### 6.10 Validation Strip

![Validation Strip](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI1MDAiIGhlaWdodD0iMTYwIiB2aWV3Qm94PSIwIDAgNTAwIDE2MCI+CiAgPHJlY3Qgd2lkdGg9IjUwMCIgaGVpZ2h0PSIxNjAiIGZpbGw9IiMwYjEyMjAiLz4KICA8cmVjdCB4PSIxMCIgeT0iMTAiIHdpZHRoPSI0ODAiIGhlaWdodD0iMTQwIiByeD0iMTAiIGZpbGw9IiMxZTFlMWUiIHN0cm9rZT0iIzJlMmUyZSIvPgogIDx0ZXh0IHg9IjI4IiB5PSIzNiIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmaWxsPSIjNWE3ODk4IiBmb250LXdlaWdodD0iYm9sZCI+QUREIHYxLjA6PC90ZXh0PgogIDwhLS0gQmFkZ2VzIHJvdyAxIC0tPgogIDxyZWN0IHg9IjkwIiAgeT0iMjAiIHdpZHRoPSI2MiIgaGVpZ2h0PSIyMCIgcng9IjQiIGZpbGw9InJnYmEoMCwyMDAsMTYwLDAuMSkiIHN0cm9rZT0icmdiYSgwLDIwMCwxNjAsMC4yNSkiLz4KICA8dGV4dCB4PSIxMjEiIHk9IjM0IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjMDBjOGEwIj7inJMgc3RydWN0dXJlPC90ZXh0PgogIDxyZWN0IHg9IjE1OCIgeT0iMjAiIHdpZHRoPSI4OCIgaGVpZ2h0PSIyMCIgcng9IjQiIGZpbGw9InJnYmEoMCwyMDAsMTYwLDAuMSkiIHN0cm9rZT0icmdiYSgwLDIwMCwxNjAsMC4yNSkiLz4KICA8dGV4dCB4PSIyMDIiIHk9IjM0IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjMDBjOGEwIj7inJMgY29tcHJlaGVuc2liaWxpdHk8L3RleHQ+CiAgPHJlY3QgeD0iMjUyIiB5PSIyMCIgd2lkdGg9IjY2IiBoZWlnaHQ9IjIwIiByeD0iNCIgZmlsbD0icmdiYSgwLDIwMCwxNjAsMC4xKSIgc3Ryb2tlPSJyZ2JhKDAsMjAwLDE2MCwwLjI1KSIvPgogIDx0ZXh0IHg9IjI4NSIgeT0iMzQiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZm9udC13ZWlnaHQ9ImJvbGQiIGZpbGw9IiMwMGM4YTAiPuKckyBmdW5jdGlvbmFsPC90ZXh0PgogIDxyZWN0IHg9IjMyNCIgeT0iMjAiIHdpZHRoPSI4OCIgaGVpZ2h0PSIyMCIgcng9IjQiIGZpbGw9InJnYmEoMCwyMDAsMTYwLDAuMSkiIHN0cm9rZT0icmdiYSgwLDIwMCwxNjAsMC4yNSkiLz4KICA8dGV4dCB4PSIzNjgiIHk9IjM0IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjMDBjOGEwIj7inJMgcnVsZXNfY29tcGxpYW5jZTwvdGV4dD4KICA8IS0tIEJhZGdlcyByb3cgMiAtLT4KICA8cmVjdCB4PSIyOCIgIHk9IjQ4IiB3aWR0aD0iNjIiIGhlaWdodD0iMjAiIHJ4PSI0IiBmaWxsPSJyZ2JhKDAsMjAwLDE2MCwwLjEpIiBzdHJva2U9InJnYmEoMCwyMDAsMTYwLDAuMjUpIi8+CiAgPHRleHQgeD0iNTkiICB5PSI2MiIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iIzAwYzhhMCI+4pyTIHNlY3VyaXR5PC90ZXh0PgogIDxyZWN0IHg9Ijk2IiAgeT0iNDgiIHdpZHRoPSI2NiIgaGVpZ2h0PSIyMCIgcng9IjQiIGZpbGw9InJnYmEoMCwyMDAsMTYwLDAuMSkiIHN0cm9rZT0icmdiYSgwLDIwMCwxNjAsMC4yNSkiLz4KICA8dGV4dCB4PSIxMjkiIHk9IjYyIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjgiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjMDBjOGEwIj7inJMgZGlzY292ZXJ5PC90ZXh0PgogIDxyZWN0IHg9IjE2OCIgeT0iNDgiIHdpZHRoPSIxMDgiIGhlaWdodD0iMjAiIHJ4PSI0IiBmaWxsPSJyZ2JhKDAsMjAwLDE2MCwwLjEpIiBzdHJva2U9InJnYmEoMCwyMDAsMTYwLDAuMjUpIi8+CiAgPHRleHQgeD0iMjIyIiB5PSI2MiIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI4IiBmb250LXdlaWdodD0iYm9sZCIgZmlsbD0iIzAwYzhhMCI+4pyTIHRpbWluZ19jb21wbGlhbmNlPC90ZXh0PgogIDwhLS0gUEFTU0VEIGJhZGdlIC0tPgogIDxyZWN0IHg9IjI4IiB5PSI4MiIgd2lkdGg9IjQ0MCIgaGVpZ2h0PSIyNCIgcng9IjQiIGZpbGw9InJnYmEoMCwxMzIsMjU1LDAuMSkiIHN0cm9rZT0icmdiYSgwLDEzMiwyNTUsMC4yNSkiLz4KICA8dGV4dCB4PSIyNDgiIHk9Ijk4IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjkiIGZvbnQtd2VpZ2h0PSJib2xkIiBmaWxsPSIjMDA4NGZmIj5QQVNTRUQgIMK3ICBjbGF1ZGUtc29ubmV0LTQtNiAgwrcgIDIwMjYtMDctMDcgIMK3ICBBdXRvbm9teSBMZXZlbCAxPC90ZXh0PgogIDwhLS0gQmFkZ2UgdmFyaWFudHMgbGVnZW5kIC0tPgogIDx0ZXh0IHg9IjI4IiB5PSIxMjYiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iOCIgZmlsbD0iIzVhNzg5OCI+QmFkZ2UgdmFyaWFudHM6PC90ZXh0PgogIDxyZWN0IHg9IjExOCIgeT0iMTE0IiB3aWR0aD0iNDQiIGhlaWdodD0iMTYiIHJ4PSIzIiBmaWxsPSJyZ2JhKDAsMjAwLDE2MCwwLjEpIiBzdHJva2U9InJnYmEoMCwyMDAsMTYwLDAuMjUpIi8+CiAgPHRleHQgeD0iMTQwIiB5PSIxMjYiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iNyIgZmlsbD0iIzAwYzhhMCI+cGFzc2VkPC90ZXh0PgogIDxyZWN0IHg9IjE2OCIgeT0iMTE0IiB3aWR0aD0iMzYiIGhlaWdodD0iMTYiIHJ4PSIzIiBmaWxsPSJyZ2JhKDI0NSwxNjYsMzUsMC4xKSIgc3Ryb2tlPSJyZ2JhKDI0NSwxNjYsMzUsMC4yNSkiLz4KICA8dGV4dCB4PSIxODYiIHk9IjEyNiIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZm9udC1mYW1pbHk9Im1vbm9zcGFjZSIgZm9udC1zaXplPSI3IiBmaWxsPSIjZjVhNjIzIj53YXJuPC90ZXh0PgogIDxyZWN0IHg9IjIxMCIgeT0iMTE0IiB3aWR0aD0iMzAiIGhlaWdodD0iMTYiIHJ4PSIzIiBmaWxsPSJyZ2JhKDIzMiw2NCw2NCwwLjEpIiBzdHJva2U9InJnYmEoMjMyLDY0LDY0LDAuMjUpIi8+CiAgPHRleHQgeD0iMjI1IiB5PSIxMjYiIHRleHQtYW5jaG9yPSJtaWRkbGUiIGZvbnQtZmFtaWx5PSJtb25vc3BhY2UiIGZvbnQtc2l6ZT0iNyIgZmlsbD0iI2U4NDA0MCI+ZmFpbDwvdGV4dD4KICA8cmVjdCB4PSIyNDYiIHk9IjExNCIgd2lkdGg9IjMwIiBoZWlnaHQ9IjE2IiByeD0iMyIgZmlsbD0icmdiYSgwLDEzMiwyNTUsMC4xKSIgc3Ryb2tlPSJyZ2JhKDAsMTMyLDI1NSwwLjI1KSIvPgogIDx0ZXh0IHg9IjI2MSIgeT0iMTI2IiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LWZhbWlseT0ibW9ub3NwYWNlIiBmb250LXNpemU9IjciIGZpbGw9IiMwMDg0ZmYiPmluZm88L3RleHQ+Cjwvc3ZnPg==)


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

### 7.3 Slider

For numeric range inputs (set offset, set threshold, adjust averaging window).

```html
<div class="slider-wrap">
  <div class="lbl">Wind Offset <span class="v-sm" id="offsetVal">0<span class="u">°</span></span></div>
  <input type="range" class="slider" id="offsetSlider"
         min="-180" max="180" step="1" value="0"
         oninput="document.getElementById('offsetVal').textContent = this.value + '°'"
         onchange="setOffset(this.value)">
  <div class="slider-labels"><span>-180°</span><span>0°</span><span>+180°</span></div>
</div>
```

```css
.slider-wrap { display: flex; flex-direction: column; gap: 6px; }
.slider {
  -webkit-appearance: none; appearance: none;
  width: 100%; height: 4px; border-radius: 2px;
  background: var(--input-brd); outline: none; cursor: pointer;
}
.slider::-webkit-slider-thumb {
  -webkit-appearance: none; appearance: none;
  width: 18px; height: 18px; border-radius: 50%;
  background: var(--accent); cursor: pointer;
}
.slider::-moz-range-thumb {
  width: 18px; height: 18px; border-radius: 50%;
  background: var(--accent); cursor: pointer; border: none;
}
.slider-labels {
  display: flex; justify-content: space-between;
  font-family: var(--mono); font-size: .6rem; color: var(--muted);
}
```

---

### 7.4 Dropdown Selector

For selecting from a fixed set of options (speed unit, averaging window, display mode).

```html
<div class="ctrl-wrap">
  <span class="lbl">Speed Unit</span>
  <select class="ctrl-select" id="speedUnit" onchange="setSpeedUnit(this.value)">
    <option value="kn">kn — Knots</option>
    <option value="ms">m/s — Metres per second</option>
    <option value="kmh">km/h — Kilometres per hour</option>
    <option value="bft">Bft — Beaufort</option>
  </select>
</div>
```

```css
.ctrl-wrap {
  display: flex; align-items: center; gap: 8px;
  font-family: var(--mono); font-size: .62rem; color: var(--muted);
}
.ctrl-select {
  padding: 4px 10px;
  background: var(--input-bg); border: 1px solid var(--input-brd);
  border-radius: 6px; color: var(--accent);
  font-family: var(--mono); font-size: .68rem; font-weight: 600;
  cursor: pointer; appearance: none; -webkit-appearance: none; outline: none;
}
.ctrl-select:focus { border-color: var(--accent); }
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

### 7.6 Checkbox Group

For selecting multiple options from a fixed set.

```html
<fieldset class="checkbox-group">
  <legend class="lbl">NMEA Sentences</legend>
  <label class="checkbox-item">
    <input type="checkbox" checked onchange="toggleSentence('WIMWV', this.checked)">
    <span>$WIMWV — Wind Speed and Angle</span>
  </label>
  <label class="checkbox-item">
    <input type="checkbox" checked onchange="toggleSentence('WIVWR', this.checked)">
    <span>$WIVWR — Relative Wind Speed</span>
  </label>
</fieldset>
```

```css
.checkbox-group { border: none; padding: 0; margin: 0; }
.checkbox-item {
  display: flex; align-items: center; gap: 8px;
  font-family: var(--mono); font-size: .72rem; color: var(--text);
  padding: 4px 0; cursor: pointer;
}
.checkbox-item input[type="checkbox"] { accent-color: var(--accent); width: 14px; height: 14px; }
```

---

### 7.7 Radio Group

For selecting exactly one option from a fixed set.

```html
<fieldset class="radio-group">
  <legend class="lbl">Wind Type</legend>
  <label class="radio-item">
    <input type="radio" name="windType" value="R" checked onchange="setWindType('R')">
    <span>Relative (R)</span>
  </label>
  <label class="radio-item">
    <input type="radio" name="windType" value="T" onchange="setWindType('T')">
    <span>True (T)</span>
  </label>
</fieldset>
```

```css
.radio-group { border: none; padding: 0; margin: 0; }
.radio-item {
  display: flex; align-items: center; gap: 8px;
  font-family: var(--mono); font-size: .72rem; color: var(--text);
  padding: 4px 0; cursor: pointer;
}
.radio-item input[type="radio"] { accent-color: var(--accent); width: 14px; height: 14px; }
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

## 8. Media Components

### 8.1 Image Panel

Displays a static or periodically refreshed image from the device (camera snapshot, QR code, status graphic).

```html
<div class="panel">
  <div class="lbl">Camera — Bow</div>
  <div class="media-wrap">
    <img id="cameraImg" src="" alt="Camera snapshot"
         style="width:100%; border-radius:6px; display:block">
    <div class="media-meta">
      Last update: <span id="imgTimestamp">—</span>
    </div>
  </div>
</div>
```

```javascript
function refreshImage(url) {
  var img = document.getElementById('cameraImg');
  img.src = url + '?t=' + Math.floor(Date.now() / 1000);
  document.getElementById('imgTimestamp').textContent =
    new Date().toLocaleTimeString();
}
// Refresh every 10 seconds
setInterval(function() { refreshImage('{device_host}/snapshot'); }, 10000);
```

```css
.media-wrap { position: relative; }
.media-meta {
  font-family: var(--mono); font-size: .62rem;
  color: var(--muted); margin-top: 6px;
}
```

---

### 8.2 Video Stream Panel

Displays a live MJPEG stream from the device. MJPEG is the standard format for ESP32-CAM and similar embedded cameras.

```html
<div class="panel">
  <div class="lbl">Live Video</div>
  <div class="media-wrap">
    <img id="videoStream"
         src="{device_host}/stream"
         alt="Live stream"
         style="width:100%; border-radius:6px; display:block">
    <div class="stream-overlay">
      <span class="stream-badge">● LIVE</span>
    </div>
  </div>
</div>
```

```css
.media-wrap     { position: relative; }
.stream-overlay { position: absolute; top: 8px; left: 8px; }
.stream-badge {
  background: rgba(232,64,64,.85); color: #fff;
  font-family: var(--mono); font-size: .62rem; font-weight: 700;
  padding: 2px 7px; border-radius: 4px;
}
```

Note: MJPEG streams via `<img src>` are natively supported in all browsers without JavaScript. For HLS streams, include hls.js from a CDN.

---

### 8.3 Map Panel (Leaflet / OpenStreetMap)

Displays the device's GPS position on an interactive map. Uses [Leaflet.js](https://leafletjs.com) with OpenStreetMap tiles — fully open-source, no API key required.

```html
<!-- In <head> -->
<link  rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>

<!-- Panel -->
<div class="panel">
  <div class="lbl">GPS Position</div>
  <div id="mapPanel" style="height:300px; border-radius:6px; overflow:hidden"></div>
  <div class="media-meta">
    <span id="gpsLat">—</span> / <span id="gpsLon">—</span>
    &nbsp;·&nbsp; COG: <span id="gpsCog">—</span>°
    &nbsp;·&nbsp; SOG: <span id="gpsSog">—</span> kn
  </div>
</div>
```

```javascript
var map    = null;
var marker = null;

function initMap(lat, lon) {
  map = L.map('mapPanel', { zoomControl: true, attributionControl: true });
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
  }).addTo(map);
  map.setView([lat, lon], 13);
  marker = L.marker([lat, lon]).addTo(map);
}

function updateMap(lat, lon, cog, sog) {
  if (!map) { initMap(lat, lon); return; }
  marker.setLatLng([lat, lon]);
  map.panTo([lat, lon]);
  document.getElementById('gpsLat').textContent = lat.toFixed(5);
  document.getElementById('gpsLon').textContent = lon.toFixed(5);
  document.getElementById('gpsCog').textContent = cog.toFixed(1);
  document.getElementById('gpsSog').textContent = sog.toFixed(2);
}
```

---

## 9. Pictogram Library

The ADD Dashboard Style Guide uses **Tabler Icons** as its standard pictogram library. Tabler Icons is an open-source collection of over 5,000 SVG icons, released under the MIT license.

- Repository: https://github.com/tabler/tabler-icons
- Website: https://tabler.io/icons
- License: MIT

### 9.1 Usage

Tabler Icons are used as inline SVG with `stroke="var(--accent)"`. This ensures automatic adaptation to all themes. Never use `fill` for Tabler Icons — they are stroke-based.

```html
<svg xmlns="http://www.w3.org/2000/svg"
     width="36" height="36" viewBox="0 0 24 24"
     fill="none"
     stroke="var(--accent)"
     stroke-width="1.5"
     stroke-linecap="round"
     stroke-linejoin="round">
  <!-- paste path(s) from tabler.io/icons here -->
</svg>
```

**Standard sizes:**

| Context | Size | stroke-width |
|---|---|---|
| Pictogram panel | 36×36 | 1.5 |
| Section header | 20×20 | 1.5 |
| Button icon | 16×16 | 2.0 |
| Footer / metadata | 14×14 | 2.0 |

### 9.2 Phosphor Icons

**Phosphor Icons** is a second recommended icon library, offering a different visual character and a unique weight system.

- Repository: https://github.com/phosphor-icons/web
- Website: https://phosphoricons.com
- License: MIT

**Key difference from Tabler Icons:** Phosphor provides every icon in 6 weights — Thin, Light, Regular, Bold, Fill, and Duotone. This makes it possible to encode state through weight rather than color alone, which is particularly useful for colorblind accessibility and the B&W theme.

| Weight | CSS class | Recommended use |
|---|---|---|
| `regular` | `ph ph-{name}` | Default / inactive state |
| `bold` | `ph-bold ph-{name}` | Emphasized / highlighted |
| `fill` | `ph-fill ph-{name}` | Active / selected / alarm state |
| `duotone` | `ph-duotone ph-{name}` | Decorative / secondary panels |
| `light` | `ph-light ph-{name}` | Subtle / background |
| `thin` | `ph-thin ph-{name}` | Minimal / compact layouts |

**CDN inclusion** — load only the weights you need:

```html
<!-- Regular weight (default) -->
<link rel="stylesheet"
  href="https://cdn.jsdelivr.net/npm/@phosphor-icons/web@2.1.2/src/regular/style.css"/>

<!-- Fill weight (for active/alarm states) -->
<link rel="stylesheet"
  href="https://cdn.jsdelivr.net/npm/@phosphor-icons/web@2.1.2/src/fill/style.css"/>
```

**Usage:**
```html
<!-- Regular: inactive sensor -->
<i class="ph ph-thermometer" style="color:var(--muted); font-size:36px"></i>

<!-- Fill: active / alarm state -->
<i class="ph-fill ph-thermometer" style="color:var(--danger); font-size:36px"></i>
```

**State encoding with weight** — encode active/inactive without relying on color:
```html
<!-- Inactive panel: light weight, muted color -->
<i class="ph-light ph-wind" style="color:var(--muted); font-size:36px"></i>

<!-- Active panel: fill weight, accent color -->
<i class="ph-fill ph-wind" style="color:var(--accent); font-size:36px"></i>

<!-- Alarm state: fill weight, danger color + bold for E-Ink -->
<i class="ph-fill ph-warning" style="color:var(--danger); font-size:36px"></i>
```

**When to choose Phosphor over Tabler Icons:**
- When state changes need to be encoded through icon weight (not color alone)
- When the dashboard uses duotone accents for a more modern visual character
- When font-based icon inclusion is preferred over inline SVG

**When to choose Tabler Icons:**
- When inline SVG is required (no external CSS dependency)
- When `stroke="var(--accent)"` color adaptation via CSS variable is needed directly in SVG
- When a very large icon catalog is needed (Tabler has 5000+, Phosphor has 1000+)

Both libraries may be used together in the same dashboard — use Tabler for inline SVG pictograms and Phosphor for font-based UI icons (buttons, labels, status indicators).

---

### 9.3 Standard Measurement Type Mapping

The following table maps ADD `MeasuringValues` field name patterns to recommended Tabler Icons. AI agents use this table to automatically select icons when generating pictogram panels.

| Field name pattern | Tabler Icon name | Icon description |
|---|---|---|
| `WindDirection`, `Heading`, `Bearing`, `Direction` | `compass` | Compass rose |
| `WindSpeed`, `Speed`, `Velocity` | `wind` | Wind lines |
| `AirTemperature`, `Temperature`, `Temp` | `temperature` | Thermometer |
| `DeviceTemperature`, `ChipTemp` | `cpu` | Chip |
| `AirPressure`, `Pressure`, `BaroPressure` | `gauge` | Pressure gauge |
| `AirHumidity`, `Humidity`, `RelHumidity` | `droplet` | Water drop |
| `Dewpoint`, `DewPoint` | `droplets` | Water drops |
| `Altitude`, `Elevation`, `Height` | `mountain` | Mountain |
| `Latitude`, `Longitude`, `GPS` | `map-pin` | Location pin |
| `COG`, `CourseOverGround` | `navigation` | Arrow |
| `SOG`, `SpeedOverGround` | `speedboat` | Boat |
| `Depth`, `WaterDepth` | `ruler-measure` | Ruler |
| `WaterTemp`, `SeaTemperature` | `waves` | Waves |
| `Voltage`, `BatteryVoltage` | `bolt` | Lightning bolt |
| `Current`, `Ampere` | `current-ac` | AC current |
| `Power`, `Watt` | `plug` | Plug |
| `TankLevel`, `FuelLevel` | `tank` | Tank |
| `RotationSpeed`, `RPM` | `rotate` | Rotation |
| `Frequency`, `Hz` | `wave-sine` | Sine wave |
| `Luminance`, `Light`, `Lux` | `sun` | Sun |
| `CO2`, `CarbonDioxide` | `cloud` | Cloud |
| `PM25`, `PM10`, `Particles` | `air` | Air |
| `RSSI`, `FieldStrength`, `SignalStrength` | `wifi` | WiFi |
| `FreeHeap`, `Memory`, `RAM` | `memory` | Memory chip |
| `CPUSpeed`, `CPU` | `cpu` | CPU chip |
| `FirmwareVersion`, `Version` | `file-code` | Code file |
| `Uptime`, `Runtime` | `clock` | Clock |

### 9.4 Fallback Icon

When no matching icon is found in the table above, use the generic measurement icon:

| Fallback | Tabler Icon |
|---|---|
| Unknown numeric value | `variable` |
| Unknown status | `info-circle` |
| Unknown control action | `settings` |

### 9.5 Icon Color Rules

| Context | Color |
|---|---|
| Normal / active measurement | `var(--accent)` |
| Inactive / missing value | `var(--muted)` |
| Alarm / fault state | `var(--danger)` |
| Warning state | `var(--warn)` |
| OK / normal confirmed | `var(--ok)` |
| Interactive control | `var(--accent2)` |

---

## 10. Complete Dashboard Example

The Yachta Windsensor 2.1 dashboard is the reference implementation demonstrating all Style Guide components in a real-world deployment.

### 10.1 Layout Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│ HEADER                                                              │
│ [compass icon] Yachta Windsensor 2.1          [● Online]           │
│               ESP8266 · Masthead · hostname                         │
├────────────────────────────────────────────────────────────────────-┤
│ WIND (4-column grid)                                                │
│ ┌──────────────┐ ┌──────────────┐ ┌──────────┐ ┌──────────┐      │
│ │  Live        │ │  History     │ │Direction │ │Speed     │      │
│ │  Instrument  │ │  Plot        │ │269.2°  W │ │12.34 kn  │      │
│ │  (steels.)   │ │  (polar bar) │ │Resolution│ │Bft 4     │      │
│ │  Dir + Speed │ │  Freq. dist. │ │Wind Type │ │RotSpeed  │      │
│ └──────────────┘ └──────────────┘ └──────────┘ └──────────┘      │
├─────────────────────────────────────────────────────────────────────┤
│ ATMOSPHERE (4-column grid)                                          │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐              │
│ │Air Temp  │ │Pressure  │ │Humidity  │ │Dewpoint  │              │
│ │27.1°C    │ │1013.2mbar│ │79.2%     │ │14.1°C    │              │
│ │Device:   │ │Altitude: │ │          │ │          │              │
│ │29.4°C    │ │53 m      │ │          │ │          │              │
│ └──────────┘ └──────────┘ └──────────┘ └──────────┘              │
├─────────────────────────────────────────────────────────────────────┤
│ SYSTEM (3-column grid)                                              │
│ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐               │
│ │WLAN Client   │ │AP / Server   │ │Firmware      │               │
│ │SSID · IP     │ │SSID · IP     │ │Version · CPU │               │
│ │RSSI          │ │Free Heap     │ │Averaging     │               │
│ └──────────────┘ └──────────────┘ └──────────────┘               │
├─────────────────────────────────────────────────────────────────────┤
│ NMEA 0183                                                           │
│ ┌─────────────────────────────────────────────────────────────┐   │
│ │ 08:14:22 $WIMWV,269.2,R,12.34,N,A*xx                       │   │
│ │ 08:14:22 $WIVWR,90.8,R,12.34,N,6.35,M,22.85,K*xx          │   │
│ └─────────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────────┤
│ VALIDATION                                                          │
│ ADD v1.0: ✓struct ✓compreh ✓funct ✓rules ✓sec ✓disc ✓timing      │
│           [PASSED · claude-sonnet-4-6 · 2026-07-04]               │
├─────────────────────────────────────────────────────────────────────┤
│ FOOTER                                                              │
│ Autonomy L1 · CC BY 4.0    ★Style: [Dark ▾]  🕐Update:[5s ▾] ↻  │
└─────────────────────────────────────────────────────────────────────┘
```

### 10.2 ADD Document UI Reference

```json
{
  "add_version": "1.0",
  "device": { "name": "Yachta Windsensor 2.1" },
  "ui": {
    "style_guide_url": "https://norbert-walter.github.io/ai-device-description-add/style-guide/v1.0",
    "style_guide_version": "1.0",
    "dashboard_demo_url": "https://yachta.norbert-walter.dnshome.de/dashboard_demo.html",
    "theme_default": "dark"
  }
}
```

### 10.3 MeasuringValues → Component Mapping

| JSON field | Tabler Icon | Component | Size class |
|---|---|---|---|
| `WindDirection.Value` | `compass` | Dual-Pointer Instrument + Data Panel | `.v-big` |
| `WindSpeed.Value` | `wind` | Instrument LCD + Data Panel | `.v-big` |
| `Resolution.Value` | — | Sub-row in Direction panel | `.v-sm` |
| `RotationSpeed.Value` | `rotate` | Sub-row in Speed panel | `.v-sm` |
| `DownWindSpeed.Value` | `wind` | Sub-row in Speed panel | `.v-sm` |
| `AirTemperature.Value` | `temperature` | Data Panel | `.v-med` |
| `DeviceTemperature.Value` | `cpu` | Sub-row in Temperature panel | `.sub` |
| `AirPressure.Value` | `gauge` | Data Panel | `.v-med` |
| `Altitude.Value` | `mountain` | Sub-row in Pressure panel | `.sub` |
| `AirHumidity.Value` | `droplet` | Data Panel | `.v-med` |
| `Dewpoint.Value` | `droplets` | Data Panel | `.v-med` |
| `WLANClientSSID` | `wifi` | System Panel | `.v-sm` |
| `FieldStrength.Value` | `wifi` | System Panel | `.v-sm` |
| `ConnectionQuality.Value` | — | Instrument CQ label | inline |
| `FreeHeapSize.Value` | `memory` | System Panel | `.v-sm` |
| `CPUSpeed.Value` | `cpu` | System Panel | `.v-sm` |
| `FirmwareVersion` | `file-code` | System Panel | `.v-sm` |
| `NMEAValues.*` | — | Protocol Log Panel | log |

### 10.4 AI Agent Step-by-Step Instructions

1. **Fetch ADD document** from `ui.style_guide_url`
2. **Fetch this Style Guide** from the URL in the `ui` block
3. **Classify all MeasuringValues:**
   - Has directional + scalar pair → Dual-Pointer Instrument (Section 6.1)
   - Single scalar with known range → Radial Gauge (Section 6.2)
   - Single scalar, no range → Data Panel (Section 6.5)
   - Time-series useful → add Sparkline (Section 6.7)
4. **Classify all Actions:**
   - Binary on/off → Toggle Switch (Section 7.2)
   - Numeric range → Slider (Section 7.3)
   - Fixed options → Dropdown or Radio Group (Sections 7.4, 7.7)
   - Trigger action → Button (Section 7.1)
   - `requires_confirmation: true` → wrap in Confirmation Dialog (Section 7.8)
5. **Check for media endpoints** in ADD document:
   - Image URL → Image Panel (Section 8.1)
   - MJPEG stream → Video Panel (Section 8.2)
   - GPS coordinates → Map Panel (Section 8.3)
6. **Choose layout** using the automatic rule from Section 3.2
7. **Assign Tabler Icons** using the mapping table in Section 9.2
8. **Apply theme** from `ui.theme_default`, defaulting to `theme-dark`
9. **Embed required libraries:** tween-min.js and steelseries_micro.js from device host; Leaflet from CDN if GPS present
10. **Generate self-contained HTML** — all CSS inline, all JS inline, no external dependencies except fonts and libraries
11. **Add validation strip** from ADD document `validated_by` field
12. **Add footer** with copyright, ADD Spec link, theme selector, rate selector

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

## Appendix B — SteelSeries Configuration Reference

SteelSeries Canvas: https://github.com/HanSolo/SteelSeries-Canvas

### Available Gauge Types

| Class | Description | ADD use case |
|---|---|---|
| `steelseries.Radial` | Full/partial circle, 1 pointer | Single scalar with range (pressure, temp) |
| `steelseries.RadialBargraph` | Radial bar instead of pointer | Level, percentage |
| `steelseries.RadialVertical` | Vertical semicircle | Compact single value |
| `steelseries.Linear` | Horizontal or vertical bar | Fill level, signal strength |
| `steelseries.LinearBargraph` | Linear bargraph | Same as Linear |
| `steelseries.WindDirection2` | Full circle, 2 pointers, LCD | Direction + speed (primary ADD instrument) |
| `steelseries.Compass` | Compass rose with heading | GPS heading, magnetic bearing |
| `steelseries.Level` | Clinometer / spirit level | Heel angle, trim |
| `steelseries.Battery` | Battery icon gauge | Battery / charge level |
| `steelseries.StopWatch` | Stopwatch display | Uptime, elapsed time |
| `steelseries.Clock` | Analog clock | Current time, UTC |
| `steelseries.DisplaySingle` | LCD display only, no gauge | Raw numeric value with unit |
| `steelseries.DisplayMulti` | Multi-line LCD | Multiple values on one display |

### Key Shared Parameters

| Parameter | Type | Description |
|---|---|---|
| `size` | int | Canvas size in pixels. WindDirection2: 340. Radial: 200–400. |
| `frameDesign` | FrameDesign | `BLACK_METAL`, `STEEL`, `CHROME`, `BRASS`, `SHINY_METAL` |
| `backgroundColor` | BackgroundColor | `ANTHRACITE`, `DARK_GRAY`, `SATIN_GRAY`, `LIGHT_GRAY`, `WHITE`, `BLACK` |
| `lcdColor` | LcdColor | `STANDARD`, `STANDARD_GREEN`, `BLUE`, `RED`, `ORANGE`, `BLACK` |
| `pointerColor` | ColorDef | `RED`, `GREEN`, `BLUE`, `ORANGE`, `WHITE`, `BLACK` |
| `pointerType` | PointerType | `TYPE1`–`TYPE16` — shape of needle |
| `section` | Section[] | Colored arc segments — `steelseries.Section(from, to, color)` |
| `threshold` | float | Value at which threshold indicator appears |
| `minValue` | float | Scale minimum |
| `maxValue` | float | Scale maximum |
| `unitString` | string | Unit label on gauge |
| `titleString` | string | Title label on gauge |
| `lcdVisible` | bool | Show LCD panel |
| `foregroundVisible` | bool | Show glass-reflection foreground layer |

### WindDirection2-Specific Parameters

| Parameter | Type | Description |
|---|---|---|
| `degreeScaleHalf` | bool | `true` = −180…+180°, `false` = 0…360° |
| `degreeScale` | bool | Show numeric degree scale |
| `pointSymbolsVisible` | bool | Show N/E/S/W cardinal labels |
| `lcdTitleStrings` | string[] | Two LCD labels: `['Top label', 'Bottom label']` |
| `pointerTypeLatest` | PointerType | Shape of the latest-value pointer |
| `pointerTypeAverage` | PointerType | Shape of the average-value pointer |
| `pointerColorAverage` | ColorDef | Color of the average-value pointer |

### Recommended Combinations for ADD Dashboards

| Use case | frameDesign | backgroundColor | lcdColor | pointerColor |
|---|---|---|---|---|
| Dark theme, primary | `BLACK_METAL` | `ANTHRACITE` | `STANDARD` | `RED` |
| Dark theme, secondary | `BLACK_METAL` | `DARK_GRAY` | `STANDARD_GREEN` | `GREEN` |
| Light theme | `STEEL` | `SATIN_GRAY` | `STANDARD` | `RED` |
| Night theme | `BLACK_METAL` | `BLACK` | `ORANGE` | `ORANGE` |
| B&W theme | `STEEL` | `LIGHT_GRAY` | `STANDARD` | `BLACK` |

---

## Appendix D — Color Palette Guide

### D.1 Why Color Harmony Matters

A dashboard that uses arbitrarily chosen colors feels visually noisy and unprofessional — even when individual colors are attractive in isolation. Color harmony is the principle that colors in a set relate to each other in a predictable, mathematically defined way on the color wheel. Harmonious palettes are easier on the eye over long periods of use, reduce cognitive load, and communicate structure more effectively.

For ADD dashboards this matters especially in two contexts:

- **Night watch** — a poorly chosen palette with high blue content impairs dark adaptation
- **E-Ink / B&W** — a palette that relies on hue alone collapses to an unreadable gray soup when rendered without color

A well-chosen 5-color base palette — background, panel, text, primary accent, secondary accent — is sufficient to derive all CSS variables needed for a complete ADD theme.

### D.2 Color Harmony Systems

The following harmony types are relevant for dashboard design. All can be explored interactively in Adobe Color (see Section D.4).

**Monochromatic**
One hue, multiple lightness/saturation steps. The most readable and least fatiguing for long-duration use. The B&W theme is a pure monochromatic palette (gray axis). Suitable for: night watch, industrial control rooms, medical displays.

```
Example: Base hue 210° (blue)
  #0a1628  background
  #112240  panel
  #1a3a6e  border
  #4a9eff  accent
  #caf0f8  text
```

**Analogous**
Three adjacent hues (within 30–60° of each other). Harmonious and calm. The Night theme uses an analogous warm palette (dark brown → amber → orange). Suitable for: any theme where a consistent "mood" is important.

```
Example: Warm analog (20°–60°)
  #1a0800  background
  #2d1200  panel
  #cc5500  accent
  #ff9900  text-strong
```

**Complementary**
Two hues 180° apart. High contrast, visually dynamic. The Dark theme uses a complementary pair: teal (180°) and blue (240°) as dual accents on a near-neutral dark background. Best used with one dominant color and the complement as a small accent only — equal use of two complementary colors is visually jarring.

```
Example: Teal + coral
  #00c8a0  primary accent (teal, 168°)
  #ff5c5c  danger / complementary accent (0°)
```

**Split-Complementary**
One base hue plus two hues adjacent to its complement. More versatile than pure complementary — the two accent colors are distinct enough to encode different states (ok vs. warning) without clashing. Recommended for dashboards that need three semantic colors (ok/warn/danger).

```
Example: Blue base + yellow-orange + red-orange accents
  #0084ff  accent (primary, 210°)
  #f5a623  warn (split, 35°)
  #e84040  danger (split, 0°)
```

**Triadic**
Three hues equally spaced (120° apart). Vibrant and varied — use carefully. More suitable for light themes and consumer applications than for industrial dashboards where color encodes safety-critical states.

### D.3 Semantic Color Assignments

Regardless of the chosen harmony type, the following semantic assignments are mandatory in all ADD themes. These mappings must remain consistent across themes so that operators who switch between Dark and Light modes always interpret colors the same way.

| Variable | Semantic meaning | Constraint |
|---|---|---|
| `--accent` | Active / ok / primary interactive | Must contrast ≥ 4.5:1 against `--panel` (WCAG AA) |
| `--accent2` | Secondary interactive / numeric values | Must be visually distinct from `--accent` |
| `--danger` | Fault / alarm / destructive action | Must be in the red spectrum (0°–15°) |
| `--warn` | Caution / threshold exceeded | Must be in the yellow/orange spectrum (30°–50°) |
| `--ok` | Confirmed normal state | May equal `--accent` or be a lighter variant |
| `--muted` | Labels, inactive text | Must contrast ≥ 3:1 against `--panel` (WCAG AA Large) |

**The danger and warn colors are not part of the harmony palette** — they are fixed semantic colors that override harmony considerations. A night-watch amber theme still uses red for danger and a distinct yellow-orange for warnings, even though these colors are outside the monochromatic amber palette. Safety communication always overrides aesthetic harmony.

### D.4 Adobe Color — Interactive Palette Tool

**Adobe Color** is the recommended tool for developing and validating custom ADD theme palettes.

URL: https://color.adobe.com/create/color-wheel

Adobe Color is free to use without registration for palette creation. An Adobe account is required only to save palettes to the cloud.

**Recommended workflow for a custom ADD theme:**

**Step 1 — Define the base hue**
Choose a hue that reflects the device's application domain or the manufacturer's brand identity. Enter it in the central color field of the Adobe Color wheel.

**Step 2 — Choose a harmony type**
Select the harmony type from the top menu (Monochromatic, Analogous, Complementary, Split Complementary, Triadic). For industrial and maritime dashboards, Monochromatic or Analogous are recommended as starting points.

**Step 3 — Extract five base colors**
From the generated palette, identify:
- Background color (darkest for dark themes, lightest for light themes)
- Panel color (slightly lighter/darker than background)
- Primary accent (the most saturated color in the palette)
- Secondary accent (a second saturated color or a lighter variant of the primary)
- Text color (highest contrast against panel)

**Step 4 — Check contrast ratios**
Switch to the **Contrast Checker** tab in Adobe Color (or navigate to https://color.adobe.com/create/color-contrast-analyzer). Enter each foreground/background pair:

| Pair to check | Minimum ratio | WCAG level |
|---|---|---|
| Primary value text on panel | 7:1 | AAA |
| Label text on panel | 4.5:1 | AA |
| Accent color on panel | 4.5:1 | AA |
| Muted text on panel | 3:1 | AA Large |

Adjust lightness values until all ratios are met. Do not sacrifice contrast for aesthetic reasons — legibility is a safety requirement for operational dashboards.

**Step 5 — Simulate color blindness**
Switch to the **Color Blindness** tab in Adobe Color. Test the palette under:
- **Deuteranopia** (red-green, most common — ~6% of males)
- **Protanopia** (red-green variant)
- **Tritanopia** (blue-yellow, rare)

Verify that `--accent`, `--accent2`, `--danger`, and `--warn` remain distinguishable under all simulations. If any two semantic colors merge, adjust their hues or add a secondary encoding (icon, label, shape) to compensate.

**Step 6 — Derive all CSS variables**
Map the five base colors to the full CSS variable set defined in Appendix A. Derive border and muted colors as lightness variants of the panel and text colors:

```css
/* Example: derived from a deep blue base palette */
body.theme-ocean {
  --bg-color:    #0a1628;           /* darkest — background */
  --panel:       #112240;           /* slightly lighter */
  --border:      #1a3356;           /* panel + 10% lighter */
  --text:        #8aabcc;           /* mid-tone text */
  --text-strong: #caf0f8;           /* lightest — primary values */
  --muted:       #3a5878;           /* panel + text midpoint */
  --label:       #5a80a0;           /* between muted and text */
  --accent:      #00b4d8;           /* primary accent */
  --accent2:     #0077b6;           /* secondary accent */
  --danger:      #e84040;           /* fixed semantic — do not harmonize */
  --warn:        #f5a623;           /* fixed semantic — do not harmonize */
  --ok:          #00b4d8;           /* equals accent for simple themes */
  --nmea-bg:     #060e1a;           /* darker than background */
  --nmea-ts:     #2a4060;           /* muted, near background */
  --instr-bg:    #0d1e36;           /* between bg and panel */
  --instr-brd:   #1a3356;           /* equals border */
  --v-big-color: #caf0f8;           /* equals text-strong */
  --v-med-color: #caf0f8;           /* equals text-strong */
  --input-bg:    #080f1e;           /* darker than background */
  --input-brd:   #1a3356;           /* equals border */
  --pulse-anim:  pulse 2s infinite;
}
```

**Step 7 — Validate in B&W**
Temporarily set all color values to grayscale by converting each hex to its luminance equivalent. Verify that the dashboard remains readable — all values visible, all labels present, hierarchy maintained. This confirms the palette encodes information through lightness contrast, not hue alone.

### D.5 Palette Naming Convention

Custom themes should be named descriptively, combining a visual or conceptual quality with a domain reference:

```
theme-ocean       → deep blue, maritime
theme-forest      → dark green, environmental monitoring
theme-ember       → warm orange, industrial/furnace
theme-arctic      → cool gray-blue, laboratory/climate
theme-dusk        → purple-gray, general purpose
theme-industrial  → steel gray, factory floor
```

The theme name becomes the CSS body class (`body.theme-ocean`) and the value in the theme selector dropdown.

### D.6 Summary — Custom Theme Checklist

Before declaring a custom theme complete, verify all items:

- [ ] Base palette derived from a defined color harmony type
- [ ] All 5 base colors identified: background, panel, accent, accent2, text
- [ ] `--danger` is red (0°–15°), not harmonized
- [ ] `--warn` is yellow/orange (30°–50°), not harmonized
- [ ] Primary value text contrast ≥ 7:1 on panel (WCAG AAA)
- [ ] Label text contrast ≥ 4.5:1 on panel (WCAG AA)
- [ ] Accent contrast ≥ 4.5:1 on panel (WCAG AA)
- [ ] Palette tested under Deuteranopia and Protanopia simulation
- [ ] All semantic colors distinguishable under color blindness simulation
- [ ] Dashboard readable in grayscale (luminance-only)
- [ ] All 23 CSS variables from Appendix A defined
- [ ] B&W theme variant verified (4 grayscales only, no animation)
- [ ] Theme name follows naming convention
- [ ] Theme added to dropdown selector in footer

---
## Appendix C — Changelog

### v1.0 — 2026-07-07

- Initial release
- Display components: Dual-Pointer Instrument, Single-Pointer Instrument, Linear Gauge, Circular History Plot, Data Panel, Pictogram Panel, Sparkline, Status Pill, Protocol Log, Validation Strip
- Input controls: Button, Toggle Switch, Slider, Dropdown Selector, Text Input, Checkbox Group, Radio Group, Confirmation Dialog
- Media components: Image Panel, Video Stream Panel (MJPEG), Map Panel (Leaflet / OpenStreetMap)
- Themes: Dark, Light, Night, B&W (E-Ink, 4 grayscales)
- Grid System: 1U = 160px, 1–4 column layouts, 3 responsive breakpoints
- Typography: Ubuntu / Ubuntu Mono via bunny.net (GDPR-compliant)
- Pictogram Library: Tabler Icons (MIT), 30-entry ADD field mapping table
- Reference implementation: Yachta Windsensor 2.1 (Open Boat Projects)
- SteelSeries Canvas configuration reference (all gauge types)
- Color Palette Guide (Appendix D): Adobe Color workflow, harmony systems, semantic color rules, custom theme checklist
- Pictogram Library: added Phosphor Icons (MIT) as second library alongside Tabler Icons; weight-based state encoding guide
- License: CC BY 4.0

---

*ADD Dashboard Style Guide v1.0*
*Part of the AI Device Description (ADD) open standard*
*© 2026 Norbert Walter / Open Boat Projects — CC BY 4.0*
*https://github.com/norbert-walter/ai-device-description-add*

---
