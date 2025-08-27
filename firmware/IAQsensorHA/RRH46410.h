#ifndef RRH46410_H
#define RRH46410_H

#include "Arduino.h"
#include "Wire.h"

#define RRH46410_I2C_ADDR 0x38

// Commands
#define CMD_GET_PRODUCT_ID 0x00
#define CMD_GET_OPMODE 0x10
#define CMD_SET_OPMODE 0x11
#define CMD_GET_MEASUREMENT_RESULTS 0x18
#define CMD_RESET 0x8F
#define CMD_SET_ENVIRONMENT_DATA 0x12

// Opmode
#define OPMODE_IAQ_2ND_GEN 0x01


struct IAQResults {
  uint8_t iaq;
  uint16_t tvoc;
  uint16_t etoh;
  uint16_t eco2;
  uint8_t relative_iaq;
  uint8_t sample_counter;
};

class RRH46410 {
public:
  RRH46410();
  int begin(TwoWire &wirePort = Wire);
  int begin_no_reset(TwoWire &wirePort = Wire);
  bool setOpmode(uint8_t mode);
  int getOpmode();
  int getMeasurementResults(IAQResults &results);
  void setEnvironmentData(float temperature, float humidity);
  int getDebugData(uint8_t* buffer);
  void bytesToFloat(uint8_t* bytes, float &result);

private:
  TwoWire *_i2cPort;
  uint8_t calculateChecksum(uint8_t *data, uint8_t len);
  int checkProductID();
};

#endif