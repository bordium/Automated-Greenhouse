#include "StateMachine.h"
#include "Config.h"

const char* modeName(GhMode m) {
    return (m == GhMode::AUTO) ? "AUTO" : "MANUAL";
}

String actionsLabel(ActionMask m) {
    if (m == 0) return String("IDLE");
    String s;
    if (m & ACT_PUMPING) { if (s.length()) s += "+"; s += "PUMPING"; }
    if (m & ACT_HEATING) { if (s.length()) s += "+"; s += "HEATING"; }
    if (m & ACT_VENTING) { if (s.length()) s += "+"; s += "VENTING"; }
    return s;
}

String actionsShortLabel(ActionMask m) {
    if (m == 0) return String("IDLE");
    String s;
    if (m & ACT_PUMPING) { if (s.length()) s += "+"; s += "PUMP"; }
    if (m & ACT_HEATING) { if (s.length()) s += "+"; s += "HEAT"; }
    if (m & ACT_VENTING) { if (s.length()) s += "+"; s += "VENT"; }
    return s;
}

StateMachine::StateMachine(Actuators& actuators)
: _act(actuators) {
    _targets.airTempC   = DEFAULT_TARGET_AIR_TEMP_C;
    _targets.waterTempC = DEFAULT_TARGET_WATER_TEMP_C;
    _targets.humidity   = DEFAULT_TARGET_HUMIDITY;
    _targets.soilBin    = DEFAULT_TARGET_SOIL_BIN;
}

void StateMachine::setMode(GhMode m) {
    if (m == _mode) return;
    _mode = m;
    Serial.print("[FSM] mode -> ");
    Serial.println(modeName(_mode));

    // Whichever direction we move, park outputs so the new mode starts clean.
    _actions = 0;
    if (_mode == GhMode::MANUAL) {
        _act.pumpOff();
        _act.fanOff();
        _act.heatersOff();
        // LED stays — it's user-controlled in both modes.
    } else {
        _applyOutputs();
    }
}

float StateMachine::_soilTargetPct() const {
    int b = _targets.soilBin;
    if (b < 1) b = 1;
    if (b > 3) b = 3;
    return SOIL_BIN_PCT[b];
}

// ---- per-action control loops ----

void StateMachine::_updatePumping(const SensorReading& r, uint32_t now) {
    bool pumping = (_actions & ACT_PUMPING) != 0;

    if (pumping) {
        // Stop after PUMP_RUN_MS and start a cooldown.
        if (now - _pumpStartedAt >= PUMP_RUN_MS) {
            _actions &= ~ACT_PUMPING;
            _pumpCooldownUntil = now + PUMP_COOLDOWN_MS;
        }
        return;
    }

    if (now < _pumpCooldownUntil) return;
    if (!r.soilOk || isnan(r.soilPct)) return;

    float targetPct = _soilTargetPct();
    if (r.soilPct < (targetPct - SOIL_DEADBAND_PCT)) {
        _actions |= ACT_PUMPING;
        _pumpStartedAt = now;
    }
}

void StateMachine::_updateHeating(const SensorReading& r) {
    if (!r.shtOk) return;
    bool heating = (_actions & ACT_HEATING) != 0;

    bool tooCold     = r.airTempC <= (_targets.airTempC - TEMP_DEADBAND_C);
    bool warmEnough  = r.airTempC >= _targets.airTempC;

    if (!heating && tooCold)        _actions |= ACT_HEATING;
    else if (heating && warmEnough) _actions &= ~ACT_HEATING;
}

void StateMachine::_updateVenting(const SensorReading& r) {
    if (!r.shtOk) return;
    bool venting = (_actions & ACT_VENTING) != 0;

    bool tooHot   = r.airTempC    >= (_targets.airTempC + TEMP_DEADBAND_C);
    bool tooHumid = r.airHumidPct >= (_targets.humidity + HUMID_DEADBAND_PCT);
    bool fine     = r.airTempC    <= _targets.airTempC
                 && r.airHumidPct <= _targets.humidity;

    if (!venting && (tooHot || tooHumid)) _actions |= ACT_VENTING;
    else if (venting && fine)             _actions &= ~ACT_VENTING;
}

void StateMachine::_applyOutputs() {
    if (_actions & ACT_PUMPING) _act.pumpOn();    else _act.pumpOff();
    if (_actions & ACT_VENTING) _act.fanOn();     else _act.fanOff();
    if (_actions & ACT_HEATING) _act.heatersOn(); else _act.heatersOff();
}

void StateMachine::tick(const SensorReading& r) {
    if (_mode == GhMode::MANUAL) return;

    ActionMask before = _actions;
    uint32_t   now    = millis();

    _updatePumping(r, now);
    _updateHeating(r);
    _updateVenting(r);

    if (_actions != before) {
        Serial.print("[FSM] actions -> ");
        Serial.println(actionsLabel(_actions));
    }

    _applyOutputs();
}
