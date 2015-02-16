#ifndef LOGGER_PROTO_H_
#define LOGGER_PROTO_H_

#include <Arduino.h>
#include <FixNum.h>

const uint8_t FAN_CONTROL_ADDR = 'L';

struct FanControlData {
  FixNum<int,1> tempIn;
  FixNum<byte,0> rhIn;
  FixNum<int,1> tempOut;
  FixNum<byte,0> rhOut;
  FixNum<int,1> voltage;
  bool fanPower;
  FixNum<int,0> fanRPM;
  uint8_t crc;

  void clear();
  void fillCRC();
  bool checkCRC();

  inline FanControlData() { clear(); }
};

#endif
