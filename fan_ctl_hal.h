#ifndef FAN_CTL_HAL_H_
#define FAN_CTL_HAL_H_

#include <Arduino.h>
#include <FixNum.h>

typedef FixNum<int,0> rpm_t;
typedef FixNum<int,1> voltage_t;

void setupFan();
bool checkFan();
bool isFanPower();
void setFanPower(bool on);
rpm_t getFanRPM();
voltage_t getVoltage();

#endif
