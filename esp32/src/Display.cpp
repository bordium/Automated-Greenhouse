#include "Display.h"
#include "Config.h"

Display::Display()
: _spi(HSPI), _tft(&_spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST) {}

void Display::begin() {
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    _spi.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
    _tft.init(TFT_WIDTH, TFT_HEIGHT);
    _tft.setRotation(0);
    _tft.fillScreen(ST77XX_BLACK);
    _tft.setTextWrap(false);
    _tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    _tft.setTextSize(2);
    _tft.setCursor(10, 10);
    _tft.print("Greenhouse");

    Serial.println("[Display] Initialized");
}

void Display::renderStatus(const SensorReading& r, const char* stateLabel) {
    _tft.fillRect(0, 40, TFT_WIDTH, TFT_HEIGHT - 40, ST77XX_BLACK);
    _tft.setTextSize(2);

    int y = 50;
    _tft.setCursor(10, y); y += 24;
    _tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    _tft.print("State: "); _tft.print(stateLabel);

    _tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    _tft.setCursor(10, y); y += 24;
    _tft.print("Air T: ");
    if (r.shtOk) { _tft.print(r.airTempC, 1); _tft.print(" C"); }
    else _tft.print("--");

    _tft.setCursor(10, y); y += 24;
    _tft.print("Humid: ");
    if (r.shtOk) { _tft.print(r.airHumidPct, 1); _tft.print(" %"); }
    else _tft.print("--");

    _tft.setCursor(10, y); y += 24;
    _tft.print("Water: ");
    if (r.waterOk) { _tft.print(r.waterTempC, 1); _tft.print(" C"); }
    else _tft.print("--");

    _tft.setCursor(10, y); y += 24;
    _tft.print("Soil: ");
    _tft.setTextColor(r.soilDry ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
    _tft.print(r.soilDry ? "DRY" : "WET");
}
