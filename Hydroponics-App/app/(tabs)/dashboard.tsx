import { useMemo, useState } from "react";
import {
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from "react-native";

import { InfoRow } from "@/components/info-row";
import { GreenhouseTheme, actionColor, stateColor } from "@/constants/greenhouse-theme";
import { ActionLabel, MqttStatus, useMqtt } from "@/hooks/mqtt-context";
import plantsData from "../../database/Database.json";

type Plant = {
  common_name: string;
  scientific_name: string;
  temp_air_c: string;
  temp_water_c: string;
  soil_moisture: string;
  humidity: string;
  nutrient: string;
  ph: string;
  min_germination_days: string;
  max_germination_days: string;
  instructions: string;
};

const SOIL_BIN_PCT: Record<number, number> = { 1: 20, 2: 40, 3: 60 };

const fmtNum = (v: number | null | undefined, suffix = "", digits = 1) =>
  v === null || v === undefined || Number.isNaN(v)
    ? undefined
    : `${v.toFixed(digits)}${suffix}`;

const fmtTargetTemp = (v: number | undefined) => fmtNum(v, " °C", 0);
const fmtTargetHumid = (v: number | undefined) => fmtNum(v, "%", 0);
const fmtTargetSoilBin = (b: number | undefined) =>
  b !== undefined && SOIL_BIN_PCT[b] !== undefined
    ? `${SOIL_BIN_PCT[b]}% • bin ${b}`
    : undefined;

const parseSoilBin = (raw?: string) => {
  if (!raw) return undefined;
  const n = parseInt(raw, 10);
  if (Number.isNaN(n) || n < 1 || n > 3) return undefined;
  return n;
};

const statusLabel = (s: MqttStatus) =>
  ({ connecting: "Connecting…", connected: "Live", reconnecting: "Reconnecting…", offline: "Offline" }[s]);

const statusDotColor = (s: MqttStatus) =>
  s === "connected" ? GreenhouseTheme.statusOk
    : s === "offline" ? GreenhouseTheme.statusBad
    : GreenhouseTheme.statusWarn;

export default function DashboardScreen() {
  const { telemetry, status, publish } = useMqtt();
  const targets = telemetry?.targets;

  // ---- Plant Tracking state ----
  const plants = plantsData as Plant[];
  const uniquePlants = useMemo(() => {
    const seen = new Set<string>();
    return plants.filter((p) => {
      const name = p.common_name?.trim();
      if (!name || seen.has(name.toLowerCase())) return false;
      seen.add(name.toLowerCase());
      return true;
    });
  }, [plants]);

  const [searchText, setSearchText]       = useState("");
  const [selectedPlant, setSelectedPlant] = useState<Plant | null>(null);
  const [tracking, setTracking]           = useState<string | null>(null);

  const suggestions = useMemo(() => {
    const q = searchText.trim().toLowerCase();
    if (!q || selectedPlant?.common_name === searchText) return [];
    return uniquePlants
      .filter((p) => p.common_name.toLowerCase().includes(q))
      .slice(0, 6);
  }, [searchText, selectedPlant, uniquePlants]);

  const startTracking = (plant: Plant) => {
    const payload: Record<string, number> = {};
    if (plant.temp_air_c?.trim())   payload.air_temp   = parseFloat(plant.temp_air_c);
    if (plant.temp_water_c?.trim()) payload.water_temp = parseFloat(plant.temp_water_c);
    if (plant.humidity?.trim())     payload.humidity   = parseFloat(plant.humidity);
    const bin = parseSoilBin(plant.soil_moisture);
    if (bin !== undefined) payload.soil_bin = bin;

    publish("cmd/targets", JSON.stringify(payload));
    publish("cmd/mode", "AUTO");
    setTracking(plant.common_name);
  };

  const stopTracking = () => {
    setTracking(null);
    setSelectedPlant(null);
    setSearchText("");
  };

  return (
    <ScrollView style={styles.container} keyboardShouldPersistTaps="handled">
      <Text style={styles.title}>Greenhouse</Text>

      {/* Status bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusPillRow}>
          <View style={[styles.dot, { backgroundColor: statusDotColor(status) }]} />
          <Text style={styles.statusText}>{statusLabel(status)}</Text>
        </View>
        <View style={[
          styles.modePill,
          { backgroundColor: telemetry?.mode === "AUTO" ? GreenhouseTheme.primarySoft : "#fef3c7" },
        ]}>
          <Text style={[
            styles.modePillText,
            { color: telemetry?.mode === "AUTO" ? GreenhouseTheme.primaryDark : "#b45309" },
          ]}>
            {telemetry?.mode ?? "—"}
          </Text>
        </View>
      </View>

      {/* Plant Tracking */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Plant Tracking</Text>
        {tracking ? (
          <>
            <View style={styles.trackingHeader}>
              <Text style={styles.trackingLabel}>Currently tracking</Text>
              <Text style={styles.trackingName}>🌱 {tracking}</Text>
            </View>
            <Pressable style={styles.stopBtn} onPress={stopTracking}>
              <Text style={styles.stopBtnText}>Stop tracking</Text>
            </Pressable>
          </>
        ) : (
          <>
            <Text style={styles.helpText}>
              Pick a plant — the greenhouse will switch to AUTO mode and match its target conditions.
            </Text>
            <TextInput
              style={styles.search}
              placeholder="Type a plant, e.g. Lettuce"
              value={searchText}
              onChangeText={(t) => {
                setSearchText(t);
                setSelectedPlant(null);
              }}
            />
            {suggestions.length > 0 && (
              <View style={styles.suggestionBox}>
                {suggestions.map((p) => (
                  <Pressable
                    key={p.common_name}
                    style={styles.suggestionItem}
                    onPress={() => {
                      setSelectedPlant(p);
                      setSearchText(p.common_name);
                    }}
                  >
                    <Text style={styles.suggestionTitle}>{p.common_name}</Text>
                    <Text style={styles.suggestionSub}>{p.scientific_name}</Text>
                  </Pressable>
                ))}
              </View>
            )}
            {selectedPlant && (
              <View style={styles.selectedPlantBox}>
                <Text style={styles.selectedPlantTitle}>{selectedPlant.common_name}</Text>
                <Text style={styles.selectedPlantSub}>
                  {selectedPlant.temp_air_c} °C air • {selectedPlant.humidity}% RH • soil bin {selectedPlant.soil_moisture}
                </Text>
                <Pressable
                  style={styles.trackBtn}
                  onPress={() => startTracking(selectedPlant)}
                >
                  <Text style={styles.trackBtnText}>Track {selectedPlant.common_name}</Text>
                </Pressable>
              </View>
            )}
          </>
        )}
      </View>

      {/* Big state card */}
      <View style={[styles.stateCard, { borderLeftColor: stateColor(telemetry?.state) }]}>
        <Text style={styles.stateLabel}>State</Text>
        {(() => {
          const actions: ActionLabel[] =
            telemetry?.actions && telemetry.actions.length > 0
              ? telemetry.actions
              : [];
          if (actions.length === 0) {
            return (
              <Text style={[styles.stateValue, { color: GreenhouseTheme.stateIdle }]}>IDLE</Text>
            );
          }
          return (
            <View style={styles.actionBadgeRow}>
              {actions.map((a) => (
                <View
                  key={a}
                  style={[styles.actionBadge, { backgroundColor: actionColor(a) }]}
                >
                  <Text style={styles.actionBadgeText}>{a}</Text>
                </View>
              ))}
            </View>
          );
        })()}
      </View>

      {/* Live readings */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Live Readings</Text>

        <InfoRow
          label="Air Temperature"
          current={fmtNum(telemetry?.air_temp, " °C")}
          target={fmtTargetTemp(targets?.air_temp)}
        />
        <InfoRow
          label="Humidity"
          current={fmtNum(telemetry?.humidity, "%")}
          target={fmtTargetHumid(targets?.humidity)}
        />
        <InfoRow
          label="Water Temperature"
          current={fmtNum(telemetry?.water_temp, " °C")}
          target={fmtTargetTemp(targets?.water_temp)}
        />
        <InfoRow
          label="Soil Moisture"
          current={fmtNum(telemetry?.soil_pct, "%", 0)}
          target={fmtTargetSoilBin(targets?.soil_bin)}
        />
        <InfoRow label="Water Level" current={telemetry?.water_level} />
      </View>

      {/* Actuator block */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Actuators</Text>
        <View style={styles.actuatorGrid}>
          <ActuatorPill label="Pump"     value={telemetry?.pump} />
          <ActuatorPill label="Fan"      value={telemetry?.fan} />
          <ActuatorPill label="Heater 1" value={telemetry?.heater1} />
          <ActuatorPill label="Heater 2" value={telemetry?.heater2} />
          <ActuatorPill label="LED"      value={telemetry?.led} />
        </View>
      </View>
    </ScrollView>
  );
}

function ActuatorPill({
  label,
  value,
}: {
  label: string;
  value?: number;
}) {
  const duty = value ?? 0;
  const on = duty > 0;
  const pct = Math.round((duty / 255) * 100);
  return (
    <View
      style={[
        styles.actuatorPill,
        { backgroundColor: on ? GreenhouseTheme.primarySoft : "#f3f4f6" },
      ]}
    >
      <Text style={[styles.actuatorLabel, { color: on ? GreenhouseTheme.primaryDark : GreenhouseTheme.textMuted }]}>
        {label}
      </Text>
      <Text style={[styles.actuatorValue, { color: on ? GreenhouseTheme.primaryDark : GreenhouseTheme.textLight }]}>
        {on ? `${pct}%` : "OFF"}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: GreenhouseTheme.bg, padding: 20 },
  title: {
    fontSize: 30, fontWeight: "800", marginTop: 60, marginBottom: 16,
    color: GreenhouseTheme.text,
  },
  statusBar: {
    flexDirection: "row", justifyContent: "space-between", alignItems: "center",
    marginBottom: 16,
  },
  statusPillRow: {
    flexDirection: "row", alignItems: "center",
    backgroundColor: GreenhouseTheme.card,
    paddingVertical: 6, paddingHorizontal: 12, borderRadius: 999,
  },
  dot: { width: 8, height: 8, borderRadius: 999, marginRight: 8 },
  statusText: { fontSize: 13, color: GreenhouseTheme.text, fontWeight: "600" },
  modePill: {
    paddingVertical: 6, paddingHorizontal: 14, borderRadius: 999,
  },
  modePillText: { fontWeight: "800", fontSize: 13, letterSpacing: 0.5 },

  card: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 18, padding: 20, marginBottom: 16,
  },
  sectionTitle: {
    fontSize: 17, fontWeight: "800", marginBottom: 12, color: GreenhouseTheme.text,
  },
  helpText: { fontSize: 13, color: GreenhouseTheme.textMuted, marginBottom: 12 },

  search: {
    backgroundColor: "#f3f4f6", borderRadius: 12, padding: 12, fontSize: 16,
    marginBottom: 10, borderWidth: 1, borderColor: "#e5e7eb",
  },
  suggestionBox: {
    backgroundColor: "#f9fafb", borderRadius: 12, marginBottom: 10,
    borderWidth: 1, borderColor: "#e5e7eb",
  },
  suggestionItem: { padding: 12, borderBottomWidth: 1, borderBottomColor: "#e5e7eb" },
  suggestionTitle: { fontSize: 16, fontWeight: "700", color: GreenhouseTheme.text },
  suggestionSub: { fontSize: 12, color: GreenhouseTheme.textMuted, fontStyle: "italic" },

  selectedPlantBox: {
    backgroundColor: "#f9fafb", borderRadius: 12, padding: 14, marginTop: 4,
    borderWidth: 1, borderColor: "#e5e7eb",
  },
  selectedPlantTitle: { fontSize: 18, fontWeight: "800", color: GreenhouseTheme.text },
  selectedPlantSub: { fontSize: 13, color: GreenhouseTheme.textMuted, marginTop: 4, marginBottom: 12 },
  trackBtn: {
    backgroundColor: GreenhouseTheme.primary,
    paddingVertical: 14, borderRadius: 12, alignItems: "center",
  },
  trackBtnText: { color: "white", fontWeight: "800", fontSize: 16, letterSpacing: 0.3 },

  trackingHeader: { marginBottom: 14 },
  trackingLabel: { fontSize: 12, color: GreenhouseTheme.textMuted, textTransform: "uppercase", letterSpacing: 0.5 },
  trackingName: { fontSize: 24, fontWeight: "800", color: GreenhouseTheme.primaryDark, marginTop: 4 },
  stopBtn: {
    backgroundColor: "#f3f4f6",
    paddingVertical: 12, borderRadius: 12, alignItems: "center",
  },
  stopBtnText: { color: GreenhouseTheme.text, fontWeight: "700", fontSize: 14 },

  stateCard: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 18, padding: 20,
    marginBottom: 16, borderLeftWidth: 6,
  },
  stateLabel: {
    fontSize: 13, color: GreenhouseTheme.textMuted,
    textTransform: "uppercase", letterSpacing: 0.5,
  },
  stateValue: { fontSize: 36, fontWeight: "900", marginTop: 4 },
  actionBadgeRow: { flexDirection: "row", flexWrap: "wrap", gap: 8, marginTop: 8 },
  actionBadge: {
    paddingVertical: 8, paddingHorizontal: 14, borderRadius: 999,
  },
  actionBadgeText: {
    color: "white", fontSize: 15, fontWeight: "800", letterSpacing: 0.5,
  },

  actuatorGrid: {
    flexDirection: "row", flexWrap: "wrap", gap: 10,
  },
  actuatorPill: {
    paddingVertical: 10, paddingHorizontal: 14, borderRadius: 12,
    minWidth: 90, alignItems: "center",
  },
  actuatorLabel: { fontSize: 12, fontWeight: "700", letterSpacing: 0.3 },
  actuatorValue: { fontSize: 18, fontWeight: "800", marginTop: 2 },
});