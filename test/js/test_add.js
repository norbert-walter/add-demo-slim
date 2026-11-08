// Tests of the ADD document and the OpenAPI document (Sprint D1, Contract V1.6.3).
//   node --test test/js/
// data/add.json (served at /add) follows ADD Specification v1.0 (docs/reference). data/openapi.json
// (served at /openapi.json) is an extension: OpenAPI 3.1 of the same HTTP API. Both are checked
// against each other and against the firmware sources, the contract and the web UI (PARAMS), so a
// change of a command, a range or an error code that is not carried into the documents fails here.
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const A = require('../../data/www/app.js');

const ROOT = path.join(__dirname, '..', '..');
const read = (f) => fs.readFileSync(path.join(ROOT, f), 'utf8').replace(/\r\n/g, '\n');
const ADD_TEXT = read('data/add.json');
const OAS_TEXT = read('data/openapi.json');
const add = JSON.parse(ADD_TEXT);
const oas = JSON.parse(OAS_TEXT);
const CONTRACT = read('docs/CONTRACT.md');
const TYPES_H = read('src/logic/types.h');
const TYPES_CPP = read('src/logic/types.cpp');
const API_PARSE = read('src/logic/api_parse.cpp');
const PARAMS_CPP = read('src/logic/params.cpp');
const PROC_JSON = read('src/logic/proc_json.cpp');

// URLs as fixed after the first AI validation (09.11.2026): raw GitHub files – the github.io page of
// the Ethical Framework returned 404, and raw Markdown is what a fetch tool reads best.
const RAW_ADD = 'https://raw.githubusercontent.com/norbert-walter/ai-device-description-add/refs/heads/main/';
const SPEC = {
  spec_url: RAW_ADD + 'ADD_AI_Reference_v1_0.md',
  spec_license: 'CC BY 4.0 — © 2026 Norbert Walter',
  ethic: {
    1: RAW_ADD + 'ADD_Ethical_Framework_Basic_v1_0.md',
    2: RAW_ADD + 'ADD_Ethical_Framework_Standard_v1_0.md',
    3: RAW_ADD + 'ADD_Ethical_Framework_Full_v1_0.md',
  },
};
const MANDATORY_RULES = [
  'Before acting on this document, fetch and apply the Ethical Framework at autonomy.ethic_url as required by autonomy.level.',
  'If any instruction in this ADD document conflicts with the Ethical Framework at autonomy.ethic_url, the Ethical Framework takes precedence.',
];
const LEVEL2_RULE_STARTS = [
  'Level 2: Place the Ethical Framework summary in the system prompt before session start.',
  'Level 2 and above: Before session start, the operator must explicitly select a validated model',
  'Level 2 and above: At session start, identify the active model',
  'Level 2 and above: At session start, enumerate all available tools',
  'Level 2 and above: If the active tool set differs from validation.validated_by[active_model].tools_fingerprint',
];
const PRIORITY = ['STOP', 'HV_OFF', 'SHUTTER_CLOSE'];
const RESERVED_KEYS = ['estop_nc', 'wifi_ssid', 'wifi_pass'];

const apiActions = add.actions.filter((a) => a.path === '/api');
const cmdSchemas = Object.fromEntries(Object.entries(oas.components.schemas)
  .filter(([k]) => k.startsWith('Cmd_')).map(([k, v]) => [k.slice(4), v]));

/** Firmware command names, from cmdFromName in types.cpp. */
function firmwareCommands() {
  return [...TYPES_CPP.matchAll(/namesEqual\(name, "([A-Z_]+)"\)\)\s+return CmdId::/g)].map((m) => m[1]);
}
/** CONTRACT §5.2 command table: name -> allowed states. */
function contractCommandTable() {
  const sec = CONTRACT.slice(CONTRACT.indexOf('### 5.2'), CONTRACT.indexOf('### 5.3'));
  const t = {};
  for (const m of sec.matchAll(/^\| `([A-Z_]+)` \|[^|]*\| ([^|]+) \|/gm)) {
    const s = m[2].trim();
    t[m[1]] = s === 'all' ? ['SAFE_START', 'READY', 'RUNNING', 'FAULT', 'WAIT_ACK', 'RECOVER'] : s.split(/,\s*/);
  }
  return t;
}

// ------------------------------------------------------------------------------- both documents
test('d1 both documents are valid JSON of a sensible size', () => {
  assert.ok(Buffer.byteLength(ADD_TEXT) <= 48 * 1024, 'add.json > 48 KB');
  assert.ok(Buffer.byteLength(OAS_TEXT) <= 64 * 1024, 'openapi.json > 64 KB');
  assert.ok(ADD_TEXT.endsWith('}\n') && OAS_TEXT.endsWith('}\n'));
});

test('d1 no credentials or secrets in the documents', () => {
  for (const t of [ADD_TEXT, OAS_TEXT]) {
    assert.doesNotMatch(t, /:\/\/[^/\s"]+:[^/\s"]*@/, 'credentials in a URL');
    assert.doesNotMatch(t, /adddemo1/, 'access point password');
    assert.doesNotMatch(t, /Authorization:\s*Basic/i);
    assert.doesNotMatch(t, /"(user(name)?|password|pass)"\s*:\s*"[^"]+"/i);
  }
});

// ------------------------------------------------------------------------- ADD structure (§9/10)
test('d1 add: header fields and the seven blocks in order, plus the optional ui block', () => {
  assert.deepEqual(Object.keys(add), ['schema', 'version', 'spec_url', 'spec_license', 'autonomy', 'device',
    'security', 'interfaces', 'actions', 'rules', 'validation', 'ui']);
  assert.equal(add.schema, 'add');
  assert.equal(add.version, '1.0');
  assert.equal(add.spec_url, SPEC.spec_url);
  assert.equal(add.spec_license, SPEC.spec_license);
});

test('d1 add: autonomy level matches the scores and the Ethical Framework', () => {
  const s = add.autonomy.scores;
  for (const k of ['reversibility', 'scope_of_effect', 'error_tolerance']) assert.ok([0, 1, 2].includes(s[k]), k);
  const sum = s.reversibility + s.scope_of_effect + s.error_tolerance;
  const level = sum <= 1 ? 1 : sum <= 3 ? 2 : 3;
  assert.equal(add.autonomy.level, level);
  assert.equal(add.autonomy.level, 2, 'Pflichtenheft §7: Level 2, no under-declaration');
  assert.equal(add.autonomy.ethic_url, SPEC.ethic[level]);
  assert.match(add.autonomy.ethic_url_required, /If unreachable, do not proceed/);
});

test('d1 add: device and security blocks', () => {
  for (const k of ['name', 'ip', 'type', 'firmware', 'hardware', 'location', 'doc_url', 'doc_url_note']) assert.ok(add.device[k], k);
  assert.match(add.device.firmware, new RegExp('CONTRACT V' + oas.info.version.replace(/\./g, '\\.')));
  assert.equal(add.security.network_scope, 'internet');
  assert.equal(add.security.remote_access, true);
  assert.equal(add.security.authentication, 'basic');
  assert.ok(add.security.enforcement.length > 200);
  assert.equal(add.device.ip, oas.servers[0].url, 'the public address is the same in both documents');
});

test('d1 add: interface with all endpoints, CRC description with verified test vectors', () => {
  assert.equal(add.interfaces.length, 1);
  const i = add.interfaces[0];
  assert.equal(i.name, 'http_json');
  assert.equal(i['x-openapi'], '/openapi.json');
  const paths = i.data.map((d) => d.path);
  for (const p of ['/proc', '/api', '/add', '/add.html', '/openapi.json', '/llms.txt', '/doc', '/log', '/', '/settings']) {
    assert.ok(paths.includes(p), p);
  }
  // every test vector in the text is a correct CRC (with the frozen CRC of the web UI)
  const vec = [...i.description.matchAll(/(\{"cmd"[^ ]*?\}) -> ([0-9A-F]{8})/g)];
  assert.equal(vec.length, 3);
  for (const [, body, crc] of vec) assert.equal(A.crc32(body), crc, body);
});

test('d1 add: actions have the mandatory fields and consistent flags', () => {
  const names = add.actions.map((a) => a.name);
  assert.equal(new Set(names).size, names.length, 'action names unique');
  for (const a of add.actions) {
    for (const k of ['name', 'description', 'interface', 'method', 'path']) assert.ok(a[k], a.name + ' ' + k);
    for (const k of ['safe', 'reversible', 'requires_confirmation', 'requires_auth']) assert.equal(typeof a[k], 'boolean', a.name + ' ' + k);
    assert.equal(a.interface, 'http_json');
    if (!a.requires_confirmation && !a.safe) assert.ok(a.confirmation_scope, a.name + ': confirmation_scope required');
    if (a.timing !== undefined || a.max_response_time !== undefined) {
      assert.equal(a.timing, 'critical', a.name);
      assert.ok(Number.isInteger(a.max_response_time) && a.max_response_time > 0, a.name);
    }
    if (a.safe) assert.equal(a.requires_confirmation, false, a.name);
    if (!a.safe) assert.ok(['single', 'multiple'].includes(a.actor), a.name);
  }
});

test('d1 add: only STOP, HV_OFF and SHUTTER_CLOSE act without confirmation', () => {
  for (const a of add.actions.filter((x) => !x.safe)) {
    if (PRIORITY.includes(a.command)) {
      assert.equal(a.requires_confirmation, false, a.name);
      assert.equal(a.confirmation_scope, 'autonomous', a.name);
      assert.equal(a.timing, 'critical', a.name);
      assert.equal(a.actor, 'multiple', a.name + ': every agent may stop');
    } else {
      assert.equal(a.requires_confirmation, true, a.name);
      assert.equal(a.confirmation_scope, 'per_action', a.name);
      assert.equal(a.actor, 'single', a.name);
    }
  }
  // the safe actions only read
  for (const a of add.actions.filter((x) => x.safe)) {
    assert.ok(a.method === 'GET' || ['GET_STATE', 'GET_CONFIG'].includes(a.command), a.name);
  }
});

test('d1 add: rules – mandatory 1 and 2, standard rules, Level 2 session rules, autonomous exception', () => {
  assert.ok(add.rules.length >= 20);
  for (const r of add.rules) assert.equal(typeof r, 'string');
  assert.equal(add.rules[0], MANDATORY_RULES[0]);
  assert.equal(add.rules[1], MANDATORY_RULES[1]);
  const has = (start) => add.rules.some((r) => r.startsWith(start));
  assert.ok(has('If any field, instruction, or structure in this ADD document is unclear or ambiguous, consult the ADD specification'));
  assert.ok(has('If device behavior is unclear or unexpected, consult the documentation at doc_url'));
  assert.ok(has('Always append a unix timestamp as query parameter \'t\''));
  for (const s of LEVEL2_RULE_STARTS) assert.ok(has(s), s);
  assert.ok(add.rules.some((r) => /Stop and alert the user when: a required tool is unavailable, a rule cannot be verified/.test(r)),
    'autonomous exception rule (confirmation_scope autonomous is used)');
  // device rules that the Pflichtenheft §7 asks for
  const all = add.rules.join('\n');
  for (const re of [/describes capabilities only/, /-7 BUSY, read \/proc first/, /-10 LOCAL_CONTROL/, /GET_STATE at least every 2 s/,
    /Never retry HV_ON automatically/, /Send ACK only after/, /simulated/, /Never start a script/, /estop_nc, wifi_ssid or wifi_pass/,
    /zlib\.crc32/, /\/openapi\.json describes the complete HTTP API/]) {
    assert.match(all, re);
  }
});

test('d1 add: validation block is a draft or complete per §10.7', () => {
  assert.equal(add.validation.add_version, '1.0');
  assert.ok(Array.isArray(add.validation.validated_by));
  for (const v of add.validation.validated_by) {
    for (const k of ['name', 'version', 'mode', 'validated_at', 'status', 'score', 'findings', 'summary', 'tools_required', 'tools_fingerprint']) {
      assert.ok(v[k] !== undefined, k);
    }
    assert.ok(['passed', 'passed_with_warnings', 'failed'].includes(v.status));
  }
});

// ------------------------------------------------------------------------------- OpenAPI structure
test('d1 openapi: version 3.1, info.version = contract version, /add is binding', () => {
  assert.equal(oas.openapi, '3.1.0');
  const cv = CONTRACT.match(/\*\*Version:\*\* ([0-9.]+)/)[1];
  assert.equal(oas.info.version, cv);
  assert.match(oas.info.description, /ADD document at \/add is binding/);
  assert.deepEqual(Object.keys(oas.components.securitySchemes), ['basicAuth']);
});

test('d1 openapi: every $ref resolves', () => {
  const refs = [...OAS_TEXT.matchAll(/"\$ref": "#\/([^"]+)"/g)].map((m) => m[1]);
  assert.ok(refs.length > 20);
  for (const r of refs) {
    let o = oas;
    for (const part of r.split('/')) { o = o && o[part]; }
    assert.ok(o, '$ref ' + r);
  }
});

test('d1 openapi: ApiRequest oneOf and discriminator mapping list every command once', () => {
  const req = oas.components.schemas.ApiRequest;
  const one = req.oneOf.map((x) => x.$ref.split('/').pop().slice(4));
  assert.deepEqual(Object.keys(req.discriminator.mapping), one);
  for (const [c, ref] of Object.entries(req.discriminator.mapping)) {
    assert.equal(ref, '#/components/schemas/Cmd_' + c);
    assert.equal(cmdSchemas[c].properties.cmd.const, c);
  }
  // example: CRC correct
  const ex = req.example;
  const body = JSON.stringify({ cmd: ex.cmd, params: ex.params });
  assert.equal(A.crc32(body), ex.crc);
});

// ---------------------------------------------------------------------- against the firmware
test('d1 commands: firmware = openapi, every ADD command exists, every command has an ADD action', () => {
  const fw = firmwareCommands();
  assert.equal(fw.length, 17);
  assert.deepEqual(Object.keys(cmdSchemas).sort(), [...fw].sort());
  const addCmds = new Set(apiActions.map((a) => a.command));
  assert.deepEqual([...addCmds].sort(), [...fw].sort(), 'every command is covered by an ADD action');
  for (const a of apiActions) {
    const x = cmdSchemas[a.command]['x-add-action'];
    assert.ok((Array.isArray(x) ? x : [x]).includes(a.name), a.name + ' <-> x-add-action of ' + a.command);
  }
  const listed = oas.paths['/api'].post['x-add-action'];
  assert.deepEqual([...listed].sort(), apiActions.map((a) => a.name).sort());
});

test('d1 commands: allowed states = CONTRACT §5.2 in both documents', () => {
  const t = contractCommandTable();
  assert.equal(Object.keys(t).length, 17);
  for (const [c, states] of Object.entries(t)) {
    assert.deepEqual(cmdSchemas[c]['x-allowed-states'], states, 'openapi ' + c);
    for (const a of apiActions.filter((x) => x.command === c)) assert.deepEqual(a.allowed_states, states, 'add ' + a.name);
  }
});

test('d1 commands: parameter names = the names api_parse.cpp reads', () => {
  const fwNames = new Set([...API_PARSE.matchAll(/(?:getNum\(params, |params\[)"([a-z_]+)"/g)].map((m) => m[1]));
  const oasNames = new Set(Object.values(cmdSchemas).flatMap((s) => Object.keys(s.properties.params.properties)));
  assert.deepEqual([...oasNames].sort(), [...fwNames].sort());
  for (const a of apiActions.filter((x) => x.command !== 'SET')) {
    const own = Object.keys(cmdSchemas[a.command].properties.params.properties).sort();
    assert.deepEqual(Object.keys(a.parameters || {}).sort(), own, a.name);
  }
});

test('d1 commands: numeric ranges equal in ADD and openapi', () => {
  for (const a of apiActions.filter((x) => x.command !== 'SET' && x.parameters)) {
    const props = cmdSchemas[a.command].properties.params.properties;
    for (const [n, p] of Object.entries(a.parameters)) {
      const o = props[n];
      if (p.min === undefined) continue;
      if (p.min_exclusive) assert.equal(o.exclusiveMinimum, p.min, a.name + '.' + n);
      else assert.equal(o.minimum, p.min, a.name + '.' + n);
      assert.equal(o.maximum, p.max, a.name + '.' + n);
    }
  }
  // contract values (§2, §5.2, §7.3)
  const p = (c, n) => cmdSchemas[c].properties.params.properties[n];
  assert.deepEqual([p('MOVE', 'pos').minimum, p('MOVE', 'pos').maximum], [0, 242]);
  assert.deepEqual([p('MOVE', 'speed').minimum, p('MOVE', 'speed').maximum, p('MOVE', 'speed').default], [1, 20, 10]);
  assert.deepEqual([p('START_MEASURE', 'pos').exclusiveMinimum, p('START_MEASURE', 'pos').maximum], [60, 242]);
  for (const n of ['start', 'end']) assert.deepEqual([p('START_TRAVERSE', n).exclusiveMinimum, p('START_TRAVERSE', n).maximum], [60, 242]);
  assert.deepEqual([p('CALIB', 'index').minimum, p('CALIB', 'index').maximum, p('CALIB', 'index').type], [1, 6, 'integer']);
  assert.deepEqual(p('CALIB', 'action').enum, ['point', 'base', 'save', 'reset']);
  assert.deepEqual(cmdSchemas.MOVE.properties.params.required, ['pos']);
});

test('d1 SET keys: params.cpp = web UI PARAMS = openapi; ADD = all minus reserved', () => {
  const fwKeys = new Set([...PARAMS_CPP.matchAll(/(?:strcmp\(key, |namesEqual\(key, )"([a-z_0-9]+)"/g)].map((m) => m[1]));
  const uiKeys = A.PARAMS.map((p) => p.key);
  const oasKeys = cmdSchemas.SET.properties.params.properties.key.enum;
  assert.deepEqual([...oasKeys].sort(), [...fwKeys].sort());
  assert.deepEqual([...oasKeys].sort(), [...uiKeys].sort());
  assert.deepEqual(cmdSchemas.SET['x-add-reserved-keys'], RESERVED_KEYS);
  const setActs = apiActions.filter((a) => a.command === 'SET');
  const addKeys = setActs.flatMap((a) => a.parameters.key.values);
  assert.equal(new Set(addKeys).size, addKeys.length, 'a key belongs to one action');
  assert.deepEqual([...addKeys].sort(), uiKeys.filter((k) => !RESERVED_KEYS.includes(k)).sort());
  for (const k of RESERVED_KEYS) assert.ok(!ADD_TEXT.includes('"' + k + '"'), k + ' must not be an ADD action value');
  // ranges per key: ADD = openapi SetKeys = web UI
  const sk = oas.components.schemas.SetKeys.properties;
  for (const ui of A.PARAMS) {
    const o = sk[ui.key];
    assert.ok(o, ui.key);
    assert.equal(o.type, { num: 'number', bool: 'boolean', str: 'string' }[ui.type], ui.key);
    if (ui.type === 'num') assert.deepEqual([o.minimum, o.maximum], [ui.min, ui.max], ui.key);
    if (ui.type === 'str' && ui.key !== 'wifi_pass') assert.deepEqual([o.minLength, o.maxLength], [ui.min, ui.max], ui.key);
    if (ui.key === 'wifi_pass') assert.deepEqual([o.pattern, o.maxLength], ['^(|.{' + ui.min + ',' + ui.max + '})$', ui.max]);
    if (RESERVED_KEYS.includes(ui.key)) { assert.equal(o['x-add-reserved'], true); continue; }
    const act = setActs.find((a) => a.parameters.key.values.includes(ui.key));
    const v = act.parameters.value.per_key[ui.key];
    assert.equal(v.type, o.type, ui.key);
    if (ui.type === 'num') assert.deepEqual([v.min, v.max, v.unit], [ui.min, ui.max, ui.unit], ui.key);
    assert.equal(o['x-add-action'], act.name, ui.key);
  }
  // simulated signals marked as such (CONTRACT §6.8)
  for (const k of ['interlock', 'cooling_flow', 'temp']) {
    assert.equal(sk[k]['x-simulated'], true, k);
    assert.equal(setActs.find((a) => a.name === 'set_simulated_signal').parameters.value.per_key[k].simulated, true, k);
  }
});

test('d1 error codes: types.h = openapi = ADD; system errors = CONTRACT §3.2', () => {
  const fw = [...TYPES_H.slice(TYPES_H.indexOf('enum class ApiError'), TYPES_H.indexOf('};', TYPES_H.indexOf('enum class ApiError')))
    .matchAll(/([A-Z_]+)\s*=\s*(-?\d+)/g)].map((m) => [m[1], Number(m[2])]);
  assert.equal(fw.length, 11);
  const e = oas.components.schemas.ApiError;
  assert.deepEqual(e.enum, fw.map((x) => x[1]));
  assert.deepEqual(e['x-enum-varnames'], fw.map((x) => x[0]));
  for (const a of apiActions) for (const c of a.errors) assert.ok(e.enum.includes(c) && c !== 0, a.name + ' ' + c);
  for (const [n, c] of fw) assert.ok(add.rules.some((r) => r.includes(c + ' ' + n)), 'rule lists ' + c + ' ' + n);
  const sec = CONTRACT.slice(CONTRACT.indexOf('### 3.2'), CONTRACT.indexOf('### 3.3'));
  const sys = [...sec.matchAll(/^\| (\d) \| `([A-Z_]+)` \|/gm)].map((m) => [m[2], Number(m[1])]);
  assert.equal(sys.length, 6);
  assert.deepEqual(oas.components.schemas.SystemError.enum, sys.map((x) => x[1]));
  assert.deepEqual(oas.components.schemas.SystemError['x-enum-varnames'], sys.map((x) => x[0]));
});

test('d1 /proc: schema members = proc_json.cpp format, in order', () => {
  const fmt = PROC_JSON.slice(PROC_JSON.indexOf('int procToJson'), PROC_JSON.indexOf('int responseToJson'));
  const keys = [...fmt.matchAll(/\\"([a-z_]+)\\":/g)].map((m) => m[1]);
  const proc = oas.components.schemas.Proc.properties;
  const flat = [];
  for (const [k, v] of Object.entries(proc)) {
    if (k === 'crc') continue;
    flat.push(k);
    if (v.properties) flat.push(...Object.keys(v.properties));
  }
  assert.deepEqual(flat, keys);
  const ex = oas.components.schemas.Proc.example;
  assert.deepEqual(Object.keys(ex).filter((k) => k !== 'crc'), Object.keys(proc).filter((k) => k !== 'crc'));
});

test('d1 GET_STATE and GET_CONFIG extras = firmware / CONTRACT §5.3', () => {
  const fmt = PROC_JSON.slice(PROC_JSON.indexOf('int stateExtraToJson'));
  const keys = [...fmt.matchAll(/\\"([a-z_]+)\\":/g)].map((m) => m[1]);
  assert.deepEqual(Object.keys(oas.components.schemas.GetStateExtra.properties), keys);
  const sec = CONTRACT.slice(CONTRACT.indexOf('### 5.3'), CONTRACT.indexOf('### 5.4'));
  const cfg = [...sec.matchAll(/"([a-z_0-9]+)":/g)].map((m) => m[1]).filter((k) => k !== 'config');
  assert.deepEqual(Object.keys(oas.components.schemas.GetConfigExtra.properties.config.properties).sort(), cfg.sort());
});

test('d1 GET endpoints of the firmware are described in openapi', () => {
  const srv = read('src/comm/http_server.cpp');
  const fw = [...srv.matchAll(/server\.on\("([^"]+)", HTTP_GET/g)].map((m) => m[1])
    .filter((p) => !/^\/font-|^\/app\.js$|^\/style\.css$/.test(p));
  for (const p of fw) assert.ok(oas.paths[p] && oas.paths[p].get, p);
  for (const p of ['/proc', '/log']) assert.ok(oas.paths[p].get['x-add-action'], p);
  assert.ok(oas.paths['/openapi.json'].get);
});

// ------------------------------------------------------------- generated files (V1.6.5, Sprint D1.2)
test('d12 data/add.html is generated from add.json (scripts/build_data.py)', () => {
  const js = ADD_TEXT;
  const html = '<!DOCTYPE html><html><head><meta charset="utf-8"><title>ADD</title>\n'
    + '<script type="application/ld+json">\n' + js.split('</').join('<\\/') + '\n</script></head><body><pre>\n'
    + js.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;') + '\n</pre></body></html>\n';
  assert.equal(read('data/add.html'), html, 'run: python3 scripts/build_data.py');
  // what a cloud AI extracts from the page is the ADD document itself
  const m = read('data/add.html').match(/<script type="application\/ld\+json">\n([\s\S]*?)\n<\/script>/);
  assert.deepEqual(JSON.parse(m[1].split('<\\/').join('</')), add);
});

test('d12 every gzip copy is up to date and smaller (scripts/build_data.py)', () => {
  const zlib = require('node:zlib');
  const GZ = ['www/index.html', 'www/app.js', 'doc.html', 'add.json', 'add.html', 'openapi.json'];
  for (const f of GZ) {
    const raw = fs.readFileSync(path.join(ROOT, 'data', f));
    const gz = fs.readFileSync(path.join(ROOT, 'data', f + '.gz'));
    assert.ok(zlib.gunzipSync(gz).equals(raw), f + '.gz is out of date – run: python3 scripts/build_data.py');
    assert.ok(gz.length < raw.length / 2, f);
    assert.ok(('/' + f + '.gz').length <= 31, f + ': SPIFFS name length');
  }
  // only these: every .gz in data/ belongs to the list (a forgotten old copy would be served)
  const all = [];
  const walk = (d) => { for (const e of fs.readdirSync(d, { withFileTypes: true })) {
    if (e.isDirectory()) walk(path.join(d, e.name)); else if (e.name.endsWith('.gz')) all.push(path.relative(path.join(ROOT, 'data'), path.join(d, e.name)).split(path.sep).join('/'));
  } };
  walk(path.join(ROOT, 'data'));
  assert.deepEqual(all.sort(), GZ.map((f) => f + '.gz').sort());
});
