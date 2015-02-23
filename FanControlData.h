#ifndef LOGGER_PROTO_H_
#define LOGGER_PROTO_H_

#include <Arduino.h>
#include <FixNum.h>

const uint8_t FAN_CONTROL_ADDR = 'L';

// ---------------- condition ----------------

enum Cond {
  COND_NA, // not available
  COND_HOT, // too hot inside
  COND_COLD, // too cold inside
  COND_DRY, // dry inside 
  COND_DAMP, // damp inside -- turn on the fan to dry it out (!)
};

// ---------------- data ----------------
// FanControl master transmit to FAN_CONTROL_ADDR
// Logger slaver receive

struct FanControlData {
  FixNum<int,1> tempIn;
  FixNum<byte,0> rhIn;
  FixNum<int,1> tempOut;
  FixNum<byte,0> rhOut;
  Cond cond;
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
