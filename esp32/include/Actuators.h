#ifndef GREENHOUSE_ACTUATORS_H
#define GREENHOUSE_ACTUATORS_H

#include <Arduino.h>

class Actuators {
public:
    void begin();

    // Duty values are 0..PWM_MAX_DUTY (0..255 at 8-bit resolution).
    void setPump(uint8_t duty);
    void setFan(uint8_t duty);
    void setHeater1(bool on);   // honors fixed 5% duty when on
    void setHeater2(bool on);
    void setLed(uint8_t duty);

    void pumpOn()  { setPump(255); }
    void pumpOff() { setPump(0); }
    void fanOn()   { setFan(255); }
    void fanOff()  { setFan(0); }
    void heatersOn();
    void heatersOff();

    uint8_t ledDuty()    const { return _ledDuty; }
    uint8_t pumpDuty()   const { return _pumpDuty; }
    uint8_t fanDuty()    const { return _fanDuty; }
    bool    heater1On()  const { return _heater1On; }
    bool    heater2On()  const { return _heater2On; }

private:
    uint8_t _pumpDuty  = 0;
    uint8_t _fanDuty   = 0;
    uint8_t _ledDuty   = 0;
    bool    _heater1On = false;
    bool    _heater2On = false;
};

#endif
