
#include <Arduino.h>
#include <util/crc16.h>
#include "FanControlData.h"

static uint8_t crc8(uint8_t *ptr, uint8_t n) {
  uint8_t crc = 0;
  while (n-- > 0) 
    crc =_crc_ibutton_update(crc, *(ptr++));
  return crc;
}

void FanControlData::clear() {
  *this = FanControlData();
}

void FanControlData::prepare() {
  crc = crc8((uint8_t*)this, sizeof(FanControlData) - 1);
}

bool FanControlData::ok() const {
  return crc == crc8((uint8_t*)this, sizeof(FanControlData) - 1);
}

void LoggerData::clear() {
  *this = LoggerData();
}

void LoggerData::prepare() {
  crc = crc8((uint8_t*)this, sizeof(LoggerData) - 1);
}

bool LoggerData::ok() const {
  return crc == crc8((uint8_t*)this, sizeof(LoggerData) - 1) && zero == 0;
}