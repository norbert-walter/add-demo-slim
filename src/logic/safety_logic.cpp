/**
 * @file safety_logic.cpp
 * @brief Pure safety state machine (CONTRACT §6, V1.5). Sprint S2.1.
 */
#include "safety_logic.h"

// ---------------------------------------------------------------------------
// Helper: initial status
// ---------------------------------------------------------------------------
SafetyStatus safetyInitialStatus() {
    SafetyStatus s{};
    s.state = SafetyState::SAFE_START;
    return s;
}

// ---------------------------------------------------------------------------
// Helper: HV conditions check (CONTRACT §6.5)
// ---------------------------------------------------------------------------
bool safetyHvConditionsOk(const SafetyInputs& in, const SafetyConfig& cfg) {
    return in.cooling_flow && !in.estop && in.interlock && (in.temp_c <= cfg.temp_warn_c);
}

// ---------------------------------------------------------------------------
// Helper: active fault cause (priority: ESTOP > TEMP > MOTOR > COMM)
// ---------------------------------------------------------------------------
ErrorCode safetyFaultCause(const SafetyInputs& in, const SafetyConfig& cfg) {
    if (in.estop)             return ErrorCode::ESTOP;
    if (in.temp_c > cfg.temp_fault_c)  return ErrorCode::TEMP_HIGH;
    if (in.motor_fault)       return ErrorCode::MOTOR_FAULT;
    if (in.comm_timeout)      return ErrorCode::COMM_TIMEOUT;
    return ErrorCode::NONE;
}

// ---------------------------------------------------------------------------
// safetyStep — main state machine (R1–R7)
// ---------------------------------------------------------------------------
SafetyResult safetyStep(const SafetyStatus& cur, SafetyEvent ev, const SafetyInputs& in, const SafetyConfig& cfg) {
    SafetyStatus n = cur;  // copy current state

    // ===== TICK in FAULT → WAIT_ACK — highest priority override =====
    if (ev == SafetyEvent::TICK && cur.state == SafetyState::FAULT) {
        n.state = SafetyState::WAIT_ACK;  // keep same err
        n.hv_on       = false;
        n.shutter_open    = false;
        n.shutter_auto    = false;

        SafetyResult r{};
        r.next   = n;
        r.motor_stop = true;  // R2: motor_stop on every event in FAULT/WAIT_ACK
        r.api    = ApiError::OK;
        return r;
    }

    // ===== INIT_FAILED: always faults — preserves existing fault cause from inputs =====
    // Per API table: if already in a fault state (FAULT/WAIT_ACK), stay there; otherwise enter FAULT.
    if (ev == SafetyEvent::INIT_FAILED) {
        if (cur.state == SafetyState::FAULT || cur.state == SafetyState::WAIT_ACK) {
            // Already faulted → stay, return STATE (not a recognized command).
            n.hv_on       = false;
            n.shutter_open    = false;
            n.shutter_auto    = false;
            SafetyResult r{};
            r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
            return r;
        }
        ErrorCode initErr = safetyFaultCause(in, cfg);
        if (initErr == ErrorCode::NONE) {
            initErr = ErrorCode::HOMING_FAILED;
        }
        n.state = SafetyState::FAULT;
        n.err   = initErr;
        n.hv_on       = false;
        n.shutter_open    = false;
        n.shutter_auto    = false;

        SafetyResult r{};
        r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
        return r;
    }

    // ===== INIT_DONE from RUNNING: bypass fault detection (V1.5.5) =====
    if (ev == SafetyEvent::INIT_DONE && cur.state == SafetyState::RUNNING) {
        // Active fault → R1 applies, enter FAULT.
        ErrorCode faultCause = safetyFaultCause(in, cfg);
        if (faultCause != ErrorCode::NONE) {
            n.state = SafetyState::FAULT;
            n.err   = faultCause;
            n.hv_on       = false;
            n.shutter_open    = false;
            n.shutter_auto    = false;
            SafetyResult r{};
            r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
            return r;
        }
        // No active fault — if conditions are lost, transition to READY (not FAULT).
        if (!safetyHvConditionsOk(in, cfg)) {
            n.state = SafetyState::READY;
            n.hv_on       = false;
            n.shutter_open    = false;
            n.shutter_auto    = false;
            SafetyResult r{};
            r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
            return r;
        }
        // Conditions ok — apply closing rules (pos-based shutter close).
        if (n.shutter_open && n.shutter_auto && in.pos_mm <= cfg.maint_max_mm) {
            n.shutter_open    = false;
            n.shutter_auto    = false;
        } else if (n.shutter_open && in.pos_mm <= cfg.park_pos_mm) {
            n.shutter_open    = false;
        }
        SafetyResult r{};
        r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
        return r;
    }

    // ===== ACK from WAIT_ACK: check if fault has cleared (before R1) =====
    if (ev == SafetyEvent::ACK && cur.state == SafetyState::WAIT_ACK) {
        ErrorCode cause = safetyFaultCause(in, cfg);
        if (cause == ErrorCode::NONE) {
            n.state = SafetyState::RECOVER;
            n.err   = ErrorCode::NONE;
            n.hv_on       = false;
            n.shutter_open    = false;
            n.shutter_auto    = false;
            SafetyResult r{};
            r.next   = n; r.motor_stop = true; r.api = ApiError::OK;
            return r;
        } else {
            // Fault still present → reject, stay in WAIT_ACK.
            SafetyResult r{};
            r.next   = n; r.motor_stop = true; r.api = ApiError::PRECONDITION;
            return r;
        }
    }

    // ===== R1: fault entry — applies to EVERY event without exception =====
    // Does not re-trigger from FAULT or WAIT_ACK (already fault-related).
    ErrorCode faultCause = safetyFaultCause(in, cfg);
    if (faultCause != ErrorCode::NONE &&
        cur.state != SafetyState::FAULT &&
        cur.state != SafetyState::WAIT_ACK) {
        n.state = SafetyState::FAULT;
        n.err   = faultCause;
        n.hv_on       = false;
        n.shutter_open    = false;
        n.shutter_auto    = false;

        SafetyResult r{};
        r.next   = n; r.motor_stop = true;
        r.api    = (ev == SafetyEvent::TICK || ev == SafetyEvent::HV_OFF ||
                    ev == SafetyEvent::SHUTTER_CLOSE) ? ApiError::OK : ApiError::STATE;
        return r;
    }

    // ===== INIT_DONE from non-RUNNING states — after fault detection =====
    if (ev == SafetyEvent::INIT_DONE) {
        switch (cur.state) {
            case SafetyState::SAFE_START:
            case SafetyState::READY:
                n.state = SafetyState::READY;
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                break;
            case SafetyState::FAULT:
            case SafetyState::WAIT_ACK: {
                // INIT_DONE rejected in fault states → stay in WAIT_ACK.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
                return r;
            }
            case SafetyState::RECOVER: {
                // INIT_DONE rejected in RECOVER → stay, return STATE.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
        }

        SafetyResult r{};
        r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
        return r;
    }

    // ===== State-specific handling =====
    switch (cur.state) {

        // ---- R2: FAULT -------------------------------------------------------
        case SafetyState::FAULT: {
            // TICK is handled at top-level (transition to WAIT_ACK).
            if (ev == SafetyEvent::HV_OFF || ev == SafetyEvent::SHUTTER_CLOSE) {
                // Benign events in FAULT — stay in FAULT, motor stays stopped.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::OK;
                return r;
            }
            // All other events → stay in FAULT, STATE.
            n.hv_on       = false;
            n.shutter_open    = false;
            n.shutter_auto    = false;
            {
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
                return r;
            }
        }

        // ---- R3: WAIT_ACK ------------------------------------------------------
        case SafetyState::WAIT_ACK: {
            // ACK is handled at top-level (before R1 fault entry).

            // R3: non-ACK events → stay in WAIT_ACK regardless of fault state.
            // Only ACK checks whether fault has cleared.
            if (ev == SafetyEvent::HV_ON) {
                // HV_ON is always rejected in WAIT_ACK.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
                return r;
            }
            // All other non-ACK events → stay in WAIT_ACK.
            if (ev == SafetyEvent::TICK || ev == SafetyEvent::HV_OFF || ev == SafetyEvent::SHUTTER_CLOSE) {
                // Benign events → OK, stay in WAIT_ACK.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::OK;
                return r;
            }
            // All remaining events → STATE.
            {
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::STATE;
                return r;
            }
        }

        // ---- R4: RECOVER -------------------------------------------------------
        case SafetyState::RECOVER: {
            if (ev == SafetyEvent::TICK) {
                n.state = in.homed ? SafetyState::READY : SafetyState::SAFE_START;
                n.err   = ErrorCode::NONE;
                // Outputs forced off when leaving RECOVER.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::OK;
                return r;
            }
            // Non-TICK events in RECOVER → stay in RECOVER.
            // "Outputs forced off outside RUNNING" — clear all outputs.
            if (ev == SafetyEvent::HV_OFF || ev == SafetyEvent::SHUTTER_CLOSE) {
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = true; r.api = ApiError::OK;
                return r;
            }
            {
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
        }

        // ---- R5: SAFE_START ----------------------------------------------------
        case SafetyState::SAFE_START: {
            // INIT_DONE is handled above (unconditional → READY).
            if (ev == SafetyEvent::TICK || ev == SafetyEvent::HV_OFF || ev == SafetyEvent::SHUTTER_CLOSE) {
                // Outputs forced off outside RUNNING.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                return r;
            }
            // Non-TICK events in SAFE_START → stay in SAFE_START.
            // "Outputs forced off outside RUNNING" — clear all outputs.
            n.hv_on       = false;
            n.shutter_open    = false;
            n.shutter_auto    = false;
            {
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
        }

        // ---- R6: READY ---------------------------------------------------------
        case SafetyState::READY: {
            if (ev == SafetyEvent::HV_ON) {
                if (safetyHvConditionsOk(in, cfg)) {
                    n.state = SafetyState::RUNNING;
                    n.hv_on       = true;
                    SafetyResult r{};
                    r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                    return r;
                } else {
                    // Conditions not met → stay READY, PRECONDITION. Outputs forced off.
                    n.hv_on       = false;
                    n.shutter_open    = false;
                    n.shutter_auto    = false;
                    SafetyResult r{};
                    r.next   = n; r.motor_stop = false; r.api = ApiError::PRECONDITION;
                    return r;
                }
            }
            if (ev == SafetyEvent::INIT_DONE || ev == SafetyEvent::TICK || ev == SafetyEvent::HV_OFF || ev == SafetyEvent::SHUTTER_CLOSE) {
                // Stay in READY — outputs forced off outside RUNNING.
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                return r;
            }
            // SHUTTER_OPEN, ACK → STATE (not allowed in READY).
            if (ev == SafetyEvent::SHUTTER_OPEN || ev == SafetyEvent::ACK) {
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
            // SHUTTER_AUTO_OPEN → STATE in READY (not allowed outside RUNNING).
            if (ev == SafetyEvent::SHUTTER_AUTO_OPEN) {
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
            // Unhandled events in READY → STATE.
            {
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
        }

        // ---- R7: RUNNING -------------------------------------------------------
        case SafetyState::RUNNING: {
            // --- R7a: condition loss → drop to READY ---------------------------
            if (!safetyHvConditionsOk(in, cfg)) {
                n.state = SafetyState::READY;
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;

                ApiError api;
                switch (ev) {
                    case SafetyEvent::TICK:
                    case SafetyEvent::HV_OFF:
                    case SafetyEvent::SHUTTER_CLOSE:
                        api = ApiError::OK;
                        break;
                    case SafetyEvent::HV_ON:
                    case SafetyEvent::SHUTTER_OPEN:
                    case SafetyEvent::SHUTTER_AUTO_OPEN:
                        api = ApiError::PRECONDITION;
                        break;
                    case SafetyEvent::ACK:
                    default:
                        api = ApiError::STATE;
                        break;
                }

                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = api;
                return r;
            }

            // --- R7b: position-based shutter management ---------------------
            // V1.5: automatic shutter closes when entering park/service zone,
            // even for non-TICK events — the position check is state-based.
            if (n.shutter_auto && in.pos_mm <= cfg.park_pos_mm) {
                n.shutter_open    = false;
                n.shutter_auto    = false;
            }
            if (n.shutter_auto && in.pos_mm > cfg.park_pos_mm && in.pos_mm <= cfg.maint_max_mm) {
                n.shutter_open    = false;
                n.shutter_auto    = false;
            }
            // V1.5: manually opened shutter closes when entering park zone.
            if (!n.shutter_auto && n.shutter_open && in.pos_mm <= cfg.park_pos_mm) {
                n.shutter_open    = false;
            }

            // --- R7c: command-specific handling (conditions OK) -------------
            if (ev == SafetyEvent::HV_ON) {
                // Idempotent: stay RUNNING, keep hv_on.
                n.hv_on       = true;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                return r;
            }

            if (ev == SafetyEvent::HV_OFF) {
                // Drop to READY, everything off.
                n.state = SafetyState::READY;
                n.hv_on       = false;
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                return r;
            }

            if (ev == SafetyEvent::SHUTTER_OPEN) {
                // Manual open: only in service zone (park_pos < pos <= maint_max).
                if (in.pos_mm > cfg.park_pos_mm && in.pos_mm <= cfg.maint_max_mm) {
                    n.shutter_open    = true;
                    n.shutter_auto    = false;
                    SafetyResult r{};
                    r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                    return r;
                }
                // Outside service zone → PRECONDITION.
                {
                    SafetyResult r{};
                    r.next   = n; r.motor_stop = false; r.api = ApiError::PRECONDITION;
                    return r;
                }
            }

            if (ev == SafetyEvent::SHUTTER_AUTO_OPEN) {
                // Auto open: only in measuring zone (pos > maint_max).
                if (in.pos_mm > cfg.maint_max_mm) {
                    n.shutter_open    = true;
                    n.shutter_auto    = true;
                    SafetyResult r{};
                    r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                    return r;
                }
                // Outside measuring zone → PRECONDITION.
                {
                    SafetyResult r{};
                    r.next   = n; r.motor_stop = false; r.api = ApiError::PRECONDITION;
                    return r;
                }
            }

            if (ev == SafetyEvent::SHUTTER_CLOSE) {
                // Always allowed, close shutter and clear auto flag.
                n.shutter_open    = false;
                n.shutter_auto    = false;
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                return r;
            }

            if (ev == SafetyEvent::TICK) {
                // TICK is already handled above (pos checks ran before command dispatch).
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::OK;
                return r;
            }

            // ACK and any other events → STATE.
            {
                SafetyResult r{};
                r.next   = n; r.motor_stop = false; r.api = ApiError::STATE;
                return r;
            }
        }

    }  // switch(cur.state)

    // Should never reach here — all states are covered above.
    SafetyResult r{};
    r.next   = cur; r.motor_stop = false; r.api = ApiError::STATE;
    return r;
}


// ---------------------------------------------------------------------------
// safetyCmdAllowed — command/state matrix (CONTRACT §6.3)
// ---------------------------------------------------------------------------
ApiError safetyCmdAllowed(SafetyState state, CmdId cmd) {
    switch (cmd) {
        case CmdId::GET_STATE:
        case CmdId::GET_CONFIG:
            // Allowed in every state.
            return ApiError::OK;

        case CmdId::STOP:
        case CmdId::SET:
        case CmdId::HV_OFF:
        case CmdId::SHUTTER_CLOSE:
            // Allowed in every state.
            return ApiError::OK;

        case CmdId::INIT:
            return (state == SafetyState::SAFE_START ||
                    state == SafetyState::READY) ? ApiError::OK : ApiError::STATE;

        case CmdId::MOVE:
        case CmdId::PARK:
        case CmdId::SERVICE:
        case CmdId::HV_ON:
            return (state == SafetyState::READY ||
                    state == SafetyState::RUNNING) ? ApiError::OK : ApiError::STATE;

        case CmdId::START_MEASURE:
        case CmdId::START_TRAVERSE:
        case CmdId::SHUTTER_OPEN:
            return (state == SafetyState::RUNNING) ? ApiError::OK : ApiError::STATE;

        case CmdId::CALIB:
            return (state == SafetyState::SAFE_START ||
                    state == SafetyState::READY ||
                    state == SafetyState::RUNNING) ? ApiError::OK : ApiError::STATE;

        case CmdId::ACK:
            return (state == SafetyState::WAIT_ACK) ? ApiError::OK : ApiError::STATE;

        case CmdId::REBOOT:
            return (state == SafetyState::SAFE_START ||
                    state == SafetyState::READY ||
                    state == SafetyState::WAIT_ACK) ? ApiError::OK : ApiError::STATE;

        case CmdId::NONE:
        default:
            return ApiError::STATE;
    }
}

// ---------------------------------------------------------------------------
// safetyShutterAutomation — V1.5 automated shutter decisions (CONTRACT §6.9)
// ---------------------------------------------------------------------------
SafetyEvent safetyShutterAutomation(bool acquiring, bool program_active, uint32_t idle_ms,
                                    uint32_t hold_ms, const SafetyStatus& s, float pos_mm,
                                    const SafetyConfig& cfg) {
    // 1) Auto-close: not acquiring & not active & holding long enough → close.
    if (s.shutter_open && s.shutter_auto && !program_active &&
        idle_ms >= hold_ms && s.state == SafetyState::RUNNING) {
        return SafetyEvent::SHUTTER_CLOSE;
    }

    // 2) Auto-open: acquiring & RUNNING & measuring zone & not already open+auto.
    if (acquiring && s.state == SafetyState::RUNNING && pos_mm > cfg.maint_max_mm &&
        (!s.shutter_open || !s.shutter_auto)) {
        return SafetyEvent::SHUTTER_AUTO_OPEN;
    }

    // 3) No action needed.
    return SafetyEvent::TICK;
}

// ---------------------------------------------------------------------------
// safetyCommTimeout — communication watchdog (CONTRACT §6.8, wrap-around safe)
// ---------------------------------------------------------------------------
bool safetyCommTimeout(bool armed, uint32_t now_ms, uint32_t last_rx_ms, uint32_t timeout_ms) {
    if (!armed) return false;
    return (now_ms - last_rx_ms) > timeout_ms;
}

