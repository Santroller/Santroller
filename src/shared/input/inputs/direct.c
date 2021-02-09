#include "direct.h"
#include "../pins/pins.h"
#include "config/eeprom.h"
#include "guitar.h"
#include "output/descriptors.h"
#include "util/util.h"
#include <stdlib.h>
Pin_t pinData[16];
int validPins = 0;
uint8_t detectedPin = 0xff;
bool lookingForDigital = false;
bool lookingForAnalog = false;
int lastAnalogValue[NUM_ANALOG_INPUTS];
bool lastDigitalValue[NUM_DIGITAL_PINS];
void initDirectInput() {
  uint8_t *pins = (uint8_t *)&config.pins;
  validPins = 0;
  setUpValidPins();
  if (config.main.inputType == DIRECT) {
    for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
      if (pins[i] != INVALID_PIN) {
        bool is_fret = (i >= XBOX_A || i == XBOX_LB || i == XBOX_RB);
        Pin_t pin = {};
        pin.offset = i;
        pin.mask = digitalPinToBitMask(pins[i]);
        pin.port = portInputRegister(digitalPinToPort((pins[i])));
        pin.pmask = _BV(i);
        pin.eq = is_fret && config.main.fretLEDMode == LEDS_INLINE;
        if (isDrum(config.main.subType) && is_fret) {
          // We should probably keep a list of drum specific buttons, instead of
          // using isfret
          // ADC is 10 bit, thereshold is specified as an 8 bit value, so shift
          // it
          setUpAnalogDigitalPin(pin, pins[i], config.axis.drumThreshold << 3);
        } else {
          pinMode(pins[i], pin.eq ? INPUT : INPUT_PULLUP);
          pinData[validPins++] = pin;
        }
      }
    }
  }
}
bool shouldSkipPin(uint8_t i) {
  // On the 328p, due to an inline LED, it isn't possible to check pin 13
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega328P__)
  if (i == 13 || i == 0 || i == 1) return true;
#endif
  // Skip sda + scl when using peripherials utilising I2C
  if ((config.main.tiltType == MPU_6050) &&
      (i == PIN_WIRE_SDA || i == PIN_WIRE_SCL)) {
    return true;
  }
  // Skip RF related pins (such as spi) when using an RF transmitter
#ifdef RF_TX
#  if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega328P__)
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
    if (!shouldSkipPin(i)) { pinMode(i, INPUT_PULLUP); }
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
    // Change this to keep track of changes.
    for (int i = 2; i < NUM_DIGITAL_PINS; i++) {
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
  Pin_t pin;
  for (uint8_t i = 0; i < validPins; i++) {
    pin = pinData[i];
    if (((*pin.port & pin.mask) != 0) == pin.eq) { controller->buttons |= pin.pmask; }
  }
  AnalogInfo_t info;
  ControllerCombined_t *combinedController = (ControllerCombined_t *)controller;
  for (int8_t i = 0; i < validAnalog; i++) {
    info = joyData[i];
    if (info.hasDigital) {
      if (info.value > info.threshold) {
        controller->buttons |= info.digital.pmask;
      }
      controller->drumVelocity[info.offset - 8] = info.value;
    } else if (info.offset >= 2) {
      combinedController->sticks[info.offset - 2] = info.value;
    } else {
      combinedController->triggers[info.offset] = info.value >> 8;
    }
  }
}
