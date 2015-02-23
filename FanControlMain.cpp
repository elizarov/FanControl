#include <Arduino.h>

// Arduino Libraries
#include <LiquidCrystal.h>

// haworkslibs
#include <Timeout.h>
#include <Button.h>
#include <BlinkLed.h>
#include <FixNum.h>
#include <TWIMaster.h>
#include <SHT1X.h>
#include <HIH.h>
#include <Humidity.h>

// project libs
#include "Pins.h"
#include "FanControlData.h"
#include "LCDLog.h"
#include "fan_ctl_hal.h"

#include <avr/sleep.h>

// ---------------- constant definitions ----------------

const uint8_t HEIGHT = 2; 
const uint8_t WIDTH = 16;
const uint8_t BACKLIGHT = 128;

const long BLINK_INTERVAL = Timeout::SECOND;
const long UPLOAD_FAIL_BLINK_INTERVAL = 250;
const long UPDATE_INTERVAL = Timeout::SECOND;

const long LONG_BUTTON_PRESSED = Timeout::SECOND;

const int TEMP_DELTA = 1; // assume +/- 1 degree error in temperature
const int RH_DELTA = 5; // assume +/- 5% error in humidity

const int TEMP_HOT = 5;
const int TEMP_COLD = 1;
const int RH_DRY = 30;

// ---------------- object and variable definitions ----------------

LCDLog lcd(RS_PIN, RW_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN, WIDTH, HEIGHT);
BlinkLed blinkLed(STATUS_LED_PIN);
Button button(BUTTON_PIN);
SHT1X sensorIn(SHT1X_CLK_PIN, SHT1X_DATA_PIN);
HIH sensorOut;
Timeout tempInUpdated;
Timeout tempOutUpdated;
Timeout updateTimeout(UPDATE_INTERVAL);
Timeout debugDumpTimeout(0);
Timeout forceFanTimeout;
FanControlData data;

wvp_t wvpIn;
wvp_t wvpOut;
Cond cond;

uint8_t uploadState = 0xff; // 0 when no error, error code otherwise

// ---------------- code ----------------

inline Cond computeCond() {
  if (!sensorIn.getTemp() || !sensorOut.getTemp())
    return COND_NA;
  if (sensorIn.getTemp() > TEMP_HOT && sensorOut.getTemp() > sensorIn.getTemp())
    return COND_HOT;
  if (sensorIn.getTemp() < TEMP_COLD && sensorOut.getTemp() < sensorIn.getTemp())
    return COND_COLD;
  if (!wvpIn || !wvpOut)
    return COND_NA;
  if (wvpIn <= wvpOut)
    return COND_DRY;
  return COND_DAMP;  
}

void updateCond() {
  cond = computeCond();
}

void updateFanState() {
  setFanPower(cond == COND_DAMP);
}

inline void updateData() {
  // update data
  FanControlData d;
  d.tempIn = sensorIn.getTemp();
  d.rhIn = sensorIn.getRH();
  d.tempOut = sensorOut.getTemp();           
  d.rhOut = sensorOut.getRH();
  d.cond = cond;
  d.voltage = getVoltage();
  d.fanPower = isFanPower();
  d.fanRPM = getFanRPM();
  d.fillCRC();
  // atomically write to global data
  cli();
  memcpy(&data, &d, sizeof(data));
  sei();
}

inline void uploadData() {
  uploadState = TWIMaster.transmit(FAN_CONTROL_ADDR, (uint8_t*)&data, sizeof(data));
}

// alternative display
inline void updateLCDAlt() {
  lcd.home();
  lcd.print(wvpOut.format(6, FMT_RIGHT | 3));
  lcd.println();

  lcd.print(wvpIn.format(6, FMT_RIGHT | 3));
  lcd.print(' ');
  lcd.print(data.voltage.format(4, FMT_RIGHT | 1));
  lcd.print("V");
  lcd.println();
}

inline void printCond() {
  switch (cond) {
  case COND_NA:   lcd.print(F("????")); break;
  case COND_HOT:  lcd.print(F(" HOT")); break;
  case COND_COLD: lcd.print(F("COLD")); break;
  case COND_DRY:  lcd.print(F(" DRY")); break;
  case COND_DAMP: lcd.print(F("DAMP")); break;
  }
}

// main display 
inline void updateLCD() {
  if (button.pressed() > LONG_BUTTON_PRESSED) {
    updateLCDAlt(); // alternative display
    return;
  }
  lcd.home();
  lcd.print(data.tempOut.format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(data.rhOut.format(3, FMT_RIGHT));
  lcd.print('%');
  lcd.print(tempOutUpdated.enabled() ? '*' : ' '); 
  lcd.print(' ');
  printCond();
  lcd.println();

  lcd.print(data.tempIn.format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(data.rhIn.format(3, FMT_RIGHT));
  lcd.print('%');
  lcd.print(tempInUpdated.enabled() ? '*' : ' '); 
  lcd.print(data.fanRPM.format(4, FMT_RIGHT));
  lcd.print('r');
  lcd.println();
}

inline void debugDump() {
  if (!debugDumpTimeout.check()) 
    return;
  Serial.println(F("--- FanCtrl ---"));  
  Serial.print(F("OUT=")); 
  Serial.print(data.tempOut.format());
  Serial.print(' ');
  Serial.print(data.rhOut.format());
  Serial.print(F("% state="));
  Serial.println(sensorOut.getState(), HEX);
  
  Serial.print(F(" IN=")); 
  Serial.print(data.tempIn.format());
  Serial.print(' ');
  Serial.print(data.rhIn.format());
  Serial.print(F("% state="));
  Serial.println(sensorIn.getState(), HEX);
  
  Serial.print(F("  V="));
  Serial.print(data.voltage.format());
  Serial.print(F(" upload="));
  Serial.println(uploadState, HEX);
  
  debugDumpTimeout.reset(1000); 
}
                                                                                             
void setup() {
  Serial.begin(57600); // debug output only
  Serial.println(F("*** FanCtrl ***"));
  analogWrite(BL_PIN, BACKLIGHT);
  lcd.println(F("*** FanCtrl ***"));
  setupFan();
}

void loop() {
  bool update = false;
  if (sensorIn.check()) {
    wvpIn = waterVaporPressure(sensorIn.getTemp() - TEMP_DELTA, sensorIn.getRH() - RH_DELTA);
    updateCond();
    tempInUpdated.reset(BLINK_INTERVAL);
    update = true;
  }
  if (sensorOut.check()) {
    wvpOut = waterVaporPressure(sensorOut.getTemp() + TEMP_DELTA, sensorOut.getRH() + RH_DELTA);
    updateCond();
    tempOutUpdated.reset(BLINK_INTERVAL);
    update = true;
  }

  update |= checkFan();
  update |= updateTimeout.check();
  update |= tempInUpdated.check();
  update |= tempOutUpdated.check();
  update |= button.check();

  if (update) {
    updateFanState();
    updateData();
    uploadData();
    updateLCD();
    updateTimeout.reset(UPDATE_INTERVAL); // force periodic update regardless
  }

  debugDump();
  blinkLed.blink(uploadState != 0 ? UPLOAD_FAIL_BLINK_INTERVAL : BLINK_INTERVAL);
  sleep_mode(); // save some power
}

