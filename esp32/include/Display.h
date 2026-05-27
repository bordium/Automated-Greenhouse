#ifndef GREENHOUSE_DISPLAY_H
#define GREENHOUSE_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "Sensors.h"

class Display {
public:
    Display();
    void begin();
    void renderStatus(const SensorReading& r, const char* stateLabel);

private:
    SPIClass _spi;
    Adafruit_ST7789 _tft;
};

#endif
