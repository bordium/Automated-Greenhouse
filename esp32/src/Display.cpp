#include "Display.h"
#include "Config.h"

// Landscape rotation. The ST7789 panel is 240x320 in its native (portrait)
// orientation; rotation 1 turns it 90° so it becomes 320 wide x 240 tall.
// If the screen ends up upside-down after assembly, switch this to 3.
static constexpr uint8_t TFT_ROTATION = 1;

Display::Display()
: _spi(HSPI), _tft(&_spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST) {}

void Display::begin() {
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    _spi.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
    _tft.init(TFT_WIDTH, TFT_HEIGHT);   // native panel size (portrait)
    _tft.setRotation(TFT_ROTATION);     // landscape
    _tft.fillScreen(ST77XX_BLACK);
    _tft.setTextWrap(false);

    _drawStaticLayout();
    Serial.println("[Display] Initialized");
}

void Display::_drawStaticLayout() {
    int w = _tft.width();   // 320
    _tft.fillScreen(ST77XX_BLACK);

    _tft.fillRect(0, 0, w, 40, 0x05A0);   // dark green title bar
    _tft.setTextColor(ST77XX_WHITE);
    _tft.setTextSize(2);
    _tft.setCursor(10, 12);
    _tft.print("Greenhouse");

    _layoutDrawn = true;
}

void Display::renderBanner(const char* line) {
    if (!_layoutDrawn) _drawStaticLayout();
    int w = _tft.width();
    _tft.fillRect(0, 44, w, 28, ST77XX_BLACK);
    _tft.setTextSize(2);
    _tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    _tft.setCursor(10, 50);
    _tft.print(line);
}

void Display::_printRow(int y, const char* label, const String& value, uint16_t color) {
    int w = _tft.width();
    _tft.fillRect(0, y, w, 30, ST77XX_BLACK);

    _tft.setTextSize(1);
    _tft.setTextColor(0x8410);
    _tft.setCursor(10, y);
    _tft.print(label);

    _tft.setTextSize(2);
    _tft.setTextColor(color);
    _tft.setCursor(10, y + 10);
    _tft.print(value);
}

// Half-width cell used for the 2x2 sensor grid in landscape.
static void _printCell(Adafruit_ST7789& tft, int x, int y, int w,
                       const char* label, const String& value, uint16_t color) {
    tft.fillRect(x, y, w, 32, ST77XX_BLACK);

    tft.setTextSize(1);
    tft.setTextColor(0x8410);
    tft.setCursor(x + 8, y);
    tft.print(label);

    tft.setTextSize(2);
    tft.setTextColor(color);
    tft.setCursor(x + 8, y + 12);
    tft.print(value);
}

void Display::_printFooter(bool wifiOk, bool mqttOk) {
    int w = _tft.width();
    int h = _tft.height();
    int y = h - 22;
    _tft.fillRect(0, y, w, 22, 0x10A2);

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
    int w = _tft.width();   // 320

    // MODE pill in top-right of title bar
    _tft.fillRect(w - 90, 8, 86, 24, 0x03E0);
    _tft.setTextColor(ST77XX_WHITE);
    _tft.setTextSize(2);
    _tft.setCursor(w - 80, 12);
    _tft.print(modeLabel);

    // STATUS row across full width
    _printRow(46, "STATUS", String(stateLabel), ST77XX_CYAN);

    // 2x2 sensor grid below the STATUS row
    int colW  = w / 2;     // 160
    int rowY1 = 88;
    int rowY2 = 138;

    String air = r.shtOk
        ? String(r.airTempC, 1) + " C (" + String(targets.airTempC, 0) + ")"
        : "--";
    _printCell(_tft, 0, rowY1, colW, "AIR TEMP", air, ST77XX_WHITE);

    String hum = r.shtOk
        ? String(r.airHumidPct, 1) + "% (" + String(targets.humidity, 0) + ")"
        : "--";
    _printCell(_tft, colW, rowY1, colW, "HUMIDITY", hum, ST77XX_WHITE);

    String water = r.waterOk
        ? String(r.waterTempC, 1) + " C (" + String(targets.waterTempC, 0) + ")"
        : "WATER N/A";
    _printCell(_tft, 0, rowY2, colW, "WATER TEMP", water,
               r.waterOk ? ST77XX_WHITE : ST77XX_RED);

    int bin = targets.soilBin;
    if (bin < 1) bin = 1;
    if (bin > 3) bin = 3;
    float targetSoilPct = SOIL_BIN_PCT[bin];
    String soil = r.soilOk
        ? String(r.soilPct, 0) + "% (" + String(targetSoilPct, 0) + ")"
        : "--";
    _printCell(_tft, colW, rowY2, colW, "SOIL", soil, ST77XX_WHITE);

    _printFooter(wifiOk, mqttOk);
}
