
#include <Arduino.h>
#include <util/crc16.h>
#include "LoggerData.h"

static uint8_t crc8(uint8_t *ptr, uint8_t n) {
  uint8_t crc = 0;
  while (n-- > 0) 
    crc = _crc_ibutton_update(crc, *(ptr++));
  return crc;
}

void LoggerIn::clear() {
  size = 0;
  buf[0] = 0;
}

void LoggerIn::prepare() {
  buf[size] = 0;
  buf[size + 1] = crc8((uint8_t*)this, size + 2);
}

bool LoggerIn::ok(uint8_t rxSize) const {
  return rxSize > 0 && rxSize == size + 3 && buf[size] == 0 &&
    buf[size + 1] == crc8((uint8_t*)this, size + 2);
}

void LoggerOut::clear() {
  *this = LoggerOut();
}

void LoggerOut::prepare() {
  crc = crc8((uint8_t*)this, sizeof(LoggerOut) - 1);
}

bool LoggerOut::ok() const {
  return zero == 0 && crc == crc8((uint8_t*)this, sizeof(LoggerOut) - 1);
}
