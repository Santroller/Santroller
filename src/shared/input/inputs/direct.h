#pragma once
#include "controller/controller.h"
#include "eeprom/eeprom.h"
#include "guitar.h"
#include "output/descriptors.h"
#include "pins/pins.h"
#include "util/util.h"
#include <stdlib.h>
int validPins = 0;
uint8_t detectedPin = 0xff;
bool lookingForDigital = false;
bool lookingForAnalog = false;
int lastAnalogValue[NUM_ANALOG_INPUTS];
bool lastDigitalValue[NUM_DIGITAL_PINS];
AnalogInfo_t joyData[NUM_ANALOG_INPUTS];
int16_t analogueData[XBOX_AXIS_COUNT];
void initDirectInput(void) {
  uint8_t *pins = (uint8_t *)&config.pins;
  validPins = 0;
  setUpValidPins();
  if (config.pinsSP != INVALID_PIN) { pinMode(config.pinsSP, OUTPUT); }
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    if (config.main.inputType == DIRECT) {
      if (pins[i] != INVALID_PIN) {
        bool is_fret = (i >= XBOX_A || i == XBOX_LB || i == XBOX_RB);
        Pin_t pin = setUpDigital(
            pins[i], i, is_fret && config.main.fretLEDMode == LEDS_INLINE);
        if (isDrum(config.main.subType) && is_fret && pins[i] >= PIN_A0) {
          // We should probably keep a list of drum specific buttons, instead of
          // using isfret
          // ADC is 10 bit, thereshold is specified as an 8 bit value, so shift
          // it
          setUpAnalogDigitalPin(&pin, pins[i], config.axis.drumThreshold << 3);
          pinData[validPins++] = pin;
        } else {
          pinMode(pins[i], pin.eq ? INPUT : INPUT_PULLUP);
          pinData[validPins++] = pin;
          if (isGuitar(config.main.subType) &&
              (i == XBOX_DPAD_DOWN || i == XBOX_DPAD_UP)) {
            pin.milliDeBounce = config.debounce.strum;
          }
        }
      }
    } else {
      // Fill data for debounce in wii and ps2_cnt
      Pin_t pin = setUpDigital(0, i, false);
      if (isGuitar(config.main.subType) &&
          (i == XBOX_DPAD_DOWN || i == XBOX_DPAD_UP)) {
        pin.milliDeBounce = config.debounce.strum;
      }
      pinData[validPins++] = pin;
    }
  }
}
bool shouldSkipPin(uint8_t i) {
  // On the 328p, due to an inline LED, it isn't possible to check pin 13
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) ||              \
    defined(__AVR_ATmega328P__)
  if (i == 13 || i == 0 || i == 1) return true;
#endif
  // Skip sda + scl when using peripherials utilising I2C
  if ((config.main.tiltType == MPU_6050 || config.main.inputType == WII) &&
      (i == PIN_WIRE_SDA || i == PIN_WIRE_SCL)) {
    return true;
  }
  // Skip RF related pins (such as spi) when using an RF transmitter
#ifdef RF_TX
#  if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) ||            \
      defined(__AVR_ATmega328P__)
  if (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
      i == PIN_SPI_SS || i == 8 || i == 2)
    return true;
#  else
  if (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
      i == PIN_SPI_SS || i == 0 || i == 1)
    return true;
#  endif
#endif
  // Skip SPI pins when using peripherials that utilise SPI
  if ((config.main.fretLEDMode == APA102) &&
      (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
       i == PIN_SPI_SS)) {
    return true;
  }
  return false;
}

void findDigitalPin(void) {
  if (lookingForDigital) return;
  detectedPin = 0xff;
  stopReading();
  for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
    if (!shouldSkipPin(i)) {
      pinMode(i, INPUT_PULLUP);
      lastDigitalValue[i] = digitalRead(i);
    }
  }
  lookingForDigital = true;
}

void findAnalogPin(void) {
  if (lookingForAnalog) return;
  detectedPin = 0xff;
  stopReading();
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
    pinMode(PIN_A0 + i, INPUT_PULLUP);
    lastAnalogValue[i] = analogRead(i);
  }
  lookingForAnalog = true;
}

void stopSearching(void) {
  if (lookingForDigital) {
    for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
      if (!shouldSkipPin(i)) { pinMode(i, INPUT); }
      lastDigitalValue[i] = digitalRead(i);
    }
  }
  lookingForDigital = lookingForAnalog = false;
  initDirectInput();
}

void setSP(bool sp) {
  if (config.pinsSP != INVALID_PIN) { digitalWrite(config.pinsSP, sp); }
}

void tickDirectInput(Controller_t *controller) {
  if (lookingForAnalog) {
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
      if (abs(analogRead(i) - lastAnalogValue[i]) > 10) {
        initDirectInput();
        detectedPin = i + PIN_A0;
        lookingForAnalog = false;
        return;
      }
    }
    return;
  }
  if (lookingForDigital) {
    for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
      if (!shouldSkipPin(i)) {
        if (digitalRead(i) != lastDigitalValue[i]) {
          stopSearching();
          detectedPin = i;
          lookingForDigital = false;
          return;
        }
      }
    }
    return;
  }
  tickAnalog();
  AnalogInfo_t info;
  ControllerCombined_t *combinedController = (ControllerCombined_t *)controller;
  AxisScale_t *scales = &config.axisScale.lt;
  AxisScale_t scale;
  for (int8_t i = 0; i < validAnalog; i++) {
    if (i == XBOX_TILT && isGuitar(config.main.subType) &&
        config.main.tiltType == DIGITAL) {
      continue;
    }
    info = joyData[i];
    analogueData[info.offset] = info.value;
    scale = scales[info.offset];
    int32_t val = info.value;
    val -= scale.offset;
    val *= scale.multiplier;
    val /= 1024;
    val += INT16_MIN;
    if (val > INT16_MAX) val = INT16_MAX;
    if (val < INT16_MIN) val = INT16_MIN;
    // Triggers center at -32767, sticks center at 0. Whammy works similar to a
    // trigger, so we also count it here.
    if (info.offset < 2 ||
        (isGuitar(config.main.subType) && info.offset == XBOX_WHAMMY)) {
      if (val < scale.deadzone) { val = INT16_MIN; }
    } else if (val < scale.deadzone && val > -scale.deadzone) {
      val = 0;
    }
    if (info.hasDigital) {
      controller->drumVelocity[info.offset - 8] = val;
    } else if (info.offset >= 2) {
      combinedController->sticks[info.offset - 2] = val;
    } else {
      combinedController->triggers[info.offset] = ((uint16_t)val) >> 8;
    }
  }
}
