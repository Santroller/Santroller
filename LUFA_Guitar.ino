extern "C" {
  #include "XInputPad.h"
  #include "util.h"
}
#include <NintendoExtensionCtrl.h>
GuitarController guitar;
void setup()
{
  xbox_init(true);
  // Disable JTAG
  bit_set(MCUCR, 1 << JTD);
  bit_set(MCUCR, 1 << JTD);
  pinMode(4, INPUT_PULLUP); 
  guitar.begin();
  while (!guitar.connect()) {
    delay(1000);
  }
}
uint8_t pad_up, pad_down, pad_left, pad_right, pad_y, pad_b, pad_x, pad_a, pad_black, pad_start, pad_select;
void loop()
{
  guitar.update();
  xbox_reset_watchdog();
  pad_up = guitar.strumUp() || guitar.joyY()>40;
  pad_down = guitar.strumDown() || guitar.joyY()<20;
  pad_left = guitar.joyX()<20;
  pad_right = guitar.joyX()>40;
  pad_a = guitar.fretGreen();
  pad_b = guitar.fretRed();
  pad_y = guitar.fretYellow();
  pad_x = guitar.fretBlue();
  pad_black =  guitar.fretOrange();
  pad_start =  guitar.buttonPlus();
  pad_select =  guitar.buttonMinus();
  gamepad_state.r_y = 0;
  gamepad_state.r_x = (guitar.whammyBar()-14)*1024;
  
  if(!digitalRead(4)) {
    gamepad_state.r_y = 32767;
  }

  pad_up    ? bit_set(gamepad_state.digital_buttons_1, XBOX_DPAD_UP)    : bit_clear(gamepad_state.digital_buttons_1, XBOX_DPAD_UP);
  pad_down  ? bit_set(gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN)  : bit_clear(gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN);
  pad_left  ? bit_set(gamepad_state.digital_buttons_1, XBOX_DPAD_LEFT)  : bit_clear(gamepad_state.digital_buttons_1, XBOX_DPAD_LEFT);
  pad_right ? bit_set(gamepad_state.digital_buttons_1, XBOX_DPAD_RIGHT) : bit_clear(gamepad_state.digital_buttons_1, XBOX_DPAD_RIGHT);

  pad_start  ? bit_set(gamepad_state.digital_buttons_1, XBOX_START)       : bit_clear(gamepad_state.digital_buttons_1, XBOX_START);
  pad_select ? bit_set(gamepad_state.digital_buttons_1, XBOX_BACK)        : bit_clear(gamepad_state.digital_buttons_1, XBOX_BACK);

  pad_a ? bit_set(gamepad_state.digital_buttons_2, XBOX_A)    : bit_clear(gamepad_state.digital_buttons_2, XBOX_A);
  pad_b ? bit_set(gamepad_state.digital_buttons_2, XBOX_B)  : bit_clear(gamepad_state.digital_buttons_2, XBOX_B);
  pad_x ? bit_set(gamepad_state.digital_buttons_2, XBOX_X)  : bit_clear(gamepad_state.digital_buttons_2, XBOX_X);
  pad_y ? bit_set(gamepad_state.digital_buttons_2, XBOX_Y) : bit_clear(gamepad_state.digital_buttons_2, XBOX_Y);

  pad_black ? bit_set(gamepad_state.digital_buttons_2, XBOX_LB)    : bit_clear(gamepad_state.digital_buttons_2, XBOX_LB);

  xbox_send_pad_state();
}
