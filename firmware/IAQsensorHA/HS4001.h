#ifndef HS4001_H
#define HS4001_H

#include "Arduino.h"
#include "Wire.h"

#define HS4001_I2C_ADDR 0x54

// Commands
#define HS4001_CMD_HOLD_TEMP 0xE3
#define HS4001_CMD_NOHOLD_TEMP 0xF3
#define HS4001_CMD_HOLD_HUMID_TEMP 0xE5
#define HS4001_CMD_NOHOLD_HUMID_TEMP 0xF5
#define HS4001_CMD_READ_REG 0xA7
#define HS4001_CMD_WRITE_REG 0xA6
#define HS4001_CMD_STOP_PERIODIC 0x30
#define HS4001_CMD_READ_ID 0xD7

// Registers
#define HS4001_REG_RESOLUTION 0x00
#define HS4001_REG_PERIODIC 0x02
#define HS4001_REG_ALERT 0x03

class HS4001 {
public:
  HS4001();
  bool begin(TwoWire &wirePort = Wire);
  float readTemperature();
  float readHumidity();
  void setResolution(uint8_t resolution);

private:
  TwoWire *_i2c;
  uint8_t _calculateCRC(const uint8_t* data, uint8_t len);
  void _writeData(uint8_t command, uint8_t data);
};

#endif
