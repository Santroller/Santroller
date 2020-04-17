#include "input_handler.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "i2c/twi.h"
#include "input_direct.h"
#include "input_guitar.h"
#include "input_wii_ext.h"
#include "leds.h"
#include "pins/pins.h"
// #include <stdlib.h>
void (*tick_function)(controller_t *);
int jth;
void input_init() {
  switch (config.main.input_type) {
  case WII:
    tick_function = wii_ext_tick;
    break;
  case DIRECT:
    direct_init();
    tick_function = direct_tick;
  }
  twi_init();
  enableADC();
  sei();
  guitar_init();
  led_init();
  jth = config.axis.threshold_joy << 8;
}
int i = _BV(XBOX_A);
int j = _BV(XBOX_B);
void input_tick(controller_t *controller) {
  controller->buttons = 0;
  tick_function(controller);
  // controller->l_x=rand();
  if (config.main.map_joy_to_dpad) {
    CHECK_JOY(l_x, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT);
    CHECK_JOY(l_y, XBOX_DPAD_DOWN, XBOX_DPAD_UP);
  }
  if (config.main.map_start_select_to_home) {
    if (bit_check(controller->buttons, XBOX_START) &&
        bit_check(controller->buttons, XBOX_BACK)) {
      bit_clear(controller->buttons, XBOX_START);
      bit_clear(controller->buttons, XBOX_BACK);
      bit_set(controller->buttons, XBOX_HOME);
    }
  }
  guitar_tick(controller);
  led_tick(controller);
}