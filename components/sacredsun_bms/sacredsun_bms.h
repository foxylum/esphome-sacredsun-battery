#pragma once

#include <array>
#include <string>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace sacredsun_bms {

class SacredSunBms : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void set_address(uint8_t address) { this->address_ = address; }

  void set_state_of_charge_sensor(sensor::Sensor *sensor) { this->state_of_charge_sensor_ = sensor; }
  void set_total_voltage_sensor(sensor::Sensor *sensor) { this->total_voltage_sensor_ = sensor; }
  void set_current_sensor(sensor::Sensor *sensor) { this->current_sensor_ = sensor; }
  void set_state_of_health_sensor(sensor::Sensor *sensor) { this->state_of_health_sensor_ = sensor; }
  void set_nominal_capacity_sensor(sensor::Sensor *sensor) { this->nominal_capacity_sensor_ = sensor; }
  void set_remaining_capacity_sensor(sensor::Sensor *sensor) { this->remaining_capacity_sensor_ = sensor; }
  void set_charging_cycles_sensor(sensor::Sensor *sensor) { this->charging_cycles_sensor_ = sensor; }
  void set_cell_count_sensor(sensor::Sensor *sensor) { this->cell_count_sensor_ = sensor; }
  void set_temperature_sensor_count_sensor(sensor::Sensor *sensor) { this->temperature_sensor_count_sensor_ = sensor; }

  void set_cell_voltage_sensor(size_t index, sensor::Sensor *sensor) {
    if (index < this->cell_voltage_sensors_.size())
      this->cell_voltage_sensors_[index] = sensor;
  }

  void set_bms_temperature_sensor(size_t index, sensor::Sensor *sensor) {
    if (index < this->bms_temperature_sensors_.size())
      this->bms_temperature_sensors_[index] = sensor;
  }

  void set_battery_temperature_sensor(size_t index, sensor::Sensor *sensor) {
    if (index < this->battery_temperature_sensors_.size())
      this->battery_temperature_sensors_[index] = sensor;
  }

 protected:
  static constexpr size_t MIN_FRAME_LENGTH = 211;
  static constexpr size_t MAX_FRAME_LENGTH = 256;

  void send_request_();
  bool parse_frame_(const std::string &frame);

  static int hex_nibble_(char c);
  static bool hex_byte_(const std::string &data, size_t offset, uint8_t &value);
  static bool hex_u16_(const std::string &data, size_t offset, uint16_t &value);
  static bool hex_i16_(const std::string &data, size_t offset, int16_t &value);
  static uint8_t checksum_(const std::string &data);

  uint8_t address_{1};
  std::string rx_buffer_;

  sensor::Sensor *state_of_charge_sensor_{nullptr};
  sensor::Sensor *total_voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *state_of_health_sensor_{nullptr};
  sensor::Sensor *nominal_capacity_sensor_{nullptr};
  sensor::Sensor *remaining_capacity_sensor_{nullptr};
  sensor::Sensor *charging_cycles_sensor_{nullptr};
  sensor::Sensor *cell_count_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_count_sensor_{nullptr};

  std::array<sensor::Sensor *, 15> cell_voltage_sensors_{};
  std::array<sensor::Sensor *, 3> bms_temperature_sensors_{};
  std::array<sensor::Sensor *, 4> battery_temperature_sensors_{};
};

}  // namespace sacredsun_bms
}  // namespace esphome
