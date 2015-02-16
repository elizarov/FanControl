
#include "Pins.h"
#include "fan_ctl_hal.h"
#include <avr/interrupt.h>

const int RH = 510;
const int RL = 33;

const unsigned long RPM_UPDATE = 1000L;
const unsigned long RPM_REPORT = 60000L;

uint16_t pulseCnt;
uint16_t fanRPM;
unsigned long lastCheck;

// count high-to-low transisitions
ISR(PCINT1_vect) {
  if (digitalRead(SENSE_PIN) == 0)
    pulseCnt++;
}

void setupFan() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(SENSE_PIN, 1); // pull-up sense pin
  PCMSK1 |= _BV(PCINT8); // PCINT8 
  PCICR |= _BV(PCIE1); // enable PCI1 vector
  PCIFR |= _BV(PCIF1); // clear PCI1 flag
}

void checkFan() {
  unsigned long time = millis();
  unsigned long dt = time - lastCheck;
  if (dt < RPM_UPDATE) 
    return;
  uint16_t cnt;
  cli();  
  cnt = pulseCnt; // atomic counter read and rest
  pulseCnt = 0;
  sei();
  fanRPM = cnt * RPM_REPORT / dt; 
  lastCheck = time;
}

bool isFanPower() {
  return digitalRead(POWER_PIN);
}

void setFanPower(bool on) {
  digitalWrite(POWER_PIN, on);
}

rpm_t getFanRPM() {
  return fanRPM;
}

voltage_t getVoltage() {
  return (analogRead(V_PIN) * 5L * voltage_t::multiplier * (RH + RL)) / (RL * 1024L);
}

