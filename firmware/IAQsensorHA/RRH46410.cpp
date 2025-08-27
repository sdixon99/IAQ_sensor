#include "RRH46410.h"

RRH46410::RRH46410() {}

int RRH46410::begin(TwoWire &wirePort) {
  _i2cPort = &wirePort;
  _i2cPort->begin();

  // Reset the sensor
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(CMD_RESET);
  _i2cPort->endTransmission();
  delay(100); // Wait for the sensor to reset

  return checkProductID();
}

int RRH46410::begin_no_reset(TwoWire &wirePort) {
  _i2cPort = &wirePort;
  _i2cPort->begin();

  return checkProductID();
}

int RRH46410::checkProductID() {
  // Check product ID
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(CMD_GET_PRODUCT_ID);
  uint8_t data[1] = {CMD_GET_PRODUCT_ID};
  _i2cPort->write(calculateChecksum(data, 1));
  if (_i2cPort->endTransmission() != 0) {
    return -1; // I2C Error
  }

  delay(1);

  // Response is 1 status byte + 5 data bytes + 1 checksum byte = 7 bytes
  if (_i2cPort->requestFrom((uint8_t)RRH46410_I2C_ADDR, (uint8_t)7) != 7) {
    return -2; // I2C Error
  }

  uint8_t response_buf[7];
  for (int i = 0; i < 7; i++) {
    response_buf[i] = _i2cPort->read();
  }

  // Verify checksum
  uint8_t received_checksum = response_buf[6];
  uint8_t checksum_data[6];
  for (int i = 0; i < 6; i++) {
    checksum_data[i] = response_buf[i];
  }
  if (calculateChecksum(checksum_data, 6) != received_checksum) {
    return -3; // Checksum Error
  }

  // Check status byte
  if (response_buf[0] != 0x00) {
    return response_buf[0]; // Return status byte on error
  }

  uint8_t productId[2];
  productId[0] = response_buf[1]; // Product ID is the first 2 bytes of the payload
  productId[1] = response_buf[2];

  // The product ID is 0x2310, which is sent as 0x10 0x23 (little-endian)
  if (productId[0] != 0x10 || productId[1] != 0x23) {
    return -4; // Incorrect Product ID
  }

  return 0; // Success
}

bool RRH46410::setOpmode(uint8_t mode) {
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(CMD_SET_OPMODE);
  _i2cPort->write(mode);
  uint8_t data[2] = {CMD_SET_OPMODE, mode};
  _i2cPort->write(calculateChecksum(data, 2));
  return (_i2cPort->endTransmission() == 0);
}

int RRH46410::getOpmode() {
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(CMD_GET_OPMODE);
  uint8_t data[1] = {CMD_GET_OPMODE};
  _i2cPort->write(calculateChecksum(data, 1));

  if (_i2cPort->endTransmission() != 0) {
    return -1; // I2C Error
  }

  delay(1);

  // Response is 1 status byte + 1 data byte + 1 checksum byte = 3 bytes
  if (_i2cPort->requestFrom((uint8_t)RRH46410_I2C_ADDR, (uint8_t)3) != 3) {
    return -2; // I2C Error
  }

  uint8_t response_buf[3];
  for (int i = 0; i < 3; i++) {
    response_buf[i] = _i2cPort->read();
  }

  // Verify checksum
  uint8_t received_checksum = response_buf[2];
  uint8_t checksum_data[2];
  for (int i = 0; i < 2; i++) {
    checksum_data[i] = response_buf[i];
  }
  if (calculateChecksum(checksum_data, 2) != received_checksum) {
    return -3; // Checksum Error
  }

  // Check status byte
  if (response_buf[0] != 0x00) {
    return response_buf[0]; // Return status byte on error
  }

  return response_buf[1]; // Return the opmode
}

int RRH46410::getMeasurementResults(IAQResults &results) {
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(CMD_GET_MEASUREMENT_RESULTS);
  uint8_t data[1] = {CMD_GET_MEASUREMENT_RESULTS};
  _i2cPort->write(calculateChecksum(data, 1));

  if (_i2cPort->endTransmission() != 0) {
    return -1; // I2C Error
  }

  delay(1);

  // Response is 1 status byte + 9 data bytes + 1 checksum byte = 11 bytes
  if (_i2cPort->requestFrom((uint8_t)RRH46410_I2C_ADDR, (uint8_t)11) != 11) {
    return -2; // I2C Error
  }

  uint8_t response_buf[11];
  for (int i = 0; i < 11; i++) {
    response_buf[i] = _i2cPort->read();
  }

  // Verify checksum
  uint8_t received_checksum = response_buf[10];
  uint8_t checksum_data[10];
  for (int i = 0; i < 10; i++) {
    checksum_data[i] = response_buf[i];
  }
  if (calculateChecksum(checksum_data, 10) != received_checksum) {
    return -3; // Checksum Error
  }

  // Check status byte
  if (response_buf[0] != 0x00) {
    return response_buf[0]; // Return status byte on error
  }

  results.sample_counter = response_buf[1];
  results.iaq = response_buf[2];

  uint8_t tvoc_bytes[2] = {response_buf[3], response_buf[4]};
  uint8_t etoh_bytes[2] = {response_buf[5], response_buf[6]};
  uint8_t eco2_bytes[2] = {response_buf[7], response_buf[8]};

  // Data is little-endian, so assemble LSB first
  results.tvoc = (tvoc_bytes[1] << 8) | tvoc_bytes[0];
  results.etoh = (etoh_bytes[1] << 8) | etoh_bytes[0];
  results.eco2 = (eco2_bytes[1] << 8) | eco2_bytes[0];

  results.relative_iaq = response_buf[9];

  return 0; // Success
}

void RRH46410::setEnvironmentData(float temperature, float humidity) {
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(CMD_SET_ENVIRONMENT_DATA);

  // The datasheet specifies that the temperature and humidity are sent as
  // 16-bit unsigned integers, little-endian, in units of 1/100th of a
  // degree Celcius and 1/100th of a percent relative humidity.
  uint16_t temp_int = (uint16_t)(temperature * 100.0);
  uint16_t humid_int = (uint16_t)(humidity * 100.0);

  uint8_t data[5] = {CMD_SET_ENVIRONMENT_DATA, (uint8_t)temp_int, (uint8_t)(temp_int >> 8), (uint8_t)humid_int, (uint8_t)(humid_int >> 8)};
  
  _i2cPort->write(data[1]);
  _i2cPort->write(data[2]);
  _i2cPort->write(data[3]);
  _i2cPort->write(data[4]);
  
  _i2cPort->write(calculateChecksum(data, 5));
  _i2cPort->endTransmission();
}

uint8_t RRH46410::calculateChecksum(uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(~sum);
}

int RRH46410::getDebugData(uint8_t* buffer) {
  _i2cPort->beginTransmission(RRH46410_I2C_ADDR);
  _i2cPort->write(0x31); // Read Debug Data command
  uint8_t data[1] = {0x31};
  _i2cPort->write(calculateChecksum(data, 1));

  if (_i2cPort->endTransmission() != 0) {
    return -1; // I2C Error
  }

  delay(1);

  // Response is 1 status byte + 81 data bytes + 1 checksum byte = 83 bytes
  if (_i2cPort->requestFrom((uint8_t)RRH46410_I2C_ADDR, (uint8_t)83) != 83) {
    return -2; // I2C Error
  }

  uint8_t response_buf[83];
  for (int i = 0; i < 83; i++) {
    response_buf[i] = _i2cPort->read();
  }

  // Verify checksum
  uint8_t received_checksum = response_buf[82];
  uint8_t checksum_data[82];
  for (int i = 0; i < 82; i++) {
    checksum_data[i] = response_buf[i];
  }
  if (calculateChecksum(checksum_data, 82) != received_checksum) {
    return -3; // Checksum Error
  }

  // Check status byte
  if (response_buf[0] != 0x00) {
    return response_buf[0]; // Return status byte on error
  }

  // Copy the 81 data bytes to the buffer
  for (int i = 0; i < 81; i++) {
    buffer[i] = response_buf[i + 1];
  }

  return 0; // Success
}

void RRH46410::bytesToFloat(uint8_t* bytes, float &result) {
  union {
    float f;
    uint8_t b[4];
  } data;

  // The datasheet specifies that floating point values are always little-endian.
  // The target MCU (RP2040) is also little-endian, so we can copy bytes directly.
  data.b[0] = bytes[0];
  data.b[1] = bytes[1];
  data.b[2] = bytes[2];
  data.b[3] = bytes[3];

  result = data.f;
}