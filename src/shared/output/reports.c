#include "reports.h"
#include "../util.h"

// Bindings to go from controller to ps3
static const uint8_t PROGMEM ps3ButtonBindings[] = {
    XBOX_Y,    XBOX_A,     XBOX_B,          XBOX_X,
    0xff,      0xff,       XBOX_LB,         XBOX_RB,
    XBOX_BACK, XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME, XBOX_UNUSED};

static const uint8_t PROGMEM ps3AxisBindings[] = {
    XBOX_DPAD_UP, XBOX_DPAD_RIGHT, XBOX_DPAD_DOWN, XBOX_DPAD_LEFT, 0xFF,
    0xFF,         XBOX_LB,         XBOX_RB,        XBOX_Y,         XBOX_B,
    XBOX_A,       XBOX_X};
static const uint8_t PROGMEM ghAxisBindings[] = {
    XBOX_DPAD_LEFT, XBOX_DPAD_DOWN, XBOX_DPAD_RIGHT,
    XBOX_DPAD_UP,   XBOX_X,         XBOX_B};
// Annoyingly, axis bindings for GH guitars overlap. This is the set of axis
// that overlap the previous list.
static const uint8_t PROGMEM ghAxisBindings2[] = {0xff, XBOX_LB, XBOX_Y,
                                                  XBOX_A};
static const uint8_t PROGMEM hat_bindings[] = {
    0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
static const uint8_t *currentAxisBindings;
static uint8_t currentAxisBindingsLen = 0;
void report_init(void) {
  if (config.main.sub_type > SWITCH_GAMEPAD) {
    currentAxisBindings = ps3AxisBindings;
    currentAxisBindingsLen = sizeof(ps3AxisBindings);
    if (config.main.sub_type > PS3_GAMEPAD) {
      currentAxisBindings = ghAxisBindings;
      currentAxisBindingsLen = sizeof(ghAxisBindings);
    }
  }
}

void check_joy_key(int neg, int pos, int val, int thresh, uint8_t *used,
                   USB_KeyboardReport_Data_t *KeyboardReport) {
  if (*used < SIMULTANEOUS_KEYS) {
    if (neg && val < -thresh) { KeyboardReport->KeyCode[*used++] = neg; }
    if (pos && val > thresh) { KeyboardReport->KeyCode[*used++] = pos; }
  }
}

void create_report(void *ReportData, uint16_t *const ReportSize,
                   controller_t controller) {

  if (config.main.sub_type <= XINPUT_ARCADE_PAD) {
    *ReportSize = sizeof(USB_XInputReport_Data_t);

    USB_XInputReport_Data_t *JoystickReport =
        (USB_XInputReport_Data_t *)ReportData;
    JoystickReport->rsize = sizeof(USB_XInputReport_Data_t);
    // Don't copy the extra device information tagged on the end of the
    // controller.
    memcpy(&JoystickReport->buttons, &controller,
           sizeof(controller_t) - sizeof(uint16_t));
  } else if (config.main.sub_type == KEYBOARD) {
    USB_KeyboardReport_Data_t *KeyboardReport =
        (USB_KeyboardReport_Data_t *)ReportData;
    uint8_t usedKeys = 0;
    uint8_t *keys = (uint8_t *)&config.keys;
    for (int i = 0; i <= XBOX_Y && usedKeys < SIMULTANEOUS_KEYS; i++) {
      uint8_t binding = keys[i];
      if (binding && bit_check(controller.buttons, i)) {
        KeyboardReport->KeyCode[usedKeys++] = binding;
      }
    }
    CHECK_JOY_KEY(l_x);
    CHECK_JOY_KEY(l_y);
    CHECK_JOY_KEY(r_x);
    CHECK_JOY_KEY(r_y);
    CHECK_TRIGGER_KEY(lt);
    CHECK_TRIGGER_KEY(rt);
    *ReportSize = sizeof(USB_KeyboardReport_Data_t);
  } else {
    USB_PS3Report_Data_t *JoystickReport = (USB_PS3Report_Data_t *)ReportData;
    uint8_t button;
    for (uint8_t i = 0; i < sizeof(ps3ButtonBindings); i++) {
      button = pgm_read_byte(ps3ButtonBindings + i);
      if (button == 0xff) continue;
      bool bit_set = bit_check(controller.buttons, button);
      bit_write(bit_set, JoystickReport->buttons, i);
    }
    if (config.main.sub_type == SWITCH_GAMEPAD) {
      // Swap a and b on the switch
      COPY(A, B);
      COPY(B, A);
    }
    for (uint8_t i = 0; i < currentAxisBindingsLen; i++) {
      button = pgm_read_byte(currentAxisBindings + i);
      if (button == 0xff) continue;
      bool bit_set = bit_check(controller.buttons, button);
      if (config.main.sub_type == PS3_GUITAR_HERO_GUITAR &&
          i < sizeof(ghAxisBindings2)) {
        button = pgm_read_byte(ghAxisBindings2 + i);
        bit_set |= bit_check(controller.buttons, button);
      }
      JoystickReport->axis[i] = bit_set ? 0xFF : 0x00;
    }

    // Hat Switch
    button = controller.buttons & 0xF;
    JoystickReport->hat =
        button > 0x0a ? 0x08 : pgm_read_byte(hat_bindings + button);

    // Tilt / whammy
    bool tilt = controller.r_y == 32767;
    if (config.main.sub_type == PS3_GUITAR_HERO_GUITAR) {
      JoystickReport->r_x = (controller.r_x >> 8) + 128;
      // GH PS3 guitars have a tilt axis
      JoystickReport->accel[0] = tilt ? 0x0184 : 0x01f7;
    }
    if (config.main.sub_type == PS3_ROCK_BAND_GUITAR) {
      JoystickReport->r_x = 128 - (controller.r_x >> 8);
      // RB PS3 guitars use R for a tilt bit
      bit_write(tilt, JoystickReport->buttons, SWITCH_R);
      // Swap y and x, as RB controllers have them inverted
      COPY(X, Y);
      COPY(Y, X);
    }
    if (config.main.sub_type == PS3_GUITAR_HERO_GUITAR ||
        config.main.sub_type == PS3_ROCK_BAND_GUITAR) {
      // XINPUT guitars use LB for orange, PS3 uses L
      COPY(LB, L);
    }
    if (config.main.sub_type == PS3_GUITAR_HERO_DRUMS ||
        config.main.sub_type == PS3_ROCK_BAND_DRUMS) {

      // XINPUT guitars use LB for orange, PS3 uses R
      COPY(LB, R);
      // XINPUT guitars use RB for bass, PS3 uses L
      COPY(RB, L);
    }
    if (config.main.sub_type == PS3_GAMEPAD ||
        config.main.sub_type == SWITCH_GAMEPAD) {
      bit_write(controller.lt > 50, JoystickReport->buttons, SWITCH_L);
      bit_write(controller.rt > 50, JoystickReport->buttons, SWITCH_R);
      JoystickReport->axis[4] = controller.lt;
      JoystickReport->axis[5] = controller.rt;
      JoystickReport->l_x = (controller.l_x >> 8) + 128;
      JoystickReport->l_y = (controller.l_y >> 8) + 128;
      JoystickReport->r_x = (controller.r_x >> 8) + 128;
      JoystickReport->r_y = (controller.r_y >> 8) + 128;
    } else {
      JoystickReport->l_x = 0x80;
      JoystickReport->l_y = 0x80;
      // r_y is tap, so lets disable it.
      JoystickReport->r_y = 0x7d;
    }
    *ReportSize = sizeof(USB_PS3Report_Data_t);
  }
}