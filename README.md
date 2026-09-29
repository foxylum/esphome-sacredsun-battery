# ESPHome SacredSun Battery

Modern ESPHome external component for SacredSun SCIFP48100 / compatible Tian BMS batteries using the RS485 ASCII protocol.

This project is based on protocol research from [wire67/esphome_sacredsun_rs485](https://github.com/wire67/esphome_sacredsun_rs485), ported away from the removed `platform: custom` API to the current ESPHome external-component model.

## Protocol

- RS485
- 9600 baud
- 8N1
- ASCII frames
- SacredSun connector: Pin 1 = B-, Pin 2 = A+
- Battery address is selected by the battery DIP switches

## ESPHome example

```yaml
external_components:
  - source: github://foxylum/esphome-sacredsun-battery@main
    components: [ sacredsun_bms ]

uart:
  - id: sacredsun_uart
    tx_pin: GPIO25
    rx_pin: GPIO26
    baud_rate: 9600

sacredsun_bms:
  id: sacredsun
  uart_id: sacredsun_uart
  address: 1
  update_interval: 5s

sensor:
  - platform: sacredsun_bms
    sacredsun_bms_id: sacredsun

    state_of_charge:
      name: "SacredSun SOC"

    total_voltage:
      name: "SacredSun Voltage"

    current:
      name: "SacredSun Current"

    state_of_health:
      name: "SacredSun SOH"

    nominal_capacity:
      name: "SacredSun Nominal Capacity"

    remaining_capacity:
      name: "SacredSun Remaining Capacity"

    charging_cycles:
      name: "SacredSun Cycles"

    cell_voltage_1:
      name: "SacredSun Cell 1"

    cell_voltage_15:
      name: "SacredSun Cell 15"
```

The RS485 transceiver must handle half-duplex direction correctly. An auto-direction RS485-to-TTL module is the easiest option.

## Status

Initial modern port. The frame layout follows the original SacredSun/Tian BMS reverse engineering and should be hardware-tested before relying on all decoded values.
