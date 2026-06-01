import { ScrollView, StyleSheet, Text, View } from "react-native";

import { InfoRow } from "@/components/info-row";
import { GreenhouseTheme, actionColor, stateColor } from "@/constants/greenhouse-theme";
import { ActionLabel, MqttStatus, useMqtt } from "@/hooks/mqtt-context";

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

const statusLabel = (s: MqttStatus) =>
  ({ connecting: "Connecting…", connected: "Live", reconnecting: "Reconnecting…", offline: "Offline" }[s]);

const statusDotColor = (s: MqttStatus) =>
  s === "connected" ? GreenhouseTheme.statusOk
    : s === "offline" ? GreenhouseTheme.statusBad
    : GreenhouseTheme.statusWarn;

export default function DashboardScreen() {
  const { telemetry, status } = useMqtt();
  const targets = telemetry?.targets;

  return (
    <ScrollView style={styles.container}>
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
          <ActuatorPill label="Pump" on={!!telemetry?.pump} value={telemetry?.pump} />
          <ActuatorPill label="Fan" on={(telemetry?.fan ?? 0) > 0} value={telemetry?.fan} duty />
          <ActuatorPill label="Heater 1" on={!!telemetry?.heater1} value={telemetry?.heater1} />
          <ActuatorPill label="Heater 2" on={!!telemetry?.heater2} value={telemetry?.heater2} />
          <ActuatorPill label="LED" on={(telemetry?.led ?? 0) > 0} value={telemetry?.led} duty />
        </View>
      </View>
    </ScrollView>
  );
}

function ActuatorPill({
  label,
  on,
  value,
  duty,
}: {
  label: string;
  on: boolean;
  value?: number;
  duty?: boolean;
}) {
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
        {duty
          ? `${Math.round(((value ?? 0) / 255) * 100)}%`
          : on ? "ON" : "OFF"}
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
  card: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 18, padding: 20, marginBottom: 16,
  },
  sectionTitle: {
    fontSize: 17, fontWeight: "800", marginBottom: 14, color: GreenhouseTheme.text,
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
