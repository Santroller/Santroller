#include "pins/pins.h"
#include "eeprom/eeprom.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "controller/guitar_includes.h"
#include "stddef.h"
#include "util/util.h"

void digitalWrite(uint8_t pin, uint8_t val) {
  gpio_put(pin, val);
}

int digitalRead(uint8_t pin) { return gpio_get(pin); }
Pin_t setUpDigital(uint8_t pinNum, uint8_t offset, bool inverted) {
  Pin_t pin = {};
  pin.offset = offset;
  pin.pin = pinNum;
  pin.pmask = _BV(offset);
  pin.eq = inverted;
  gpio_init(pinNum);
}
bool digitalReadPin(Pin_t pin) {
  return gpio_get(pin.pin) == pin.eq;
}
void digitalWritePin(Pin_t pin, bool value) {
  return gpio_put(pin.pin, value);
}
void setUpAnalogPin(uint8_t offset) {
  AnalogInfo_t ret = {0};
  ret.offset = offset;
  AnalogPin_t apin = ((PinsCombined_t *)&config.pins)->axis[offset];
  uint8_t pin = apin.pin;
  if (pin == INVALID_PIN) { return; }
  if (ret.offset == 5 && isGuitar(config.main.subType) &&
      config.main.tiltType != ANALOGUE) {
    return;
  }
  ret.hasDigital = false;
  ret.inverted = apin.inverted;
  pinMode(PIN_A0 + pin, INPUT);
  joyData[validAnalog++] = ret;
}
void setUpAnalogDigitalPin(Pin_t button, uint8_t pin, uint16_t threshold) {
  AnalogInfo_t ret = {0};
  ret.offset = pin;
  ret.hasDigital = true;
  ret.digital = button;
  ret.threshold = threshold;
  ret.pin = pin;
  pinMode(PIN_A0 + pin, INPUT);
  joyData[validAnalog++] = ret;
}
void tickAnalog(void) {
  if (validAnalog == 0) return;
  for (int i = 0; i < validAnalog; i++) {
    AnalogInfo_t *info = &joyData[i];
    uint16_t data = analogRead(info->pin);
    if (!joyData[i].hasDigital) {
      if (info->inverted) data *= -1;
      data = (data - 512) * 64;
    }
    info->value = data;
  }
}

int analogRead(uint8_t pin) {
  adc_select_input(pin);
  return adc_read();
}
void pinMode(uint8_t pin, uint8_t mode) {
  gpio_set_dir(pin, mode == INPUT || mode == INPUT_PULLUP);
  gpio_set_pulls(pin, mode == INPUT_PULLUP, false);
  if (mode == INPUT) { adc_gpio_init(pin); }
}

void setupADC(void) { adc_init(); }

void setUpValidPins(void) {
  for (int i = 0; i < 6; i++) { setUpAnalogPin(i); }
}