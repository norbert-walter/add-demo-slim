/**
 * app.js – pure logic of the ADD Demo web UI (Contract V1.5.22 §5.5, Sprint W1).
 *
 * No DOM, no fetch, no timers: every function takes plain data and returns plain data, so that
 * test/js/test_app.js can test it with node. The pages (index.html, settings.html) load it with
 * <script src="/app.js"></script> and use the global ADDAPP; node uses require().
 *
 * SKELETON by the reviewer: constants and the CRC functions are given. Fill in the functions marked
 * TODO(W1.x) exactly as their comment says; do not add or rename functions, do not change the given part.
 * ES5 syntax plus const/let, arrow functions and template strings (every current browser).
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.ADDAPP = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  // ---- constants (given) ---------------------------------------------------------------------
  const ERR_NAMES = { 0: 'OK', '-1': 'BAD_CRC', '-2': 'BAD_JSON', '-3': 'UNKNOWN_CMD', '-4': 'BAD_PARAM',
    '-5': 'STATE', '-6': 'PRECONDITION', '-7': 'BUSY', '-8': 'NOT_STABLE', '-9': 'CALIB_INVALID',
    '-10': 'LOCAL_CONTROL' };                                       // CONTRACT §3.3
  const SYS_ERR_NAMES = ['NONE', 'ESTOP', 'TEMP_HIGH', 'COMM_TIMEOUT', 'MOTOR_FAULT', 'HOMING_FAILED'];  // §3.2
  const TEMP_FAULT_C = 32.0;                                         // §3.2 TEMP_HIGH
  const HEARTBEAT_MS = 2000;
  const SERIES_MAX = 100;
  /** Settings keys in display order (CONTRACT §9). type: 'num' | 'bool' | 'str'. */
  const PARAMS = [
    { key: 'measure_pos',    type: 'num', min: 61,  max: 242,  unit: 'mm' },
    { key: 'maint_pos',      type: 'num', min: 21,  max: 60,   unit: 'mm' },
    { key: 'traverse_start', type: 'num', min: 61,  max: 242,  unit: 'mm' },
    { key: 'traverse_end',   type: 'num', min: 61,  max: 242,  unit: 'mm' },
    { key: 'traverse_speed', type: 'num', min: 1,   max: 20,   unit: 'mm/s' },
    { key: 'shutter_hold',   type: 'num', min: 0,   max: 10,   unit: 's' },
    { key: 'edge_threshold', type: 'num', min: 0.5, max: 100,  unit: 'mm' },
    { key: 'base_dist',      type: 'num', min: 40,  max: 300,  unit: 'mm' },
    { key: 'calib_near_ref', type: 'num', min: 40,  max: 300,  unit: 'mm' },
    { key: 'calib_far_ref',  type: 'num', min: 40,  max: 300,  unit: 'mm' },
    { key: 'temp',           type: 'num', min: -20, max: 99.9, unit: '°C' },
    { key: 'interlock',      type: 'bool' },
    { key: 'cooling_flow',   type: 'bool' },
    { key: 'estop_nc',       type: 'bool' },
    { key: 'wifi_ssid',      type: 'str', min: 0, max: 32 },
    { key: 'wifi_pass',      type: 'str', min: 8, max: 63 },          // empty = keep the stored password
  ];

  // ---- CRC frame (given: CONTRACT §4, verbatim) -------------------------------------------------
  function crc32(str) {
    let c, crc = 0xFFFFFFFF; const b = new TextEncoder().encode(str);
    for (const x of b) { c = (crc ^ x) & 0xFF; for (let k = 0; k < 8; k++) c = c & 1 ? (c >>> 1) ^ 0xEDB88320 : c >>> 1; crc = (crc >>> 8) ^ c; }
    return ((crc ^ 0xFFFFFFFF) >>> 0).toString(16).toUpperCase().padStart(8, '0');
  }
  function frame(obj) { const body = JSON.stringify(obj); return body.slice(0, -1) + ',"crc":"' + crc32(body) + '"}'; }
  function verify(t) {
    t = t.trimEnd(); const i = t.lastIndexOf(',"crc":"');
    return i >= 0 && t.length === i + 18 && t.endsWith('"}') && crc32(t.slice(0, i) + '}') === t.substr(i + 8, 8).toUpperCase();
  }

  // ---- W1.1: protocol and formatting -------------------------------------------------------------
  /** Framed text -> {ok:true, data: JSON.parse(text)} (the crc member stays in data) |
   *  {ok:false, error:'crc'} (not a string or verify fails) |
   *  {ok:false, error:'json'} (CRC fine, but JSON.parse throws or the result is not a plain object). */
  function parseFramed(text) {
    if (typeof text !== 'string') return { ok: false, error: 'crc' };
    const trimmed = text.trimEnd();
    if (!verify(trimmed)) return { ok: false, error: 'crc' };
    let data;
    try { data = JSON.parse(trimmed); } catch (_) { return { ok: false, error: 'json' }; }
    if (data === null || typeof data !== 'object' || Array.isArray(data)) return { ok: false, error: 'json' };
    return { ok: true, data };
  }
  /** Request body for POST /api: frame({cmd, params}); params defaults to {} (also for null). */
  function apiBody(cmd, params) {
    return frame({ cmd, params: params ?? {} });
  }
  /** API result name (§3.3); unknown code c -> 'ERR c'. */
  function errName(code) {
    const name = ERR_NAMES[String(code)];
    return name ?? `ERR ${code}`;
  }
  /** System error name (§3.2); unknown code c -> 'ERR c'. */
  function sysErrName(code) {
    const name = SYS_ERR_NAMES[code];
    return name ?? `ERR ${code}`;
  }
  /** Number with `digits` decimals (default 1); not a finite number (incl. null/undefined) -> '---'. */
  function fmt(x, digits) {
    if (x === null || x === undefined || !Number.isFinite(x)) return '---';
    const d = digits !== undefined ? digits : 1;
    return x.toFixed(d);
  }
  /** Toast text of an /api reply object: err 0 -> '<cmd>: OK'; else '<cmd>: <errName>' plus
   *  ' - <msg>' if msg is a non-empty string. No reply (null) -> 'no reply'. */
  function replyText(resp) {
    if (resp === null) return 'no reply';
    const cmd = resp.cmd;
    if (resp.err === 0) return `${cmd}: OK`;
    let s = `${cmd}: ${errName(resp.err)}`;
    if (typeof resp.msg === 'string' && resp.msg !== '') s += ` - ${resp.msg}`;
    return s;
  }

  // ---- W1.2: process view rules ------------------------------------------------------------------
  /** Colour class of a safety state: READY/RUNNING 'ok', FAULT/WAIT_ACK 'danger', every other 'warn'. */
  function stateClass(state) {
    throw new Error('TODO(W1.2) stateClass');
  }
  /** Display strings of a /proc object p:
   *  { state, stateCls: stateClass(state), pos, vel (fmt), dist: fmt if measure.valid else '---',
   *    thick: fmt if measure.valid && measure.on_material else '---', avg: fmt if measure.n > 0 else '---',
   *    n: String(measure.n), mode: motion.mode, err: '' if err === 0 else sysErrName(err),
   *    hv: 'ON'|'OFF', shutter: 'OPEN'|'CLOSED' (safety.shutter === 'open'), local: !!p.local } */
  function view(p) {
    throw new Error('TODO(W1.2) view');
  }
  /** Safety flag chips, always these four in this order:
   *  {id:'estop', text:'E-STOP ACTIVE'|'E-STOP OK', cls: estop ? 'danger' : 'ok'},
   *  {id:'interlock', text:'INTERLOCK (SIM)', cls: interlock ? 'ok' : 'danger'},
   *  {id:'cooling', text:'COOLING (SIM)', cls: cooling_flow ? 'ok' : 'danger'},
   *  {id:'temp', text:'TEMP (SIM) <fmt(temp)> °C', cls: temp_ok ? 'ok' : (temp > TEMP_FAULT_C ? 'danger' : 'warn')} */
  function flags(p) {
    throw new Error('TODO(W1.2) flags');
  }
  /** Button rules of the process view. p = last valid /proc object or null (no data yet / offline).
   *  Returns { INIT, HV, MEASURE, TRAVERSE, STOP, ACK } each {enabled:bool}, HV additionally
   *  {label, cmd}, plus hint (string).
   *  idle = motion.state === 'idle'; local = p.local; st = p.state.
   *  INIT:     !local && (st === 'SAFE_START' || st === 'READY') && idle
   *  HV:       hv_on -> {label:'HV OFF', cmd:'HV_OFF', enabled:true}
   *            else  -> {label:'HV ON',  cmd:'HV_ON',  enabled: !local && (st === 'READY' || st === 'RUNNING') && hv_ok}
   *  MEASURE, TRAVERSE: !local && st === 'RUNNING' && motion.homed && idle
   *  STOP:     always enabled (also with p === null)
   *  ACK:      !local && st === 'WAIT_ACK'
   *  hint:     local ? 'Operator at display has priority' : ''
   *  p === null: all disabled except STOP, HV {label:'HV ON', cmd:'HV_ON'}, hint ''. */
  function buttons(p) {
    throw new Error('TODO(W1.2) buttons');
  }
  /** Commands that need the confirmation dialog (Style Guide 7.8): HV_ON, ACK, REBOOT, CALIB_RESET.
   *  Returns {title, body} or null. Texts:
   *  HV_ON:  'Switch HV on?', 'The X-ray source (HV LED) is switched on.'
   *  ACK:    'Acknowledge fault?', 'The cause is gone. The system returns to READY or SAFE_START.'
   *  REBOOT: 'Reboot device?', 'The device restarts. The connection is lost for some seconds.'
   *  CALIB_RESET: 'Reset calibration?', 'The calibration returns to the factory values.' */
  function confirmText(cmd) {
    throw new Error('TODO(W1.2) confirmText');
  }
  /** true if a GET_STATE heartbeat must be sent now: p not null, p.safety.comm_armed and
   *  (now - lastSent) >= HEARTBEAT_MS. */
  function heartbeatDue(p, now, lastSent) {
    throw new Error('TODO(W1.2) heartbeatDue');
  }
  /** Time series for the chart: if p.measure.valid push {dist, thick} (numbers from p.measure);
   *  then drop from the front until length <= max (default SERIES_MAX). Mutates and returns series. */
  function seriesPush(series, p, max) {
    throw new Error('TODO(W1.2) seriesPush');
  }
  /** Status pill: conn 'ok' -> {text:'Online', offline:false}; 'crc' -> {text:'CRC error', offline:true};
   *  every other value -> {text:'Offline', offline:true}. */
  function pill(conn) {
    throw new Error('TODO(W1.2) pill');
  }
  /** Validation strip (Style Guide 6.10) from the /add document (object or null).
   *  No add.validation.validated_by array or an empty one -> [{text:'not validated', cls:'warn'}].
   *  Else take the entry with the largest validated_at (ISO strings compare as text; the first wins
   *  on equality) and return, for each member of entry.score in object order:
   *    'pass' -> {text:'✓ <key>', cls:''}, 'warning' -> {text:'! <key>', cls:'warn'},
   *    every other value -> {text:'✗ <key>', cls:'fail'},
   *  followed by {text: '<STATUS> · <version> · <validated_at first 10 chars>', cls:'info'} with
   *  STATUS = status in upper case, '_' replaced by ' ' (e.g. 'PASSED WITH WARNINGS'). */
  function validationBadges(add) {
    throw new Error('TODO(W1.2) validationBadges');
  }

  // ---- W1.3: settings rules ----------------------------------------------------------------------
  /** Check one form value against PARAMS. raw: string from an input (for 'bool' also true/false).
   *  'num':  trimmed string, Number(raw) must be finite and min <= x <= max -> {ok:true, value:x}
   *          (empty string is invalid); else {ok:false, error:'<min> ... <max>'} (numbers as String()).
   *  'bool': true/'true'/'1' -> true, false/'false'/'0' -> false, else {ok:false, error:'true or false'}
   *  'str':  wifi_ssid: length 0..32; wifi_pass: length 8..63 (the empty string is handled by
   *          configDiff, not here); else {ok:false, error:'<min> ... <max> characters'}
   *  unknown key -> {ok:false, error:'unknown key'} */
  function checkParam(key, raw) {
    throw new Error('TODO(W1.3) checkParam');
  }
  /** SET commands for the changes of a settings form.
   *  cfg: the "config" object of GET_CONFIG; form: {key: raw value} for some or all PARAMS keys.
   *  For every PARAMS key (in PARAMS order) that is present in form:
   *    wifi_pass: skipped if form.wifi_pass === '' (keep stored password), else checked and always set;
   *    other keys: checkParam; invalid -> errors.push({key, error}); valid and different from cfg[key]
   *    -> sets.push({key, value}). Numbers count as different if |a - b| > 0.0001.
   *  Returns {sets, errors}. */
  function configDiff(cfg, form) {
    throw new Error('TODO(W1.3) configDiff');
  }

  return { ERR_NAMES, SYS_ERR_NAMES, TEMP_FAULT_C, HEARTBEAT_MS, SERIES_MAX, PARAMS,
    crc32, frame, verify, parseFramed, apiBody, errName, sysErrName, fmt, replyText,
    stateClass, view, flags, buttons, confirmText, heartbeatDue, seriesPush, pill, validationBadges,
    checkParam, configDiff };
});
