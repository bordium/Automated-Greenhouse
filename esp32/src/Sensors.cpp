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
    _ds.setResolution(10);   // ~190 ms conversion, well under 1 s loop budget

    analogReadResolution(12);   // ESP32-S3 ADC is 12-bit
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
    // 12-bit ADC range is 0..4095, but the cheap probes only swing roughly
    // 280..650 according to user calibration. The Arduino-ESP32 core scales
    // its analogRead() to the configured resolution; we keep 12-bit and use
    // the raw scale directly.
    int raw = analogRead(PIN_SOIL_DAT);
    _latest.soilAdcRaw = raw;

    if (raw <= 0) { _latest.soilOk = false; return; }

    // Wet = low ADC, Dry = high ADC. Invert so % moisture is intuitive.
    float span = (float)(SOIL_ADC_DRY - SOIL_ADC_WET);
    float pct  = (span > 0.f) ? ((SOIL_ADC_DRY - raw) / span) * 100.f : 0.f;
    if (pct < 0.f)   pct = 0.f;
    if (pct > 100.f) pct = 100.f;

    _latest.soilPct = pct;
    _latest.soilOk  = true;

    // Snap to nearest configured bin (20/40/60 %).
    int   bestBin  = 1;
    float bestDist = 1e9f;
    for (int b = 1; b <= 3; ++b) {
        float d = fabsf(pct - SOIL_BIN_PCT[b]);
        if (d < bestDist) { bestDist = d; bestBin = b; }
    }
    _latest.soilBin = bestBin;

    // Rolling stats
    _soilStats.lastRaw = raw;
    _soilStats.lastPct = pct;
    _soilStats.lastBin = bestBin;
    if (raw < _soilStats.minRaw) _soilStats.minRaw = raw;
    if (raw > _soilStats.maxRaw) _soilStats.maxRaw = raw;
    _soilStats.sumRaw += raw;
    _soilStats.sumPct += pct;
    _soilStats.count  += 1;

    // printSoilStats();
}

void Sensors::resetSoilStats() {
    _soilStats = SoilStats{};
}

void Sensors::printSoilStats() const {
    const SoilStats& s = _soilStats;
    if (s.count == 0) {
        Serial.println("[SOIL] no samples yet");
        return;
    }
    float avgRaw = (float)(s.sumRaw / s.count);
    float avgPct = (float)(s.sumPct / s.count);
    float minPct = NAN, maxPct = NAN;
    float span = (float)(SOIL_ADC_DRY - SOIL_ADC_WET);
    if (span > 0.f) {
        minPct = ((SOIL_ADC_DRY - s.maxRaw) / span) * 100.f;  // wettest sample
        maxPct = ((SOIL_ADC_DRY - s.minRaw) / span) * 100.f;  // driest sample
        if (minPct < 0.f) minPct = 0.f; if (minPct > 100.f) minPct = 100.f;
        if (maxPct < 0.f) maxPct = 0.f; if (maxPct > 100.f) maxPct = 100.f;
    }
    Serial.printf(
        "[SOIL] raw=%d pct=%.1f bin=%d | min_raw=%d max_raw=%d avg_raw=%.1f"
        " | pct_range=%.1f..%.1f avg_pct=%.1f | n=%u | cal: wet=%d dry=%d\n",
        s.lastRaw, s.lastPct, s.lastBin,
        s.minRaw, s.maxRaw, avgRaw,
        minPct, maxPct, avgPct,
        (unsigned)s.count,
        SOIL_ADC_WET, SOIL_ADC_DRY
    );
}
