
#include <Timeout.h>

#include "Pins.h"
#include "fan_ctl_hal.h"
#include <avr/interrupt.h>

const int RH = 510;
const int RL = 33;

const long RPM_UPDATE = 1000L;
const long RPM_REPORT = 60000L;

const long VOLTAGE_INTERVAL = 500;

uint16_t pulseCnt;
rpm_t fanRPM;
unsigned long lastCheck;
voltage_t voltage;

Timeout voltageTimeout;

// count high-to-low transisitions
ISR(PCINT1_vect) {
  if (digitalRead(FAN_SENSE_PIN) == 0)
    pulseCnt++;
}

void setupFan() {
  pinMode(FAN_POWER_PIN, OUTPUT);
  digitalWrite(FAN_SENSE_PIN, 1); // pull-up sense pin
  PCMSK1 |= _BV(PCINT8); // PCINT8 
  PCICR |= _BV(PCIE1); // enable PCI1 vector
  PCIFR |= _BV(PCIF1); // clear PCI1 flag
}

bool checkFan() {
  unsigned long time = millis();
  unsigned long dt = time - lastCheck;
  if (dt < RPM_UPDATE) 
    return false;
  uint16_t cnt;
  cli();  
  cnt = pulseCnt; // atomic counter read and rest
  pulseCnt = 0;
  sei();
  fanRPM = rpm_t(cnt * RPM_REPORT / dt); 
  lastCheck = time;
  return true;
}

void setFanPower(bool on) {
  digitalWrite(FAN_POWER_PIN, on);
}

rpm_t getFanRPM() {
  return fanRPM;
}

voltage_t getVoltage() {
  if (!voltageTimeout.check())
    return voltage;
  voltageTimeout.reset(VOLTAGE_INTERVAL);
  voltage = voltage_t((analogRead(V_PIN) * 5L * voltage_t::multiplier * (RH + RL)) / (RL * 1024L));
  return voltage;
}

