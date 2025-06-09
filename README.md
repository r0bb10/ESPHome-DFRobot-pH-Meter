# 🌊 dfrobot_ph_meter

An ESPHome custom component to interface with **DFRobot Analog pH sensors** using either:
- ✅ High-precision external **ADS1115 ADC**
- ✅ Native **ESP32 ADC GPIO** for simple setups

This component enables accurate pH monitoring and calibration for aquarium, hydroponics, and lab automation projects — directly integrated into ESPHome and Home Assistant.

---

## 🧪 Features

- ⚙️ **Supports dual input modes**:
  - External ADS1115 (16-bit precision)
  - ESP32 internal ADC pin (12-bit)
- 🌡️ **Temperature-compensated pH readings**
- 🔧 **2-point and 3-point calibration**
- 📈 **Optional slope, voltage, and probe status sensors**
- 💾 **EEPROM calibration persistence**
- 🎛️ **Switch to toggle calibration mode**
- 🔄 **Calibration actions: pH4, pH7, pH10, Reset**
- 🧼 **Auto-exit calibration after timeout**
- 📉 **Smoothed readings using exponential filter**
- 🧠 **Optimized for low overhead and power-efficient loop**

---

## 🧰 Hardware Compatibility

| Component | Details |
|----------|---------|
| DFRobot Analog pH Meter | [SKU: SEN0161-V2](https://www.dfrobot.com/product-1025.html) |
| ADS1115 ADC (optional) | I²C 16-bit ADC module |
| ESP32 Dev Board | e.g. ESP32-WROOM-32 |

---

## 🔌 Wiring Diagrams

### Option 1: **Using ADS1115**

```
  DFRobot pH Sensor        ADS1115         ESP32
  -----------------      ---------       ---------
  Signal (Yellow)  ───▶  A0              (via I²C SDA/SCL)
  GND (Black)      ───── GND             GND
  VCC (Red)        ───── VDD             3.3V
                      SDA ────────────── GPIO21
                      SCL ────────────── GPIO22
```

---

### Option 2: **Using ESP32 ADC GPIO**

```
  DFRobot pH Sensor        ESP32
  -----------------      ---------
  Signal (Yellow)  ───▶  GPIO36 (ADC)
  GND (Black)      ───── GND
  VCC (Red)        ───── 3.3V
```

> ⚠️ When using native ESP32 ADC, expect **less accuracy** and more noise due to its nonlinear nature.

---

## 📦 Installation

1. Copy the component files into your ESPHome project under a folder named `dfrobot_ph_meter/`.
2. Reference the folder using `external_components`.

---

## 📄 Example ESPHome Configuration (Component Only)

```yaml
external_components:
  - source: ./dfrobot_ph_meter

# 📦 pH Meter Component Configuration
dfrobot_ph_meter:
  id: ph_sensor

  # Input mode (choose one):
  use_ads1115: true        # Set to false to use ESP32 GPIO
  id_ads1115: ads_a0       # Required if using ADS1115
  channel: 0               # ADS1115 channel (0–3)

  # adc_pin: 36            # Uncomment if using native ADC mode instead of ADS1115

  # 📏 Calibration reference voltages (in millivolts)
  acid_voltage: 2032.0     # pH 4.0
  neutral_voltage: 1650.0  # pH 7.0
  alkaline_voltage: 1268.0 # pH 10.0 (optional, used in 3-point mode)

  # 🌡️ Optional temperature sensor input (in °C)
  temperature_sensor: temp_sensor_id     # Reference to an existing temperature sensor

  # ⚙️ Optional sensors for diagnostics
  raw_voltage_sensor:
    name: "pH Raw Voltage"
  slope_sensor:
    name: "pH Slope"
  temperature_output:
    name: "pH Temperature"
  probe_status_sensor:
    name: "pH Probe Status"

  # 🧪 Main pH output
  ph_sensor:
    name: "Aquarium pH"

  # 🔘 Toggle for calibration mode
  calibration_mode:
    name: "pH Calibration Mode"

  # 🔁 Update interval (how often to read and publish)
  update_interval: 10s

  # ⚖️ Calibration configuration
  calibration_mode_type: "3_point"   # "2_point" or "3_point"
  calibration_points: ["pH4", "pH7"] # Used if 2-point calibration is selected

  # 🌡️ Optional temperature unit
  temperature_unit: celsius          # or "fahrenheit"
```

> ✅ You can reference a temperature sensor defined elsewhere in your ESPHome config:
```yaml
sensor:
  - platform: dallas
    address: 0x0102030405060708
    id: temp_sensor_id
```

---

## ⚙️ Calibration Actions

You can trigger calibration via Home Assistant or YAML automations:

```yaml
button:
  - platform: template
    name: "Calibrate pH 7"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph7: ph_sensor

  - platform: template
    name: "Reset Calibration"
    on_press:
      then:
        - dfrobot_ph_meter.reset_calibration: ph_sensor
```

---

## 🧪 Notes

- Use `calibration_mode` switch to enter calibration.
- Place probe in buffer solution (e.g. pH 7), wait for stable reading, then trigger calibration.
- Component automatically saves calibration voltage and exits after timeout (5 min).

---

## 📘 License

MIT License — open-source and free to use in both commercial and personal projects.
