#ifndef GREENHOUSE_CONFIG_H
#define GREENHOUSE_CONFIG_H

#include <Arduino.h>

// ---------- I2C bus (SHT45 humidity/temp) ----------
static constexpr int PIN_I2C_SDA = 46;
static constexpr int PIN_I2C_SCL = 45;

// ---------- OneWire / single-wire sensors ----------
// KIT0021 = DFRobot waterproof DS18B20 (OneWire) on RX0
static constexpr int PIN_WATER_DAT = 44;
// Soil moisture digital output on TX0
static constexpr int PIN_SOIL_DAT  = 43;

// ---------- ST7789VW 240x320 SPI display ----------
static constexpr int PIN_TFT_MOSI = 42;
static constexpr int PIN_TFT_SCLK = 41;
static constexpr int PIN_TFT_CS   = 40;
static constexpr int PIN_TFT_DC   = 39;
static constexpr int PIN_TFT_RST  = 38;
static constexpr int PIN_TFT_BL   = 37;
static constexpr int TFT_WIDTH    = 240;
static constexpr int TFT_HEIGHT   = 320;

// ---------- Actuator PWM pins ----------
static constexpr int PIN_PUMP    = 2;
static constexpr int PIN_FAN     = 5;
static constexpr int PIN_HEATER1 = 16;
static constexpr int PIN_HEATER2 = 17;
static constexpr int PIN_LED     = 18;

// ---------- LEDC PWM channels ----------
static constexpr int CH_PUMP    = 0;
static constexpr int CH_FAN     = 1;
static constexpr int CH_HEATER1 = 2;
static constexpr int CH_HEATER2 = 3;
static constexpr int CH_LED     = 4;

static constexpr int PWM_FREQ_HZ   = 1000;
static constexpr int PWM_RES_BITS  = 8;        // 0..255
static constexpr int PWM_MAX_DUTY  = (1 << PWM_RES_BITS) - 1;

// Heater duty cycle = 1/20 = 5%
static constexpr int HEATER_DUTY = PWM_MAX_DUTY / 20;
// LED fully on by default
static constexpr int LED_DEFAULT_DUTY = PWM_MAX_DUTY;

// ---------- Control thresholds ----------
// Soil sensor digital reading: most modules pull LOW when wet, HIGH when dry.
// Override via DRY_SOIL_LEVEL in .env if your board is inverted.
#ifndef DRY_SOIL_LEVEL
#define DRY_SOIL_LEVEL HIGH
#endif

static constexpr float TEMP_MIN_C       = 18.0f;  // below -> HEATING
static constexpr float TEMP_MAX_C       = 30.0f;  // above -> VENTING
static constexpr float HUMIDITY_MAX_PCT = 85.0f;  // above -> VENTING

static constexpr uint32_t PUMP_RUN_MS       = 5000;   // run pump 5s when triggered
static constexpr uint32_t MIN_STATE_HOLD_MS = 2000;   // hysteresis: hold state at least 2s
static constexpr uint32_t SENSOR_PERIOD_MS  = 2000;   // sensor sample period
static constexpr uint32_t MQTT_PERIOD_MS    = 5000;   // telemetry publish period

// ---------- MQTT keys (subtopics under topicPrefix) ----------
namespace MqttKeys {
    static constexpr const char* AIR_TEMP    = "sensor/air_temp_c";
    static constexpr const char* AIR_HUMID   = "sensor/humidity_pct";
    static constexpr const char* WATER_TEMP  = "sensor/water_temp_c";
    static constexpr const char* SOIL_STATE  = "sensor/soil_dry";   // 1 = dry, 0 = wet
    static constexpr const char* STATE       = "state";
    static constexpr const char* TELEMETRY   = "telemetry";          // full JSON snapshot
    static constexpr const char* CMD         = "cmd/#";              // wildcard subscribe
    static constexpr const char* CMD_LED     = "cmd/led";            // 0..255 duty
    static constexpr const char* CMD_PUMP    = "cmd/pump";           // 0/1 manual
    static constexpr const char* CMD_FAN     = "cmd/fan";            // 0..255 duty
}

// ---------- Credentials / broker ----------
// These come from esp32/.env via the env_check.py build script.
// Required keys: WIFI_SSID, WIFI_PASSWORD, MQTT_CLIENT_ID, MQTT_TOPIC_PREFIX
// Optional:      WIFI_EAP_USERNAME (use WPA Enterprise if set)
#ifndef WIFI_SSID
#define WIFI_SSID "your-ssid"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your-password"
#endif
#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "greenhouse-esp32"
#endif
#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX "greenhouse/dev"
#endif

#endif
