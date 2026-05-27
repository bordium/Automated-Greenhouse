#include "StateMachine.h"
#include "Config.h"

const char* stateName(GhState s) {
    switch (s) {
        case GhState::IDLE:    return "IDLE";
        case GhState::PUMPING: return "PUMPING";
        case GhState::HEATING: return "HEATING";
        case GhState::VENTING: return "VENTING";
    }
    return "UNKNOWN";
}

StateMachine::StateMachine(Actuators& actuators)
: _act(actuators) {
    _stateEnteredAt = millis();
}

void StateMachine::_enter(GhState s) {
    if (s == _state) return;
    _state = s;
    _stateEnteredAt = millis();
    if (s == GhState::PUMPING) _pumpStartedAt = _stateEnteredAt;
    Serial.print("[FSM] -> ");
    Serial.println(stateName(s));
    _applyOutputs();
}

void StateMachine::_applyOutputs() {
    switch (_state) {
        case GhState::IDLE:
            _act.pumpOff();
            _act.fanOff();
            _act.heatersOff();
            break;
        case GhState::PUMPING:
            _act.pumpOn();
            _act.fanOff();
            _act.heatersOff();
            break;
        case GhState::HEATING:
            _act.pumpOff();
            _act.fanOff();
            _act.heatersOn();
            break;
        case GhState::VENTING:
            _act.pumpOff();
            _act.fanOn();
            _act.heatersOff();
            break;
    }
}

void StateMachine::tick(const SensorReading& r) {
    uint32_t now = millis();

    // PUMPING is time-limited so we don't flood the pot
    if (_state == GhState::PUMPING && (now - _pumpStartedAt) >= PUMP_RUN_MS) {
        _enter(GhState::IDLE);
        return;
    }

    // Hysteresis: hold each state at least MIN_STATE_HOLD_MS before re-evaluating
    if ((now - _stateEnteredAt) < MIN_STATE_HOLD_MS) return;

    // Priority: pump dry soil -> cool overheating -> warm cold air -> idle
    if (r.soilDry && _state != GhState::PUMPING) {
        _enter(GhState::PUMPING);
        return;
    }

    if (r.shtOk) {
        bool tooHot   = r.airTempC    >= TEMP_MAX_C;
        bool tooHumid = r.airHumidPct >= HUMIDITY_MAX_PCT;
        bool tooCold  = r.airTempC    <= TEMP_MIN_C;

        if (tooHot || tooHumid) { _enter(GhState::VENTING); return; }
        if (tooCold)            { _enter(GhState::HEATING); return; }
    }

    if (_state != GhState::IDLE) _enter(GhState::IDLE);
}
