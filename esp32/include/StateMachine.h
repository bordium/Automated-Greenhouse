#ifndef GREENHOUSE_STATEMACHINE_H
#define GREENHOUSE_STATEMACHINE_H

#include <Arduino.h>
#include "Sensors.h"
#include "Actuators.h"

// Each action is an independent bit. The controller can have any combination
// of them active at once (e.g. HEATING + VENTING to warm air while exhausting
// humidity). Mask == 0 is IDLE.
enum ActionFlag : uint8_t {
    ACT_PUMPING = 1 << 0,
    ACT_HEATING = 1 << 1,
    ACT_VENTING = 1 << 2,
};
using ActionMask = uint8_t;

enum class GhMode : uint8_t {
    AUTO = 0,
    MANUAL
};

struct Targets {
    float airTempC   = 16.0f;
    float waterTempC = 13.0f;
    float humidity   = 65.0f;
    int   soilBin    = 3;          // 1, 2, or 3
};

const char* modeName(GhMode m);

/** "IDLE" or "PUMPING+HEATING" etc. */
String actionsLabel(ActionMask m);
/** Compact "IDLE" / "PUMP+HEAT+VENT" — fits the 240px LCD. */
String actionsShortLabel(ActionMask m);

class StateMachine {
public:
    StateMachine(Actuators& actuators);

    void tick(const SensorReading& r);

    ActionMask  actions() const { return _actions; }
    String      actionsString() const { return actionsLabel(_actions); }
    GhMode      mode() const { return _mode; }
    const char* modeLabel() const { return modeName(_mode); }
    const Targets& targets() const { return _targets; }

    void setMode(GhMode m);
    void setTargets(const Targets& t) { _targets = t; }
    void patchTarget_airTemp(float v)   { _targets.airTempC   = v; }
    void patchTarget_waterTemp(float v) { _targets.waterTempC = v; }
    void patchTarget_humidity(float v)  { _targets.humidity   = v; }
    void patchTarget_soilBin(int b)     {
        if (b >= 1 && b <= 3) _targets.soilBin = b;
    }

private:
    Actuators& _act;
    ActionMask _actions          = 0;
    GhMode     _mode             = GhMode::MANUAL;
    Targets    _targets;
    uint32_t   _pumpStartedAt    = 0;
    uint32_t   _pumpCooldownUntil = 0;

    void _updatePumping(const SensorReading& r, uint32_t now);
    void _updateHeating(const SensorReading& r);
    void _updateVenting(const SensorReading& r);
    void _applyOutputs();
    float _soilTargetPct() const;
};

#endif
