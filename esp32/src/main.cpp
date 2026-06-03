#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

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

// MQTT runs on a dedicated FreeRTOS task pinned to core 0 (next to the WiFi
// stack) so incoming cmd/* messages get serviced within ~20 ms even when the
// main loop is busy with sensor reads or display redraws. PubSubClient isn't
// thread-safe, so all access goes through `mqttMutex`.
static SemaphoreHandle_t mqttMutex      = nullptr;
static TaskHandle_t      mqttTaskHandle = nullptr;

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
        long v = constrain(parseLong(p), 0L, (long)PWM_MAX_DUTY);
        actuators.setPump((uint8_t)v);
    } else if (t.endsWith("/cmd/fan")) {
        long v = constrain(parseLong(p), 0L, (long)PWM_MAX_DUTY);
        actuators.setFan((uint8_t)v);
    } else if (t.endsWith("/cmd/heater1")) {
        long v = constrain(parseLong(p), 0L, (long)PWM_MAX_DUTY);
        actuators.setHeater1((uint8_t)v);
    } else if (t.endsWith("/cmd/heater2")) {
        long v = constrain(parseLong(p), 0L, (long)PWM_MAX_DUTY);
        actuators.setHeater2((uint8_t)v);
    }
}

// ---------- Telemetry ----------

// FreeRTOS task — owns the polling side of PubSubClient.
static void mqttTask(void* /*param*/) {
    for (;;) {
        if (mqttMutex && xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            mqtt.loop();
            xSemaphoreGive(mqttMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(20));   // ~50 Hz
    }
}

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

    doc["pump"]    = actuators.pumpDuty();
    doc["fan"]     = actuators.fanDuty();
    doc["heater1"] = actuators.heater1Duty();
    doc["heater2"] = actuators.heater2Duty();
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

    if (mqttMutex && xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        mqtt.publishMessage(MqttKeys::TELEMETRY, payload);
        xSemaphoreGive(mqttMutex);
    }
}

// ---------- Arduino entry points ----------

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[Boot] Automated Greenhouse starting");

    actuators.begin();
    display.begin();
    display.renderBanner("Booting...");

    // ESP32 always boots in MANUAL — the operator decides when (and via what
    // plant) to enable AUTO control from the app's "Track" button. We don't
    // persist mode across reboots, so any prior AUTO state is intentionally
    // forgotten.
    fsm.setMode(GhMode::MANUAL);
    Serial.printf("[Boot] FSM mode = %s\n", fsm.modeLabel());

    sensors.begin();
    display.renderBanner("Connecting WiFi...");

    display.renderBanner("Scanning APs...");
    wifi.scanAndPrint(WIFI_SSID);

#if GREENHOUSE_USE_WPA_ENTERPRISE
    display.renderBanner("Connecting (Enterprise)...");
    bool wifiOk = wifi.connectToWPAEnterprise(
        WIFI_SSID, WIFI_EAP_USERNAME, WIFI_PASSWORD, WIFI_EAP_ANON_IDENTITY);
#else
    bool wifiOk = wifi.connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
#endif
    display.renderBanner(wifiOk ? "WiFi OK" : "WiFi: continuing offline");

    // MQTT setup is non-blocking; an unreachable broker just retries in the task.
    mqtt.setCallback(onMqttMessage);
    mqtt.connectToBroker();
    mqtt.subscribeTopic(MqttKeys::CMD);
    mqttSetupDone = true;

    // Spin up the MQTT pump on core 0 (the "PRO CPU", same core as WiFi).
    mqttMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        mqttTask,           // entry point
        "mqtt-loop",        // task name (for debug)
        8192,               // stack size in bytes
        nullptr,            // parameter
        2,                  // priority (low-medium)
        &mqttTaskHandle,    // task handle out
        0                   // pinned to core 0
    );

    Serial.println("[Boot] Ready");
}

void loop() {
    uint32_t now = millis();

    if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
        lastSensorMs = now;
        sensors.update();
        fsm.tick(sensors.latest());
        String stateStr = actionsShortLabel(fsm.actions());
        ActuatorDuties duties = {
            actuators.pumpDuty(),
            actuators.fanDuty(),
            actuators.heater1Duty(),
            actuators.heater2Duty(),
            actuators.ledDuty(),
        };
        display.renderStatus(sensors.latest(),
                             stateStr.c_str(),
                             fsm.modeLabel(),
                             fsm.targets(),
                             wifi.isConnected(),
                             mqtt.isConnected(),
                             duties);
    }

    if (now - lastPublishMs >= MQTT_PERIOD_MS) {
        lastPublishMs = now;
        publishTelemetry(sensors.latest());
    }

    // mqtt.loop() no longer called here — the dedicated mqttTask drives it on core 0.
}
