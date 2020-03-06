#include "input_direct.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "pins/pins.h"
void direct_init(void) {
  int fret_type = config.main.fret_mode == LEDS_INLINE ? INPUT : INPUT_PULLUP;
  uint8_t *pins = (uint8_t *)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    if (pins[i] != INVALID_PIN) {
      uint8_t fret_type_2 = (i < XBOX_A || i == XBOX_LB) ? INPUT_PULLUP : fret_type;
      pinMode(pins[i], fret_type_2);
    }
  }
  DEFINE_JOY(l_x);
  DEFINE_JOY(l_y);
  DEFINE_JOY(r_x);
  DEFINE_JOY(r_y);
  DEFINE_JOY(lt);
  DEFINE_JOY(rt);
}

void direct_tick(controller_t *controller) {
  uint8_t *pins = (uint8_t *)&config.pins;
  for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
    if (pins[i] != INVALID_PIN) {
      bool eq = (i < XBOX_A || i == XBOX_LB) ? false : config.main.fret_mode == LEDS_INLINE;
      bit_write(digitalRead(pins[i]) == eq, controller->buttons, i);
    }
  }
  // Why does this ADC stuff add so much of a delay?
  READ_JOY(l_x);
  READ_JOY(l_y);
  READ_JOY(r_x);
  READ_JOY(r_y);
  READ_JOY(lt);
  READ_JOY(rt);
}