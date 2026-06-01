#include "MQTT.h"

MQTT::MQTT(String clientId, String topicPrefix)
: _mqttClient(_wifiClient), _clientId(clientId), _topicPrefix(topicPrefix) {
    Serial.println("[MQTT] Initialized with client ID and topic prefix");
}

bool MQTT::connectToBroker(int port) {
    _port = port;
    _mqttClient.setServer(_broker, _port);
    _mqttClient.setBufferSize(1024);
    _mqttClient.setSocketTimeout(2);   // seconds — fast-fail when broker unreachable

    return _tryConnect();
}

bool MQTT::_tryConnect() {
    if (WiFi.status() != WL_CONNECTED) return false;

    Serial.print("[MQTT] Connecting to broker as ");
    Serial.println(_clientId);

    if (_mqttClient.connect(_clientId.c_str())) {
        Serial.println("[MQTT] Connected.");
        if (_lastSubscribe.length() > 0) {
            _mqttClient.subscribe(_lastSubscribe.c_str());
        }
        return true;
    }
    Serial.print("[MQTT] Connect failed, state: ");
    Serial.println(_mqttClient.state());
    return false;
}

bool MQTT::publishMessage(String subtopic, String message) {
    if (!_mqttClient.connected()) return false;
    String fullTopic = _topicPrefix + "/" + subtopic;
    return _mqttClient.publish(fullTopic.c_str(), message.c_str());
}

bool MQTT::subscribeTopic(String subtopic) {
    String fullTopic = _topicPrefix + "/" + subtopic;
    _lastSubscribe = fullTopic;

    Serial.print("[MQTT] Subscribing to topic: ");
    Serial.println(fullTopic);

    if (!_mqttClient.connected()) return false;
    return _mqttClient.subscribe(fullTopic.c_str());
}

void MQTT::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    _mqttClient.setCallback(callback);
}

void MQTT::loop() {
    if (_mqttClient.connected()) {
        _mqttClient.loop();
        return;
    }

    // Non-blocking, throttled reconnect.
    uint32_t now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL_MS) return;
    _lastReconnectAttempt = now;
    _tryConnect();
}
