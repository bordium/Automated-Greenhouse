#ifndef GREENHOUSE_DISPLAY_H
#define GREENHOUSE_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "Sensors.h"
#include "StateMachine.h"

/** Snapshot of all five actuator duty values (0..255) for the LCD strip. */
struct ActuatorDuties {
    uint8_t pump;
    uint8_t fan;
    uint8_t heater1;
    uint8_t heater2;
    uint8_t led;
};

class Display {
public:
    Display();
    void begin();

    /** Render a boot/banner line at the top under the title. */
    void renderBanner(const char* line);

    void renderStatus(const SensorReading& r,
                      const char* stateLabel,
                      const char* modeLabel,
                      const Targets& targets,
                      bool wifiOk,
                      bool mqttOk,
                      const ActuatorDuties& act);

private:
    SPIClass        _spi;
    Adafruit_ST7789 _tft;
    bool            _layoutDrawn = false;

    void _drawStaticLayout();
    void _printRow(int y, const char* label, const String& value, uint16_t color);
    void _printFooter(bool wifiOk, bool mqttOk);
    void _printActuatorStrip(const ActuatorDuties& act);
    void _clearActuatorStrip();
};

#endif
