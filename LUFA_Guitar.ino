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
void loop()
{
  if (!guitar.update()) {
    guitar.connect();
  }
  xbox_reset_watchdog();
  gamepad_state.r_y = 0;
  gamepad_state.r_x = (guitar.whammyBar()-14)*1024;
  
  if(!digitalRead(4)) {
    gamepad_state.r_y = 32767;
  }
  bit_write(guitar.strumUp() || guitar.joyY()>40, gamepad_state.digital_buttons_1, XBOX_DPAD_UP);
  bit_write(guitar.strumDown() || guitar.joyY()<20, gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN);
  bit_write(guitar.joyX()<20, gamepad_state.digital_buttons_1, XBOX_DPAD_LEFT);
  bit_write(guitar.joyX()>40, gamepad_state.digital_buttons_1, XBOX_DPAD_RIGHT);
  bit_write(guitar.buttonPlus(), gamepad_state.digital_buttons_1, XBOX_START);
  bit_write(guitar.buttonMinus(), gamepad_state.digital_buttons_1, XBOX_BACK);
  bit_write(guitar.fretGreen(), gamepad_state.digital_buttons_2, XBOX_A);
  bit_write(guitar.fretRed(), gamepad_state.digital_buttons_2, XBOX_B);
  bit_write(guitar.fretYellow(), gamepad_state.digital_buttons_2, XBOX_Y);
  bit_write(guitar.fretBlue(), gamepad_state.digital_buttons_2, XBOX_X);
  bit_write(guitar.fretOrange(), gamepad_state.digital_buttons_2, XBOX_LB);

  xbox_send_pad_state();
}
