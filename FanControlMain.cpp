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

const unsigned long BLINK_INTERVAL = Timeout::SECOND;
const unsigned long UPLOAD_FAIL_BLINK_INTERVAL = 250;
const unsigned long UPDATE_INTERVAL = Timeout::SECOND;

// ---------------- object definitions ----------------

LCDLog lcd(RS_PIN, RW_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN, WIDTH, HEIGHT);
BlinkLed blinkLed(STATUS_LED_PIN);
Button button(BUTTON_PIN);
SHT1X sht1x(SHT1X_CLK_PIN, SHT1X_DATA_PIN);
HIH hih;
Timeout tempInUpdated;
Timeout tempOutUpdated;
Timeout updateTimeout(UPDATE_INTERVAL);
Timeout debugDumpTimeout(0);
FanControlData data;
uint8_t uploadState = 0xff; // 0 when no error, error code otherwise

// ---------------- code ----------------

void updateState() {
  setFanPower(millis() % 60000L < 40000L);
}

inline void updateData() {
  // update data
  FanControlData d;
  d.tempIn = sht1x.getTemp();
  d.rhIn = sht1x.getRH();
  d.tempOut = hih.getTemp();           
  d.rhOut = hih.getRH();
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
  lcd.print("Alt 1");
  lcd.clearToRight();

  lcd.setCursor(0, 1);
  lcd.print("Alt 2");
  lcd.clearToRight();
}

// main display 
inline void updateLCD() {
  if (button) {
    updateLCDAlt(); // alternative display
    return;
  }
  lcd.home();
  lcd.print(data.tempOut.format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(data.rhOut.format(3, FMT_RIGHT));
  lcd.print('%');
  lcd.print(tempOutUpdated.enabled() ? '*' : ' '); 
  lcd.print(data.fanRPM.format(4, FMT_RIGHT));
  lcd.print('r');
  lcd.clearToRight();

  lcd.setCursor(0, 1);
  lcd.print(data.tempIn.format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(data.rhIn.format(3, FMT_RIGHT));
  lcd.print('%');
  lcd.print(tempInUpdated.enabled() ? '*' : ' '); 
  lcd.print(data.voltage.format(4, FMT_RIGHT | 1));
  lcd.print("V");
  lcd.clearToRight();
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
  Serial.println(hih.getState(), HEX);
  
  Serial.print(F(" IN=")); 
  Serial.print(data.tempIn.format());
  Serial.print(' ');
  Serial.print(data.rhIn.format());
  Serial.print(F("% state="));
  Serial.println(sht1x.getState(), HEX);
  
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
  checkFan();
  updateState();
  bool update = false;
  if (sht1x.check()) {
    tempInUpdated.reset(BLINK_INTERVAL);
    update = true;
  }
  if (hih.check()) {
    tempOutUpdated.reset(BLINK_INTERVAL);
    update = true;
  }

  update |= updateTimeout.check();
  update |= tempInUpdated.check();
  update |= tempOutUpdated.check();
  update |= button.check();

  if (update) {
    updateData();
    uploadData();
    updateLCD();
    updateTimeout.reset(UPDATE_INTERVAL); // force periodic update regardless
  }

  debugDump();
  blinkLed.blink(uploadState != 0 ? UPLOAD_FAIL_BLINK_INTERVAL : BLINK_INTERVAL);
  sleep_mode(); // save some power
}

