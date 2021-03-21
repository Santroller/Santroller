#include "pins/pins.h"
#include "controller/guitar_includes.h"
#include "eeprom/eeprom.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "stddef.h"
#include "util/util.h"

void digitalWrite(uint8_t pin, uint8_t val) { gpio_put(pin, val); }

bool digitalRead(uint8_t pin) { return gpio_get(pin) != 0; }
Pin_t setUpDigital(uint8_t pinNum, uint8_t offset, bool inverted) {
  Pin_t pin = {};
  pin.offset = offset;
  pin.pin = pinNum;
  pin.pmask = _BV(offset);
  pin.eq = inverted;
  pin.sioFunc = true;
  return pin;
}
bool digitalReadPin(Pin_t pin) { return (gpio_get(pin.pin) != 0) == pin.eq; }
void digitalWritePin(Pin_t pin, bool value) {
  // If SIO is disabled for a pin (aka its using a different function like i2c
  // or spi), then digitalWrite needs to override it.
  if (!pin.sioFunc) {
    if (value) {
      // Enable output
      gpio_set_oeover(pin.pin, GPIO_OVERRIDE_HIGH);
      gpio_set_outover(pin.pin, GPIO_OVERRIDE_HIGH);
    } else {
      gpio_set_oeover(pin.pin, GPIO_OVERRIDE_NORMAL);
      gpio_set_outover(pin.pin, GPIO_OVERRIDE_NORMAL);
    }
    return;
  }
  gpio_put(pin.pin, value);
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
    uint16_t data = analogRead(info->pin - PIN_A0);
    if (!joyData[i].hasDigital) {
      if (info->inverted) data *= -1;
      data = (data - 512) * 64;
    }
    info->value = data;
  }
}

uint16_t analogRead(uint8_t pin) {
  adc_select_input(pin + PIN_A0);
  // We have everything coded assuming 10 bits (as that is what the arduino
  // uses) so shift accordingly (12 -> 10)
  return adc_read() >> 2;
}
void pinMode(uint8_t pin, uint8_t mode) {
  if (mode == INPUT && pin >= PIN_A0) {
    adc_gpio_init(pin);
  } else {
    gpio_init(pin);
    gpio_set_dir(pin, mode == OUTPUT);
    gpio_set_pulls(pin, mode == INPUT_PULLUP, false);
  }
}

void setupADC(void) { adc_init(); }

void setUpValidPins(void) {
  for (int i = 0; i < 6; i++) { setUpAnalogPin(i); }
}