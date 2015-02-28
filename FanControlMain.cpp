#include <Arduino.h>

// Arduino Libraries
#include <LiquidCrystal.h>

// haworkslibs
#include <Timeout.h>
#include <Button.h>
#include <BlinkLed.h>
#include <FixNum.h>
#include <FmtRef.h>
#include <Uptime.h>
#include <TWIMaster.h>
#include <SHT1X.h>
#include <HIH.h>
#include <Humidity.h>

// project libs
#include "Pins.h"
#include "LoggerData.h"
#include "LCDLog.h"
#include "fan_ctl_hal.h"

#include <avr/sleep.h>
#include <string.h>

// ---------------- constant definitions ----------------

const uint8_t HEIGHT = 2; 
const uint8_t WIDTH = 16;
const uint8_t BACKLIGHT = 128;

const long BLINK_INTERVAL = 1000;
const long UPLOAD_FAIL_BLINK_INTERVAL = 250;

const long UPDATE_INTERVAL = 1000;
const long UPLOAD_INTERVAL = 5000;

const long FAN_POWER_INTERVAL = 30000; // don't update fan power state too often

const long LONG_BUTTON_PRESSED = 1000;

const int TEMP_DELTA = 1; // assume +/- 1 degree error in temperature
const int RH_DELTA = 5; // assume +/- 5% error in humidity

const int TEMP_HOT = 5;
const int TEMP_COLD = 1;
const int RH_DRY = 30;

const int MIN_FAN_VOLTAGE = 7; // 7V or more

// ---------------- condition ----------------

enum Cond {
  COND_NA, // not available
  COND_HOT, // too hot inside
  COND_COLD, // too cold inside
  COND_DRY, // dry inside 
  COND_DAMP, // damp inside -- turn on the fan to dry it out (!)
};

// ---------------- object and variable definitions ----------------

LCDLog lcd(RS_PIN, RW_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN, WIDTH, HEIGHT);
BlinkLed blinkLed(STATUS_LED_PIN);
Button button(BUTTON_PIN);
SHT1X sensorIn(SHT1X_CLK_PIN, SHT1X_DATA_PIN);
HIH sensorOut;
Timeout tempInUpdated;
Timeout tempOutUpdated;
Timeout updateTimeout(UPDATE_INTERVAL);
Timeout uploadTimeout(UPLOAD_INTERVAL);
Timeout debugDumpTimeout(0);
Timeout fanPowerTimeout(0);
Timeout forceFanTimeout;

wvp_t wvpIn;
wvp_t wvpOut;
Cond cond;
bool fanPower;

uint8_t uploadState = 0xff; // 0 when no error, error code otherwise

LoggerOut logger;
LoggerIn data = { 0, "i+??.? ??% o+??.? ??% v??.?p?r?????" };

FmtRef iRef(data.buf, 'i');
FmtRef ihRef(iRef);
FmtRef oRef(data.buf, 'o');
FmtRef ohRef(oRef);
FmtRef vRef(data.buf, 'v');
FmtRef pRef(data.buf, 'p');
FmtRef rRef(data.buf, 'r');

inline void setupData() {
  data.size = strlen(data.buf);
}

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

inline void updateFanPower() {
  if (forceFanTimeout.enabled()) {
    fanPower = true;  
  } else {
    if (!fanPowerTimeout.check())
      return;
    fanPower = (cond == COND_DAMP) && getVoltage() > MIN_FAN_VOLTAGE;
  }    
  fanPowerTimeout.reset(FAN_POWER_INTERVAL);  
  setFanPower(fanPower);
}

int forceFanRemainingMins() {
  long rem = forceFanTimeout.remaining();
  return rem == 0 ? 0 : (rem + Timeout::MINUTE - 1) / Timeout::MINUTE;
}

inline void onShortButtonPress() {
  int rem = forceFanRemainingMins();
  if (rem == 0)  
    rem = 1;
  else if (rem < 5)
    rem = 5;
  else if (rem < 15)
    rem = 15;
  else if (rem < 60)
    rem = 60;
  else if (rem < 120)
    rem = 120;
  else 
    rem = 0;
  forceFanTimeout.reset(rem * Timeout::MINUTE);  
}

inline void uploadData() {
  if (!uploadTimeout.check())
    return;
  uploadTimeout.reset(UPLOAD_INTERVAL);  
  // format data
  iRef = sensorIn.getTemp();
  ihRef = sensorIn.getRH();
  oRef = sensorOut.getTemp();           
  ohRef = sensorOut.getRH();
  vRef = getVoltage();
  pRef = fanPower ? 1 : 0;
  rRef = getFanRPM();
  // computes crc
  data.prepare();
  // transmit
  uploadState = TWIMaster.transmit(LOGGER_ADDR, &data, data.txSize(), true);
  if (uploadState == 0) {
    uploadState = TWIMaster.receive(LOGGER_ADDR, logger);
    if (uploadState == 0 && !logger.ok())
      uploadState = 0xfd; // wrong data received from logger
  }
  if (uploadState != 0)
    logger.clear();  
  // debug dump to serial  
  Serial.print(F("FanCtrl: ["));
  Serial.print(data.buf);
  Serial.print(F("] state="));
  Serial.println(uploadState, HEX);
}

//  Alternative display
//  0123456789012345
// +----------------+
// |??.??? +??.? ?.?|  wvpOut, tempRef, logger voltage
// |??.??? [xx] ??.?|  wvpIn, uploadState, line voltage
// +----------------+
inline void updateLCDAlt() {
  lcd.home();
  lcd.print(wvpOut.format(6, FMT_RIGHT | 3));
  lcd.print(' ');
  lcd.print(logger.tempRef.format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(logger.voltage.format(3, FMT_RIGHT | 1));  
  lcd.println();

  lcd.print(wvpIn.format(6, FMT_RIGHT | 3));
  lcd.print(' ');
  lcd.print('[');
  lcd.print(uploadState >> 4, HEX);
  lcd.print(uploadState & 0xf, HEX);
  lcd.print(']');
  lcd.print(' ');
  lcd.print(getVoltage().format(4, FMT_RIGHT | 1));
  lcd.println();
}

inline void printCond() {
  switch (cond) {
  case COND_NA:   lcd.print(F(" ????")); break;
  case COND_HOT:  lcd.print(F("  HOT")); break;
  case COND_COLD: lcd.print(F(" COLD")); break;
  case COND_DRY:  lcd.print(F("  DRY")); break;
  case COND_DAMP: lcd.print(F("!DAMP")); break;
  }
}

//  Main display 
//  0123456789012345
// +----------------+
// |+??.? ??%* !COND| tempOut, rhOut, cond
// |+??.? ??%* ?????| tempIn, rhIn, fanRPM
// +----------------+
inline void updateLCD() {
  if (button.pressed() > LONG_BUTTON_PRESSED) {
    updateLCDAlt(); // alternative display
    return;
  }
  lcd.home();
  lcd.print(sensorOut.getTemp().format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(sensorOut.getRH().format(2, FMT_RIGHT));
  lcd.print('%');
  lcd.print(tempOutUpdated.enabled() ? '*' : ' '); 
  lcd.print(' ');
  int rem = forceFanRemainingMins();
  if (rem > 0) {
    lcd.print(fixnum16_0(rem).format(4, FMT_RIGHT));
    lcd.print('m');
  } else  
    printCond();
  lcd.println();

  lcd.print(sensorIn.getTemp().format(5, FMT_SIGN | FMT_RIGHT | 1));
  lcd.print(' ');
  lcd.print(sensorIn.getRH().format(2, FMT_RIGHT));
  lcd.print('%');
  lcd.print(tempInUpdated.enabled() ? '*' : ' '); 
  lcd.print(' ');
  lcd.print(getFanRPM().format(5, FMT_RIGHT));
  lcd.println();
}
                                                                                         
void setup() {
  setupData();
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
  update |= forceFanTimeout.check();

  long buttonReleased = button.released();
  if (buttonReleased != 0 && buttonReleased < LONG_BUTTON_PRESSED)
    onShortButtonPress();
    
  if (update) {
    updateFanPower();
    updateLCD();
    updateTimeout.reset(UPDATE_INTERVAL); // force periodic update regardless
  }

  uploadData();
  blinkLed.blink(uploadState != 0 ? UPLOAD_FAIL_BLINK_INTERVAL : BLINK_INTERVAL);
  sleep_mode(); // save some power
}

