import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]
MULTI_CONF = True

CONF_SACREDSUN_BMS_ID = "sacredsun_bms_id"

sacredsun_ns = cg.esphome_ns.namespace("sacredsun_bms")
SacredSunBms = sacredsun_ns.class_("SacredSunBms", cg.PollingComponent, uart.UARTDevice)

SACREDSUN_BMS_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SACREDSUN_BMS_ID): cv.use_id(SacredSunBms),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.require_esphome_version(2025, 2, 0),
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SacredSunBms),
            cv.Optional(CONF_ADDRESS, default=1): cv.int_range(min=1, max=9),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_address(config[CONF_ADDRESS]))
