import React, {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import mqtt, { MqttClient } from "mqtt";

import {
  MQTT_BROKER_URL,
  MQTT_TELEMETRY_TOPIC,
  MQTT_TOPIC_PREFIX,
} from "@/constants/mqtt";

export type Mode = "AUTO" | "MANUAL";
export type ActionLabel = "PUMPING" | "HEATING" | "VENTING";
/** Composite label like "IDLE" or "PUMPING+HEATING+VENTING". */
export type StateLabel = string;
export type WaterLevel = "OK" | "NOT OK";

export type Targets = {
  air_temp?: number;
  water_temp?: number;
  humidity?: number;
  soil_bin?: number;
};

export type Telemetry = {
  state?: StateLabel;          // composite, e.g. "HEATING+VENTING"
  actions?: ActionLabel[];     // each active action individually
  mode?: Mode;
  humidity?: number | null;
  air_temp?: number | null;
  water_temp?: number | null;
  water_level?: WaterLevel;
  soil_pct?: number | null;
  soil_bin?: number | null;
  pump?: number;
  fan?: number;
  heater1?: number;
  heater2?: number;
  led?: number;
  uptime_ms?: number;
  targets?: Targets;
};

export type MqttStatus = "connecting" | "connected" | "reconnecting" | "offline";

type MqttContextValue = {
  telemetry: Telemetry | null;
  status: MqttStatus;
  publish: (subtopic: string, payload: string) => void;
};

const MqttContext = createContext<MqttContextValue | null>(null);

export function MqttProvider({ children }: { children: React.ReactNode }) {
  const [telemetry, setTelemetry] = useState<Telemetry | null>(null);
  const [status, setStatus] = useState<MqttStatus>("connecting");
  const clientRef = useRef<MqttClient | null>(null);

  useEffect(() => {
    const clientId = `hydroponics-app-${Math.random().toString(16).slice(2, 10)}`;
    const client = mqtt.connect(MQTT_BROKER_URL, {
      clientId,
      reconnectPeriod: 2000,
      keepalive: 30,
      clean: true,
    });
    clientRef.current = client;

    client.on("connect", () => {
      setStatus("connected");
      client.subscribe(MQTT_TELEMETRY_TOPIC, (err) => {
        if (err) console.warn("[mqtt] subscribe error", err);
      });
    });
    client.on("reconnect", () => setStatus("reconnecting"));
    client.on("offline", () => setStatus("offline"));
    client.on("close", () => setStatus("offline"));
    client.on("error", (err) => console.warn("[mqtt] error", err.message));

    client.on("message", (_topic, payload) => {
      try {
        const next = JSON.parse(payload.toString()) as Telemetry;
        setTelemetry(next);
      } catch (e) {
        console.warn("[mqtt] bad payload", payload.toString());
      }
    });

    return () => {
      client.end(true);
      clientRef.current = null;
    };
  }, []);

  const publish = useCallback((subtopic: string, payload: string) => {
    const client = clientRef.current;
    if (!client || !client.connected) {
      console.warn("[mqtt] publish dropped (not connected):", subtopic, payload);
      return;
    }
    const topic = `${MQTT_TOPIC_PREFIX}/${subtopic}`;
    client.publish(topic, payload, { qos: 0, retain: false }, (err) => {
      if (err) console.warn("[mqtt] publish error", err.message);
    });
  }, []);

  const value = useMemo(
    () => ({ telemetry, status, publish }),
    [telemetry, status, publish]
  );

  return <MqttContext.Provider value={value}>{children}</MqttContext.Provider>;
}

export function useMqtt(): MqttContextValue {
  const ctx = useContext(MqttContext);
  if (!ctx) throw new Error("useMqtt must be used inside <MqttProvider>");
  return ctx;
}
