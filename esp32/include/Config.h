#ifndef GREENHOUSE_CONFIG_H
#define GREENHOUSE_CONFIG_H

#include <Arduino.h>

// ---------- I2C bus (SHT45 humidity/temp) ----------
static constexpr int PIN_I2C_SDA = 1;
static constexpr int PIN_I2C_SCL = 2;

// ---------- OneWire / single-wire sensors ----------
// KIT0021 = DFRobot waterproof DS18B20 (OneWire)
static constexpr int PIN_WATER_DAT = 3;
// Soil moisture analog probe (ADC1)
static constexpr int PIN_SOIL_DAT  = 4;

// ---------- ST7789VW 240x320 SPI display ----------
static constexpr int PIN_TFT_BL   = 5;
static constexpr int PIN_TFT_RST  = 6;
static constexpr int PIN_TFT_DC   = 7;
static constexpr int PIN_TFT_CS   = 8;
static constexpr int PIN_TFT_SCLK = 9;
static constexpr int PIN_TFT_MOSI = 10;
static constexpr int TFT_WIDTH    = 240;
static constexpr int TFT_HEIGHT   = 320;

// ---------- Actuator PWM pins ----------
static constexpr int PIN_LED     = 21;
static constexpr int PIN_HEATER2 = 26;
static constexpr int PIN_FAN     = 42;
static constexpr int PIN_PUMP    = 45;
static constexpr int PIN_HEATER1 = 47;

// ---------- LEDC PWM channels ----------
static constexpr int CH_PUMP    = 0;
static constexpr int CH_FAN     = 1;
static constexpr int CH_HEATER1 = 2;
static constexpr int CH_HEATER2 = 3;
static constexpr int CH_LED     = 4;

static constexpr int PWM_FREQ_HZ   = 1000;
static constexpr int PWM_RES_BITS  = 8;          // 0..255 — matches MQTT contract
static constexpr int PWM_MAX_DUTY  = (1 << PWM_RES_BITS) - 1;

// Heater duty cycle when the FSM turns heaters on in AUTO mode (55 %). The
// app's manual slider is capped at 80 %, leaving headroom above the AUTO
// default in case the operator wants to push a bit harder.
static constexpr int HEATER_DUTY      = (PWM_MAX_DUTY * 55) / 100;
static constexpr int LED_DEFAULT_DUTY = PWM_MAX_DUTY;   // LED fully on by default

// ---------- Soil-moisture calibration ----------
// Raw ADC readings on PIN_SOIL_DAT.
//   ADC == SOIL_ADC_WET  -> 100 % moisture
//   ADC == SOIL_ADC_DRY  ->   0 % moisture
static constexpr int   SOIL_ADC_WET = 1607;
static constexpr int   SOIL_ADC_DRY = 2880;
// Bin -> target percent. Lettuce in the DB lists "3" -> 60 %.
static constexpr float SOIL_BIN_PCT[4] = { 0.0f, 20.0f, 40.0f, 60.0f };

// ---------- Default AUTO-mode targets (used until app overrides them) ----------
// These match lettuce so a fresh boot does something sensible.
static constexpr float DEFAULT_TARGET_AIR_TEMP_C   = 16.0f;
static constexpr float DEFAULT_TARGET_WATER_TEMP_C = 13.0f;
static constexpr float DEFAULT_TARGET_HUMIDITY    = 65.0f;
static constexpr int   DEFAULT_TARGET_SOIL_BIN    = 3;

// Hysteresis bands for AUTO mode
static constexpr float TEMP_DEADBAND_C    = 2.0f;
static constexpr float HUMID_DEADBAND_PCT = 5.0f;
static constexpr float SOIL_DEADBAND_PCT  = 5.0f;

static constexpr uint32_t PUMP_RUN_MS       = 5000;   // run pump 5 s when triggered
static constexpr uint32_t PUMP_COOLDOWN_MS  = 5000;  // wait 5 s before pumping again
static constexpr uint32_t SENSOR_PERIOD_MS  = 1000;   // sensor sample period
static constexpr uint32_t MQTT_PERIOD_MS    = 1000;   // telemetry publish period

// ---------- MQTT keys (subtopics under topicPrefix) ----------
namespace MqttKeys {
    static constexpr const char* TELEMETRY   = "telemetry";
    static constexpr const char* CMD         = "cmd/#";
    static constexpr const char* CMD_MODE    = "cmd/mode";       // "AUTO" | "MANUAL"
    static constexpr const char* CMD_TARGETS = "cmd/targets";    // JSON
    static constexpr const char* CMD_PUMP    = "cmd/pump";       // 0 | 1
    static constexpr const char* CMD_FAN     = "cmd/fan";        // 0..255
    static constexpr const char* CMD_HEATER1 = "cmd/heater1";    // 0 | 1
    static constexpr const char* CMD_HEATER2 = "cmd/heater2";    // 0 | 1
    static constexpr const char* CMD_LED     = "cmd/led";        // 0..255
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

// If WIFI_EAP_USERNAME is defined (typically via .env), boot connects using
// WPA2-Enterprise (PEAP/MSCHAPv2). Otherwise, plain WPA-PSK is used.
#ifdef WIFI_EAP_USERNAME
#define GREENHOUSE_USE_WPA_ENTERPRISE 1
#else
#define GREENHOUSE_USE_WPA_ENTERPRISE 0
#endif

// Optional anonymous outer identity. If not set in .env we re-use the
// EAP username for both outer and inner identity.
#ifndef WIFI_EAP_ANON_IDENTITY
#define WIFI_EAP_ANON_IDENTITY ""
#endif

#endif
