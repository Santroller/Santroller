#include "input_handler.h"
#include "eeprom/eeprom.h"
#include "i2c/i2c.h"
#include "inputs/direct.h"
#include "inputs/guitar.h"
#include "inputs/ps2_cnt.h"
#include "inputs/wii_ext.h"
#include "leds/leds.h"
#include "output/descriptors.h"
#include "pins/pins.h"
#include "spi/spi.h"
#include "util/util.h"
#include <stdlib.h>
void (*tick_function)(Controller_t *);
bool (*read_button_function)(Pin_t pin);
int joyThreshold;
Pin_t pinData[XBOX_BTN_COUNT] = {};
void initInputs() {
  setupADC();
  switch (config.main.inputType) {
  case WII:
    tick_function = tickWiiExtInput;
    read_button_function = readWiiButton;
    break;
  case DIRECT:
    initDirectInput();
    read_button_function = digitalReadPin;
    break;
  case PS2:
    initPS2CtrlInput();
    read_button_function = readPS2Button;
    tick_function = tickPS2CtrlInput;
    break;
  }
  if (config.main.inputType != PS2 && config.main.fretLEDMode == APA102) {
    spi_begin(F_CPU / 2, true, true);
  }
  if (config.main.inputType == WII || config.main.tiltType == MPU_6050) {
    twi_init();
  }
  initDirectInput();
  initGuitar();
  joyThreshold = config.axis.joyThreshold << 8;
}
void tickInputs(Controller_t *controller) {
  if (tick_function) { tick_function(controller); }
  tickDirectInput(controller);
  Pin_t pin;
  for (uint8_t i = 0; i < validPins; i++) {
    pin = pinData[i];
    bool val = read_button_function(pin);
    if (millis() - pin.lastMillis > pin.milliDeBounce) {
      if (val) {
        pin.lastMillis = millis();
      }
      bit_write(val, controller->buttons, pin.offset);
    }
  }
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
    if (isDrum(config.main.subType) && offset > 8 && offset < 16) {
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