#include "input_handler.h"
#include "../config/eeprom.h"
#include "../output/usb/Descriptors.h"
#include "../util.h"
#include "i2c/twi.h"
#include "input_direct.h"
#include "input_guitar.h"
#include "input_ps2_cnt.h"
#include "input_wii_ext.h"
#include "leds.h"
#include "pins/pins.h"
// #include <stdlib.h>
void (*tick_function)(controller_t *);
int jth;
extern bool skip_tick;
void input_init() {
  enableADC();
  switch (config.main.input_type) {
  case WII:
    tick_function = wii_ext_tick;
    break;
  case DIRECT:
    direct_init();
    tick_function = direct_tick;
    break;
  case PS2:
    ps2_cnt_init();
    tick_function = ps2_cnt_tick;
    break;
  }
  twi_init();
  guitar_init();
  led_init();
  jth = config.axis.threshold_joy << 8;
}
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
uint8_t get_value(controller_t *controller, uint8_t offset) {
  if (offset < XBOX_BTN_COUNT) {
    if (is_drum() && offset > 8 && offset < 16) {
      return controller->drum_axis[offset-8];
    }
    return bit_check(controller->buttons, offset) ? MIDI_STANDARD_VELOCITY : 0;
  } else if (offset > XBOX_BTN_COUNT + 2) {
    return ((((controller_a_t *)controller)
                 ->sticks[offset - XBOX_BTN_COUNT - 2]) &
            0xffff) > config.axis.threshold_trigger;
  } else {
    return (((controller_a_t *)controller)->triggers[offset - XBOX_BTN_COUNT]) >
           config.axis.threshold_trigger;
  }
}