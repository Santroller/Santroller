#include "input_handler.h"
#include "../config/eeprom.h"
#include "input_direct.h"
#include "input_wii_ext.h"
#include "input_guitar.h"
#include "i2c/i2c_dev.h"
#include "pins/pins.h"
void (*tick_function)(controller_t *);
void input_init(void) {
  switch (config.input_type) {
  case WII:
    wii_ext_init();
    tick_function = wii_ext_tick;
    break;
  case DIRECT:
    direct_init();
    tick_function = direct_tick;
  }
  i2c_init();
  enableADC();
  guitar_init();
}

void input_tick(controller_t *controller) { tick_function(controller); }