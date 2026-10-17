// FROZEN TESTS (Contract V1.5.22) – Sprint W1. Do not modify.
// Static checks of the two pages data/www/index.html and data/www/settings.html:
//   node --test test/js/test_pages.js                                  (both pages)
//   node --test --test-name-pattern="^index" test/js/test_pages.js     (one page)
// The behaviour of the pages is checked by the reviewer in a browser against scripts/fake_device.py.
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const crypto = require('node:crypto');

const WWW = path.join(__dirname, '..', '..', 'data', 'www');
const PAGES = {
  index: {
    tag: 'W1.4',
    head: '1a7d3c24ffc8e24402346e0458d3de7ca839c8de7767c32596b833aad622e045',
    functions: ['api', 'command', 'render', 'poll', 'loadValidation', 'init'],
    ids: ['pill', 'pillDot', 'pillText', 'localBadge', 'theme', 'state', 'err', 'hv', 'shutter', 'mode', 'pos', 'vel',
          'dist', 'thick', 'avg', 'n', 'chart', 'flags', 'btnInit', 'btnHv', 'btnMeasure', 'btnTraverse', 'btnAck',
          'btnStop', 'hint', 'valStrip', 'confirmOverlay', 'confirmTitle', 'confirmBody', 'confirmCancel', 'confirmOk', 'toast'],
  },
  settings: {
    tag: 'W1.5',
    head: 'd80a852920542418aff647aa728ae1a975c9c95e0ab057663ebfe4fc8e04abb1',
    functions: ['api', 'readForm', 'fillForm', 'load', 'save', 'setNow', 'init'],
    ids: ['pill', 'pillDot', 'pillText', 'theme', 'f_measure_pos', 'f_maint_pos', 'f_traverse_start', 'f_traverse_end',
          'f_traverse_speed', 'f_shutter_hold', 'f_edge_threshold', 'f_base_dist', 'f_calib_near_ref', 'f_calib_far_ref',
          'f_temp', 'f_interlock', 'f_cooling_flow', 'f_estop_nc', 'f_wifi_ssid', 'f_wifi_pass', 'calInfo', 'wifiInfo',
          'btnCalNear', 'btnCalFar', 'btnCalBase', 'btnCalSave', 'btnCalReset', 'btnReboot', 'btnSave', 'btnReload',
          'confirmOverlay', 'confirmTitle', 'confirmBody', 'confirmCancel', 'confirmOk', 'toast'],
  },
};
const TAIL = '\ninit();\n</script>\n</body>\n</html>\n';
const sha = (s) => crypto.createHash('sha256').update(s).digest('hex');
const read = (name) => fs.readFileSync(path.join(WWW, name + '.html'), 'utf8').replace(/\r\n/g, '\n');
const script = (html) => {
  const i = html.indexOf("<script>\n'use strict';");
  assert.ok(i > 0, 'inline <script> with \'use strict\' not found');
  return html.slice(i + 8, html.lastIndexOf('</script>'));
};

for (const [name, P] of Object.entries(PAGES)) {
  test(name + ': markup and given helpers unchanged (everything before "// ---- TODO(' + P.tag + ')")', () => {
    const h = read(name);
    const i = h.indexOf('// ---- TODO(' + P.tag + ')');
    assert.ok(i > 0, 'marker line "// ---- TODO(' + P.tag + ')" missing');
    assert.equal(sha(h.slice(0, i)), P.head, 'the part above the TODO marker was changed – restore it from git');
    assert.ok(h.endsWith(TAIL), 'the file must end with init(); </script></body></html> as in the skeleton');
  });
  test(name + ': script compiles and defines exactly the skeleton functions', () => {
    const js = script(read(name));
    assert.doesNotThrow(() => new vm.Script(js), 'syntax error in the inline script');
    for (const f of P.functions) assert.match(js, new RegExp('(async )?function ' + f + '\\('), 'function ' + f + ' missing');
    const defined = [...js.matchAll(/^(?:async )?function (\w+)\(/gm)].map((m) => m[1]);
    const given = ['setTheme', 'toast', 'showConfirm', 'closeConfirm', 'drawChart', 'setPill'];
    for (const f of defined) assert.ok(given.includes(f) || P.functions.includes(f), 'extra top-level function ' + f + ' – use the skeleton functions only');
  });
  test(name + ': all TODOs filled in', () => {
    const js = script(read(name));
    assert.ok(!/throw new Error\('TODO/.test(js), 'a function still throws TODO');
  });
  test(name + ': uses ADDAPP, no own protocol code, no external resources', () => {
    const h = read(name), js = script(h);
    assert.ok(!/function (crc32|frame|verify|apiBody|parseFramed)\b/.test(js), 'protocol functions belong to app.js (A.*)');
    assert.ok(!/\bJSON\.stringify\(/.test(js), 'request bodies are built with A.apiBody only');
    assert.ok(!/https?:\/\//.test(h), 'no http:// or https:// URL (no internet in AP mode)');
    for (const m of h.matchAll(/\b(?:src|href)="([^"]*)"/g)) assert.ok(m[1].startsWith('/') && !m[1].startsWith('//'), 'only device paths: ' + m[1]);
    for (const m of js.matchAll(/fetch\(\s*'([^']*)'/g)) assert.ok(['/api', '/proc?t=', '/add?t='].includes(m[1]), 'unexpected fetch target ' + m[1]);
    assert.ok(h.length < 40000, 'page larger than 40 kB');
  });
  test(name + ': element ids present and unique', () => {
    const h = read(name);
    const ids = [...h.matchAll(/\bid="([^"]+)"/g)].map((m) => m[1]);
    for (const id of P.ids) assert.ok(ids.includes(id), 'id ' + id + ' missing');
    assert.equal(new Set(ids).size, ids.length, 'duplicate id');
  });
}
