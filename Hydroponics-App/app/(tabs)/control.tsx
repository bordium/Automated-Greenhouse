import { useEffect, useRef, useState } from "react";
import {
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from "react-native";
import Slider from "@react-native-community/slider";

import { GreenhouseTheme } from "@/constants/greenhouse-theme";
import { Mode, useMqtt } from "@/hooks/mqtt-context";

type TargetsForm = {
  air_temp: string;
  water_temp: string;
  humidity: string;
  soil_bin: string;
};

const emptyForm: TargetsForm = {
  air_temp: "",
  water_temp: "",
  humidity: "",
  soil_bin: "",
};

const parseSoilBin = (raw?: string) => {
  if (!raw) return undefined;
  const n = parseInt(raw, 10);
  if (Number.isNaN(n) || n < 1 || n > 5) return undefined;
  return n;
};

// Hydroponics-App displays actuator strength as 0..100 % but the ESP32 PWM is
// 8-bit (0..255). The mapping rounds DOWN to the nearest PWM step on publish,
// and rounds to the nearest percent on receive so the slider visually snaps
// back to the same percent the user picked.
const percentToDuty = (p: number) => Math.floor((p / 100) * 255);
const dutyToPercent = (d: number) => Math.round((d / 255) * 100);

// Min time between mid-drag MQTT publishes. ~20 Hz feels smooth without
// flooding the broker; final value is still always sent on slide-complete.
const DRAG_PUBLISH_MS = 50;

export default function ControlScreen() {
  const { telemetry, publish, status } = useMqtt();
  const liveTargets = telemetry?.targets;
  const liveMode = telemetry?.mode;

  const [form, setForm] = useState<TargetsForm>(emptyForm);
  const [editingForm, setEditingForm] = useState(false);

  useEffect(() => {
    if (editingForm || !liveTargets) return;
    setForm({
      air_temp:   liveTargets.air_temp   !== undefined ? String(liveTargets.air_temp)   : "",
      water_temp: liveTargets.water_temp !== undefined ? String(liveTargets.water_temp) : "",
      humidity:   liveTargets.humidity   !== undefined ? String(liveTargets.humidity)   : "",
      soil_bin:   liveTargets.soil_bin   !== undefined ? String(liveTargets.soil_bin)   : "",
    });
  }, [liveTargets, editingForm]);

  const sendMode = (next: Mode) => publish("cmd/mode", next);

  const sendTargets = () => {
    const payload: Record<string, number> = {};
    if (form.air_temp)   payload.air_temp   = parseFloat(form.air_temp);
    if (form.water_temp) payload.water_temp = parseFloat(form.water_temp);
    if (form.humidity)   payload.humidity   = parseFloat(form.humidity);
    const bin = parseSoilBin(form.soil_bin);
    if (bin !== undefined) payload.soil_bin = bin;
    publish("cmd/targets", JSON.stringify(payload));
    setEditingForm(false);
  };

  const isManual = liveMode === "MANUAL";

  return (
    <ScrollView style={styles.container} keyboardShouldPersistTaps="handled">
      <Text style={styles.title}>Control</Text>

      {/* --- Mode --- */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Mode</Text>
        <View style={styles.modeRow}>
          <ModeButton
            label="AUTO"
            sublabel="Follow targets"
            selected={liveMode === "AUTO"}
            onPress={() => sendMode("AUTO")}
          />
          <ModeButton
            label="MANUAL"
            sublabel="Direct control"
            selected={liveMode === "MANUAL"}
            onPress={() => sendMode("MANUAL")}
          />
        </View>
        {status !== "connected" && (
          <Text style={styles.warnLine}>
            Not connected — commands may be dropped.
          </Text>
        )}
      </View>

      {/* --- Manual targets --- */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Targets</Text>
        <Text style={styles.helpText}>
          Soil bin: 1 = 20% • 2 = 40% • 3 = 60% • 4 = 80% • 5 = 100%
        </Text>

        <TargetField
          label="Air temperature (°C)"
          value={form.air_temp}
          onChangeText={(v) => { setEditingForm(true); setForm({ ...form, air_temp: v }); }}
          keyboardType="decimal-pad"
        />
        <TargetField
          label="Water temperature (°C)"
          value={form.water_temp}
          onChangeText={(v) => { setEditingForm(true); setForm({ ...form, water_temp: v }); }}
          keyboardType="decimal-pad"
        />
        <TargetField
          label="Humidity (%)"
          value={form.humidity}
          onChangeText={(v) => { setEditingForm(true); setForm({ ...form, humidity: v }); }}
          keyboardType="decimal-pad"
        />
        <TargetField
          label="Soil moisture bin (1-5)"
          value={form.soil_bin}
          onChangeText={(v) => { setEditingForm(true); setForm({ ...form, soil_bin: v }); }}
          keyboardType="number-pad"
        />

        <Pressable style={styles.applyBtn} onPress={sendTargets}>
          <Text style={styles.applyBtnText}>Send targets</Text>
        </Pressable>
      </View>

      {/* --- Manual actuator controls --- */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Actuators</Text>
        <Text style={styles.helpText}>
          {isManual
            ? "Manual mode — slide 0–100% (rounds down to the corresponding PWM step)."
            : "Pump, fan, and heaters only respond in MANUAL mode. LED is always live."}
        </Text>

        <ActuatorSlider
          label="Pump"
          duty={telemetry?.pump ?? 0}
          disabled={!isManual}
          onCommit={(d) => publish("cmd/pump", String(d))}
        />
        <ActuatorSlider
          label="Fan"
          duty={telemetry?.fan ?? 0}
          disabled={!isManual}
          onCommit={(d) => publish("cmd/fan", String(d))}
        />
        <ActuatorSlider
          label="Heater 1"
          duty={telemetry?.heater1 ?? 0}
          disabled={!isManual}
          maxPercent={80}
          onCommit={(d) => publish("cmd/heater1", String(d))}
          warn="Capped at 80 % for safety — AUTO uses 55 %."
        />
        <ActuatorSlider
          label="Heater 2"
          duty={telemetry?.heater2 ?? 0}
          disabled={!isManual}
          maxPercent={80}
          onCommit={(d) => publish("cmd/heater2", String(d))}
          warn="Capped at 80 % for safety — AUTO uses 55 %."
        />
        <ActuatorSlider
          label="LED"
          duty={telemetry?.led ?? 0}
          disabled={false}
          onCommit={(d) => publish("cmd/led", String(d))}
        />
      </View>
    </ScrollView>
  );
}

function ModeButton({
  label,
  sublabel,
  selected,
  onPress,
}: {
  label: string;
  sublabel: string;
  selected: boolean;
  onPress: () => void;
}) {
  return (
    <Pressable
      style={[
        styles.modeBtn,
        selected && { backgroundColor: GreenhouseTheme.primary, borderColor: GreenhouseTheme.primary },
      ]}
      onPress={onPress}
    >
      <Text style={[styles.modeBtnLabel, selected && { color: "white" }]}>{label}</Text>
      <Text style={[styles.modeBtnSub, selected && { color: "rgba(255,255,255,0.85)" }]}>{sublabel}</Text>
    </Pressable>
  );
}

function TargetField({
  label,
  value,
  onChangeText,
  keyboardType,
}: {
  label: string;
  value: string;
  onChangeText: (v: string) => void;
  keyboardType: "decimal-pad" | "number-pad";
}) {
  return (
    <View style={styles.field}>
      <Text style={styles.fieldLabel}>{label}</Text>
      <TextInput
        style={styles.fieldInput}
        value={value}
        onChangeText={onChangeText}
        keyboardType={keyboardType}
        placeholder="—"
      />
    </View>
  );
}

/**
 * Slider for a 0..100% power level. Snaps to the PWM step on publish — i.e.
 * 50% → floor(50/100 * 255) = 127 PWM. Mirrors live telemetry when not being
 * dragged. Publishes both during the drag (throttled) and on slide-complete.
 */
function ActuatorSlider({
  label,
  duty,
  disabled,
  onCommit,
  warn,
  maxPercent = 100,
}: {
  label: string;
  duty: number;
  disabled: boolean;
  onCommit: (duty: number) => void;
  warn?: string;
  maxPercent?: number;
}) {
  const clamp = (p: number) => Math.min(Math.max(0, p), maxPercent);
  const [localPct, setLocalPct] = useState(clamp(dutyToPercent(duty)));
  const draggingRef    = useRef(false);
  const lastPublishRef = useRef(0);

  // Snap to telemetry when we're not dragging — clamped to the cap so the
  // slider thumb never sits above the allowed maximum.
  useEffect(() => {
    if (!draggingRef.current) setLocalPct(clamp(dutyToPercent(duty)));
  }, [duty, maxPercent]);

  const on = localPct > 0;

  return (
    <View style={[styles.actuatorRow, disabled && { opacity: 0.45 }]}>
      <View style={styles.actuatorHeader}>
        <Text style={styles.actuatorLabel}>{label}</Text>
        <View style={styles.actuatorRightGroup}>
          <Text style={[
            styles.actuatorPct,
            { color: on ? GreenhouseTheme.primaryDark : GreenhouseTheme.textLight },
          ]}>
            {on ? `${localPct}%` : "OFF"}
          </Text>
          <Pressable
            style={[
              styles.offChip,
              on ? { backgroundColor: GreenhouseTheme.primarySoft } : { backgroundColor: "#f3f4f6" },
            ]}
            disabled={disabled}
            onPress={() => {
              const nextPct = on ? 0 : maxPercent;
              setLocalPct(nextPct);
              onCommit(percentToDuty(nextPct));
            }}
          >
            <Text style={[
              styles.offChipText,
              { color: on ? GreenhouseTheme.primaryDark : GreenhouseTheme.textMuted },
            ]}>
              {on ? "OFF" : "ON"}
            </Text>
          </Pressable>
        </View>
      </View>
      <Slider
        style={styles.slider}
        minimumValue={0}
        maximumValue={maxPercent}
        step={1}
        value={localPct}
        disabled={disabled}
        minimumTrackTintColor={GreenhouseTheme.primary}
        maximumTrackTintColor="#e5e7eb"
        thumbTintColor={GreenhouseTheme.primary}
        onValueChange={(v) => {
          draggingRef.current = true;
          const rounded = clamp(Math.round(v));
          setLocalPct(rounded);
          const now = Date.now();
          if (now - lastPublishRef.current >= DRAG_PUBLISH_MS) {
            lastPublishRef.current = now;
            onCommit(percentToDuty(rounded));
          }
        }}
        onSlidingComplete={(v) => {
          const finalPct = clamp(Math.round(v));
          setLocalPct(finalPct);
          draggingRef.current = false;
          lastPublishRef.current = Date.now();
          onCommit(percentToDuty(finalPct));
        }}
      />
      {warn && <Text style={styles.warnSmall}>{warn}</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: GreenhouseTheme.bg, padding: 20 },
  title: { fontSize: 30, fontWeight: "800", marginTop: 60, marginBottom: 16, color: GreenhouseTheme.text },

  card: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 18, padding: 20, marginBottom: 16,
  },
  sectionTitle: { fontSize: 17, fontWeight: "800", marginBottom: 12, color: GreenhouseTheme.text },
  helpText: { fontSize: 13, color: GreenhouseTheme.textMuted, marginBottom: 12 },

  warnLine: { fontSize: 12, color: GreenhouseTheme.warn, marginTop: 8 },
  warnSmall: { fontSize: 11, color: GreenhouseTheme.warn, marginTop: 2 },

  modeRow: { flexDirection: "row", gap: 10 },
  modeBtn: {
    flex: 1, paddingVertical: 16, borderRadius: 14, alignItems: "center",
    backgroundColor: "#f3f4f6",
    borderWidth: 1, borderColor: "#e5e7eb",
  },
  modeBtnLabel: { fontSize: 18, fontWeight: "800", color: GreenhouseTheme.text },
  modeBtnSub: { fontSize: 12, color: GreenhouseTheme.textMuted, marginTop: 2 },

  field: { marginBottom: 12 },
  fieldLabel: { fontSize: 12, color: GreenhouseTheme.textMuted, marginBottom: 4 },
  fieldInput: {
    backgroundColor: "#f3f4f6", borderRadius: 10, padding: 12, fontSize: 16,
    borderWidth: 1, borderColor: "#e5e7eb",
  },

  applyBtn: {
    backgroundColor: GreenhouseTheme.primary, padding: 14, borderRadius: 12,
    alignItems: "center", marginTop: 6,
  },
  applyBtnText: { color: "white", fontWeight: "800", fontSize: 16 },

  actuatorRow: {
    paddingVertical: 10, borderBottomWidth: 1, borderBottomColor: "#f3f4f6",
  },
  actuatorHeader: {
    flexDirection: "row", justifyContent: "space-between", alignItems: "center",
  },
  actuatorLabel: { fontSize: 16, fontWeight: "700", color: GreenhouseTheme.text },
  actuatorRightGroup: { flexDirection: "row", alignItems: "center", gap: 10 },
  actuatorPct: { fontSize: 15, fontWeight: "800", minWidth: 44, textAlign: "right" },
  offChip: {
    paddingHorizontal: 12, paddingVertical: 4, borderRadius: 999,
  },
  offChipText: { fontSize: 12, fontWeight: "700", letterSpacing: 0.5 },
  slider: { width: "100%", height: 36, marginTop: 4 },
});
