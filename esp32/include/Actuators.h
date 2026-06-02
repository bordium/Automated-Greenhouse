#ifndef GREENHOUSE_ACTUATORS_H
#define GREENHOUSE_ACTUATORS_H

#include <Arduino.h>

class Actuators {
public:
    void begin();

    // Duty values are 0..PWM_MAX_DUTY (0..255 at 8-bit resolution).
    // Any actuator at duty 0 is "off"; anything > 0 is "on" at that duty.
    void setPump(uint8_t duty);
    void setFan(uint8_t duty);
    void setHeater1(uint8_t duty);
    void setHeater2(uint8_t duty);
    void setLed(uint8_t duty);

    void pumpOn()    { setPump(255); }
    void pumpOff()   { setPump(0); }
    void fanOn()     { setFan(255); }
    void fanOff()    { setFan(0); }
    void heatersOn();   // applies HEATER_DUTY (5%) to both heaters
    void heatersOff();

    uint8_t pumpDuty()    const { return _pumpDuty; }
    uint8_t fanDuty()     const { return _fanDuty; }
    uint8_t heater1Duty() const { return _heater1Duty; }
    uint8_t heater2Duty() const { return _heater2Duty; }
    uint8_t ledDuty()     const { return _ledDuty; }
    bool    heater1On()   const { return _heater1Duty > 0; }
    bool    heater2On()   const { return _heater2Duty > 0; }

private:
    uint8_t _pumpDuty    = 0;
    uint8_t _fanDuty     = 0;
    uint8_t _heater1Duty = 0;
    uint8_t _heater2Duty = 0;
    uint8_t _ledDuty     = 0;
};

#endif
