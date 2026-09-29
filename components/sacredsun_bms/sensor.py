import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_VOLT,
)

from . import CONF_SACREDSUN_BMS_ID, SACREDSUN_BMS_COMPONENT_SCHEMA

DEPENDENCIES = ["sacredsun_bms"]

UNIT_AMPERE_HOURS = "Ah"

CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_TOTAL_VOLTAGE = "total_voltage"
CONF_CURRENT = "current"
CONF_STATE_OF_HEALTH = "state_of_health"
CONF_NOMINAL_CAPACITY = "nominal_capacity"
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_CHARGING_CYCLES = "charging_cycles"
CONF_CELL_COUNT = "cell_count"
CONF_TEMPERATURE_SENSOR_COUNT = "temperature_sensor_count"

CELLS = [f"cell_voltage_{i}" for i in range(1, 16)]
BMS_TEMPS = [f"bms_temperature_{i}" for i in range(1, 4)]
BATTERY_TEMPS = [f"battery_temperature_{i}" for i in range(1, 5)]

SENSOR_DEFS = {
    CONF_STATE_OF_CHARGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TOTAL_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_CURRENT: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_STATE_OF_HEALTH: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_NOMINAL_CAPACITY: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_REMAINING_CAPACITY: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE_HOURS,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_CHARGING_CYCLES: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    CONF_CELL_COUNT: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TEMPERATURE_SENSOR_COUNT: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}

CELL_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
)

TEMP_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = SACREDSUN_BMS_COMPONENT_SCHEMA.extend(
    {cv.Optional(key): schema for key, schema in SENSOR_DEFS.items()}
).extend(
    {cv.Optional(key): CELL_SCHEMA for key in CELLS}
).extend(
    {cv.Optional(key): TEMP_SCHEMA for key in BMS_TEMPS + BATTERY_TEMPS}
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_SACREDSUN_BMS_ID])

    for key in SENSOR_DEFS:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(hub, f"set_{key}_sensor")(sens))

    for i, key in enumerate(CELLS):
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(hub.set_cell_voltage_sensor(i, sens))

    for i, key in enumerate(BMS_TEMPS):
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(hub.set_bms_temperature_sensor(i, sens))

    for i, key in enumerate(BATTERY_TEMPS):
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(hub.set_battery_temperature_sensor(i, sens))
