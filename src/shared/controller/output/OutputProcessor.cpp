#include "OutputProcessor.h"
void OutputProcessor::process(Controller *controller)
{
  if (bit_check(controller->buttons, START) &&
      bit_check(controller->buttons, SELECT))
  {
    bootloader();
  }
#if OUTPUT_TYPE == XINPUT
  bit_write(bit_check(controller->buttons, UP), gamepad_state.digital_buttons_1,
            XBOX_DPAD_UP);
  bit_write(bit_check(controller->buttons, DOWN),
            gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN);
  bit_write(bit_check(controller->buttons, LEFT),
            gamepad_state.digital_buttons_1, XBOX_DPAD_LEFT);
  bit_write(bit_check(controller->buttons, RIGHT),
            gamepad_state.digital_buttons_1, XBOX_DPAD_RIGHT);
  bit_write(bit_check(controller->buttons, START),
            gamepad_state.digital_buttons_1, XBOX_START);
  bit_write(bit_check(controller->buttons, SELECT),
            gamepad_state.digital_buttons_1, XBOX_BACK);
  bit_write(bit_check(controller->buttons, GREEN),
            gamepad_state.digital_buttons_2, XBOX_A);
  bit_write(bit_check(controller->buttons, RED),
            gamepad_state.digital_buttons_2, XBOX_B);
  bit_write(bit_check(controller->buttons, YELLOW),
            gamepad_state.digital_buttons_2, XBOX_Y);
  bit_write(bit_check(controller->buttons, BLUE),
            gamepad_state.digital_buttons_2, XBOX_X);
  bit_write(bit_check(controller->buttons, ORANGE),
            gamepad_state.digital_buttons_2, XBOX_LB);
  bit_write(bit_check(controller->buttons, RB), gamepad_state.digital_buttons_2,
            XBOX_RB);
  bit_write(bit_check(controller->buttons, HOME),
            gamepad_state.digital_buttons_2, XBOX_HOME);

  gamepad_state.l_x = controller->l_x;
  gamepad_state.l_y = controller->l_y;
  gamepad_state.r_x = controller->r_x;
  gamepad_state.r_y = controller->r_y;
  gamepad_state.lt = controller->lt;
  gamepad_state.rt = controller->rt;
  xbox_send_pad_state();
  xbox_reset_watchdog();
#elif OUTPUT_TYPE == KEYBOARD
  bit_write(controller->r_x < -300, controller->buttons, K_WHAMMY);
  bit_write(controller->r_y == 32767, controller->buttons, SELECT);
  keyboard_state = controller->buttons;
  update();
#endif
}

void OutputProcessor::init()
{
#if OUTPUT_TYPE == XINPUT
  xbox_init(true);
#elif OUTPUT_TYPE == KEYBOARD
  SetupHardware();
#endif
}