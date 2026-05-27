#include <Arduino.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "WIFI.h"
#include "MQTT.h"
#include "Sensors.h"
#include "Actuators.h"
#include "Display.h"
#include "StateMachine.h"

static WIFI         wifi;
static MQTT         mqtt(MQTT_CLIENT_ID, MQTT_TOPIC_PREFIX);
static Sensors      sensors;
static Actuators    actuators;
static Display      display;
static StateMachine fsm(actuators);

static uint32_t lastSensorMs = 0;
static uint32_t lastPublishMs = 0;

static long parseLongPayload(uint8_t* payload, unsigned int length) {
    char buf[16] = {0};
    unsigned int n = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
    memcpy(buf, payload, n);
    return strtol(buf, nullptr, 10);
}

static void onMqttMessage(char* topic, uint8_t* payload, unsigned int length) {
    String t(topic);
    long val = parseLongPayload(payload, length);
    Serial.printf("[MQTT] %s = %ld\n", topic, val);

    if (t.endsWith("/cmd/led")) {
        val = constrain(val, 0L, (long)PWM_MAX_DUTY);
        actuators.setLed((uint8_t)val);
    } else if (t.endsWith("/cmd/pump")) {
        actuators.setPump(val ? PWM_MAX_DUTY : 0);
    } else if (t.endsWith("/cmd/fan")) {
        val = constrain(val, 0L, (long)PWM_MAX_DUTY);
        actuators.setFan((uint8_t)val);
    }
}

static void publishTelemetry(const SensorReading& r) {
    JsonDocument doc;
    doc["state"]        = fsm.stateLabel();
    doc["air_temp_c"]   = r.shtOk   ? r.airTempC    : (float)NAN;
    doc["humidity_pct"] = r.shtOk   ? r.airHumidPct : (float)NAN;
    doc["water_temp_c"] = r.waterOk ? r.waterTempC  : (float)NAN;
    doc["soil_dry"]     = r.soilDry ? 1 : 0;
    doc["pump_duty"]    = actuators.pumpDuty();
    doc["fan_duty"]     = actuators.fanDuty();
    doc["heater1"]      = actuators.heater1On() ? 1 : 0;
    doc["heater2"]      = actuators.heater2On() ? 1 : 0;
    doc["led_duty"]     = actuators.ledDuty();
    doc["uptime_ms"]    = millis();

    String payload;
    serializeJson(doc, payload);
    mqtt.publishMessage(MqttKeys::TELEMETRY, payload);

    if (r.shtOk) {
        mqtt.publishMessage(MqttKeys::AIR_TEMP,  String(r.airTempC, 2));
        mqtt.publishMessage(MqttKeys::AIR_HUMID, String(r.airHumidPct, 2));
    }
    if (r.waterOk) {
        mqtt.publishMessage(MqttKeys::WATER_TEMP, String(r.waterTempC, 2));
    }
    mqtt.publishMessage(MqttKeys::SOIL_STATE, r.soilDry ? "1" : "0");
    mqtt.publishMessage(MqttKeys::STATE, fsm.stateLabel());
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[Boot] Automated Greenhouse starting");

    actuators.begin();
    display.begin();
    sensors.begin();

    wifi.connectToWiFi(WIFI_SSID, WIFI_PASSWORD);

    mqtt.setCallback(onMqttMessage);
    if (mqtt.connectToBroker()) {
        mqtt.subscribeTopic(MqttKeys::CMD);
    }

    Serial.println("[Boot] Ready");
}

void loop() {
    uint32_t now = millis();

    if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
        lastSensorMs = now;
        sensors.update();
        fsm.tick(sensors.latest());
        display.renderStatus(sensors.latest(), fsm.stateLabel());
    } else {
        fsm.tick(sensors.latest());
    }

    if (now - lastPublishMs >= MQTT_PERIOD_MS) {
        lastPublishMs = now;
        publishTelemetry(sensors.latest());
    }

    mqtt.loop();
}
