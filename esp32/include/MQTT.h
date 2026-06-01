#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

/**
 * @brief MQTT helper around PubSubClient with non-blocking reconnect.
 *
 * `loop()` is safe to call every main-loop iteration even when WiFi or the
 * broker is unreachable — it returns immediately and throttles reconnect
 * attempts to once every RECONNECT_INTERVAL_MS.
 *
 * The PubSubClient instance is created once and reused; we never re-allocate
 * during reconnect.
 */
class MQTT {
public:
    MQTT(String clientId, String topicPrefix);

    bool connectToBroker(int port = 1883);
    bool publishMessage(String subtopic, String message);
    bool subscribeTopic(String subtopic);
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));

    /** Pump PubSubClient + try a (non-blocking) reconnect when disconnected. */
    void loop();

    bool isConnected() { return _mqttClient.connected(); }

private:
    WiFiClient   _wifiClient;
    PubSubClient _mqttClient;
    String       _clientId;
    String       _topicPrefix;
    String       _lastSubscribe;
    const char*  _broker = "broker.emqx.io";
    int          _port = 1883;
    uint32_t     _lastReconnectAttempt = 0;
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 5000;

    bool _tryConnect();
};

#endif
