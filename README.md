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
  Signal (Yellow)  ───▶  A0             (= I²C ch.0)
  GND (Black)      ───── GND             GND
  VCC (Red)        ───── VDD             3.3V
                         SDA ─────────── GPIO21
                         SCL ─────────── GPIO22
```

### Option 2: **Using ESP32 ADC GPIO**

```
  DFRobot pH Sensor         ESP32
  -----------------       ---------
  Signal (Yellow)  ───▶   GPIO36 (ADC)
  GND (Black)      ─────── GND
  VCC (Red)        ─────── 3.3V
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

### Example 1: Using ESP32 ADC (No ADS1115) and no Temp compensation (defaults to 25°C)
```yaml
# pH Meter Component Configuration
dfrobot_ph_meter:
  id: ph_sensor
  use_ads1115: false 
  adc_pin: 36
  update_interval: 10s  # Optional: Time between readings (default: 10s)
  ph_sensor:
    name: "pH Sensor"
    accuracy_decimals: 2
  calibration_mode:
    name: "pH Calibration Mode"
  probe_status_sensor:
    name: "Probe Status"
  # Optional sensors for diagnostics
  raw_voltage_sensor:
    name: "pH Raw Voltage"
  # Optional: Custom calibration buffer values
  ph4_solution: 3.02   # Default: 4.0
  ph7_solution: 6.86   # Default: 7.0
  ph10_solution: 9.18  # Default: 10.0

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

### Example 2: Using ADS1115 with the ESP32 and Temp compensation
```yaml
# pH Meter Component Configuration
dfrobot_ph_meter:
  id: ph_sensor
  use_ads1115: true
  channel: 0
  id_ads1115: ph_voltage_sensor
  update_interval: 10s 
  temperature_sensor: water_temp  # If specified is used for compensation
  ph_sensor:
    name: "pH Sensor"
    accuracy_decimals: 2
  calibration_mode:
    name: "pH Calibration Mode"
  probe_status_sensor:
    name: "Probe Status"
  # Optional sensors for diagnostics
  raw_voltage_sensor:
    name: "pH Raw Voltage"
  # Optional: Custom calibration buffer values
  ph4_solution: 3.02   # Default: 4.0
  ph7_solution: 6.86   # Default: 7.0
  ph10_solution: 9.18  # Default: 10.0

one_wire:
  - platform: gpio
    pin: GPIO22

ads1115:
  - address: 0x48

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
