#ifndef GREENHOUSE_SENSORS_H
#define GREENHOUSE_SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSht4x.h>
#include <OneWire.h>
#include <DallasTemperature.h>

struct SensorReading {
    float    airTempC      = NAN;
    float    airHumidPct   = NAN;
    float    waterTempC    = NAN;
    int      soilAdcRaw    = -1;
    float    soilPct       = NAN;
    int      soilBin       = 0;       // 1, 2, or 3 — nearest standard bin
    bool     shtOk         = false;
    bool     waterOk       = false;
    bool     soilOk        = false;
    uint32_t timestampMs   = 0;
};

/** Rolling statistics for the capacitive soil probe, cumulative since boot. */
struct SoilStats {
    int      lastRaw   = -1;
    float    lastPct   = NAN;
    int      lastBin   = 0;
    int      minRaw    = INT32_MAX;
    int      maxRaw    = INT32_MIN;
    double   sumRaw    = 0.0;
    double   sumPct    = 0.0;
    uint32_t count     = 0;
};

class Sensors {
public:
    Sensors();
    void begin();
    void update();
    const SensorReading& latest() const { return _latest; }
    const SoilStats&     soilStats() const { return _soilStats; }
    void resetSoilStats();
    void printSoilStats() const;

private:
    SensirionI2cSht4x _sht;
    OneWire           _ow;
    DallasTemperature _ds;
    SensorReading     _latest;
    SoilStats         _soilStats;

    void _readSht();
    void _readWaterTemp();
    void _readSoil();
};

#endif
