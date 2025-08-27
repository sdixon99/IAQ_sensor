
#include "HS4001.h"

HS4001::HS4001() {}

bool HS4001::begin(TwoWire &wirePort) {
  _i2c = &wirePort;
  _i2c->begin();
  _i2c->beginTransmission(HS4001_I2C_ADDR);
  return _i2c->endTransmission() == 0;
}

float HS4001::readTemperature() {
  _i2c->beginTransmission(HS4001_I2C_ADDR);
  _i2c->write(HS4001_CMD_NOHOLD_TEMP);
  _i2c->endTransmission();

  delay(10); // Wait for measurement

  _i2c->requestFrom((uint8_t)HS4001_I2C_ADDR, (uint8_t)3);
  if (_i2c->available() < 3) {
    return -999.0; // Error
  }

  uint8_t temp_msb = _i2c->read();
  uint8_t temp_lsb = _i2c->read();
  uint8_t received_crc = _i2c->read();

  uint8_t crc_data[4] = {0x00, 0x00, temp_msb, temp_lsb};
  uint8_t calculated_crc = _calculateCRC(crc_data, 4);

  if (calculated_crc != received_crc) {
    Serial.println("Temperature CRC error!");
    return -999.0; // CRC Error
  }

  uint16_t rawTemp = (temp_msb << 8) | temp_lsb;
  return (float)rawTemp / 16383.0 * 165.0 - 40.0;
}

float HS4001::readHumidity() {
  _i2c->beginTransmission(HS4001_I2C_ADDR);
  _i2c->write(HS4001_CMD_NOHOLD_HUMID_TEMP);
  _i2c->endTransmission();
  delay(20); // Wait for measurement

  _i2c->requestFrom((uint8_t)HS4001_I2C_ADDR, (uint8_t)5);
  if (_i2c->available() < 5) {
    return -999.0; // Error
  }

  uint8_t humid_msb = _i2c->read();
  uint8_t humid_lsb = _i2c->read();
  uint8_t temp_msb = _i2c->read();
  uint8_t temp_lsb = _i2c->read();
  uint8_t received_crc = _i2c->read();

  uint8_t crc_data[4] = {humid_msb, humid_lsb, temp_msb, temp_lsb};
  uint8_t calculated_crc = _calculateCRC(crc_data, 4);

  if (calculated_crc != received_crc) {
    Serial.println("Humidity CRC error!");
    return -999.0; // CRC Error
  }

  uint16_t rawHumid = (humid_msb << 8) | humid_lsb;
  return (float)rawHumid / 16383.0 * 100.0;
}

void HS4001::setResolution(uint8_t resolution) {
  // Not implemented
}

uint8_t HS4001::_calculateCRC(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0xFF; // Initial value
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x1D; // Polynomial 0x1D (x^8 + x^4 + x^3 + x^2 + 1)
      } else {
        crc <<= 1;
      }
    }
  }
  return crc; // Final XOR value is 0x00
}

void HS4001::_writeData(uint8_t command, uint8_t data) {
  _i2c->beginTransmission(HS4001_I2C_ADDR);
  _i2c->write(command);
  _i2c->write(data);
  _i2c->endTransmission();
}

