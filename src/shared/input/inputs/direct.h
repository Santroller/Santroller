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
bool usingI2C;
bool usingSPI;
uint8_t spPin;
uint8_t tiltType;
uint8_t drumVelocity[8];
AxisScale_t scales[6];
void reinitDirectInput(void) {
  if (spPin != INVALID_PIN) { pinMode(spPin, OUTPUT); }
  for (int i = 0; i < validPins; i++) {
    Pin_t p = pinData[i];
    pinMode(p.pin,
            (p.eq || (p.analogOffset != INVALID_PIN)) ? INPUT : INPUT_PULLUP);
  }
}
void initDirectInput(Configuration_t *config) {
  usingI2C =
      (config->main.tiltType == MPU_6050 || config->main.inputType == WII);
  usingSPI =
      (config->main.fretLEDMode == APA102) || config->main.inputType == PS2;
  spPin = config->pinsSP;
  tiltType = config->main.tiltType;
  uint8_t *pins = (uint8_t *)&config->pins;
  memcpy(scales, &config->axisScale, sizeof(scales));
  validPins = 0;
  setUpValidPins(config);
  if (config->pinsSP != INVALID_PIN) { pinMode(config->pinsSP, OUTPUT); }
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    if (config->main.inputType == DIRECT) {
      if (pins[i] != INVALID_PIN) {
        bool is_fret = (i >= XBOX_A || i == XBOX_LB || i == XBOX_RB);
        Pin_t pin = setUpDigital(
            config, pins[i], i,
            is_fret && config->main.fretLEDMode == LEDS_INLINE, false);
        if (typeIsDrum && is_fret && pins[i] >= PIN_A0) {
          // We should probably keep a list of drum specific buttons, instead of
          // using isfret
          // ADC is 10 bit, thereshold is specified as an 8 bit value, so shift
          // it
          setUpAnalogDigitalPin(&pin, pins[i], config->axis.drumThreshold << 3);
        } else {
          pinMode(pins[i], pin.eq ? INPUT : INPUT_PULLUP);
          if (typeIsGuitar && (i == XBOX_DPAD_DOWN || i == XBOX_DPAD_UP)) {
            pin.milliDeBounce = config->debounce.strum;
          }
        }
        pinData[validPins++] = pin;
      }
    } else {
      // Fill data for debounce in wii and ps2_cnt
      Pin_t pin = setUpDigital(config, 0, i, false, false);
      if (typeIsGuitar && (i == XBOX_DPAD_DOWN || i == XBOX_DPAD_UP)) {
        pin.milliDeBounce = config->debounce.strum;
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
  if (usingI2C && (i == PIN_WIRE_SDA || i == PIN_WIRE_SCL)) { return true; }
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
  if (usingSPI && (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
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
}

void setSP(bool sp) {
  if (spPin != INVALID_PIN) { digitalWrite(spPin, sp); }
}

void tickDirectInput(Controller_t *controller) {
  if (lookingForAnalog) {
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
      if (abs(analogRead(i) - lastAnalogValue[i]) > 10) {
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
  AxisScale_t scale;
  for (int8_t i = 0; i < validAnalog; i++) {
    info = joyData[i];
    if (info.hasDigital) {
      drumVelocity[info.offset - 8] = info.value;
    } else {
      if (i == XBOX_TILT && typeIsGuitar && tiltType == DIGITAL) { continue; }
      analogueData[info.offset] = info.value;
      scale = scales[info.offset];
      int32_t val = info.value;
      val -= scale.offset;
      val *= scale.multiplier;
      val /= 1024;
      val += INT16_MIN;
      if (val > INT16_MAX) val = INT16_MAX;
      if (val < INT16_MIN) val = INT16_MIN;
      // Triggers center at -32767, sticks center at 0. Whammy works similar to
      // a trigger, so we also count it here.
      if (info.offset < 2 || (typeIsGuitar && info.offset == XBOX_WHAMMY)) {
        if (val < scale.deadzone) { val = INT16_MIN; }
      } else if (val < scale.deadzone && val > -scale.deadzone) {
        val = 0;
      }
      if (info.offset >= 2) {
        combinedController->sticks[info.offset - 2] = val;
      } else {
        combinedController->triggers[info.offset] = ((uint16_t)val) >> 8;
      }
    }
  }
}
