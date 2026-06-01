import { View, Text, StyleSheet } from "react-native";

import { GreenhouseTheme } from "@/constants/greenhouse-theme";

/**
 * Two-line row used throughout the dashboard:
 *   <label>
 *   <current dark bold>  (<target lighter>)
 *
 * Either field is optional. If neither is present we render an em-dash.
 */
export function InfoRow({
  label,
  current,
  target,
}: {
  label: string;
  current?: string;
  target?: string;
}) {
  const hasCurrent = current !== undefined && current !== "";
  const hasTarget = target !== undefined && target !== "";

  return (
    <View style={styles.row}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.valueLine}>
        {hasCurrent ? (
          <Text style={styles.current}>{current}</Text>
        ) : (
          <Text style={styles.missing}>—</Text>
        )}
        {hasTarget && (
          <Text style={styles.target}>
            {hasCurrent ? "  " : " "}({target})
          </Text>
        )}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  row: {
    marginBottom: 14,
  },
  label: {
    fontSize: 13,
    color: GreenhouseTheme.textMuted,
    textTransform: "uppercase",
    letterSpacing: 0.5,
  },
  valueLine: {
    fontSize: 20,
    marginTop: 4,
  },
  current: {
    color: GreenhouseTheme.text,
    fontWeight: "800",
  },
  target: {
    color: GreenhouseTheme.textLight,
    fontWeight: "400",
  },
  missing: {
    color: GreenhouseTheme.textLight,
    fontWeight: "600",
  },
});
