#include "input_handler.h"
#include "config/eeprom.h"
#include "leds/leds.h"
#include "output/descriptors.h"
#include "util/util.h"
#include "i2c/i2c.h"
#include "inputs/direct.h"
#include "inputs/guitar.h"
#include "inputs/ps2_cnt.h"
#include "inputs/wii_ext.h"
#include "pins/pins.h"
// #include <stdlib.h>
void (*tick_function)(Controller_t *);
int joyThreshold;
void initInputs() {
  setupADC();
  setupMicrosTimer();
  switch (config.main.inputType) {
  case WII:
    tick_function = tickWiiExtInput;
    break;
  case DIRECT:
    initDirectInput();
    break;
  case PS2:
    initPS2CtrlInput();
    tick_function = tickPS2CtrlInput;
    break;
  }
  initI2C();
  initGuitar();
  joyThreshold = config.axis.joyThreshold << 8;
}
void tickInputs(Controller_t *controller) {
  controller->buttons = 0;
  if (tick_function) {
    tick_function(controller);
  }
  tickDirectInput(controller);
  if (config.main.mapLeftJoystickToDPad) {
    CHECK_JOY(l_x, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT);
    CHECK_JOY(l_y, XBOX_DPAD_DOWN, XBOX_DPAD_UP);
  }
  if (config.main.mapStartSelectToHome) {
    if (bit_check(controller->buttons, XBOX_START) &&
        bit_check(controller->buttons, XBOX_BACK)) {
      bit_clear(controller->buttons, XBOX_START);
      bit_clear(controller->buttons, XBOX_BACK);
      bit_set(controller->buttons, XBOX_HOME);
    }
  }
  tickGuitar(controller);
}
uint8_t getVelocity(Controller_t *controller, uint8_t offset) {
  if (offset < XBOX_BTN_COUNT) {
    if (isDrum() && offset > 8 && offset < 16) {
      return controller->drumVelocity[offset - 8];
    }
    return bit_check(controller->buttons, offset) ? MIDI_STANDARD_VELOCITY : 0;
  } else if (offset > XBOX_BTN_COUNT + 2) {
    return ((((ControllerCombined_t *)controller)
                 ->sticks[offset - XBOX_BTN_COUNT - 2]) &
            0xffff) > config.axis.triggerThreshold;
  } else {
    return (((ControllerCombined_t *)controller)
                ->triggers[offset - XBOX_BTN_COUNT]) >
           config.axis.triggerThreshold;
  }
}