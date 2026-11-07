// Tests of the web pages (Contract V1.5.26, §5.5) – the IMS operator UI replaces the W1 pages.
//   node --test test/js/test_pages.js
// data/www/index.html is ONE file for two uses: served by the device at "/" and opened as a local
// file (connection dialog). It embeds a byte-identical copy of data/www/app.js (ADDAPP).
// The behaviour is checked by the reviewer in a browser against scripts/fake_device.py.
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const WWW = path.join(__dirname, '..', '..', 'data', 'www');
const read = (name) => fs.readFileSync(path.join(WWW, name), 'utf8').replace(/\r\n/g, '\n');
const scripts = (html) => [...html.matchAll(/<script>\n?([\s\S]*?)<\/script>/g)].map((m) => m[1]);
const IDS = [
  // shell
  'urlModal', 'hostInput', 'portInput', 'btnConnect', 'btnDemo', 'confirmModal', 'confirmTitle', 'confirmBody',
  'confirmCancel', 'confirmOk', 'navConnect', 'alarmBox', 'alarmTitle', 'alarmDetail', 'sboxHv', 'sboxShutter',
  'deviceId', 'pageTitle', 'connDot', 'connLabel', 'clockDisplay', 'toast',
  // pages
  'page-process', 'page-status', 'page-settings', 'page-calibration', 'page-log',
  // process
  'matState', 'matPos', 'matVel', 'matDist', 'matThick', 'matTemp', 'mMax', 'mAvg', 'mMin', 'mAct', 'mN', 'mPos',
  'gaugeCanvas', 'gaugeValue', 'btnInit', 'btnHv', 'btnMeasure', 'btnTraverse', 'btnPark', 'btnService', 'btnAck',
  'btnStop', 'ctrlHint', 'trendChart', 'distChart',
  // status
  'valList', 'ssCycle', 'ssPeriod', 'ssPrio', 'ssOverruns', 'ioArmed', 'ioLocal',
  // settings
  'f_measure_pos', 'f_maint_pos', 'f_traverse_start', 'f_traverse_end', 'f_traverse_speed', 'f_shutter_hold',
  'f_edge_threshold', 'f_base_dist', 'f_estop_nc', 'f_interlock', 'f_cooling_flow', 'f_temp', 'f_wifi_ssid',
  'f_wifi_pass', 'btnSave', 'btnReload', 'btnSetTemp', 'btnReboot',
  // calibration
  'f_calib_near_ref', 'f_calib_far_ref', 'btnCalMoveNear', 'btnCalNear', 'btnCalMoveFar', 'btnCalFar', 'btnCalSave',
  'btnCalBase', 'btnCalReset', 'btnCalRefs', 'calibChart',
  // log
  'devLog', 'btnDevLog', 'logTableBody',
  // help (V1.5.27)
  'navHelp',
];

test('index: embeds a byte-identical copy of app.js', () => {
  const s = scripts(read('index.html'));
  assert.equal(s.length, 2, 'two inline scripts: ADDAPP copy and page script');
  const app = read('app.js').replace(/<\/script>/g, '<\\/script>');
  assert.ok(s[0].includes(app), 'the first inline script must contain data/www/app.js unchanged – rebuild index.html');
});

test('index: scripts compile and the page script uses ADDAPP for protocol and rules', () => {
  const s = scripts(read('index.html'));
  for (const js of s) assert.doesNotThrow(() => new vm.Script(js), 'syntax error in an inline script');
  const page = s[1];
  assert.match(page, /const A = window\.ADDAPP;/);
  assert.ok(!/function (crc32|frame|verify|parseFramed|apiBody)\b/.test(page), 'protocol functions belong to ADDAPP');
  for (const f of ['A.parseFramed(', 'A.apiBody(', 'A.buttons(', 'A.configDiff(', 'A.checkParam(', 'A.validationBadges(']) {
    assert.ok(page.includes(f), 'page script must use ' + f);
  }
});

test('index: no external resources (no internet in AP mode)', () => {
  const h = read('index.html');
  for (const m of h.matchAll(/\b(?:src|href)="([^"]*)"/g)) assert.ok(m[1].startsWith('/') && !m[1].startsWith('//'), 'only device paths: ' + m[1]);
  for (const m of h.matchAll(/url\('([^']*)'\)/g)) assert.ok(m[1].startsWith('/'), 'only device paths in CSS: ' + m[1]);
  assert.ok(!/fonts\.googleapis|cdn\.|unpkg|jsdelivr/.test(h), 'no CDN');
  // the only absolute URLs: 'http://' + HOST for a local file and the SVG namespace
  const abs = [...h.matchAll(/https?:\/\/[^\s'"`<)]*/g)].map((m) => m[0]);
  for (const u of abs) assert.ok(['http://', 'http://www.w3.org/2000/svg'].includes(u), 'unexpected URL ' + u);
});

test('index: device paths and endpoints', () => {
  const page = scripts(read('index.html'))[1];
  for (const p of ["'/api'", "'/proc?t='", "'/add'", "'/log?t='"]) assert.ok(page.includes(p), 'endpoint ' + p);
  assert.match(page, /const ON_DEVICE = location\.protocol === 'http:' \|\| location\.protocol === 'https:';/);
  assert.match(page, /function baseUrl\(\) \{ return HOST \? 'http:\/\/' \+ HOST : ''; \}/, 'own origin on the device');
});

test('index: rules from §5.5 are present', () => {
  const page = scripts(read('index.html'))[1];
  assert.match(page, /\$\('btnStop'\)\.onclick = \(\) => api\('STOP'\);/, 'STOP without dialog');
  for (const c of ['HV_ON', 'ACK', 'REBOOT', 'CALIB_RESET']) assert.match(page, new RegExp('\\b' + c + ': \\['), 'confirmation for ' + c);
  assert.match(page, /const POLL_MS = 500;/);
  assert.match(page, /const HEARTBEAT_MS = 1000;/);
  assert.match(page, /comm_armed && Date\.now\(\) - lastApi >= HEARTBEAT_MS/, 'heartbeat only while armed');
  assert.match(page, /const tv = !hv \? 0 : !m\.valid \? null : m\.on_material \? m\.thick : 0;/, 'thickness 0 with HV off');
  assert.ok(!/setInterval\(\s*\(\)\s*=>\s*api\(/.test(page), 'commands are never repeated automatically');
});

test('index: element ids present and unique', () => {
  const h = read('index.html');
  const ids = [...h.matchAll(/\bid="([^"]+)"/g)].map((m) => m[1]);
  for (const id of IDS) assert.ok(ids.includes(id), 'id ' + id + ' missing');
  assert.equal(new Set(ids).size, ids.length, 'duplicate id');
  const page = scripts(h)[1];
  for (const m of page.matchAll(/\$\('([^']+)'\)/g)) assert.ok(ids.includes(m[1]), 'script uses unknown id ' + m[1]);
  assert.ok(h.length < 120000, 'page larger than 120 kB');
});

test('index: help link opens /doc in a new tab (V1.5.27)', () => {
  const h = read('index.html');
  assert.match(h, /<a class="nav-item" id="navHelp" href="\/doc" target="_blank" rel="noopener" data-tip="Hilfe">/);
  const page = scripts(h)[1];
  assert.match(page, /\$\('navHelp'\)\.href = baseUrl\(\) \+ '\/doc';/, 'own origin on the device, host when opened as a file');
});

test('web-ui/ims-demo-ui.html is a byte-identical copy of data/www/index.html', () => {
  const copy = fs.readFileSync(path.join(__dirname, '..', '..', 'web-ui', 'ims-demo-ui.html'), 'utf8').replace(/\r\n/g, '\n');
  assert.equal(copy, read('index.html'), 'copy data/www/index.html to web-ui/ims-demo-ui.html');
});

test('settings: redirects to the settings page of the single-page UI', () => {
  const h = read('settings.html');
  assert.match(h, /<meta http-equiv="refresh" content="0; url=\/#settings">/);
  assert.ok(h.length < 1000);
});
