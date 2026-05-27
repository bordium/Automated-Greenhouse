#include "Sensors.h"
#include "Config.h"

Sensors::Sensors()
: _ow(PIN_WATER_DAT), _ds(&_ow) {}

void Sensors::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    _sht.begin(Wire, SHT40_I2C_ADDR_44);
    _sht.softReset();
    delay(10);

    _ds.begin();
    _ds.setResolution(12);

    pinMode(PIN_SOIL_DAT, INPUT);

    Serial.println("[Sensors] Initialized");
}

void Sensors::update() {
    _readSht();
    _readWaterTemp();
    _readSoil();
    _latest.timestampMs = millis();
}

void Sensors::_readSht() {
    float t = NAN, rh = NAN;
    uint16_t err = _sht.measureHighPrecision(t, rh);
    if (err == 0 && !isnan(t) && !isnan(rh)) {
        _latest.airTempC    = t;
        _latest.airHumidPct = rh;
        _latest.shtOk       = true;
    } else {
        _latest.shtOk = false;
        Serial.print("[Sensors] SHT45 read error: ");
        Serial.println(err);
    }
}

void Sensors::_readWaterTemp() {
    _ds.requestTemperatures();
    float t = _ds.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C || t < -55.0f) {
        _latest.waterOk = false;
    } else {
        _latest.waterTempC = t;
        _latest.waterOk    = true;
    }
}

void Sensors::_readSoil() {
    int level = digitalRead(PIN_SOIL_DAT);
    _latest.soilDry = (level == DRY_SOIL_LEVEL);
}
