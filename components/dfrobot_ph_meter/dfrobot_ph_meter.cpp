#include "dfrobot_ph_meter.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#if defined(ESP_PLATFORM)
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#endif

namespace esphome {
namespace dfrobot_ph_meter {

static const char *const TAG = "DFRobotPHMeter";
static constexpr float NERNST_REFERENCE_TEMP = 25.0f;
static constexpr float KELVIN_OFFSET = 273.15f;

void DFRobotPHMeter::setup() {
  // Initialize preferences and load stored calibration voltages
  acid_voltage_pref_ = global_preferences->make_preference<float>(fnv1_hash("ph4_voltage"));
  neutral_voltage_pref_ = global_preferences->make_preference<float>(fnv1_hash("ph7_voltage"));
  alkaline_voltage_pref_ = global_preferences->make_preference<float>(fnv1_hash("ph10_voltage"));

  float stored;
  if (acid_voltage_pref_.load(&stored) && stored > MIN_CALIBRATION_VOLTAGE)
    acid_voltage_ = stored;
  else
    acid_voltage_ = acid_voltage_default_;

  if (neutral_voltage_pref_.load(&stored) && stored > MIN_CALIBRATION_VOLTAGE)
    neutral_voltage_ = stored;
  else
    neutral_voltage_ = neutral_voltage_default_;

  if (alkaline_voltage_pref_.load(&stored) && stored > MIN_CALIBRATION_VOLTAGE)
    alkaline_voltage_ = stored;
  else
    alkaline_voltage_ = alkaline_voltage_default_;

  // ADC configuration for native ADC mode
  if (input_mode_ == MODE_NATIVE_ADC && adc_gpio_ >= 0) {
#if defined(ARDUINO)
    // Arduino framework: set ADC resolution and attenuation
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
#elif defined(ESP_PLATFORM)
    // ESP-IDF: configure ADC oneshot unit and channel
    static adc_oneshot_unit_handle_t adc_handle = nullptr;
    static adc_oneshot_chan_cfg_t chan_cfg;
    static bool adc_initialized = false;
    if (!adc_initialized) {
      adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
      };
      adc_oneshot_new_unit(&init_cfg, &adc_handle);
      chan_cfg.bitwidth = ADC_BITWIDTH_12;
      chan_cfg.atten = ADC_ATTEN_DB_12;
      adc_oneshot_config_channel(adc_handle, static_cast<adc_channel_t>(adc_gpio_ - 32), &chan_cfg);
      adc_initialized = true;
    }
#endif
  }

  // Load custom calibration solutions from YAML
  ph4_solution_ = 4.0f;
  ph7_solution_ = 7.0f;
  ph10_solution_ = 10.0f;
}

void DFRobotPHMeter::reset_calibration() {
  // Reset calibration voltages to defaults and save them
  save_calibration_voltage_(acid_voltage_pref_, acid_voltage_, acid_voltage_default_, "pH4");
  save_calibration_voltage_(neutral_voltage_pref_, neutral_voltage_, neutral_voltage_default_, "pH7");
  save_calibration_voltage_(alkaline_voltage_pref_, alkaline_voltage_, alkaline_voltage_default_, "pH10");

  ESP_LOGI(TAG, "Calibration reset to default voltages: pH4=%.2f, pH7=%.2f, pH10=%.2f",
           acid_voltage_, neutral_voltage_, alkaline_voltage_);

  if (probe_status_sensor_) probe_status_sensor_->publish_state("RESET_DONE");
  calibration_stage_ = NONE;
#if defined(ARDUINO)
  status_reset_timer_ = millis();
#elif defined(ESP_PLATFORM)
  status_reset_timer_ = static_cast<uint32_t>(esp_timer_get_time() / 1000); // microseconds to ms
#endif
}

bool DFRobotPHMeter::save_calibration_voltage_(ESPPreferenceObject &pref, float &internal_value, float new_value, const char *label) {
  // Save calibration voltage if it has changed significantly
  if (fabs(internal_value - new_value) > 0.1f) {
    internal_value = new_value;
    pref.save(&internal_value);
    ESP_LOGI(TAG, "Saved %s calibration: %.2f mV", label, new_value);
    return true;
  }
  return false;
}

void DFRobotPHMeter::set_calibration_stage(int stage) {
  // Set the current calibration stage based on the provided pH value
  if (stage == 4) calibration_stage_ = PH4;
  else if (stage == 7) calibration_stage_ = PH7;
  else if (stage == 10) calibration_stage_ = PH10;
}

void DFRobotPHMeter::update_probe_status_() {
  // Update the status of the probe based on its current state
  if (!probe_status_sensor_) return;

  if (!voltage_initialized_) {
    probe_status_sensor_->publish_state("Booting");
    return;
  }

  if (calibration_mode_switch_ && calibration_mode_switch_->state) {
    switch (calibration_stage_) {
      case PH4: probe_status_sensor_->publish_state("Calibrating pH 4"); break;
      case PH7: probe_status_sensor_->publish_state("Calibrating pH 7"); break;
      case PH10: probe_status_sensor_->publish_state("Calibrating pH 10"); break;
      default: probe_status_sensor_->publish_state("Calibration Mode"); break;
    }
    return;
  }

  probe_status_sensor_->publish_state("Measuring");
}

float DFRobotPHMeter::get_temperature_() const {
  // Retrieve the current temperature from the sensor or use the default value
  float t = temperature_;
  if (temperature_sensor_ && temperature_sensor_->has_state())
    t = temperature_sensor_->state;
  return t;
}

float DFRobotPHMeter::clamp_ph_(float ph) const {
  // Ensure the pH value is within the valid range (0-14)
  if (ph < 0.0f) return 0.0f;
  if (ph > 14.0f) return 14.0f;
  return ph;
}

float DFRobotPHMeter::calculate_ph_(float voltage, float temp_c, float &out_slope) {
  // Calculate the pH value based on voltage, temperature, and calibration points
  if (use_three_point_) {
    out_slope = (voltage < neutral_voltage_)
                  ? (neutral_voltage_ - acid_voltage_) / (ph7_solution_ - ph4_solution_)
                  : (alkaline_voltage_ - neutral_voltage_) / (ph10_solution_ - ph7_solution_);
  } else {
    float v1, v2;
    float p1 = cal_point_1_;
    float p2 = cal_point_2_;
    if ((p1 == 4 && p2 == 7) || (p1 == 7 && p2 == 4)) {
      v1 = acid_voltage_;
      v2 = neutral_voltage_;
    } else if ((p1 == 7 && p2 == 10) || (p1 == 10 && p2 == 7)) {
      v1 = neutral_voltage_;
      v2 = alkaline_voltage_;
    } else {
      v1 = acid_voltage_;
      v2 = alkaline_voltage_;
    }
    out_slope = (p2 - p1) / (v2 - v1);
  }

  const float temp_factor = (temp_c + KELVIN_OFFSET) / (NERNST_REFERENCE_TEMP + KELVIN_OFFSET);
  out_slope *= temp_factor;

  return 7.0f + (voltage - neutral_voltage_) / out_slope;
}

void DFRobotPHMeter::log_readings_(float voltage, float temp, float slope, float ph) {
  ESP_LOGD(TAG, "Voltage: %.2f mV | Temp: %.2f °C | Slope: %.4f | pH: %.2f", voltage, temp, slope, ph);
}

void DFRobotPHMeter::evaluate_calibration_mode_() {
  // Determine the calibration mode based on stored voltages
  float ph4_v = 0.0f, ph7_v = 0.0f, ph10_v = 0.0f;

  // Load stored voltages
  acid_voltage_pref_.load(&ph4_v);
  neutral_voltage_pref_.load(&ph7_v);
  alkaline_voltage_pref_.load(&ph10_v);

  // Only consider as calibrated if different from default
  bool has_ph4 = fabs(ph4_v - acid_voltage_default_) > 1.0f;
  bool has_ph7 = fabs(ph7_v - neutral_voltage_default_) > 1.0f;
  bool has_ph10 = fabs(ph10_v - alkaline_voltage_default_) > 1.0f;

  int count = int(has_ph4) + int(has_ph7) + int(has_ph10);

  if (count >= 3) {
    use_three_point_ = true;
    ESP_LOGI(TAG, "Detected 3-point calibration (pH4, pH7, pH10)");
  } else if (count == 2) {
    use_three_point_ = false;
    if (has_ph4 && has_ph7)
      set_calibration_pair(4, 7);
    else if (has_ph7 && has_ph10)
      set_calibration_pair(7, 10);
    else
      set_calibration_pair(4, 10);
    ESP_LOGI(TAG, "Detected 2-point calibration (pH%d, pH%d)", cal_point_1_, cal_point_2_);
  } else {
    use_three_point_ = true;
    ESP_LOGI(TAG, "Insufficient calibration — using default 3-point mode");
  }
}

void DFRobotPHMeter::check_reset_status_() {
  // Check if the reset status timer has expired and update the status
  uint32_t now;
#if defined(ARDUINO)
  now = millis();
#elif defined(ESP_PLATFORM)
  now = static_cast<uint32_t>(esp_timer_get_time() / 1000); // microseconds to ms
#endif
  if (status_reset_timer_ > 0 && now - status_reset_timer_ > 10000) {
    status_reset_timer_ = 0;
    if (probe_status_sensor_) probe_status_sensor_->publish_state("IDLE");
  }
}

void DFRobotPHMeter::loop() {
  uint32_t now;
#if defined(ARDUINO)
  now = millis();
#elif defined(ESP_PLATFORM)
  now = static_cast<uint32_t>(esp_timer_get_time() / 1000); // microseconds to ms
#endif
  if (now - last_update_ < update_interval_) return;
  last_update_ = now;

  update_probe_status_();

  float voltage = 0.0f;

  if (input_mode_ == MODE_ADS1115) {
    if (!ads1115_ || !ads1115_->has_state()) return;
    voltage = ads1115_->state * 1000.0f;
  } else if (input_mode_ == MODE_NATIVE_ADC) {
    if (adc_gpio_ < 0) return;
#if defined(ARDUINO)
    int raw = analogRead(adc_gpio_);
    voltage = (raw / 4095.0f) * 3300.0f;
#elif defined(ESP_PLATFORM)
    // ESP-IDF: use ADC oneshot API for GPIOs 32-39
    static adc_oneshot_unit_handle_t adc_handle = nullptr;
    int raw = 0;
    if (adc_handle && adc_gpio_ >= 32 && adc_gpio_ <= 39) {
      adc_oneshot_read(adc_handle, static_cast<adc_channel_t>(adc_gpio_ - 32), &raw);
      voltage = (raw / 4095.0f) * 3300.0f;
    } else {
      voltage = 0.0f;
    }
#endif
  }

  if (voltage < MIN_VALID_VOLTAGE || voltage > MAX_VALID_VOLTAGE) return;

  if (!voltage_initialized_) {
    ESP_LOGI(TAG, "First valid voltage received, starting pH calculation");
    voltage_initialized_ = true;
  }

  if (raw_voltage_sensor_) raw_voltage_sensor_->publish_state(voltage);

  if (calibration_mode_switch_ && calibration_mode_switch_->state) {
    if (calibration_entered_at_ == 0) {
      calibration_entered_at_ = now;
      ESP_LOGI(TAG, "Entered calibration mode — will auto-exit after 5 minutes");
    } else if (now - calibration_entered_at_ > CALIBRATION_TIMEOUT_MS) {
      calibration_mode_switch_->publish_state(false);
      ESP_LOGI(TAG, "Auto-exiting calibration mode after timeout");
    }

    if (calibration_stage_ != NONE && now - last_calibration_write_ > CALIBRATION_WRITE_COOLDOWN_MS) {
      if (calibration_stage_ == PH4)
        save_calibration_voltage_(acid_voltage_pref_, acid_voltage_, voltage, "pH4");
      else if (calibration_stage_ == PH7)
        save_calibration_voltage_(neutral_voltage_pref_, neutral_voltage_, voltage, "pH7");
      else if (calibration_stage_ == PH10)
        save_calibration_voltage_(alkaline_voltage_pref_, alkaline_voltage_, voltage, "pH10");

      calibration_stage_ = NONE;
      last_calibration_write_ = now;
    }
    return;
  } else if (calibration_entered_at_ != 0) {
    calibration_entered_at_ = 0;
    evaluate_calibration_mode_();
  }

  float temp_c = get_temperature_();
  float slope = 0.0f;
  float ph = clamp_ph_(calculate_ph_(voltage, temp_c, slope));

  // Apply smoothing to the calculated pH value
  if (std::isnan(smoothed_ph_))
    smoothed_ph_ = ph;
  else
    smoothed_ph_ = PH_SMOOTHING_ALPHA * ph + (1.0f - PH_SMOOTHING_ALPHA) * smoothed_ph_;

  log_readings_(voltage, temp_c, slope, smoothed_ph_);

  // Convert temperature to Fahrenheit if required
  ESP_LOGI(TAG, "Voltage: %.2f mV, Temp: %.2f %s, pH: %.2f", voltage,
           use_fahrenheit_ ? (temp_c * 9.0f / 5.0f + 32.0f) : temp_c,
           use_fahrenheit_ ? "°F" : "°C", smoothed_ph_);

  if (ph_sensor_) ph_sensor_->publish_state(smoothed_ph_);
  if (temperature_output_sensor_) {
    float t_out = use_fahrenheit_ ? (temp_c * 9.0f / 5.0f + 32.0f) : temp_c;
    temperature_output_sensor_->publish_state(t_out);
  }
  if (current_slope_sensor_) current_slope_sensor_->publish_state(slope);

  check_reset_status_();
}

void CalibratePHAction::play() {
  // Perform the calibration action based on the current stage
  if (!this->parent_) return;
  if (stage_ == 0) this->parent_->reset_calibration();
  else this->parent_->set_calibration_stage(stage_);
}

}  // namespace dfrobot_ph_meter
}  // namespace esphome
