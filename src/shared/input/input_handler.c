#include "input_handler.h"
#include "../config/eeprom.h"
#include "i2c/i2c_dev.h"
#include "input_direct.h"
#include "input_guitar.h"
#include "input_wii_ext.h"
#include "pins/pins.h"
#include "../util.h"
void (*tick_function)(controller_t *);
void input_init(void) {
  switch (config.input_type) {
  case WII:
    tick_function = wii_ext_tick;
    break;
  case DIRECT:
    direct_init();
    tick_function = direct_tick;
  }
  sei();
  i2c_init();
  enableADC();
  guitar_init();
}

void input_tick(controller_t *controller) {
  tick_function(controller);
  if (config.map_joy_to_dpad) {
    CHECK_JOY(l_x, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT);
    CHECK_JOY(l_y, XBOX_DPAD_DOWN, XBOX_DPAD_UP);
  }
  guitar_tick(controller);
}