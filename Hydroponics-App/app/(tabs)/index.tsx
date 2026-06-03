import { router } from "expo-router";
import { Pressable, StyleSheet, Text, View } from "react-native";
import { GreenhouseTheme } from "@/constants/greenhouse-theme";

export default function HomeScreen() {
  const goToDashboard = () => {
    router.push("/dashboard");
  };

  return (
    <View style={styles.container}>
      <View style={styles.heroCard}>
        <Text style={styles.logo}>🌱</Text>
        <Text style={styles.title}>Welcome to AutoGreen</Text>
        <Text style={styles.subtitle}>Automated Greenhouse</Text>
        <Text style={styles.description}>
          Track your current plants or start growing something new with smart
          greenhouse support.
        </Text>
      </View>

      <View style={styles.optionsContainer}>
        <Pressable style={styles.primaryButton} onPress={goToDashboard}>
          <Text style={styles.primaryButtonText}>Track Existing Plant</Text>
        </Pressable>

        <Pressable style={styles.secondaryButton} onPress={goToDashboard}>
          <Text style={styles.secondaryButtonText}>Grow New Plant</Text>
        </Pressable>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: GreenhouseTheme.bg,
    padding: 24,
    justifyContent: "center",
  },
  heroCard: {
    backgroundColor: GreenhouseTheme.card,
    borderRadius: 28,
    padding: 28,
    alignItems: "center",
    marginBottom: 28,
  },
  logo: {
    fontSize: 64,
    marginBottom: 14,
  },
  title: {
    fontSize: 32,
    fontWeight: "900",
    color: GreenhouseTheme.text,
    textAlign: "center",
  },
  subtitle: {
    fontSize: 21,
    fontWeight: "800",
    color: GreenhouseTheme.primary,
    marginTop: 6,
    textAlign: "center",
  },
  description: {
    fontSize: 15,
    color: GreenhouseTheme.textMuted,
    textAlign: "center",
    lineHeight: 22,
    marginTop: 18,
  },
  optionsContainer: {
    gap: 14,
  },
  primaryButton: {
    backgroundColor: GreenhouseTheme.primary,
    paddingVertical: 18,
    borderRadius: 18,
    alignItems: "center",
  },
  primaryButtonText: {
    color: "white",
    fontSize: 17,
    fontWeight: "800",
  },
  secondaryButton: {
    backgroundColor: GreenhouseTheme.primarySoft,
    paddingVertical: 18,
    borderRadius: 18,
    alignItems: "center",
    borderWidth: 1,
    borderColor: GreenhouseTheme.primary,
  },
  secondaryButtonText: {
    color: GreenhouseTheme.primaryDark,
    fontSize: 17,
    fontWeight: "800",
  },
});
