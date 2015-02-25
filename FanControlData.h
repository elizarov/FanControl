#ifndef FAN_CONTROL_DATA_H_
#define FAN_CONTROL_DATA_H_

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

// ---------------- FanControlData ----------------
// FanControl master transmit to FAN_CONTROL_ADDR
// Logger slave receive

struct FanControlData {
  FixNum<int16_t,1> tempIn;
  FixNum<int8_t,0> rhIn;
  FixNum<int16_t,1> tempOut;
  FixNum<int8_t,0> rhOut;
  Cond cond;
  FixNum<int16_t,1> voltage; // source voltage
  bool fanPower;
  FixNum<int16_t,0> fanRPM;
  uint8_t crc;

  void clear();
  void prepare();
  bool ok() const;
};

// ---------------- charge ----------------

enum Charge {
  CHARGE_OFF,
  CHARGE_ON,
  CHARGE_DONE,
  CHARGE_ERROR
};

// ---------------- LoggerData ----------------
// FanControl master receive from FAN_CONTROL_ADDR
// Logger slave transmit
struct LoggerData {
  FixNum<int16_t,1> tempRef; // reference temperature
  FixNum<int16_t,2> voltage; // regulated 
  Charge charge;
  byte zero; // always zero
  uint8_t crc;

  void clear();
  void prepare();
  bool ok() const;
};

#endif
