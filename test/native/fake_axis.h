// Shared test helper (FROZEN): simulated stepper driver + limit switches.
// Physical coordinate: phys_steps. Backward switch active at phys <= 0,
// forward switch active at phys >= fwd_limit_steps. The driver counter is
// phys_steps - zero_offset (ZERO_HERE moves the offset).
#pragma once
#include <stdint.h>
#include <math.h>
#include "logic/motion_seq.h"

struct FakeAxis {
    double   phys_steps      = 0;
    int32_t  zero_offset     = 0;
    bool     running         = false;
    bool     run_backward    = false;
    int32_t  target          = 0;      // driver coordinates
    double   speed_hz        = 0;
    int32_t  fwd_limit_steps = 19334;  // V1.4: measured 0a: phys 246.3 mm = axis 244.3 mm (axis 0 = phys 2 mm)
    bool     block           = false;  // simulates a blocked axis (no motion)
    int      force_stops     = 0;

    int32_t drvPos() const { return (int32_t)lround(phys_steps) - zero_offset; }
    bool limitBwd() const { return phys_steps <= 0.0; }
    bool limitFwd() const { return phys_steps >= fwd_limit_steps; }

    void apply(const DrvRequest& r) {
        switch (r.op) {
            case DrvOp::NONE: break;
            case DrvOp::MOVE_TO: target = r.target_steps; speed_hz = r.speed_hz; run_backward = false;
                running = (drvPos() != target); break;
            case DrvOp::RUN_BACKWARD: run_backward = true; speed_hz = r.speed_hz; running = true; break;
            case DrvOp::STOP: case DrvOp::FORCE_STOP:
                if (r.op == DrvOp::FORCE_STOP) force_stops++;
                running = false; run_backward = false; break;
            case DrvOp::ZERO_HERE: running = false; run_backward = false;
                zero_offset = (int32_t)lround(phys_steps); break;
        }
    }
    // advance by dt_ms
    void step(uint32_t dt_ms) {
        if (!running || block) return;
        double d = speed_hz * dt_ms / 1000.0;
        if (run_backward) { phys_steps -= d; if (phys_steps < -2000) phys_steps = -2000; return; }
        int32_t pos = drvPos();
        if (pos < target) { phys_steps += d; if (drvPos() >= target) { phys_steps = target + zero_offset; running = false; } }
        else if (pos > target) { phys_steps -= d; if (drvPos() <= target) { phys_steps = target + zero_offset; running = false; } }
        else running = false;
    }
    MotionInputs inputs(uint32_t now) const {
        MotionInputs m; m.now_ms = now; m.limit_bwd = limitBwd(); m.limit_fwd = limitFwd();
        m.drv_running = running; m.drv_pos_steps = drvPos(); return m;
    }
};
