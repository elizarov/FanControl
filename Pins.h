/*
Hardware is based on Sparkfun Serial Enabled LCD Kit.

SHT1x Sensor measure internal temperature and humidity
  - D10 connected to SHT1x CLK
  - D11 connected to SHT1x DATA

External (~12V) voltage is divided with 1% resistors 
  - RH = 5.1 MOm (high side)
  - RL = 330 KOm (low side)

All pins and ports (see Pins.h for constants):

D0    PD0   RXD  \ Serial
D1    PD1   TXD  /
D2    PD2   RS   \
D3    PD3   R/W  |
D4    PD4   E    |
D5    PD5   DB4  | LCD
D6    PD6   DB5  | 
D7    PD7   DB6  |
D8    PB0   DB7  /
D9    PB1   BL-EN (LCD Backlight enable)
D10   PB2   SHT1x CLK	     // SHT1X_CLK_PIN 
D11   PB3   SHT1x DATA       // SHT1X_DATA_PIN
D12   PB4   BUTTON           // BUTTON_PIN
D13   PB5   STATUS LED       // STATUS_LED_PIN
           
A0    PC0   FAN SENSE        // FAN_SENSE_PIN 
A1    PC1   FAN ON           // FAN_POWER_PIN 
A2    PC2   VOLTAGE          // V_PIN 
A3    PC3   WATER            // WATER PIN
A4    PC4   SDA  \ I2C
A5    PC5   SCL  /
*/

#ifndef PINS_H_
#define PINS_H_

#include <Arduino.h>

// LED PINS for Serial Enabled LCD
const uint8_t RS_PIN = 2;
const uint8_t RW_PIN = 3;
const uint8_t EN_PIN = 4;
const uint8_t D4_PIN = 5;
const uint8_t D5_PIN = 6;
const uint8_t D6_PIN = 7;
const uint8_t D7_PIN = 8;
const uint8_t BL_PIN = 9;

// Project pins
const uint8_t SHT1X_CLK_PIN = 10;
const uint8_t SHT1X_DATA_PIN = 11;
const uint8_t BUTTON_PIN = 12;
const uint8_t STATUS_LED_PIN = 13;

const uint8_t FAN_SENSE_PIN = A0; // PC0/PCINT8
const uint8_t FAN_POWER_PIN = A1;
const uint8_t V_PIN = A2;
const uint8_t WATER_PIN = A3;

#endif
