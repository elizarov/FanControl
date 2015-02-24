
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
  tempIn.clear();
  rhIn.clear();
  tempOut.clear();
  rhOut.clear();
  cond = COND_NA;
  voltage.clear();
  fanPower = false;
  fanRPM.clear();
}

void FanControlData::fillCRC() {
  crc = crc8((uint8_t*)this, sizeof(FanControlData) - 1);
}

bool FanControlData::checkCRC() {
  if (crc == crc8((uint8_t*)this, sizeof(FanControlData) - 1))
    return true; // ok
  clear();
  return false;
}

