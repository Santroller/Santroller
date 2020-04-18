#include "input_direct.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "pins/pins.h"
typedef struct {
  uint8_t mask;
  volatile uint8_t *port;
  uint16_t pmask;
  bool eq;
} pin_t;
pin_t pinData[16];
int validPins = 0;
typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
  ledstate_t leds;
} controller_a_t;
void direct_init() {
  int fret_type = config.main.fret_mode == LEDS_INLINE ? INPUT : INPUT_PULLUP;
  uint8_t *pins = (uint8_t *)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    if (pins[i] != INVALID_PIN) {
      uint8_t fret_type_2 =
          (i < XBOX_A || i == XBOX_LB) ? INPUT_PULLUP : fret_type;
      pinMode(pins[i], fret_type_2);
      pin_t pin = {};
      pin.mask = digitalPinToBitMask(pins[i]);
      pin.port = portInputRegister(digitalPinToPort((pins[i])));
      pin.pmask = _BV(i);
      pin.eq = (i < XBOX_A || i == XBOX_LB)
                   ? false
                   : config.main.fret_mode == LEDS_INLINE;
      pinData[validPins++] = pin;
    }
  }
  for (int i = 0; i < 6; i++) { setUpPin(i); }
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
    if (info.offset >= 2) {
      ((controller_a_t *)controller)->sticks[info.offset - 2] = info.value;
    } else {
      ((controller_a_t *)controller)->triggers[info.offset] = info.value;
    }
  }
  resetADC();
}