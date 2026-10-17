// FROZEN TESTS (Contract V1.5.22) – Sprint W1 must make these pass. Do not modify.
// Tests the pure logic of the web UI, data/www/app.js, with node's built-in test runner:
//   node --test test/js/
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const A = require('../../data/www/app.js');

/** A /proc object as the device sends it (CONTRACT §5.1); overrides are merged per section. */
function proc(o) {
  o = o || {};
  const p = {
    ts: 123456, state: 'READY', err: 0, local: false,
    motion: { pos: 120.0, vel: 10.0, state: 'idle', mode: 'none', homed: true, limit_fwd: false, limit_bwd: false },
    measure: { dist: 187.7, thick: 12.3, on_material: true, valid: true, in_range: true, avg: 12.3, n: 143 },
    safety: { estop: false, hv_on: false, hv_ok: true, shutter: 'closed', interlock: true, cooling_flow: true,
              temp: 23.0, temp_ok: true, comm_armed: false },
  };
  for (const k of Object.keys(o)) {
    if (o[k] !== null && typeof o[k] === 'object' && typeof p[k] === 'object') Object.assign(p[k], o[k]);
    else p[k] = o[k];
  }
  return p;
}

// ---------------------------------------------------------------- W1.1 protocol and formatting
test('w11 crc32 and frame: frozen test vectors of CONTRACT §4', () => {
  assert.equal(A.crc32('123456789'), 'CBF43926');
  assert.equal(A.crc32(''), '00000000');
  assert.equal(A.crc32('{"cmd":"HV_ON","params":{}}'), '88F7C802');
  assert.equal(A.crc32('{"cmd":"SET","params":{"key":"temp","value":35.0}}'), 'C93F2D33');
  assert.equal(A.frame({ cmd: 'SET', params: { key: 'temp', value: 35 } }),
    '{"cmd":"SET","params":{"key":"temp","value":35},"crc":"F2B0BAC5"}');
});
test('w11 verify accepts good frames, trailing whitespace and lower-case hex', () => {
  assert.equal(A.verify('{"cmd":"HV_ON","params":{},"crc":"88F7C802"}'), true);
  assert.equal(A.verify('{"cmd":"HV_ON","params":{},"crc":"88f7c802"}\n'), true);
  assert.equal(A.verify('{"cmd":"HV_ON","params":{},"crc":"88F7C803"}'), false);
  assert.equal(A.verify('{"cmd":"HV_ON","params":{}}'), false);
  assert.equal(A.verify('{"cmd":"HV_ON","params":{},"crc":"88F7C802XX"}'), false);
});
test('w11 parseFramed: ok, crc error, json error', () => {
  const good = A.frame({ status: 'ok', err: 0, cmd: 'STOP' });
  assert.deepEqual(A.parseFramed(good), { ok: true,
    data: { status: 'ok', err: 0, cmd: 'STOP', crc: A.crc32('{"status":"ok","err":0,"cmd":"STOP"}') } });   // crc member kept
  assert.deepEqual(A.parseFramed(good.replace('STOP', 'STOQ')), { ok: false, error: 'crc' });
  assert.deepEqual(A.parseFramed(''), { ok: false, error: 'crc' });
  assert.deepEqual(A.parseFramed(null), { ok: false, error: 'crc' });
  // CRC fine, but not a JSON object
  const notJson = '{x,"crc":"' + A.crc32('{x}') + '"}';
  assert.deepEqual(A.parseFramed(notJson), { ok: false, error: 'json' });
});
test('w11 parseFramed reads a real /proc line', () => {
  const p = proc();
  const r = A.parseFramed(A.frame(p) + '\n');
  assert.equal(r.ok, true);
  assert.match(r.data.crc, /^[0-9A-F]{8}$/);
  delete r.data.crc;
  assert.deepEqual(r.data, p);
});
test('w11 apiBody: params default to {} and are framed', () => {
  assert.equal(A.apiBody('HV_ON'), '{"cmd":"HV_ON","params":{},"crc":"88F7C802"}');
  assert.equal(A.apiBody('HV_ON', null), '{"cmd":"HV_ON","params":{},"crc":"88F7C802"}');
  const b = A.apiBody('CALIB', { action: 'near' });
  assert.ok(b.startsWith('{"cmd":"CALIB","params":{"action":"near"},"crc":"'));
  assert.equal(A.verify(b), true);
});
test('w11 errName and sysErrName', () => {
  const names = { 0: 'OK', '-1': 'BAD_CRC', '-2': 'BAD_JSON', '-3': 'UNKNOWN_CMD', '-4': 'BAD_PARAM', '-5': 'STATE',
    '-6': 'PRECONDITION', '-7': 'BUSY', '-8': 'NOT_STABLE', '-9': 'CALIB_INVALID', '-10': 'LOCAL_CONTROL' };
  for (const k of Object.keys(names)) assert.equal(A.errName(Number(k)), names[k], k);
  assert.equal(A.errName(-11), 'ERR -11');
  assert.equal(A.errName(3), 'ERR 3');
  const sys = ['NONE', 'ESTOP', 'TEMP_HIGH', 'COMM_TIMEOUT', 'MOTOR_FAULT', 'HOMING_FAILED'];
  sys.forEach((n, i) => assert.equal(A.sysErrName(i), n));
  assert.equal(A.sysErrName(6), 'ERR 6');
  assert.equal(A.sysErrName(-1), 'ERR -1');
});
test('w11 fmt', () => {
  assert.equal(A.fmt(155), '155.0');
  assert.equal(A.fmt(12.345, 2), '12.35');
  assert.equal(A.fmt(-0.04), '-0.0');
  assert.equal(A.fmt(7, 0), '7');
  assert.equal(A.fmt(NaN), '---');
  assert.equal(A.fmt(Infinity), '---');
  assert.equal(A.fmt(null), '---');
  assert.equal(A.fmt(undefined), '---');
  assert.equal(A.fmt('12'), '---');
});
test('w11 replyText', () => {
  assert.equal(A.replyText({ status: 'ok', err: 0, cmd: 'STOP' }), 'STOP: OK');
  assert.equal(A.replyText({ status: 'error', err: -6, cmd: 'HV_ON', msg: 'precondition not met' }),
    'HV_ON: PRECONDITION - precondition not met');
  assert.equal(A.replyText({ status: 'error', err: -7, cmd: 'INIT' }), 'INIT: BUSY');
  assert.equal(A.replyText({ status: 'error', err: -10, cmd: 'INIT', msg: '' }), 'INIT: LOCAL_CONTROL');
  assert.equal(A.replyText(null), 'no reply');
  assert.equal(A.replyText({ status: 'error', cmd: 'STOP' }), 'STOP: ERR undefined');   // err missing is not OK
});

// ---------------------------------------------------------------- W1.2 process view rules
test('w12 stateClass', () => {
  assert.equal(A.stateClass('READY'), 'ok');
  assert.equal(A.stateClass('RUNNING'), 'ok');
  assert.equal(A.stateClass('FAULT'), 'danger');
  assert.equal(A.stateClass('WAIT_ACK'), 'danger');
  assert.equal(A.stateClass('SAFE_START'), 'warn');
  assert.equal(A.stateClass('RECOVER'), 'warn');
  assert.equal(A.stateClass('anything'), 'warn');
});
test('w12 view: normal values', () => {
  const v = A.view(proc({ state: 'RUNNING', motion: { pos: 155, vel: 9.96, mode: 'traverse' },
    safety: { hv_on: true, shutter: 'open' }, local: true }));
  assert.deepEqual(v, { state: 'RUNNING', stateCls: 'ok', pos: '155.0', vel: '10.0', dist: '187.7', thick: '12.3',
    avg: '12.3', n: '143', mode: 'traverse', err: '', hv: 'ON', shutter: 'OPEN', local: true });
});
test('w12 view: no valid measurement, off material, no samples, fault', () => {
  let v = A.view(proc({ measure: { valid: false } }));
  assert.equal(v.dist, '---'); assert.equal(v.thick, '---');
  v = A.view(proc({ measure: { on_material: false } }));
  assert.equal(v.dist, '187.7'); assert.equal(v.thick, '---');
  v = A.view(proc({ measure: { n: 0, avg: 0 } }));
  assert.equal(v.avg, '---'); assert.equal(v.n, '0');
  v = A.view(proc({ state: 'WAIT_ACK', err: 4 }));
  assert.equal(v.err, 'MOTOR_FAULT'); assert.equal(v.stateCls, 'danger');
  assert.equal(v.hv, 'OFF'); assert.equal(v.shutter, 'CLOSED'); assert.equal(v.local, false);
});
test('w12 flags: order, texts and classes', () => {
  assert.deepEqual(A.flags(proc()), [
    { id: 'estop', text: 'E-STOP OK', cls: 'ok' },
    { id: 'interlock', text: 'INTERLOCK (SIM)', cls: 'ok' },
    { id: 'cooling', text: 'COOLING (SIM)', cls: 'ok' },
    { id: 'temp', text: 'TEMP (SIM) 23.0 °C', cls: 'ok' },
  ]);
  const f = A.flags(proc({ safety: { estop: true, interlock: false, cooling_flow: false, temp: 30.5, temp_ok: false } }));
  assert.deepEqual(f.map(x => x.cls), ['danger', 'danger', 'danger', 'warn']);
  assert.equal(f[0].text, 'E-STOP ACTIVE');
  assert.deepEqual(A.flags(proc({ safety: { interlock: true, cooling_flow: false } })).map(x => x.cls), ['ok', 'ok', 'danger', 'ok']);
  assert.deepEqual(A.flags(proc({ safety: { interlock: false, cooling_flow: true } })).map(x => x.cls), ['ok', 'danger', 'ok', 'ok']);
  assert.equal(f[3].text, 'TEMP (SIM) 30.5 °C');
  assert.equal(A.flags(proc({ safety: { temp: 32.0, temp_ok: false } }))[3].cls, 'warn');     // 32.0 is not > 32
  assert.equal(A.flags(proc({ safety: { temp: 32.1, temp_ok: false } }))[3].cls, 'danger');
});
function en(b) { return Object.fromEntries(['INIT', 'HV', 'MEASURE', 'TRAVERSE', 'STOP', 'ACK'].map(k => [k, b[k].enabled])); }
test('w12 buttons without data: only STOP', () => {
  const b = A.buttons(null);
  assert.deepEqual(en(b), { INIT: false, HV: false, MEASURE: false, TRAVERSE: false, STOP: true, ACK: false });
  assert.equal(b.HV.label, 'HV ON'); assert.equal(b.HV.cmd, 'HV_ON'); assert.equal(b.hint, '');
});
test('w12 buttons in SAFE_START and READY', () => {
  let b = A.buttons(proc({ state: 'SAFE_START', motion: { homed: false }, safety: { hv_ok: false } }));
  assert.deepEqual(en(b), { INIT: true, HV: false, MEASURE: false, TRAVERSE: false, STOP: true, ACK: false });
  b = A.buttons(proc({ state: 'READY' }));
  assert.deepEqual(en(b), { INIT: true, HV: true, MEASURE: false, TRAVERSE: false, STOP: true, ACK: false });
  assert.equal(b.HV.label, 'HV ON'); assert.equal(b.HV.cmd, 'HV_ON');
  b = A.buttons(proc({ state: 'READY', safety: { hv_ok: false } }));               // HV conditions not met
  assert.equal(b.HV.enabled, false);
  b = A.buttons(proc({ state: 'READY', motion: { state: 'homing' } }));             // homing running
  assert.equal(b.INIT.enabled, false);
  b = A.buttons(proc({ state: 'READY', motion: { state: 'moving' } }));
  assert.equal(b.INIT.enabled, false);
});
test('w12 buttons in RUNNING', () => {
  let b = A.buttons(proc({ state: 'RUNNING', safety: { hv_on: true } }));
  assert.deepEqual(en(b), { INIT: false, HV: true, MEASURE: true, TRAVERSE: true, STOP: true, ACK: false });
  assert.equal(b.HV.label, 'HV OFF'); assert.equal(b.HV.cmd, 'HV_OFF');
  b = A.buttons(proc({ state: 'RUNNING', safety: { hv_on: true }, motion: { state: 'moving' } }));
  assert.equal(b.MEASURE.enabled, false); assert.equal(b.TRAVERSE.enabled, false);
  b = A.buttons(proc({ state: 'RUNNING', safety: { hv_on: true }, motion: { homed: false } }));
  assert.equal(b.MEASURE.enabled, false); assert.equal(b.TRAVERSE.enabled, false);
  b = A.buttons(proc({ state: 'RUNNING', safety: { hv_on: false, hv_ok: true } }));   // between HV off and READY
  assert.equal(b.HV.cmd, 'HV_ON'); assert.equal(b.HV.enabled, true);
});
test('w12 buttons in FAULT and WAIT_ACK', () => {
  let b = A.buttons(proc({ state: 'FAULT', err: 1 }));
  assert.deepEqual(en(b), { INIT: false, HV: false, MEASURE: false, TRAVERSE: false, STOP: true, ACK: false });
  b = A.buttons(proc({ state: 'WAIT_ACK', err: 1 }));
  assert.deepEqual(en(b), { INIT: false, HV: false, MEASURE: false, TRAVERSE: false, STOP: true, ACK: true });
  b = A.buttons(proc({ state: 'WAIT_ACK', err: 1, safety: { hv_on: true } }));        // HV OFF is always possible
  assert.equal(b.HV.enabled, true); assert.equal(b.HV.cmd, 'HV_OFF');
});
test('w12 buttons while LOCAL: start buttons off, STOP and HV OFF stay, hint', () => {
  let b = A.buttons(proc({ state: 'READY', local: true }));
  assert.deepEqual(en(b), { INIT: false, HV: false, MEASURE: false, TRAVERSE: false, STOP: true, ACK: false });
  assert.equal(b.hint, 'Operator at display has priority');
  b = A.buttons(proc({ state: 'RUNNING', local: true, safety: { hv_on: true } }));
  assert.deepEqual(en(b), { INIT: false, HV: true, MEASURE: false, TRAVERSE: false, STOP: true, ACK: false });
  b = A.buttons(proc({ state: 'WAIT_ACK', local: true }));
  assert.equal(b.ACK.enabled, false);
  assert.equal(A.buttons(proc({ state: 'READY' })).hint, '');
});
test('w12 confirmText', () => {
  assert.deepEqual(A.confirmText('HV_ON'), { title: 'Switch HV on?', body: 'The X-ray source (HV LED) is switched on.' });
  assert.deepEqual(A.confirmText('ACK'), { title: 'Acknowledge fault?', body: 'The cause is gone. The system returns to READY or SAFE_START.' });
  assert.deepEqual(A.confirmText('REBOOT'), { title: 'Reboot device?', body: 'The device restarts. The connection is lost for some seconds.' });
  assert.deepEqual(A.confirmText('CALIB_RESET'), { title: 'Reset calibration?', body: 'The calibration returns to the factory values.' });
  for (const c of ['HV_OFF', 'STOP', 'INIT', 'START_MEASURE', 'CALIB', 'SET']) assert.equal(A.confirmText(c), null, c);
});
test('w12 heartbeatDue', () => {
  const armed = proc({ safety: { comm_armed: true } });
  assert.equal(A.heartbeatDue(armed, 10000, 8000), true);
  assert.equal(A.heartbeatDue(armed, 10000, 8001), false);
  assert.equal(A.heartbeatDue(armed, 10000, 0), true);
  assert.equal(A.heartbeatDue(proc(), 10000, 0), false);
  assert.equal(A.heartbeatDue(null, 10000, 0), false);
  assert.equal(A.HEARTBEAT_MS, 2000);
});
test('w12 seriesPush keeps the last 100 valid samples', () => {
  const s = [];
  assert.equal(A.seriesPush(s, proc()), s);                          // same array
  assert.deepEqual(s, [{ dist: 187.7, thick: 12.3 }]);
  A.seriesPush(s, proc({ measure: { valid: false, dist: 1, thick: 2 } }));
  assert.equal(s.length, 1);
  for (let i = 0; i < 150; i++) A.seriesPush(s, proc({ measure: { dist: i, thick: i / 10 } }));
  assert.equal(s.length, 100);
  assert.deepEqual(s[0], { dist: 50, thick: 5 });
  assert.deepEqual(s[99], { dist: 149, thick: 14.9 });
  const t = [];
  for (let i = 0; i < 5; i++) A.seriesPush(t, proc({ measure: { dist: i } }), 3);
  assert.deepEqual(t.map(x => x.dist), [2, 3, 4]);
  const u = [1, 2, 3, 4, 5].map(i => ({ dist: i, thick: 0 }));             // already too long
  A.seriesPush(u, proc({ measure: { valid: false } }), 3);
  assert.deepEqual(u.map(x => x.dist), [3, 4, 5]);
  assert.equal(A.SERIES_MAX, 100);
});
test('w12 pill', () => {
  assert.deepEqual(A.pill('ok'), { text: 'Online', offline: false });
  assert.deepEqual(A.pill('crc'), { text: 'CRC error', offline: true });
  assert.deepEqual(A.pill('offline'), { text: 'Offline', offline: true });
  assert.deepEqual(A.pill(undefined), { text: 'Offline', offline: true });
});
test('w12 validationBadges', () => {
  const nv = [{ text: 'not validated', cls: 'warn' }];
  assert.deepEqual(A.validationBadges(null), nv);
  assert.deepEqual(A.validationBadges({}), nv);
  assert.deepEqual(A.validationBadges({ validation: { validated_by: [] } }), nv);
  assert.deepEqual(A.validationBadges({ validation: { validated_by: 'x' } }), nv);
  const add = { validation: { validated_by: [
    { version: 'old-model', validated_at: '2026-01-01T00:00:00Z', status: 'failed', score: { structure: 'fail' } },
    { version: 'claude-opus-5-5', validated_at: '2026-10-20T08:00:00Z', status: 'passed_with_warnings',
      score: { structure: 'pass', security: 'warning', functional: 'pass', timing_compliance: 'fail' } },
    { version: 'same-date', validated_at: '2026-10-20T08:00:00Z', status: 'passed', score: {} },
  ] } };
  assert.deepEqual(A.validationBadges(add), [
    { text: '✓ structure', cls: '' },
    { text: '! security', cls: 'warn' },
    { text: '✓ functional', cls: '' },
    { text: '✗ timing_compliance', cls: 'fail' },
    { text: 'PASSED WITH WARNINGS · claude-opus-5-5 · 2026-10-20', cls: 'info' },
  ]);
});

// ---------------------------------------------------------------- W1.3 settings rules
test('w13 PARAMS: keys, order and ranges of CONTRACT §9', () => {
  assert.deepEqual(A.PARAMS.map(d => d.key), ['measure_pos', 'maint_pos', 'traverse_start', 'traverse_end',
    'traverse_speed', 'shutter_hold', 'edge_threshold', 'base_dist', 'calib_near_ref', 'calib_far_ref', 'temp',
    'interlock', 'cooling_flow', 'estop_nc', 'wifi_ssid', 'wifi_pass']);
  const r = Object.fromEntries(A.PARAMS.filter(d => d.type === 'num').map(d => [d.key, [d.min, d.max]]));
  assert.deepEqual(r.maint_pos, [21, 60]);
  assert.deepEqual(r.temp, [-20, 99.9]);
  assert.deepEqual(r.edge_threshold, [0.5, 100]);
});
test('w13 checkParam numbers', () => {
  assert.deepEqual(A.checkParam('maint_pos', '40'), { ok: true, value: 40 });
  assert.deepEqual(A.checkParam('maint_pos', ' 21 '), { ok: true, value: 21 });
  assert.deepEqual(A.checkParam('maint_pos', '60'), { ok: true, value: 60 });
  assert.deepEqual(A.checkParam('maint_pos', '20.9'), { ok: false, error: '21 ... 60' });
  assert.deepEqual(A.checkParam('maint_pos', '60.1'), { ok: false, error: '21 ... 60' });
  assert.deepEqual(A.checkParam('maint_pos', ''), { ok: false, error: '21 ... 60' });
  assert.deepEqual(A.checkParam('maint_pos', 'abc'), { ok: false, error: '21 ... 60' });
  assert.deepEqual(A.checkParam('maint_pos', 'NaN'), { ok: false, error: '21 ... 60' });
  assert.deepEqual(A.checkParam('shutter_hold', ''), { ok: false, error: '0 ... 10' });     // empty is not 0
  assert.deepEqual(A.checkParam('shutter_hold', '0'), { ok: true, value: 0 });
  assert.deepEqual(A.checkParam('temp', '-20'), { ok: true, value: -20 });
  assert.deepEqual(A.checkParam('temp', '100'), { ok: false, error: '-20 ... 99.9' });
  assert.deepEqual(A.checkParam('edge_threshold', '0.4'), { ok: false, error: '0.5 ... 100' });
  assert.deepEqual(A.checkParam('nonsense', '1'), { ok: false, error: 'unknown key' });
});
test('w13 checkParam booleans and strings', () => {
  for (const t of [true, 'true', '1']) assert.deepEqual(A.checkParam('interlock', t), { ok: true, value: true });
  for (const f of [false, 'false', '0']) assert.deepEqual(A.checkParam('estop_nc', f), { ok: true, value: false });
  assert.deepEqual(A.checkParam('cooling_flow', 'yes'), { ok: false, error: 'true or false' });
  assert.deepEqual(A.checkParam('cooling_flow', ''), { ok: false, error: 'true or false' });
  assert.deepEqual(A.checkParam('wifi_ssid', ''), { ok: true, value: '' });
  assert.deepEqual(A.checkParam('wifi_ssid', 'x'.repeat(32)), { ok: true, value: 'x'.repeat(32) });
  assert.deepEqual(A.checkParam('wifi_ssid', 'x'.repeat(33)), { ok: false, error: '0 ... 32 characters' });
  assert.deepEqual(A.checkParam('wifi_pass', '1234567'), { ok: false, error: '8 ... 63 characters' });
  assert.deepEqual(A.checkParam('wifi_pass', '12345678'), { ok: true, value: '12345678' });
  assert.deepEqual(A.checkParam('wifi_pass', 'p'.repeat(64)), { ok: false, error: '8 ... 63 characters' });
});
const CFG = { measure_pos: 155.0, maint_pos: 40.0, traverse_start: 80.0, traverse_end: 240.0, traverse_speed: 10.0,
  shutter_hold: 3.0, edge_threshold: 3.0, base_dist: 70.0, calib_near_raw: 40.0, calib_near_ref: 40.0,
  calib_far_raw: 65.0, calib_far_ref: 65.0, calib_valid: true, interlock: false, cooling_flow: false, temp: 18.0,
  estop_nc: false, wifi_ssid: '', wifi_pass_set: false };
test('w13 configDiff: unchanged form gives no SET', () => {
  const form = { measure_pos: '155.0', maint_pos: '40', traverse_start: '80.0', traverse_end: '240', traverse_speed: '10',
    shutter_hold: '3', edge_threshold: '3.0', base_dist: '70', calib_near_ref: '40', calib_far_ref: '65', temp: '18',
    interlock: false, cooling_flow: 'false', estop_nc: false, wifi_ssid: '', wifi_pass: '' };
  assert.deepEqual(A.configDiff(CFG, form), { sets: [], errors: [] });
});
test('w13 configDiff: changes in PARAMS order, errors collected, password only when typed', () => {
  const form = { wifi_pass: 'secret123', maint_pos: '45', temp: '35.5', measure_pos: '300', interlock: true,
    wifi_ssid: 'Lab', traverse_speed: '10.00005' };
  assert.deepEqual(A.configDiff(CFG, form), {
    sets: [{ key: 'maint_pos', value: 45 }, { key: 'temp', value: 35.5 }, { key: 'interlock', value: true },
           { key: 'wifi_ssid', value: 'Lab' }, { key: 'wifi_pass', value: 'secret123' }],
    errors: [{ key: 'measure_pos', error: '61 ... 242' }],
  });
  assert.deepEqual(A.configDiff({ wifi_pass: 'secret123' }, { wifi_pass: 'secret123' }),   // a typed password is always sent
    { sets: [{ key: 'wifi_pass', value: 'secret123' }], errors: [] });
  assert.deepEqual(A.configDiff(CFG, { wifi_pass: 'short' }), { sets: [], errors: [{ key: 'wifi_pass', error: '8 ... 63 characters' }] });
  assert.deepEqual(A.configDiff(CFG, { calib_near_raw: '1', unknown: '2' }), { sets: [], errors: [] });   // not settable: ignored
  assert.deepEqual(A.configDiff(CFG, { traverse_speed: '10.001' }), { sets: [{ key: 'traverse_speed', value: 10.001 }], errors: [] });
  assert.deepEqual(A.configDiff({}, { maint_pos: '40' }), { sets: [{ key: 'maint_pos', value: 40 }], errors: [] });   // no old value
});
