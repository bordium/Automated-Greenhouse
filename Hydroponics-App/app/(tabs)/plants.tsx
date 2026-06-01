import { useMemo, useState } from "react";
import {
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from "react-native";

import plantsData from "../../database/Database.json";
import { GreenhouseTheme } from "@/constants/greenhouse-theme";

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

const formatValue = (value?: string, suffix = "") => {
  if (!value || value.trim() === "") return "Not available";
  return `${value}${suffix}`;
};

const SOIL_BIN_PCT: Record<string, string> = {
  "1": "20% (dry-loving)",
  "2": "40% (moderate)",
  "3": "60% (moisture-loving)",
};

const fmtSoilBin = (raw?: string) => {
  const t = raw?.trim();
  if (!t) return "Not available";
  return SOIL_BIN_PCT[t] ?? t;
};

export default function PlantsScreen() {
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

  const [searchText, setSearchText] = useState("");
  const [selectedPlant, setSelectedPlant] = useState<Plant | null>(null);

  const suggestions = useMemo(() => {
    const q = searchText.trim().toLowerCase();
    if (!q || selectedPlant?.common_name === searchText) return [];
    return uniquePlants.filter((p) => p.common_name.toLowerCase().includes(q)).slice(0, 8);
  }, [searchText, selectedPlant, uniquePlants]);

  return (
    <ScrollView style={styles.container} keyboardShouldPersistTaps="handled">
      <Text style={styles.title}>Plant Database 🌿</Text>
      <Text style={styles.subtitle}>
        Browse target growing conditions. Use the Control tab to push a preset to the greenhouse.
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

      {!selectedPlant && (
        <Text style={styles.empty}>Search and select a plant to view its requirements.</Text>
      )}

      {selectedPlant && (
        <View style={styles.card}>
          <Text style={styles.plantName}>{selectedPlant.common_name}</Text>
          <Text style={styles.scientific}>{selectedPlant.scientific_name}</Text>

          <DetailRow label="Air Temperature" value={formatValue(selectedPlant.temp_air_c, " °C")} />
          <DetailRow label="Water Temperature" value={formatValue(selectedPlant.temp_water_c, " °C")} />
          <DetailRow label="Soil Moisture" value={fmtSoilBin(selectedPlant.soil_moisture)} />
          <DetailRow label="Humidity" value={formatValue(selectedPlant.humidity, "%")} />
          <DetailRow label="Nutrient" value={formatValue(selectedPlant.nutrient)} />
          <DetailRow label="pH" value={formatValue(selectedPlant.ph)} />
          <DetailRow
            label="Germination Days"
            value={
              selectedPlant.min_germination_days || selectedPlant.max_germination_days
                ? `${selectedPlant.min_germination_days || "?"} - ${selectedPlant.max_germination_days || "?"} days`
                : "Not available"
            }
          />

          <Text style={styles.instructionsTitle}>Instructions</Text>
          <Text style={styles.instructions}>
            {selectedPlant.instructions || "No instructions available."}
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

function DetailRow({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.row}>
      <Text style={styles.rowLabel}>{label}</Text>
      <Text style={styles.rowValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: GreenhouseTheme.bg, padding: 20 },
  title: { fontSize: 30, fontWeight: "800", marginTop: 60, marginBottom: 6, color: GreenhouseTheme.text },
  subtitle: { fontSize: 13, color: GreenhouseTheme.textMuted, marginBottom: 18 },

  search: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 14, padding: 14, fontSize: 16,
    borderWidth: 1, borderColor: GreenhouseTheme.divider, marginBottom: 10,
  },
  suggestionBox: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 14, marginBottom: 18,
    overflow: "hidden", borderWidth: 1, borderColor: GreenhouseTheme.divider,
  },
  suggestionItem: { padding: 14, borderBottomWidth: 1, borderBottomColor: "#e5e7eb" },
  suggestionTitle: { fontSize: 17, fontWeight: "700", color: GreenhouseTheme.text },
  suggestionSub: { fontSize: 13, color: GreenhouseTheme.textMuted, fontStyle: "italic", marginTop: 3 },

  empty: { color: GreenhouseTheme.textMuted, fontSize: 15, lineHeight: 22, marginTop: 12 },

  card: {
    backgroundColor: GreenhouseTheme.card, borderRadius: 18, padding: 20,
    marginBottom: 40, marginTop: 6,
  },
  plantName: { fontSize: 28, fontWeight: "800", color: GreenhouseTheme.text },
  scientific: {
    fontSize: 16, color: GreenhouseTheme.textMuted, fontStyle: "italic",
    marginBottom: 18,
  },

  row: { marginBottom: 12 },
  rowLabel: { fontSize: 13, color: GreenhouseTheme.textMuted, textTransform: "uppercase", letterSpacing: 0.5 },
  rowValue: { fontSize: 18, fontWeight: "700", marginTop: 2, color: GreenhouseTheme.text },

  instructionsTitle: { fontSize: 17, fontWeight: "800", marginTop: 18, marginBottom: 6, color: GreenhouseTheme.text },
  instructions: { fontSize: 15, lineHeight: 22, color: "#374151" },
});
