#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace dfrobot_ph_meter {

class DigitalSwitch : public switch_::Switch {
 public:
  void write_state(bool state) override {
    this->publish_state(state);
    this->pin_state_ = state;
  }
  bool pin_state_;
};

class DFRobotPHMeter : public Component {
 public:
  void setup() override;
  void loop() override;

  void set_ads1115_sensor(sensor::Sensor *adc) { ads1115_ = adc; }
  void set_channel(int ch) { channel_ = ch; }

  void set_acid_voltage(float v) {
    acid_voltage_ = v;
    acid_voltage_default_ = v;
  }
  void set_neutral_voltage(float v) {
    neutral_voltage_ = v;
    neutral_voltage_default_ = v;
  }

  void set_temperature(float t) { temperature_ = t; }
  void set_update_interval(uint32_t interval) { update_interval_ = interval; }

  void set_use_fahrenheit(bool value) { use_fahrenheit_ = value; }

  void set_calibration_pair(int p1, int p2) {
    cal_point_1_ = p1;
    cal_point_2_ = p2;
  }

  void set_ph_sensor(sensor::Sensor *s) { ph_sensor_ = s; }
  void set_temperature_output_sensor(sensor::Sensor *s) { temperature_output_sensor_ = s; }
  void set_calibration_mode_switch(DigitalSwitch *sw) { calibration_mode_switch_ = sw; }
  void set_temperature_sensor(sensor::Sensor *s) { temperature_sensor_ = s; }
  void set_status_sensor(text_sensor::TextSensor *s) { probe_status_sensor_ = s; }
  void set_raw_voltage_sensor(sensor::Sensor *s) { raw_voltage_sensor_ = s; }
  void set_slope_sensor(sensor::Sensor *s) { current_slope_sensor_ = s; }

  void set_input_mode_ads1115() { input_mode_ = MODE_ADS1115; }
  void set_input_mode_native_adc(int gpio) { input_mode_ = MODE_NATIVE_ADC; adc_gpio_ = gpio; }

  void reset_calibration();
  void set_calibration_stage(int stage);
  void evaluate_calibration_mode_();

 protected:
  void update_probe_status_();
  void check_reset_status_();
  bool save_calibration_voltage_(ESPPreferenceObject &pref, float &internal_value, float new_value, const char *label);
  float get_temperature_() const;
  float clamp_ph_(float ph) const;
  float calculate_ph_(float voltage, float temp, float &out_slope);
  void log_readings_(float voltage, float temp, float slope, float ph);

  enum InputMode { MODE_ADS1115, MODE_NATIVE_ADC };

  sensor::Sensor *ads1115_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *ph_sensor_{nullptr};
  sensor::Sensor *temperature_output_sensor_{nullptr};
  sensor::Sensor *raw_voltage_sensor_{nullptr};
  sensor::Sensor *current_slope_sensor_{nullptr};
  text_sensor::TextSensor *probe_status_sensor_{nullptr};
  DigitalSwitch *calibration_mode_switch_{nullptr};

  int channel_;
  float acid_voltage_ = 2032.0f;
  float neutral_voltage_ = 1650.0f;
  float alkaline_voltage_ = 1268.0f;
  float acid_voltage_default_ = 2032.0f;
  float neutral_voltage_default_ = 1650.0f;
  float alkaline_voltage_default_ = 1268.0f;
  float temperature_{25.0f};
  uint32_t update_interval_{10000};
  uint32_t last_update_{0};
  uint32_t status_reset_timer_{0};
  uint32_t calibration_entered_at_ = 0;
  uint32_t last_calibration_write_{0};
  static constexpr uint32_t CALIBRATION_WRITE_COOLDOWN_MS = 2000;
  static constexpr uint32_t CALIBRATION_TIMEOUT_MS = 300000;

  bool use_three_point_{true};
  bool voltage_initialized_{false};
  bool use_fahrenheit_{false};

  float smoothed_ph_{NAN};
  static constexpr float PH_SMOOTHING_ALPHA = 0.2f;

  int cal_point_1_{4};
  int cal_point_2_{7};

  enum CalibrationStage { NONE, PH4, PH7, PH10 };
  CalibrationStage calibration_stage_{NONE};

  ESPPreferenceObject acid_voltage_pref_;
  ESPPreferenceObject neutral_voltage_pref_;
  ESPPreferenceObject alkaline_voltage_pref_;

  static constexpr float MIN_VALID_VOLTAGE = 400.0f;
  static constexpr float MAX_VALID_VOLTAGE = 3000.0f;
  static constexpr float DEFAULT_TEMPERATURE = 25.0f;
  static constexpr float MIN_CALIBRATION_VOLTAGE = 500.0f;

  InputMode input_mode_{MODE_ADS1115};
  int adc_gpio_{-1};
};

class CalibratePHAction : public esphome::Action<> {
 public:
  explicit CalibratePHAction(DFRobotPHMeter *parent) : parent_(parent) {}
  void set_stage(int stage) { stage_ = stage; }
  void play() override;

 protected:
  DFRobotPHMeter *parent_;
  int stage_;
};

}  // namespace dfrobot_ph_meter
}  // namespace esphome
