#ifndef GREENHOUSE_SENSORS_H
#define GREENHOUSE_SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSht4x.h>
#include <OneWire.h>
#include <DallasTemperature.h>

struct SensorReading {
    float airTempC      = NAN;
    float airHumidPct   = NAN;
    float waterTempC    = NAN;
    bool  soilDry       = false;
    bool  shtOk         = false;
    bool  waterOk       = false;
    uint32_t timestampMs = 0;
};

class Sensors {
public:
    Sensors();
    void begin();
    void update();
    const SensorReading& latest() const { return _latest; }

private:
    SensirionI2cSht4x _sht;
    OneWire _ow;
    DallasTemperature _ds;
    SensorReading _latest;

    void _readSht();
    void _readWaterTemp();
    void _readSoil();
};

#endif
