#include "sacredsun_bms.h"

#include <cstdio>

#include "esphome/core/log.h"

namespace esphome {
namespace sacredsun_bms {

static const char *const TAG = "sacredsun_bms";

static const char *const REQUESTS[9] = {
    "~22014A42E00201FD28\r",
    "~22024A42E00201FD27\r",
    "~22034A42E00201FD26\r",
    "~22044A42E00201FD25\r",
    "~22054A42E00201FD24\r",
    "~22064A42E00201FD23\r",
    "~22074A42E00201FD22\r",
    "~22084A42E00201FD21\r",
    "~22094A42E00201FD20\r",
};

void SacredSunBms::setup() {
  this->rx_buffer_.reserve(MAX_FRAME_LENGTH);
}

void SacredSunBms::dump_config() {
  ESP_LOGCONFIG(TAG, "SacredSun BMS:");
  ESP_LOGCONFIG(TAG, "  Address: %u", this->address_);
  ESP_LOGCONFIG(TAG, "  Protocol: SacredSun/Tian ASCII RS485");
  ESP_LOGCONFIG(TAG, "  Expected UART: 9600 baud, 8N1");
}

void SacredSunBms::update() {
  this->send_request_();
}

void SacredSunBms::send_request_() {
  if (this->address_ < 1 || this->address_ > 9) {
    ESP_LOGE(TAG, "Invalid BMS address %u", this->address_);
    return;
  }

  // Drop stale bytes before starting a new request.
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
  }

  this->rx_buffer_.clear();
  const char *request = REQUESTS[this->address_ - 1];
  ESP_LOGD(TAG, "Requesting SacredSun pack address %u", this->address_);
  this->write_str(request);
}

void SacredSunBms::loop() {
  while (this->available()) {
    uint8_t byte;
    if (!this->read_byte(&byte))
      return;

    if (byte == '~') {
      this->rx_buffer_.clear();
      this->rx_buffer_.push_back(static_cast<char>(byte));
      continue;
    }

    if (this->rx_buffer_.empty())
      continue;

    if (byte == '\r') {
      if (!this->parse_frame_(this->rx_buffer_)) {
        ESP_LOGW(TAG, "Invalid SacredSun response (%u bytes)", static_cast<unsigned>(this->rx_buffer_.size()));
      }
      this->rx_buffer_.clear();
      continue;
    }

    if (this->rx_buffer_.size() >= MAX_FRAME_LENGTH) {
      ESP_LOGW(TAG, "SacredSun response exceeded buffer size");
      this->rx_buffer_.clear();
      continue;
    }

    this->rx_buffer_.push_back(static_cast<char>(byte));
  }
}

int SacredSunBms::hex_nibble_(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}

bool SacredSunBms::hex_byte_(const std::string &data, size_t offset, uint8_t &value) {
  if (offset + 2 > data.size())
    return false;
  int hi = hex_nibble_(data[offset]);
  int lo = hex_nibble_(data[offset + 1]);
  if (hi < 0 || lo < 0)
    return false;
  value = static_cast<uint8_t>((hi << 4) | lo);
  return true;
}

bool SacredSunBms::hex_u16_(const std::string &data, size_t offset, uint16_t &value) {
  if (offset + 4 > data.size())
    return false;

  uint16_t result = 0;
  for (size_t i = 0; i < 4; i++) {
    int nibble = hex_nibble_(data[offset + i]);
    if (nibble < 0)
      return false;
    result = static_cast<uint16_t>((result << 4) | nibble);
  }

  value = result;
  return true;
}

bool SacredSunBms::hex_i16_(const std::string &data, size_t offset, int16_t &value) {
  uint16_t raw;
  if (!hex_u16_(data, offset, raw))
    return false;
  value = static_cast<int16_t>(raw);
  return true;
}

uint8_t SacredSunBms::checksum_(const std::string &data) {
  // Original implementation sums ASCII bytes at indexes 1..206.
  uint8_t sum = 0;
  for (size_t i = 1; i <= 206 && i < data.size(); i++)
    sum = static_cast<uint8_t>(sum + static_cast<uint8_t>(data[i]));
  return static_cast<uint8_t>(0U - sum);
}

bool SacredSunBms::parse_frame_(const std::string &frame) {
  // Some auto-direction RS485 modules echo the transmitted request back to RX.
  // Ignore that local echo so it is not reported as an invalid BMS response.
  if (this->address_ >= 1 && this->address_ <= 9) {
    std::string request_echo = REQUESTS[this->address_ - 1];
    if (!request_echo.empty() && request_echo.back() == '\r')
      request_echo.pop_back();
    if (frame == request_echo) {
      ESP_LOGV(TAG, "Ignoring local RS485 request echo");
      return true;
    }
  }

  if (frame.size() < MIN_FRAME_LENGTH)
    return false;

  if (frame[0] != '~' || frame[1] != '2' || frame[2] != '2')
    return false;

  uint8_t frame_address;
  if (!hex_byte_(frame, 3, frame_address) || frame_address != this->address_) {
    ESP_LOGW(TAG, "Received response for unexpected BMS address");
    return false;
  }

  // The reverse-engineered frame uses indexes 207..208 as trailer and
  // indexes 209..210 as the transmitted checksum.
  uint8_t wanted_checksum;
  if (!hex_byte_(frame, 209, wanted_checksum))
    return false;

  const uint8_t calculated_checksum = checksum_(frame);
  if (calculated_checksum != wanted_checksum) {
    ESP_LOGW(TAG, "Checksum mismatch: calculated 0x%02X, received 0x%02X",
             calculated_checksum, wanted_checksum);
    return false;
  }

  uint16_t raw_u16;
  int16_t raw_i16;
  uint8_t raw_u8;

  if (hex_u16_(frame, 15, raw_u16) && this->state_of_charge_sensor_ != nullptr)
    this->state_of_charge_sensor_->publish_state(raw_u16 * 0.01f);

  if (hex_u16_(frame, 19, raw_u16) && this->total_voltage_sensor_ != nullptr)
    this->total_voltage_sensor_->publish_state(raw_u16 * 0.01f);

  if (hex_byte_(frame, 23, raw_u8) && this->cell_count_sensor_ != nullptr)
    this->cell_count_sensor_->publish_state(raw_u8);

  uint8_t cell_count = 15;
  if (hex_byte_(frame, 23, raw_u8))
    cell_count = raw_u8 > 15 ? 15 : raw_u8;

  for (size_t i = 0; i < cell_count; i++) {
    if (this->cell_voltage_sensors_[i] != nullptr && hex_u16_(frame, 25 + i * 4, raw_u16))
      this->cell_voltage_sensors_[i]->publish_state(raw_u16 * 0.001f);
  }

  for (size_t i = 0; i < this->bms_temperature_sensors_.size(); i++) {
    if (this->bms_temperature_sensors_[i] != nullptr && hex_i16_(frame, 85 + i * 4, raw_i16))
      this->bms_temperature_sensors_[i]->publish_state(raw_i16 * 0.1f);
  }

  if (hex_byte_(frame, 97, raw_u8) && this->temperature_sensor_count_sensor_ != nullptr)
    this->temperature_sensor_count_sensor_->publish_state(raw_u8);

  for (size_t i = 0; i < this->battery_temperature_sensors_.size(); i++) {
    if (this->battery_temperature_sensors_[i] != nullptr && hex_i16_(frame, 99 + i * 4, raw_i16))
      this->battery_temperature_sensors_[i]->publish_state(raw_i16 * 0.1f);
  }

  if (hex_i16_(frame, 115, raw_i16) && this->current_sensor_ != nullptr)
    this->current_sensor_->publish_state(raw_i16 * 0.01f);

  if (hex_u16_(frame, 123, raw_u16) && this->state_of_health_sensor_ != nullptr)
    this->state_of_health_sensor_->publish_state(raw_u16);

  if (hex_u16_(frame, 129, raw_u16) && this->nominal_capacity_sensor_ != nullptr)
    this->nominal_capacity_sensor_->publish_state(raw_u16 * 0.01f);

  if (hex_u16_(frame, 133, raw_u16) && this->remaining_capacity_sensor_ != nullptr)
    this->remaining_capacity_sensor_->publish_state(raw_u16 * 0.01f);

  if (hex_u16_(frame, 137, raw_u16) && this->charging_cycles_sensor_ != nullptr)
    this->charging_cycles_sensor_->publish_state(raw_u16);

  ESP_LOGD(TAG, "SacredSun pack %u response decoded successfully", this->address_);
  return true;
}

}  // namespace sacredsun_bms
}  // namespace esphome
