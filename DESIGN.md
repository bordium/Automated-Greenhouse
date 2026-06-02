# Automated Greenhouse — Design Notes

Two halves of the system:

```
+-----------------+        MQTT (broker.emqx.io)       +-----------------+
|  ESP32-S3 Mini  | <---- greenhouse/dev/cmd/* ------- | Hydroponics-App |
|  (firmware/)    | ----- greenhouse/dev/telemetry --> |   (Expo / RN)   |
|  + ST7789 LCD   |                                    |                 |
+-----------------+                                    +-----------------+
        |
        |  Sensors: SHT45 (air T/RH), DS18B20 (water T), soil ADC
        |  Actuators: pump, fan, heater1, heater2, LED (all PWM)
```

The broker and topic prefix are configured the same way on both sides:
- Firmware: `MQTT_TOPIC_PREFIX` in `esp32/.env`
- App:      `MQTT_TOPIC_PREFIX` in `Hydroponics-App/constants/mqtt.ts`

Default prefix is `greenhouse/dev`. **They must match.**

---

## Pin map (ESP32-S3 Mini-1)

| Pin    | Function                            |
|--------|-------------------------------------|
| IO1    | I2C SDA (SHT45)                     |
| IO2    | I2C SCL (SHT45)                     |
| IO3    | DS18B20 OneWire (KIT0021)           |
| IO4    | Soil moisture analog (ADC1)         |
| IO5    | TFT backlight                       |
| IO6    | TFT reset                           |
| IO7    | TFT DC                              |
| IO8    | TFT CS                              |
| IO9    | TFT SCLK                            |
| IO10   | TFT MOSI                            |
| IO21   | LED (PWM)                           |
| IO26   | Heater 2 (PWM, 5% duty when on)     |
| IO42   | Fan (PWM)                           |
| IO45   | Water pump (PWM)                    |
| IO47   | Heater 1 (PWM, 5% duty when on)     |

---

## Soil-moisture conversion

The probe gives an analog voltage that ESP32 reads via `analogRead()`:

| Raw ADC | Condition |
|---------|-----------|
| ~1400   | submerged (100% wet)  |
| ~2880   | air dry (0% moisture) |

Conversion:
```
pct = clamp((SOIL_ADC_DRY - raw) / (SOIL_ADC_DRY - SOIL_ADC_WET) * 100, 0, 100)
```

Calibration constants live at [esp32/include/Config.h](esp32/include/Config.h)
(`SOIL_ADC_WET` / `SOIL_ADC_DRY`). Watch the `[SOIL]` line on serial output
after reflashing to recalibrate.

Database "bin" mapping (used as the AUTO target):
```
bin 1 -> 20%   (dry-loving)
bin 2 -> 40%   (moderate)
bin 3 -> 60%   (moisture-loving)
```

Lettuce, for example, lists `soil_moisture: "3"` → target 60%.

---

## MQTT contract

All topics are under `${MQTT_TOPIC_PREFIX}/`.

### Firmware → App: `telemetry` (published every 1 s, JSON)

```jsonc
{
  "state":          "IDLE | PUMPING | HEATING | VENTING | PUMPING+HEATING | ...",
  "actions":        ["PUMPING", "HEATING"],   // each currently-active action
  "mode":           "AUTO | MANUAL",
  "humidity":       55.3,       // % RH or null on sensor fail
  "air_temp":       21.2,       // °C or null
  "water_temp":     14.7,       // °C or null
  "water_level":    "OK",       // "OK" or "NOT OK" (proxy: DS18B20 health)
  "soil_pct":       42.1,       // 0..100 (or null on sensor fail)
  "soil_bin":       2,          // 1, 2, or 3 (nearest bin)
  "pump":           0,          // 0..255 duty
  "fan":            128,        // 0..255 duty
  "heater1":        12,         // 0..255 duty (AUTO uses 12 ≈ 5%)
  "heater2":        12,
  "led":            255,        // 0..255 duty
  "uptime_ms":      123456,
  "targets": {                  // echoed back so the app stays in sync
    "air_temp":      16.0,
    "water_temp":    13.0,
    "humidity":      65.0,
    "soil_bin":      3
  }
}
```

### App → Firmware: `cmd/*`

| Topic                 | Payload                              | Notes |
|-----------------------|--------------------------------------|-------|
| `cmd/mode`            | `"AUTO"` or `"MANUAL"`               | switches FSM source |
| `cmd/targets`         | JSON (see below)                     | overwrite any subset of targets |
| `cmd/pump`            | `0..255` (PWM duty)                  | manual mode only; 0 = off |
| `cmd/fan`             | `0..255` (PWM duty)                  | manual mode only |
| `cmd/heater1`         | `0..255` (PWM duty)                  | manual mode only; AUTO uses 5 % |
| `cmd/heater2`         | `0..255` (PWM duty)                  | manual mode only; AUTO uses 5 % |
| `cmd/led`             | `0..255` (PWM duty)                  | always honored regardless of mode |

`cmd/targets` payload:
```json
{ "air_temp": 16.0, "water_temp": 13.0, "humidity": 65.0, "soil_bin": 3 }
```
Every key is optional — only the keys present are updated.

---

## State machine (AUTO mode)

Three **independent** control loops, re-evaluated every sensor cycle (1 s).
Any combination of actions can be active simultaneously (e.g. HEATING +
VENTING to warm air while exhausting humidity).

| Action  | Turns ON when                                                     | Turns OFF when                                              | Extra timing                                |
|---------|--------------------------------------------------------------------|--------------------------------------------------------------|---------------------------------------------|
| PUMPING | `soil_pct < target_pct − 5`                                        | After `PUMP_RUN_MS` (5 s) of continuous run                  | `PUMP_COOLDOWN_MS` (30 s) before re-arming  |
| HEATING | `air_temp ≤ target_air − 2`                                        | `air_temp ≥ target_air`                                      | Pure deadband hysteresis                    |
| VENTING | `air_temp ≥ target_air + 2`  **or** `humidity ≥ target_humid + 5`  | `air_temp ≤ target_air` **and** `humidity ≤ target_humid`    | Pure deadband hysteresis                    |

Outputs per active action (additive — flags OR together):

| Active flag(s) | Pump | Fan | Heater1 | Heater2 | LED       |
|----------------|------|-----|---------|---------|-----------|
| none (IDLE)    | off  | off | off     | off     | user duty |
| PUMPING        | ON   | -   | -       | -       | user duty |
| VENTING        | -    | ON  | -       | -       | user duty |
| HEATING        | -    | -   | ON (5%) | ON (5%) | user duty |

(`-` means "unchanged by this flag" — the actuator's state is determined by
whichever flags are set; if no flag claims it, it's off.)

LED is intentionally outside the FSM — it's always driven by the most recent
`cmd/led`, defaulting to 255 (full on).

In **MANUAL** mode the FSM stops touching actuators entirely; only the app's
`cmd/*` topics drive them.

---

## App tabs

1. **Dashboard** — live readings vs targets (current value dark bold, target lighter in parentheses), big state pill, connection indicator.
2. **Control** — Mode toggle (AUTO/MANUAL), preset picker (loads target from the plant database), four numeric target inputs, five actuator switches/sliders.
3. **Plants** — search the existing plant database, view full row.

---

## Files of interest

- `esp32/include/Config.h` — single source of truth for pins, channels, thresholds, MQTT topic keys.
- `esp32/src/main.cpp` — wiring of MQTT, FSM, and the per-second loop.
- `esp32/src/Sensors.cpp` — sensor reads (SHT45 / DS18B20 / soil analog).
- `esp32/src/StateMachine.cpp` — AUTO-mode decisions.
- `Hydroponics-App/hooks/use-mqtt-telemetry.ts` — telemetry subscribe.
- `Hydroponics-App/hooks/use-mqtt-publish.ts` — `cmd/*` publish.
- `Hydroponics-App/app/(tabs)/` — Dashboard / Control / Plants screens.
