// Matches MQTT_TOPIC_PREFIX in esp32/.env. The firmware publishes JSON to
// `${MQTT_TOPIC_PREFIX}/telemetry` once per second.
export const MQTT_BROKER_URL = "ws://broker.emqx.io:8083/mqtt";
export const MQTT_TOPIC_PREFIX = "greenhouse/dev";
export const MQTT_TELEMETRY_TOPIC = `${MQTT_TOPIC_PREFIX}/telemetry`;
