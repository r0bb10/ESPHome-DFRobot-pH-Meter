# DFRobot pH Probe for ESPHome

ESPHome external component for DFRobot analog pH sensor boards, such as the Gravity Analog pH Meter Kit V2.

The component reads an existing ESPHome voltage sensor, converts that voltage to pH, supports calibration from Home Assistant actions, and stores calibration values in ESPHome preferences.

The voltage source can be any ESPHome sensor that publishes volts, for example:

- ESP32 native ADC using `platform: adc`
- ADS1115 using `platform: ads1115`
- Another external ADC exposed as an ESPHome voltage sensor

The component itself does not read ADC hardware directly.

## Features

- Uses any ESPHome voltage sensor as input
- Works with ESP-IDF and Arduino builds through ESPHome's sensor platforms
- Temperature-compensated pH calculation
- 2-point and 3-point calibration support
- Calibration actions for pH 4, pH 7, pH 10, and reset
- Persistent calibration storage using ESPHome preferences
- Optional status text sensor
- Optional raw voltage, temperature, and slope diagnostic sensors
- Configurable smoothing
- Configurable calibration buffer pH values

## Hardware

| Component | Details |
|----------|---------|
| DFRobot Analog pH Meter | Gravity Analog pH Meter Kit V2 / SEN0161-V2 |
| ADC | ESPHome `adc`, ESPHome `ads1115`, or another voltage sensor |
| Controller | Any ESPHome-supported board with a suitable voltage sensor source |

Native ESP32 ADC can work for simple setups, but an external ADC such as ADS1115 is usually more stable for pH probes.

## Wiring Examples

### ADS1115

```text
DFRobot pH Sensor      ADS1115       ESP32
-----------------      -------       -----
Signal                 A0            I2C ADC input
GND                    GND           GND
VCC                    VDD           3.3V
                       SDA           GPIO21
                       SCL           GPIO22
```

### ESP32 ADC

```text
DFRobot pH Sensor      ESP32
-----------------      -----
Signal                 GPIO36
GND                    GND
VCC                    3.3V
```

## Installation

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/r0bb10/esphome-dfrobot-ph-meter.git
      ref: main
    components: [dfrobot_ph_meter]
```

## Voltage Sensor Requirement

`voltage_sensor` must reference an ESPHome sensor that publishes volts.

The component converts volts to millivolts internally:

```cpp
millivolts = voltage_sensor_state * 1000.0f
```

For example, an ADS1115 state of `1.650 V` is treated as `1650 mV`.

## Configuration Reference

```yaml
dfrobot_ph_meter:
  id: ph_meter
  voltage_sensor: ph_voltage_sensor
  update_interval: 10s
  ph_sensor:
    name: "pH Sensor"
  calibration_mode:
    name: "pH Calibration Mode"
```

### Required Options

| Option | Description |
|--------|-------------|
| `id` | Component ID used by calibration actions |
| `voltage_sensor` | ID of an ESPHome voltage sensor that publishes volts |
| `ph_sensor` | Output pH sensor |
| `calibration_mode` | Switch used to enable calibration mode |

### Optional Options

| Option | Default | Description |
|--------|---------|-------------|
| `update_interval` | `10s` | pH calculation interval. Must be greater than zero. |
| `temperature_sensor` | none | Sensor used for temperature compensation. |
| `temperature_output` | none | Optional sensor that republishes the temperature used by the component. |
| `temperature_unit` | `celsius` | `celsius` or `fahrenheit` for the optional temperature output/log display. |
| `probe_status_sensor` | none | Optional text sensor for component/calibration status. |
| `raw_voltage_sensor` | none | Optional diagnostic sensor publishing the input voltage in millivolts. |
| `slope_sensor` | none | Optional diagnostic sensor publishing the active slope in `mV/pH`. |
| `ph4_solution` | `4.0` | Actual pH value of the low calibration buffer. |
| `ph7_solution` | `7.0` | Actual pH value of the neutral calibration buffer. |
| `ph10_solution` | `10.0` | Actual pH value of the high calibration buffer. |
| `smoothing_alpha` | `0.2` | Exponential smoothing factor. Lower values smooth more heavily. |

### Fallback Calibration Defaults

If no stored calibration exists, the component uses fallback voltages:

| Point | Default Voltage |
|-------|-----------------|
| pH 4 | `2032 mV` |
| pH 7 | `1650 mV` |
| pH 10 | `1268 mV` |

These are only startup fallback values. For accurate readings, calibrate with known buffer solutions.

## Example: ESP32 ADC

```yaml
dfrobot_ph_meter:
  id: ph_meter
  voltage_sensor: ph_voltage_sensor
  update_interval: 10s
  ph_sensor:
    name: "pH Sensor"
    accuracy_decimals: 2
  calibration_mode:
    name: "pH Calibration Mode"
  probe_status_sensor:
    name: "pH Probe Status"
  raw_voltage_sensor:
    name: "pH Raw Voltage"
  slope_sensor:
    name: "pH Current Slope"
  smoothing_alpha: 0.2

sensor:
  - platform: adc
    id: ph_voltage_sensor
    pin: GPIO36
    attenuation: 12db
    update_interval: 1s
    filters:
      - median:
          window_size: 5
          send_every: 1

button:
  - platform: template
    name: "Calibrate pH 4"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph4:
            id: ph_meter
  - platform: template
    name: "Calibrate pH 7"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph7:
            id: ph_meter
  - platform: template
    name: "Calibrate pH 10"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph10:
            id: ph_meter
  - platform: template
    name: "Reset pH Calibration"
    on_press:
      then:
        - dfrobot_ph_meter.reset_calibration:
            id: ph_meter
```

## Example: ADS1115 With Temperature Compensation

```yaml
dfrobot_ph_meter:
  id: ph_meter
  voltage_sensor: ph_voltage_sensor
  update_interval: 10s
  temperature_sensor: water_temp
  ph_sensor:
    name: "pH Sensor"
    accuracy_decimals: 2
  calibration_mode:
    name: "pH Calibration Mode"
  probe_status_sensor:
    name: "pH Probe Status"
  raw_voltage_sensor:
    name: "pH Raw Voltage"
  temperature_output:
    name: "pH Compensation Temperature"
  slope_sensor:
    name: "pH Current Slope"
  smoothing_alpha: 0.2

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
    update_interval: 1s
    filters:
      - median:
          window_size: 5
          send_every: 1
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
            id: ph_meter
  - platform: template
    name: "Calibrate pH 7"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph7:
            id: ph_meter
  - platform: template
    name: "Calibrate pH 10"
    on_press:
      then:
        - dfrobot_ph_meter.calibrate_ph10:
            id: ph_meter
  - platform: template
    name: "Reset pH Calibration"
    on_press:
      then:
        - dfrobot_ph_meter.reset_calibration:
            id: ph_meter
```

## Calibration Workflow

1. Enable the `calibration_mode` switch.
2. Place the probe in the appropriate buffer solution.
3. Wait for the source voltage sensor to stabilize.
4. Press the matching calibration button, for example `Calibrate pH 7`.
5. Repeat for other buffer points if needed.
6. Disable `calibration_mode`, or let it auto-exit after 5 minutes.

Calibration values are saved automatically when a calibration action is triggered while calibration mode is active.

The component can operate with 2-point or 3-point calibration depending on which saved calibration points are available.

## Noise Reduction

Use ESPHome filters on the source voltage sensor for ADC noise and use `smoothing_alpha` for output pH smoothing.

```yaml
dfrobot_ph_meter:
  smoothing_alpha: 0.05
  update_interval: 30s

sensor:
  - platform: adc
    id: ph_voltage_sensor
    pin: GPIO36
    attenuation: 12db
    update_interval: 1s
    filters:
      - median:
          window_size: 10
          send_every: 1
```

Hardware recommendations:

- Use shielded cable for the pH probe signal wire.
- Keep pumps, relays, and switching power supplies away from the pH signal path.
- Ensure common grounding between the pH board, ADC, and ESP device.
- Consider an external ADC such as ADS1115 for better stability.
- Consider an RC low-pass filter on the analog input if needed.

## Validation

The current component has been validated with ESPHome `2026.6.1` using Docker:

- `esphome config` with ESP-IDF
- `esphome config` with Arduino
- `esphome compile` with ESP-IDF
- `esphome compile` with Arduino
