export const GreenhouseTheme = {
  bg:           "#eef7ef",
  card:         "#ffffff",
  divider:      "#e5e7eb",
  text:         "#111827",
  textMuted:    "#6b7280",
  textLight:    "#9ca3af",
  primary:      "#16a34a",
  primaryDark:  "#15803d",
  primarySoft:  "#dcfce7",
  accent:       "#0ea5e9",
  warn:         "#f59e0b",
  danger:       "#ef4444",

  // State pills
  stateIdle:    "#94a3b8",
  statePump:    "#0ea5e9",
  stateHeat:    "#ef4444",
  stateVent:    "#22c55e",

  // Status pill (connection)
  statusOk:     "#10b981",
  statusWarn:   "#f59e0b",
  statusBad:    "#ef4444",
};

export const actionColor = (action?: string): string => {
  switch (action) {
    case "PUMPING": return GreenhouseTheme.statePump;
    case "HEATING": return GreenhouseTheme.stateHeat;
    case "VENTING": return GreenhouseTheme.stateVent;
    default:        return GreenhouseTheme.stateIdle;
  }
};

/**
 * For a composite state string like "HEATING+VENTING", color by the first
 * action; "IDLE" / empty returns the muted color.
 */
export const stateColor = (state?: string): string => {
  if (!state || state === "IDLE") return GreenhouseTheme.stateIdle;
  const first = state.split("+")[0];
  return actionColor(first);
};
