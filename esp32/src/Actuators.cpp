#include "Actuators.h"
#include "Config.h"

void Actuators::begin() {
    ledcSetup(CH_PUMP,    PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(CH_FAN,     PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(CH_HEATER1, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(CH_HEATER2, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(CH_LED,     PWM_FREQ_HZ, PWM_RES_BITS);

    ledcAttachPin(PIN_PUMP,    CH_PUMP);
    ledcAttachPin(PIN_FAN,     CH_FAN);
    ledcAttachPin(PIN_HEATER1, CH_HEATER1);
    ledcAttachPin(PIN_HEATER2, CH_HEATER2);
    ledcAttachPin(PIN_LED,     CH_LED);

    setPump(0);
    setFan(0);
    setHeater1(false);
    setHeater2(false);
    setLed(LED_DEFAULT_DUTY);

    Serial.println("[Actuators] Initialized");
}

void Actuators::setPump(uint8_t duty) {
    _pumpDuty = duty;
    ledcWrite(CH_PUMP, duty);
}

void Actuators::setFan(uint8_t duty) {
    _fanDuty = duty;
    ledcWrite(CH_FAN, duty);
}

void Actuators::setHeater1(bool on) {
    _heater1On = on;
    ledcWrite(CH_HEATER1, on ? HEATER_DUTY : 0);
}

void Actuators::setHeater2(bool on) {
    _heater2On = on;
    ledcWrite(CH_HEATER2, on ? HEATER_DUTY : 0);
}

void Actuators::setLed(uint8_t duty) {
    _ledDuty = duty;
    ledcWrite(CH_LED, duty);
}

void Actuators::heatersOn() {
    setHeater1(true);
    setHeater2(true);
}

void Actuators::heatersOff() {
    setHeater1(false);
    setHeater2(false);
}
