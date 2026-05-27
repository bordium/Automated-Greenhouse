#ifndef GREENHOUSE_STATEMACHINE_H
#define GREENHOUSE_STATEMACHINE_H

#include <Arduino.h>
#include "Sensors.h"
#include "Actuators.h"

enum class GhState : uint8_t {
    IDLE = 0,
    PUMPING,
    HEATING,
    VENTING
};

const char* stateName(GhState s);

class StateMachine {
public:
    StateMachine(Actuators& actuators);
    void tick(const SensorReading& r);
    GhState state() const { return _state; }
    const char* stateLabel() const { return stateName(_state); }

private:
    Actuators& _act;
    GhState   _state         = GhState::IDLE;
    uint32_t  _stateEnteredAt = 0;
    uint32_t  _pumpStartedAt  = 0;

    void _enter(GhState s);
    void _applyOutputs();
};

#endif
