import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, switch, text_sensor
import esphome.automation as auto
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    CONF_TEMPERATURE,
    CONF_ACCURACY_DECIMALS,
)

AUTO_LOAD = ["sensor", "switch", "text_sensor"]
DEPENDENCIES = ["ads1115"]
MULTI_CONF = True

ph_meter_ns = cg.esphome_ns.namespace("dfrobot_ph_meter")
DFRobotPHMeter = ph_meter_ns.class_("DFRobotPHMeter", cg.Component)
DigitalSwitch = ph_meter_ns.class_("DigitalSwitch", switch.Switch)
CalibratePHAction = ph_meter_ns.class_("CalibratePHAction", auto.Action)

CONF_ID_ADS1115 = "id_ads1115"
CONF_CHANNEL = "channel"
CONF_ACID_VOLTAGE = "acid_voltage"
CONF_NEUTRAL_VOLTAGE = "neutral_voltage"
CONF_ALKALINE_VOLTAGE = "alkaline_voltage"
CONF_PH_SENSOR = "ph_sensor"
CONF_CALIBRATION_MODE = "calibration_mode"
CONF_TEMPERATURE_SENSOR = "temperature_sensor"
CONF_TEMPERATURE_OUTPUT = "temperature_output"
CONF_PROBE_STATUS_SENSOR = "probe_status_sensor"
CONF_CALIBRATION_MODE_TYPE = "calibration_mode_type"
CONF_CALIBRATION_POINTS = "calibration_points"
CONF_TEMPERATURE_UNIT = "temperature_unit"
CONF_RAW_VOLTAGE_SENSOR = "raw_voltage_sensor"
CONF_SLOPE_SENSOR = "slope_sensor"

CALIBRATION_LABELS = {"pH4": 4, "pH7": 7, "pH10": 10}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DFRobotPHMeter),

    # Optional dual-mode configuration
    cv.Optional("use_ads1115", default=False): cv.boolean,
    cv.Optional("adc_pin"): cv.int_range(min=0, max=39),

    # ADS1115-specific options
    cv.Optional(CONF_ID_ADS1115): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_CHANNEL, default=0): cv.int_range(min=0, max=3),

    cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_TEMPERATURE_OUTPUT): sensor.sensor_schema(
        unit_of_measurement="°C",
        accuracy_decimals=1,
        state_class="measurement",
    ),
    cv.Optional(CONF_RAW_VOLTAGE_SENSOR): sensor.sensor_schema(
        unit_of_measurement="mV",
        accuracy_decimals=1,
        state_class="measurement",
    ),
    cv.Optional(CONF_SLOPE_SENSOR): sensor.sensor_schema(
        unit_of_measurement="mV/pH",
        accuracy_decimals=2,
        state_class="measurement",
    ),
    cv.Optional(CONF_TEMPERATURE_UNIT, default="celsius"): cv.one_of("celsius", "fahrenheit", lower=True),
    cv.Required(CONF_UPDATE_INTERVAL): cv.update_interval,
    cv.Required(CONF_CALIBRATION_MODE): switch.switch_schema(DigitalSwitch),
    cv.Required(CONF_PH_SENSOR): sensor.sensor_schema(
        unit_of_measurement="pH",
        accuracy_decimals=2,
        state_class="measurement",
    ),
    cv.Optional(CONF_PROBE_STATUS_SENSOR): text_sensor.text_sensor_schema(),

    # Optional custom calibration buffer values
    cv.Optional("ph4_solution", default=4.0): cv.float_,
    cv.Optional("ph7_solution", default=7.0): cv.float_,
    cv.Optional("ph10_solution", default=10.0): cv.float_,
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if config.get("use_ads1115", True):
        ads_sensor = await cg.get_variable(config[CONF_ID_ADS1115])
        cg.add(var.set_input_mode_ads1115())
        cg.add(var.set_ads1115_sensor(ads_sensor))
        cg.add(var.set_channel(config.get(CONF_CHANNEL, 0)))
    else:
        cg.add(var.set_input_mode_native_adc(config["adc_pin"]))

    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))

    if CONF_TEMPERATURE_SENSOR in config:
        temp_sensor = await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])
        cg.add(var.set_temperature_sensor(temp_sensor))

    if config[CONF_TEMPERATURE_UNIT] == "fahrenheit":
        cg.add(var.set_use_fahrenheit(True))

    ph_s = await sensor.new_sensor(config[CONF_PH_SENSOR])
    cg.add(var.set_ph_sensor(ph_s))

    if CONF_TEMPERATURE_OUTPUT in config:
        temp_out = await sensor.new_sensor(config[CONF_TEMPERATURE_OUTPUT])
        cg.add(var.set_temperature_output_sensor(temp_out))

    if CONF_RAW_VOLTAGE_SENSOR in config:
        raw = await sensor.new_sensor(config[CONF_RAW_VOLTAGE_SENSOR])
        cg.add(var.set_raw_voltage_sensor(raw))

    if CONF_SLOPE_SENSOR in config:
        slope_s = await sensor.new_sensor(config[CONF_SLOPE_SENSOR])
        cg.add(var.set_slope_sensor(slope_s))

    cal_sw = await switch.new_switch(config[CONF_CALIBRATION_MODE])
    cg.add(var.set_calibration_mode_switch(cal_sw))

    if CONF_PROBE_STATUS_SENSOR in config:
        status_ts = await text_sensor.new_text_sensor(config[CONF_PROBE_STATUS_SENSOR])
        cg.add(var.set_status_sensor(status_ts))

# --- Actions ---
@auto.register_action("dfrobot_ph_meter.calibrate_ph4", CalibratePHAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DFRobotPHMeter)}))
async def calibrate_ph4_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    act = cg.new_Pvariable(action_id, var)
    cg.add(act.set_stage(4))
    return act

@auto.register_action("dfrobot_ph_meter.calibrate_ph7", CalibratePHAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DFRobotPHMeter)}))
async def calibrate_ph7_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    act = cg.new_Pvariable(action_id, var)
    cg.add(act.set_stage(7))
    return act

@auto.register_action("dfrobot_ph_meter.calibrate_ph10", CalibratePHAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DFRobotPHMeter)}))
async def calibrate_ph10_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    act = cg.new_Pvariable(action_id, var)
    cg.add(act.set_stage(10))
    return act

@auto.register_action("dfrobot_ph_meter.reset_calibration", CalibratePHAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DFRobotPHMeter)}))
async def reset_calibration_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    act = cg.new_Pvariable(action_id, var)
    cg.add(act.set_stage(0))
    return act
