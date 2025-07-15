# DFRobot pH Probe for ESPHome

An ESPHome custom component to interface with **DFRobot Analog pH sensors** using either:
- ✅ High-precision external **ADS1115 ADC**
- ✅ Native **ESP32 ADC GPIO** for simple setups

This component enables accurate pH monitoring and calibration for aquarium, hydroponics, and lab automation projects — directly integrated into ESPHome and Home Assistant.



## 🧪 Features

- ⚙️ **Supports dual input modes**:
  - External ADS1115 (16-bit precision)
  - ESP32 internal ADC pin (12-bit)
- 🌡️ **Temperature-compensated pH readings**
- 🔧 **2-point and 3-point calibration**
- 💾 **EEPROM calibration persistence**
- 🎛️ **Switch to toggle calibration mode**
- 🔄 **Calibration actions: pH4, pH7, pH10, Reset**
- 📉 **Smoothed readings using exponential filter**
- 🛠️ **Customizable calibration buffer values** (e.g., pH 3.0, 6.86, etc.)

## 🧰 Hardware Compatibility

| Component | Details |
|----------|---------|
| DFRobot Analog pH Meter | [SKU: SEN0161-V2](https://www.dfrobot.com/product-1025.html) |
| ADS1115 ADC (optional) | I²C 16-bit ADC module |
| ESP32 Dev Board | e.g. ESP32-WROOM-32 |


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

### Option 2: **Using ESP32 ADC GPIO**

```
  DFRobot pH Sensor        ESP32
  -----------------      ---------
  Signal (Yellow)  ───▶  GPIO36 (ADC)
  GND (Black)      ───── GND
  VCC (Red)        ───── 3.3V
```

> ⚠️ When using native ESP32 ADC, expect **less accuracy** and more noise due to its nonlinear nature.


## 📦 Installation

The cleanest way and easiest way to keep your component up-to-date is to install it via GitHub directly.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/r0bb10/esphome-dfrobot-ph-meter.git
      ref: main
    components: [dfrobot_ph_meter]
```

## ⚙️ Configuration

```yaml
# pH Meter Component Configuration
dfrobot_ph_meter:
  id: ph_sensor
  # Input mode (choose one):
  use_ads1115: false   # Set to true to use ADS1115 (default is false — uses ESP32 ADC pin).
  adc_pin: 36          # Specifies ESP32 analog GPIO, ignored if use_ads1115 is true.
  id_ads1115: ads_a0   # Required only when use_ads1115 is true, ignored if use_ads1115 is false.
  channel: 0           # ADS1115 channel (0–3), ignored if use_ads1115 is false.
  # Optional: Output current temperature as a sensor
  temperature_output:
    name: "Water Temperature (used by pH)"
    accuracy_decimals: 1
  # Optional: Set temperature unit for display & output
  temperature_unit: celsius
  # Optional: Time between readings (default: 10s)
  update_interval: 10s
  # Output pH value
  ph_sensor:
    name: "pH Sensor"
    accuracy_decimals: 2
  # Toggle switch to enable calibration mode
  calibration_mode:
    name: "pH Calibration Mode"
  # Text sensor showing current operation status
  probe_status_sensor:
    name: "Probe Status"
  # Optional sensors for diagnostics
  raw_voltage_sensor:
    name: "pH Raw Voltage"
  # Optional: Custom calibration buffer values
  ph4_solution: 3.02   # Default: 4.0
  ph7_solution: 6.86   # Default: 7.0
  ph10_solution: 9.18  # Default: 10.0
```

```yaml
sensor:
  - platform: ads1115
    id: ph_voltage_sensor
    multiplexer: A0_GND
    gain: 4.096
  - platform: dallas_temp
    name: "Water Temperature"
    id: water_temp
    update_interval: 5s

button:
  - platform: template
    name: "Calibrate pH 4"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph4:
            id: ph1
  - platform: template
    name: "Calibrate pH 7"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph7:
            id: ph1
  - platform: template
    name: "Calibrate pH 10"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph10:
            id: ph1
  - platform: template
    name: "Reset pH Calibration"
    on_press:
      then:
        - dfrobot_ph_meter.reset_calibration:
            id: ph1
```


## 🧪 Notes

- Use `calibration_mode` switch to enter calibration.
- Place probe in buffer solution (e.g. pH 7), wait for stable reading, then trigger calibration.
- Component automatically saves calibration voltage and exits after timeout (5 min).
