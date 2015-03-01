#ifndef LOGGER_DATA_H_
#define LOGGER_DATA_H_

#include <Arduino.h>
#include <FixNum.h>

const uint8_t LOGGER_ADDR = 'L';

// ---------------- LoggerIn ----------------
// Master transmits to LOGGER_ADDR, Logger slave receives

const uint8_t MAX_LOGGER_IN_SIZE = 64;

// send this struct as size + 3 (1 byte of size + size bytes of chars + zero + crc)
struct LoggerIn {
  uint8_t size;                     // number of actual chars <= MAX_LOGGER_IN_SIZE
  char buf[MAX_LOGGER_IN_SIZE + 2]; // last char after size zero, then crc

  void clear();
  void prepare(); // computes crc
  bool ok(uint8_t rxSize) const;
  inline uint8_t txSize() const { return size + 3; }
};

// ---------------- charge ----------------

enum Charge {
  CHARGE_OFF,
  CHARGE_ON,
  CHARGE_DONE,
  CHARGE_ERROR
};

// ---------------- LoggerOut ----------------
// Master receives from LOGGER_ADDR, Logger slave transmits
struct LoggerOut {
  FixNum<int16_t,1> tempRef; // reference temperature from DS18B20
  FixNum<int16_t,2> voltage; // input voltage at Logger
  Charge charge = CHARGE_OFF;
  uint8_t lastError = 0; // last logger erorr (if non-zero)
  uint8_t crc;

  void clear();
  void prepare();
  bool ok() const;
};

#endif
