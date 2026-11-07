// logic_sim.cpp – runs the REAL device logic (src/logic, the same code as on the ESP32) on a PC,
// with a simulated axis (test/native/fake_axis.h) and the demo stair body as distance sensor.
// Driven line by line over stdin/stdout by scripts/sim_device.py (HTTP wrapper with the
// endpoints of the device). Not part of the firmware. Build: see scripts/sim_device.py.
//
// Protocol (one request line -> one JSON reply line):
//   T <ms>          advance the simulated time to <ms> (1 ms control cycles)
//   P               -> {"proc":"<framed /proc text>"}
//   A <body>        POST /api -> {"code":200|400,"body":"<framed reply>"}
//   H <name> <0|1>  hooks: estop, press_fwd, press_bwd (hold a limit switch), block (axis stuck)
// Every reply also carries "log":[...] (new log lines) and "ts" (ms since the last boot).
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include "logic/app_logic.h"
#include "logic/api_parse.h"
#include "logic/proc_json.h"
#include "logic/params.h"
#include "../../test/native/fake_axis.h"

static AppConfig C;
static AppState  S;
static FakeAxis  A;
static uint32_t  now = 0, boot_ms = 0, last_rx = 0;
static bool      estop = false, press_fwd = false, press_bwd = false;
static Params    nvs;                  // "flash"
static bool      persist_pending = false, deferred_logged = false;
static long      reboot_at = -1;
static std::vector<std::string> logs;
static std::vector<Command> queue;     // normal commands from the API (FIFO), executed one per cycle
static CycleStats cs;                  // timing is not simulated: 0

static void logLine(const std::string& s) { logs.push_back(s); }

// demo geometry (V1.4.2): stair 105..225 mm, steps centred at 115 ... 215 mm, distances 65 ... 40 mm;
// beside the material the sensor sees the lower boom (about 186 mm, smoketest 0a)
static float distAt(float axis_mm) {
    if (axis_mm < 105.0f || axis_mm >= 225.0f) return 186.0f;
    return 65.0f - 5.0f * (float)(int)((axis_mm - 105.0f) / 20.0f);
}
static float mvForDist(float d) { return 1000.0f * powf(d / C.sensor.k_mm, 1.0f / C.sensor.exponent); }
static float axisMm() { return (float)(A.phys_steps / C.motion.steps_per_mm) - 2.0f; }

static HwInputs hw() {
    HwInputs h;
    h.now_ms = now; h.estop = estop;
    h.limit_fwd = A.limitFwd() || press_fwd; h.limit_bwd = A.limitBwd() || press_bwd;
    h.drv_running = A.running; h.drv_pos_steps = A.drvPos();
    h.drv_speed_mm_s = A.running ? (float)(A.speed_hz / C.motion.steps_per_mm) : 0.0f;
    h.last_api_rx_ms = last_rx;
    return h;
}

static void boot() {
    C = AppConfig{}; C.local_hold_ms = 0;          // LOCAL_HOLD_MS = 0 as in config.h
    appInit(S); S.params = nvs;
    A.zero_offset = (int32_t)lround(A.phys_steps); // the driver counter starts at 0
    A.running = false; A.run_backward = false;
    boot_ms = now; queue.clear(); persist_pending = false; deferred_logged = false; reboot_at = -1;
    logLine("boot: ready, FW sim");
}

// pending results of API commands executed in a cycle
struct Pending { Command c; bool done = false; ApiError r = ApiError::OK; };
static std::vector<Pending*> waiting;

static void cycle() {
    now++;
    A.step(1);
    if (now % 20 == 0) appMeasure(S, C, mvForDist(distAt(axisMm())), now);
    HwOutputs out = appTick(S, hw(), C);
    // priority commands first (all), then at most one normal command if no driver request
    for (auto* p : waiting) if (!p->done && appCmdIsPriority(p->c.id)) { p->r = appCommand(S, p->c, hw(), C, out); p->done = true; }
    if (out.drv.op == DrvOp::NONE) {
        for (auto* p : waiting) if (!p->done) { p->r = appCommand(S, p->c, hw(), C, out); p->done = true; break; }
    }
    A.apply(out.drv);
    if (out.persist) persist_pending = true;
    if (out.reboot && reboot_at < 0) {
        reboot_at = (long)now + 700; logLine("reboot: in 700 ms");
    }
    // network task: deferred NVS save (CONTRACT 1.5.25)
    bool idle = S.seq.phase == SeqPhase::IDLE && !A.running && !S.safety.hv_on;
    if (persist_pending) {
        if (idle || reboot_at >= 0) { nvs = S.params; persist_pending = false; deferred_logged = false; logLine("nvs: saved"); }
        else if (!deferred_logged) { deferred_logged = true; logLine("nvs: save deferred until axis idle and HV off"); }
    }
    if (reboot_at >= 0 && (long)now >= reboot_at) boot();
}

static void advance(uint32_t t) { while (now < t) cycle(); }

static std::string esc(const char* s) {
    std::string o;
    for (; *s; ++s) { if (*s == '"' || *s == '\\') o += '\\'; o += *s; }
    return o;
}
static void reply(const std::string& members) {
    std::string l = "[";
    for (size_t i = 0; i < logs.size(); ++i) l += (i ? ",\"" : "\"") + esc(logs[i].c_str()) + "\"";
    l += "]"; logs.clear();
    printf("{%s%s\"log\":%s,\"ts\":%u}\n", members.c_str(), members.empty() ? "" : ",", l.c_str(), now - boot_ms);
    fflush(stdout);
}

static void api(const char* body) {
    Command c;
    char buf[1024], extra[600];                // as on the device (http_server.cpp)
    ApiError e = apiParseRequest(body, strlen(body), c);
    if (e == ApiError::OK || e == ApiError::BAD_PARAM) last_rx = now;   // valid packet (CRC ok, known cmd)
    int code = (e == ApiError::BAD_CRC || e == ApiError::BAD_JSON) ? 400 : 200;
    if (e == ApiError::OK && c.id == CmdId::GET_STATE) {
        ProcData d; appFillProc(S, C, hw(), d);
        stateExtraToJson(S.safety.state, S.safety.err, S.local, cs, extra, sizeof extra);
        responseToJson(c.id, e, nullptr, extra, buf, sizeof buf);
    } else if (e == ApiError::OK && c.id == CmdId::GET_CONFIG) {
        paramsToJsonMember(S.params, S.flags, extra, sizeof extra);
        responseToJson(c.id, e, nullptr, extra, buf, sizeof buf);
    } else if (e == ApiError::OK) {
        c.source = CmdSource::API; c.want_reply = true;
        Pending p; p.c = c; waiting.push_back(&p);
        for (int i = 0; i < 100 && !p.done; ++i) cycle();       // the device answers BUSY after 100 ms
        waiting.clear();
        if (p.done) last_rx = now;
        responseToJson(c.id, p.done ? p.r : ApiError::BUSY, nullptr, nullptr, buf, sizeof buf);
    } else {
        responseToJson(c.id, e, nullptr, nullptr, buf, sizeof buf);
    }
    std::string b = buf;
    while (!b.empty() && (b.back() == '\n' || b.back() == '\r')) b.pop_back();
    reply("\"code\":" + std::to_string(code) + ",\"body\":\"" + esc(b.c_str()) + "\"");
}

int main() {
    A.phys_steps = 32.0 * C.motion.steps_per_mm;    // carriage in the service zone (short homing)
    nvs = Params{};
    now = 1000; boot();
    static char line[2048];
    while (fgets(line, sizeof line, stdin)) {
        size_t n = strlen(line);
        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
        if (line[0] == 'T') { advance((uint32_t)strtoul(line + 2, nullptr, 10)); reply(""); }
        else if (line[0] == 'P') {
            ProcData d; appFillProc(S, C, hw(), d);
            d.ts_ms = now - boot_ms;
            char buf[PROC_JSON_MAX]; procToJson(d, buf, sizeof buf);
            reply("\"proc\":\"" + esc(buf) + "\"");
        }
        else if (line[0] == 'A') api(line + 2);
        else if (line[0] == 'H') {
            char name[32] = {0}; int v = 0;
            sscanf(line + 2, "%31s %d", name, &v);
            if (!strcmp(name, "estop")) estop = v;
            else if (!strcmp(name, "press_fwd")) press_fwd = v;
            else if (!strcmp(name, "press_bwd")) press_bwd = v;
            else if (!strcmp(name, "block")) A.block = v;
            reply("");
        }
        else reply("\"error\":\"unknown request\"");
    }
    return 0;
}
