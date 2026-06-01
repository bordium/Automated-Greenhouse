import { useEffect, useMemo, useState } from "react";
import {
  Pressable,
  ScrollView,
  StyleSheet,
  Switch,
  Text,
  TextInput,
  View,
} from "react-native";

import { GreenhouseTheme } from "@/constants/greenhouse-theme";
import { Mode, useMqtt } from "@/hooks/mqtt-context";
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
  if (Number.isNaN(n) || n < 1 || n > 3) return undefined;
  return n;
};

export default function ControlScreen() {
  const { telemetry, publish, status } = useMqtt();
  const liveTargets = telemetry?.targets;
  const liveMode = telemetry?.mode;

  const [form, setForm] = useState<TargetsForm>(emptyForm);
  const [editingForm, setEditingForm] = useState(false);
  const [searchText, setSearchText] = useState("");
  const [selectedPlant, setSelectedPlant] = useState<Plant | null>(null);

  // Mirror live targets into the form until the user starts editing.
  useEffect(() => {
    if (editingForm || !liveTargets) return;
    setForm({
      air_temp:   liveTargets.air_temp   !== undefined ? String(liveTargets.air_temp)   : "",
      water_temp: liveTargets.water_temp !== undefined ? String(liveTargets.water_temp) : "",
      humidity:   liveTargets.humidity   !== undefined ? String(liveTargets.humidity)   : "",
      soil_bin:   liveTargets.soil_bin   !== undefined ? String(liveTargets.soil_bin)   : "",
    });
  }, [liveTargets, editingForm]);

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

  const suggestions = useMemo(() => {
    const q = searchText.trim().toLowerCase();
    if (!q || selectedPlant?.common_name === searchText) return [];
    return uniquePlants.filter((p) => p.common_name.toLowerCase().includes(q)).slice(0, 6);
  }, [searchText, selectedPlant, uniquePlants]);

  const sendMode = (next: Mode) => publish("cmd/mode", next);

  const applyPreset = (plant: Plant) => {
    setSelectedPlant(plant);
    setSearchText(plant.common_name);
    const next: TargetsForm = {
      air_temp:   plant.temp_air_c?.trim()   || form.air_temp,
      water_temp: plant.temp_water_c?.trim() || form.water_temp,
      humidity:   plant.humidity?.trim()     || form.humidity,
      soil_bin:   plant.soil_moisture?.trim() || form.soil_bin,
    };
    setForm(next);
    setEditingForm(true);

    const payload: Record<string, number> = {};
    if (next.air_temp)   payload.air_temp   = parseFloat(next.air_temp);
    if (next.water_temp) payload.water_temp = parseFloat(next.water_temp);
    if (next.humidity)   payload.humidity   = parseFloat(next.humidity);
    const bin = parseSoilBin(next.soil_bin);
    if (bin !== undefined) payload.soil_bin = bin;

    publish("cmd/targets", JSON.stringify(payload));
  };

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

      {/* --- Preset picker --- */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Plant Preset</Text>
        <Text style={styles.helpText}>
          Search the database to load a plant&apos;s recommended targets.
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
                onPress={() => applyPreset(p)}
              >
                <Text style={styles.suggestionTitle}>{p.common_name}</Text>
                <Text style={styles.suggestionSub}>{p.scientific_name}</Text>
              </Pressable>
            ))}
          </View>
        )}
        {selectedPlant && (
          <Text style={styles.helpText}>
            Loaded preset:{" "}
            <Text style={styles.bold}>{selectedPlant.common_name}</Text>
          </Text>
        )}
      </View>

      {/* --- Manual targets --- */}
      <View style={styles.card}>
        <Text style={styles.sectionTitle}>Targets</Text>
        <Text style={styles.helpText}>
          Soil bin: 1 = 20% • 2 = 40% • 3 = 60%
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
          label="Soil moisture bin (1-3)"
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
            ? "Manual mode active — toggles drive hardware directly."
            : "Pump, fan, and heaters are only honored in MANUAL mode. LED is always honored."}
        </Text>

        <ActuatorToggle
          label="Pump"
          on={!!telemetry?.pump}
          disabled={!isManual}
          onChange={(v) => publish("cmd/pump", v ? "1" : "0")}
        />
        <ActuatorToggle
          label="Fan"
          on={(telemetry?.fan ?? 0) > 0}
          disabled={!isManual}
          onChange={(v) => publish("cmd/fan", v ? "255" : "0")}
        />
        <ActuatorToggle
          label="Heater 1"
          on={!!telemetry?.heater1}
          disabled={!isManual}
          onChange={(v) => publish("cmd/heater1", v ? "1" : "0")}
        />
        <ActuatorToggle
          label="Heater 2"
          on={!!telemetry?.heater2}
          disabled={!isManual}
          onChange={(v) => publish("cmd/heater2", v ? "1" : "0")}
        />
        <ActuatorToggle
          label="LED"
          on={(telemetry?.led ?? 0) > 0}
          disabled={false}
          onChange={(v) => publish("cmd/led", v ? "255" : "0")}
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

function ActuatorToggle({
  label,
  on,
  disabled,
  onChange,
}: {
  label: string;
  on: boolean;
  disabled: boolean;
  onChange: (v: boolean) => void;
}) {
  return (
    <View style={[styles.toggleRow, disabled && { opacity: 0.5 }]}>
      <Text style={styles.toggleLabel}>{label}</Text>
      <Switch
        value={on}
        disabled={disabled}
        onValueChange={onChange}
        trackColor={{ false: "#d1d5db", true: GreenhouseTheme.primary }}
        thumbColor="white"
      />
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
  bold: { fontWeight: "700", color: GreenhouseTheme.text },

  warnLine: { fontSize: 12, color: GreenhouseTheme.warn, marginTop: 8 },

  modeRow: { flexDirection: "row", gap: 10 },
  modeBtn: {
    flex: 1, paddingVertical: 16, borderRadius: 14, alignItems: "center",
    backgroundColor: "#f3f4f6",
    borderWidth: 1, borderColor: "#e5e7eb",
  },
  modeBtnLabel: { fontSize: 18, fontWeight: "800", color: GreenhouseTheme.text },
  modeBtnSub: { fontSize: 12, color: GreenhouseTheme.textMuted, marginTop: 2 },

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

  toggleRow: {
    flexDirection: "row", alignItems: "center", justifyContent: "space-between",
    paddingVertical: 12, borderBottomWidth: 1, borderBottomColor: "#f3f4f6",
  },
  toggleLabel: { fontSize: 16, fontWeight: "700", color: GreenhouseTheme.text },
});
