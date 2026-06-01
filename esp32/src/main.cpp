#include <Arduino.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "WifiManager.h"
#include "MQTT.h"
#include "Sensors.h"
#include "Actuators.h"
#include "Display.h"
#include "StateMachine.h"

static WifiManager  wifi;
static MQTT         mqtt(MQTT_CLIENT_ID, MQTT_TOPIC_PREFIX);
static Sensors      sensors;
static Actuators    actuators;
static Display      display;
static StateMachine fsm(actuators);

static uint32_t lastSensorMs  = 0;
static uint32_t lastPublishMs = 0;
static bool     mqttSetupDone = false;

// ---------- Command parsing helpers ----------

static String payloadToString(uint8_t* payload, unsigned int length) {
    String s; s.reserve(length + 1);
    for (unsigned int i = 0; i < length; ++i) s += (char)payload[i];
    return s;
}

static long parseLong(const String& s) {
    return strtol(s.c_str(), nullptr, 10);
}

static void handleModeCommand(const String& payload) {
    String p = payload; p.trim(); p.toUpperCase();
    if (p == "AUTO")        fsm.setMode(GhMode::AUTO);
    else if (p == "MANUAL") fsm.setMode(GhMode::MANUAL);
    else Serial.printf("[CMD] unknown mode: %s\n", p.c_str());
}

static void handleTargetsCommand(const String& payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[CMD] bad targets JSON: %s\n", err.c_str());
        return;
    }
    if (doc["air_temp"].is<float>())   fsm.patchTarget_airTemp((float)doc["air_temp"]);
    if (doc["water_temp"].is<float>()) fsm.patchTarget_waterTemp((float)doc["water_temp"]);
    if (doc["humidity"].is<float>())   fsm.patchTarget_humidity((float)doc["humidity"]);
    if (doc["soil_bin"].is<int>())     fsm.patchTarget_soilBin((int)doc["soil_bin"]);

    const Targets& t = fsm.targets();
    Serial.printf("[CMD] targets: air=%.1f water=%.1f hum=%.1f soil_bin=%d\n",
                  t.airTempC, t.waterTempC, t.humidity, t.soilBin);
}

static void onMqttMessage(char* topic, uint8_t* payload, unsigned int length) {
    String t(topic);
    String p = payloadToString(payload, length);
    Serial.printf("[MQTT] %s = %s\n", topic, p.c_str());

    if (t.endsWith("/cmd/led")) {
        long v = constrain(parseLong(p), 0L, (long)PWM_MAX_DUTY);
        actuators.setLed((uint8_t)v);
        return;
    }
    if (t.endsWith("/cmd/mode")) {
        handleModeCommand(p);
        return;
    }
    if (t.endsWith("/cmd/targets")) {
        handleTargetsCommand(p);
        return;
    }

    if (fsm.mode() != GhMode::MANUAL) {
        Serial.println("[CMD] ignored (AUTO mode)");
        return;
    }

    if (t.endsWith("/cmd/pump")) {
        actuators.setPump(parseLong(p) ? PWM_MAX_DUTY : 0);
    } else if (t.endsWith("/cmd/fan")) {
        long v = constrain(parseLong(p), 0L, (long)PWM_MAX_DUTY);
        actuators.setFan((uint8_t)v);
    } else if (t.endsWith("/cmd/heater1")) {
        actuators.setHeater1(parseLong(p) != 0);
    } else if (t.endsWith("/cmd/heater2")) {
        actuators.setHeater2(parseLong(p) != 0);
    }
}

// ---------- Telemetry ----------

static void publishTelemetry(const SensorReading& r) {
    if (!mqtt.isConnected()) return;

    JsonDocument doc;
    ActionMask m = fsm.actions();
    doc["state"] = fsm.actionsString();
    JsonArray actionsArr = doc["actions"].to<JsonArray>();
    if (m & ACT_PUMPING) actionsArr.add("PUMPING");
    if (m & ACT_HEATING) actionsArr.add("HEATING");
    if (m & ACT_VENTING) actionsArr.add("VENTING");
    doc["mode"]  = fsm.modeLabel();

    if (r.shtOk) {
        doc["humidity"] = r.airHumidPct;
        doc["air_temp"] = r.airTempC;
    } else {
        doc["humidity"] = nullptr;
        doc["air_temp"] = nullptr;
    }
    if (r.waterOk) {
        doc["water_temp"]  = r.waterTempC;
        doc["water_level"] = "OK";
    } else {
        doc["water_temp"]  = nullptr;
        doc["water_level"] = "NOT OK";
    }
    if (r.soilOk) {
        doc["soil_pct"] = r.soilPct;
        doc["soil_bin"] = r.soilBin;
    } else {
        doc["soil_pct"] = nullptr;
        doc["soil_bin"] = nullptr;
    }

    doc["pump"]    = actuators.pumpDuty() > 0 ? 1 : 0;
    doc["fan"]     = actuators.fanDuty();
    doc["heater1"] = actuators.heater1On() ? 1 : 0;
    doc["heater2"] = actuators.heater2On() ? 1 : 0;
    doc["led"]     = actuators.ledDuty();
    doc["uptime_ms"] = millis();

    JsonObject targets = doc["targets"].to<JsonObject>();
    const Targets& t = fsm.targets();
    targets["air_temp"]   = t.airTempC;
    targets["water_temp"] = t.waterTempC;
    targets["humidity"]   = t.humidity;
    targets["soil_bin"]   = t.soilBin;

    String payload;
    serializeJson(doc, payload);
    mqtt.publishMessage(MqttKeys::TELEMETRY, payload);
}

// ---------- Arduino entry points ----------

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[Boot] Automated Greenhouse starting");

    actuators.begin();
    display.begin();
    display.renderBanner("Booting...");

    sensors.begin();
    display.renderBanner("Connecting WiFi...");

#if GREENHOUSE_USE_WPA_ENTERPRISE
    display.renderBanner("Connecting (Enterprise)...");
    bool wifiOk = wifi.connectToWPAEnterprise(WIFI_SSID, WIFI_EAP_USERNAME, WIFI_PASSWORD);
#else
    bool wifiOk = wifi.connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
#endif
    display.renderBanner(wifiOk ? "WiFi OK" : "WiFi: continuing offline");

    // MQTT setup is non-blocking; an unreachable broker will just retry in loop().
    mqtt.setCallback(onMqttMessage);
    mqtt.connectToBroker();
    mqtt.subscribeTopic(MqttKeys::CMD);
    mqttSetupDone = true;

    Serial.println("[Boot] Ready");
}

void loop() {
    uint32_t now = millis();

    if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
        lastSensorMs = now;
        sensors.update();
        fsm.tick(sensors.latest());
        String stateStr = actionsShortLabel(fsm.actions());
        display.renderStatus(sensors.latest(),
                             stateStr.c_str(),
                             fsm.modeLabel(),
                             fsm.targets(),
                             wifi.isConnected(),
                             mqtt.isConnected());
    }

    if (now - lastPublishMs >= MQTT_PERIOD_MS) {
        lastPublishMs = now;
        publishTelemetry(sensors.latest());
    }

    if (mqttSetupDone) mqtt.loop();
}
