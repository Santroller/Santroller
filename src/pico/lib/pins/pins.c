#include "pins/pins.h"
#include "eeprom/eeprom.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "stddef.h"
#include "util/util.h"

void digitalWrite(uint8_t pin, uint8_t val) { gpio_put(pin, val); }

bool digitalRead(uint8_t pin) { return gpio_get(pin) != 0; }
Pin_t setUpDigital(Configuration_t *config, uint8_t pinNum, uint8_t offset,
                   bool inverted, bool output) {
  Pin_t pin = {};
  pin.offset = offset;
  pin.pin = pinNum;
  pin.eq = inverted;
  pin.sioFunc = true;
  pin.analogOffset = INVALID_PIN;
  pin.milliDeBounce = config->debounce.buttons;
  pin.lastMillis = 0;
  return pin;
}
bool digitalReadPin(Pin_t pin) {
  if (pin.analogOffset == INVALID_PIN) {
    return (gpio_get(pin.pin) != 0) == pin.eq;
  }
  AnalogInfo_t info = joyData[pin.analogOffset];
  return info.value > info.threshold;
}
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
void setUpAnalogPin(Configuration_t *config, uint8_t offset) {
  AnalogInfo_t ret = {0};
  ret.offset = offset;
  AnalogPin_t apin = ((PinsCombined_t *)&config->pins)->axis[offset];
  uint8_t pin = apin.pin;
  if (pin == INVALID_PIN) { return; }
  if (ret.offset == 5 && typeIsGuitar && config->main.tiltType != ANALOGUE) {
    return;
  }
  ret.pin = pin;
  ret.hasDigital = false;
  ret.inverted = apin.inverted;
  pinMode(pin, INPUT);
  joyData[validAnalog++] = ret;
}
void setUpAnalogDigitalPin(Pin_t *button, uint8_t pin, uint16_t threshold) {
  AnalogInfo_t ret = {0};
  ret.offset = pin;
  ret.hasDigital = true;
  ret.threshold = threshold;
  ret.pin = pin;
  pinMode(pin, INPUT);
  button->analogOffset = validAnalog;
  joyData[validAnalog++] = ret;
}
void tickAnalog(void) {
  if (validAnalog == 0) return;
  for (int i = 0; i < validAnalog; i++) {
    AnalogInfo_t *info = &joyData[i];
    adc_select_input(info->pin - PIN_A0);
    // We have everything coded assuming 10 bits (as that is what the arduino
    // uses) so shift accordingly (12 -> 10)
    int16_t data = adc_read() >> 2;
    if (!joyData[i].hasDigital) {
      data = (data - 512);
      if (info->inverted) data = -data;
    }
    data = data * 64;
    info->value = data;
  }
}

uint16_t analogDetectRead(uint8_t pin) {
  // This function is used for pin detection. On the pico, we can't combine
  // pulling and the ADC, so instead, we weakly pull in both directions, and
  // that way if the signal changes we will pick it up.
  return digitalRead(PIN_A0 + pin) ? 100 : 0;
}
void pinMode(uint8_t pin, uint8_t mode) {
  if ((mode == INPUT) && pin >= PIN_A0) {
    adc_gpio_init(pin);
  } else {
    gpio_init(pin);
    gpio_set_dir(pin, mode == OUTPUT);
    gpio_set_pulls(pin, mode == INPUT_PULLUP || mode == INPUT_PULLUP_ANALOG,
                   mode == INPUT_PULLUP_ANALOG);
  }
}

void setupADC(void) { adc_init(); }

void setUpValidPins(Configuration_t *config) {
  for (int i = 0; i < 6; i++) { setUpAnalogPin(config, i); }
}