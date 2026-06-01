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

    _drawStaticLayout();
    Serial.println("[Display] Initialized");
}

void Display::_drawStaticLayout() {
    _tft.fillScreen(ST77XX_BLACK);

    _tft.fillRect(0, 0, TFT_WIDTH, 40, 0x05A0); // dark green
    _tft.setTextColor(ST77XX_WHITE);
    _tft.setTextSize(2);
    _tft.setCursor(10, 12);
    _tft.print("Greenhouse");

    _layoutDrawn = true;
}

void Display::renderBanner(const char* line) {
    if (!_layoutDrawn) _drawStaticLayout();
    _tft.fillRect(0, 44, TFT_WIDTH, 24, ST77XX_BLACK);
    _tft.setTextSize(2);
    _tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    _tft.setCursor(10, 48);
    _tft.print(line);
}

void Display::_printRow(int y, const char* label, const String& value, uint16_t color) {
    _tft.fillRect(0, y, TFT_WIDTH, 30, ST77XX_BLACK);

    _tft.setTextSize(1);
    _tft.setTextColor(0x8410);
    _tft.setCursor(10, y);
    _tft.print(label);

    _tft.setTextSize(2);
    _tft.setTextColor(color);
    _tft.setCursor(10, y + 10);
    _tft.print(value);
}

void Display::_printFooter(bool wifiOk, bool mqttOk) {
    int y = TFT_HEIGHT - 22;
    _tft.fillRect(0, y, TFT_WIDTH, 22, 0x10A2);

    _tft.setTextSize(1);
    _tft.setCursor(10, y + 7);
    _tft.setTextColor(wifiOk ? ST77XX_GREEN : ST77XX_RED);
    _tft.print(wifiOk ? "WiFi OK" : "WiFi --");

    _tft.setCursor(90, y + 7);
    _tft.setTextColor(mqttOk ? ST77XX_GREEN : ST77XX_RED);
    _tft.print(mqttOk ? "MQTT OK" : "MQTT --");
}

void Display::renderStatus(const SensorReading& r,
                           const char* stateLabel,
                           const char* modeLabel,
                           const Targets& targets,
                           bool wifiOk,
                           bool mqttOk) {
    if (!_layoutDrawn) _drawStaticLayout();

    // MODE pill in top-right of title bar
    _tft.fillRect(TFT_WIDTH - 90, 8, 86, 24, 0x03E0);
    _tft.setTextColor(ST77XX_WHITE);
    _tft.setTextSize(2);
    _tft.setCursor(TFT_WIDTH - 80, 12);
    _tft.print(modeLabel);

    int y = 50;
    _printRow(y, "STATUS", String(stateLabel), ST77XX_CYAN);
    y += 38;

    String air = r.shtOk
        ? String(r.airTempC, 1) + " C  (" + String(targets.airTempC, 0) + ")"
        : "--";
    _printRow(y, "AIR TEMP", air, ST77XX_WHITE);
    y += 38;

    String hum = r.shtOk
        ? String(r.airHumidPct, 1) + " %  (" + String(targets.humidity, 0) + ")"
        : "--";
    _printRow(y, "HUMIDITY", hum, ST77XX_WHITE);
    y += 38;

    String water = r.waterOk
        ? String(r.waterTempC, 1) + " C  (" + String(targets.waterTempC, 0) + ")"
        : "WATER NOT OK";
    _printRow(y, "WATER TEMP", water, r.waterOk ? ST77XX_WHITE : ST77XX_RED);
    y += 38;

    int bin = targets.soilBin;
    if (bin < 1) bin = 1;
    if (bin > 3) bin = 3;
    float targetSoilPct = SOIL_BIN_PCT[bin];
    String soil = r.soilOk
        ? String(r.soilPct, 0) + " %  (" + String(targetSoilPct, 0) + ")"
        : "--";
    _printRow(y, "SOIL", soil, ST77XX_WHITE);

    _printFooter(wifiOk, mqttOk);
}
