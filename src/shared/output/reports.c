#include "reports.h"
#include "../util.h"

// Bindings to go from controller to ps3
static uint8_t ps3ButtonBindings[] = {
    XBOX_Y,    XBOX_A,     XBOX_B,          XBOX_X,
    0xff,      0xff,       XBOX_LB,         XBOX_RB,
    XBOX_BACK, XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME, XBOX_UNUSED};
static const uint8_t PROGMEM psGHButtonBindings[] = {
    XBOX_Y,     XBOX_A,          XBOX_B,
    XBOX_X,     XBOX_LB,         0xff,
    0xff,       XBOX_RB,         XBOX_BACK,
    XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME,  XBOX_UNUSED};
static const uint8_t PROGMEM psRBButonBindings[] = {
    XBOX_X,     XBOX_A,          XBOX_B,
    XBOX_Y,     XBOX_LB,         0xff,
    0xff,       XBOX_RB,         XBOX_BACK,
    XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME,  XBOX_UNUSED};

static const uint8_t PROGMEM ps3DrumButtonBindings[] = {XBOX_Y,
                                                        XBOX_A,
                                                        XBOX_B,
                                                        XBOX_X,
                                                        XBOX_RB,
                                                        XBOX_LB,
                                                        0xFF,
                                                        0xFF,
                                                        XBOX_BACK,
                                                        XBOX_START,
                                                        XBOX_LEFT_STICK,
                                                        XBOX_RIGHT_STICK,
                                                        XBOX_HOME,
                                                        XBOX_UNUSED};
static uint8_t ps3AxisBindings[] = {
    XBOX_DPAD_UP, XBOX_DPAD_RIGHT, XBOX_DPAD_DOWN, XBOX_DPAD_LEFT, 0xFF,
    0xFF,         XBOX_LB,         XBOX_RB,        XBOX_Y,         XBOX_B,
    XBOX_A,       XBOX_X};
static const uint8_t ghAxisBindings[] = {XBOX_DPAD_LEFT,  XBOX_DPAD_DOWN,
                                         XBOX_DPAD_RIGHT, XBOX_DPAD_UP,
                                         XBOX_X,          XBOX_B};
// Annoyingly, axis bindings for GH guitars overlap. This is the set of axis
// that overlap the previous list.
static const uint8_t PROGMEM ghAxisBindings2[] = {0xff, XBOX_LB, XBOX_Y,
                                                  XBOX_A};
static const uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07,
                                       0x05, 0x08, 0x02, 0x01, 0x03};
uint8_t currentAxisBindingsLen = 0;
int jth;
int tth;
void check_joy_key(int neg, int pos, int val, int thresh, uint8_t *used,
                   USB_KeyboardReport_Data_t *KeyboardReport) {
  if (*used < SIMULTANEOUS_KEYS) {
    if (neg && val < -thresh) { KeyboardReport->KeyCode[*used++] = neg; }
    if (pos && val > thresh) { KeyboardReport->KeyCode[*used++] = pos; }
  }
}
void create_xinput_report(void *ReportData, uint16_t *const ReportSize,
                          controller_t controller) {
  *ReportSize = sizeof(USB_XInputReport_Data_t);

  USB_XInputReport_Data_t *JoystickReport =
      (USB_XInputReport_Data_t *)ReportData;
  JoystickReport->rsize = sizeof(USB_XInputReport_Data_t);
  // Don't copy the led info tagged on the end
  memcpy(&JoystickReport->buttons, &controller,
         sizeof(controller_t) - sizeof(ledstate_t));
}
void create_keyboard_report(void *ReportData, uint16_t *const ReportSize,
                            controller_t controller) {
  *ReportSize = sizeof(USB_KeyboardReport_Data_t);
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
}
void create_ps3_report(void *ReportData, uint16_t *const ReportSize,
                       controller_t controller) {
  *ReportSize = sizeof(USB_PS3Report_Data_t);
  USB_PS3Report_Data_t *JoystickReport = (USB_PS3Report_Data_t *)ReportData;
  uint8_t button;
  for (uint8_t i = 0; i < sizeof(ps3ButtonBindings); i++) {
    button = ps3ButtonBindings[i];
    if (button == 0xff) continue;
    bool bit_set = bit_check(controller.buttons, button);
    bit_write(bit_set, JoystickReport->buttons, i);
    if (i < currentAxisBindingsLen) {
      button = ps3AxisBindings[i];
      if (config.main.sub_type == PS3_GUITAR_HERO_GUITAR &&
          i < sizeof(ghAxisBindings2)) {
        button = ghAxisBindings2[i];
        bit_set |= bit_check(controller.buttons, button);
      }
      JoystickReport->axis[i] = bit_set ? 0xFF : 0x00;
    }
  }

  // Hat Switch
  button = controller.buttons & 0xF;
  JoystickReport->hat = button > 0x0a ? 0x08 : hat_bindings[button];

  // Tilt / whammy
  bool tilt = controller.r_y == 32767;
  if (config.main.sub_type == PS3_GUITAR_HERO_GUITAR) {
    JoystickReport->r_x = (controller.r_x >> 8) + 128;
    // GH PS3 guitars have a tilt axis
    JoystickReport->accel[0] = tilt ? 0x0184 : 0x01f7;
    // r_y is tap, so lets disable it.
    JoystickReport->r_y = 0x7d;
  } else if (config.main.sub_type == PS3_ROCK_BAND_GUITAR || config.main.sub_type == WII_ROCK_BAND_GUITAR) {
    JoystickReport->r_x = 128 - (controller.r_x >> 8);
    // RB PS3 guitars use R for a tilt bit
    bit_write(tilt, JoystickReport->buttons, SWITCH_R);
    // r_y is the tone switch. Since lt isnt used, but r_y gets used by tilt, we map fx to lt, and then fix it here
    JoystickReport->r_y = 128 - controller.lt;
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
    // l_x and l_y are unused on guitars and drums. Center them.
    JoystickReport->l_x = 0x80;
    JoystickReport->l_y = 0x80;
  }
}
void (*create_report)(void *ReportData, uint16_t *const ReportSize,
                      controller_t controller) = NULL;

void report_init(void) {
  if (config.main.sub_type > SWITCH_GAMEPAD) {
    if (config.main.sub_type > PS3_GAMEPAD) {
      memcpy_P(ps3AxisBindings, ghAxisBindings, sizeof(ghAxisBindings));
      currentAxisBindingsLen = sizeof(ghAxisBindings);
    }
  }
  if (config.main.sub_type == PS3_GUITAR_HERO_DRUMS ||
      config.main.sub_type == PS3_ROCK_BAND_DRUMS) {
    memcpy_P(ps3ButtonBindings, ps3DrumButtonBindings,
             sizeof(ps3DrumButtonBindings));
  } else if (config.main.sub_type == PS3_GUITAR_HERO_GUITAR) {
    memcpy_P(ps3ButtonBindings, psGHButtonBindings, sizeof(ps3ButtonBindings));
  } else if (config.main.sub_type == PS3_ROCK_BAND_GUITAR) {
    memcpy_P(ps3ButtonBindings, psRBButonBindings, sizeof(ps3ButtonBindings));
  }

  if (config.main.sub_type <= XINPUT_ARCADE_PAD) {
    create_report = create_xinput_report;
  } else if (config.main.sub_type == KEYBOARD) {
    create_report = create_keyboard_report;
  } else {
    create_report = create_ps3_report;
  }
  jth = config.axis.threshold_joy << 8;
  tth = config.axis.threshold_trigger << 8;
}