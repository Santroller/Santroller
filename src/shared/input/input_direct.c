#include "input_direct.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "input_guitar.h"
#include "pins/pins.h"
#include <stdlib.h>
pin_t pinData[16];
int validPins = 0;
typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
  ledstate_t leds;
} controller_a_t;
void direct_init() {
  uint8_t *pins = (uint8_t *)&config.pins;
  validPins = 0;
  setUpValidPins();
  if (config.main.input_type == DIRECT) {
    for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
      if (pins[i] != INVALID_PIN) {
        bool is_fret = (i >= XBOX_A || i == XBOX_LB);
        pin_t pin = {};
        pin.mask = digitalPinToBitMask(pins[i]);
        pin.port = portInputRegister(digitalPinToPort((pins[i])));
        pin.pmask = _BV(i);
        pin.eq = is_fret && config.main.fret_mode == LEDS_INLINE;
        if (is_drum() && is_fret) {
          // We should probably keep a list of drum specific buttons, instead of
          // using isfret
          setUpAnalogDigitalPin(pin, pins[i], config.new_items.threshold_drums);
        } else {
          pinMode(pins[i], pin.eq ? INPUT : INPUT_PULLUP);
          pinData[validPins++] = pin;
        }
      }
    }
  }
  startADC();
}
uint8_t find_digital(void) {
  for (int i = 2; i < NUM_DIGITAL_PINS; i++) { pinMode(i, INPUT_PULLUP); }
  while (true) {
    for (int i = 2; i < NUM_DIGITAL_PINS; i++) {
      if (!digitalRead(i)) {
        for (int i = 2; i < NUM_DIGITAL_PINS; i++) { pinMode(i, INPUT); }
        direct_init();
        return i;
      }
    }
  }
}
uint8_t find_analog(void) {
  stopReading();
  int last[NUM_ANALOG_INPUTS];
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
    pinMode(A0 + i, INPUT_PULLUP);
    last[i] = analogRead(i);
  }
  while (true) {
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
      if (abs(analogRead(i) - last[i]) > 10) {
        direct_init();
        return i+A0;
      }
    }
  }
}
void direct_tick(controller_t *controller) {
  pin_t pin;
  for (uint8_t i = 0; i < validPins; i++) {
    pin = pinData[i];
    if ((*pin.port & pin.mask) == pin.eq) { controller->buttons |= pin.pmask; }
  }
  analog_info_t info;
  for (int8_t i = 0; i < validAnalog; i++) {
    info = joyData[i];
    if (info.hasDigital) {
      if (info.value > info.threshold) {
        controller->buttons |= info.digital.pmask;
      }
    } else if (info.offset >= 2) {
      ((controller_a_t *)controller)->sticks[info.offset - 2] = info.value;
    } else {
      ((controller_a_t *)controller)->triggers[info.offset] = info.value >> 8;
    }
  }

  resetADC();
}
