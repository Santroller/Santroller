#include "input_handler.h"
#include "eeprom/eeprom.h"
#include "i2c/i2c.h"
#include "inputs/direct.h"
#include "inputs/dj.h"
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
bool (*read_button_function)(Pin_t *pin);
int joyThreshold;
int triggerThreshold;
bool mapJoyLeftDpad;
bool mapStartSelectHome;
bool mergedStrum;
bool ghDrum = false;
Pin_t pinData[XBOX_BTN_COUNT] = {};
Pin_t euphoriaPin;
Pin_t *downStrumPin;
int dpad_bits = ~(_BV(XBOX_DPAD_UP) | _BV(XBOX_DPAD_DOWN) |
                  _BV(XBOX_DPAD_LEFT) | _BV(XBOX_DPAD_RIGHT));
void initInputs(Configuration_t *config) {

  mapJoyLeftDpad = config->main.mapLeftJoystickToDPad;
  mapStartSelectHome = config->main.mapStartSelectToHome;
  mergedStrum = typeIsGuitar && config->debounce.combinedStrum;
  setupADC();
  switch (config->main.inputType) {
  case WII:
    initWiiExtensions(config);
    tick_function = tickWiiExtInput;
    read_button_function = readWiiButton;
    dpad_bits = 0xFF;
    break;
  case DIRECT:
    read_button_function = digitalReadPin;
    break;
  case PS2:
    initPS2CtrlInput(config);
    read_button_function = readPS2Button;
    tick_function = tickPS2CtrlInput;
    dpad_bits = 0xFF;
    break;
  }

  if (config->main.inputType != PS2 && config->main.fretLEDMode == APA102) {
    spi_begin(MIN(F_CPU / 2, 12000000), true, true, false);
  }
  if (typeIsDJ || config->main.inputType == WII ||
      config->main.tiltType == MPU_6050 || config->neck.gh5Neck ||
      config->neck.gh5NeckBar) {
    // Start off by configuring things for the slower speed when using wii
    // extensions twi_init(config->main.inputType == WII);
    twi_init(config->neck.gh5Neck || config->neck.gh5NeckBar, typeIsDJ);
  }
  initDirectInput(config);
  initGuitar(config);
  joyThreshold = config->axis.joyThreshold << 8;
  triggerThreshold = config->axis.triggerThreshold;
  for (uint8_t i = 0; i < validPins; i++) {
    Pin_t *pin = &pinData[i];
    if (pin->offset == XBOX_LEFT_STICK && typeIsDJ) {
      euphoriaPin = *pin;
      // We don't want to treat this like a normal pin, so we move whatever is
      // at the end to this slot, so it isn't treated as a pin anymore
      if (i != validPins - 1) { pinData[i] = pinData[validPins - 1]; }
      validPins--;
    }
    // If we have a pin mapped, then we don't want to be clearing it when map
    // joystick to dpad is in use
    if (pin->offset == XBOX_DPAD_UP) { dpad_bits |= (_BV(XBOX_DPAD_UP)); }
    if (pin->offset == XBOX_DPAD_DOWN) { dpad_bits |= (_BV(XBOX_DPAD_DOWN)); }
    if (pin->offset == XBOX_DPAD_LEFT) { dpad_bits |= (_BV(XBOX_DPAD_LEFT)); }
    if (pin->offset == XBOX_DPAD_RIGHT) { dpad_bits |= (_BV(XBOX_DPAD_RIGHT)); }
    if (pin->offset == XBOX_DPAD_DOWN && mergedStrum) { downStrumPin = pin; }
  }
  ghDrum = config->main.subType == XINPUT_GUITAR_HERO_DRUMS;
}
bool start = false;
bool select = false;
void tickInputs(Controller_t *controller) {
  if (tick_function) { tick_function(controller); }
  tickDirectInput(controller);
  Pin_t *pin;
  Pin_t *pin2;
  for (uint8_t i = 0; i < validPins; i++) {
    pin = &pinData[i];
    pin2 = &pinData[i];
    // If strum is merged, then we want to grab debounce data from the same
    // button for both
    if (mergedStrum && pin->offset == XBOX_DPAD_UP) { pin2 = downStrumPin; }
    if (millis() - pin2->lastMillis > pin2->milliDeBounce) {
      bool val = read_button_function(pin);
      if (mapStartSelectHome) {
        if (pin->offset == XBOX_START) { start = val; }
        if (pin->offset == XBOX_BACK) { select = val; }
      }
      // With DJ controllers, euphoria and y are going to different pins but are
      // the same output.
      if (typeIsDJ && pin->offset == XBOX_Y) {
        val |= read_button_function(&euphoriaPin);
      }
      if (val != (bit_check(controller->buttons, pin->offset))) {
        pin2->lastMillis = millis();
        bit_write(val, controller->buttons, pin->offset);
      }
    }
  }
  if (mapStartSelectHome) {
    if (start && select) {
      bit_set(controller->buttons, XBOX_HOME);
      bit_clear(controller->buttons, XBOX_START);
      bit_clear(controller->buttons, XBOX_BACK);
    } else {
      bit_clear(controller->buttons, XBOX_HOME);
    }
  }
  if (mapJoyLeftDpad) {
    // Reset any bits that were not touched above (aka any unbound directions)
    controller->buttons &= dpad_bits;
    CHECK_JOY(l_x, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT);
    CHECK_JOY(l_y, XBOX_DPAD_DOWN, XBOX_DPAD_UP);
  }
  tickGuitar(controller);
  tickDJ(controller);
  if (ghDrum) { controller->buttons |= _BV(XBOX_LEFT_STICK); }
}
uint8_t getVelocity(Controller_t *controller, uint8_t offset) {
  if (offset < XBOX_BTN_COUNT) {
    if (typeIsDrum && offset > 8 && offset < 16) {
      return drumVelocity[offset - 8];
    }
    return bit_check(controller->buttons, offset) ? MIDI_STANDARD_VELOCITY : 0;
  } else if (offset > XBOX_BTN_COUNT + 2) {
    return ((((ControllerCombined_t *)controller)
                 ->sticks[offset - XBOX_BTN_COUNT - 2]) &
            0xffff) > triggerThreshold;
  } else {
    return (((ControllerCombined_t *)controller)
                ->triggers[offset - XBOX_BTN_COUNT]) > triggerThreshold;
  }
}