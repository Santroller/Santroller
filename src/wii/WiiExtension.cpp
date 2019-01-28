#include "WiiExtension.h"
#include "../util.h"
#include <util/delay.h>
extern "C" {
#include "lufa/XInputPad.h"
}
#define constrain(amt, low, high)                                              \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

WiiExtension::WiiExtension()
    : nchuk(port), classic(port), dj(port), guitar(port), drum(port) {}
void WiiExtension::init() {
  I2Cdev::TWIInit();
  mympu_open(5);
}
void WiiExtension::read_controller() {
  if (!port.update()) {
    port.connect();
    return;
  }
  switch (port.getControllerType()) {
  case (ExtensionType::DJTurntableController):
    break;
  case (ExtensionType::Nunchuk):
    bit_write(nchuk.buttonC(), gamepad_state.digital_buttons_2, XBOX_A);
    bit_write(nchuk.buttonZ(), gamepad_state.digital_buttons_2, XBOX_B);
    gamepad_state.l_x = (nchuk.joyX() - 128) * 256;
    gamepad_state.l_y = (nchuk.joyY() - 128) * 256;
    gamepad_state.r_x = (nchuk.rollAngle()) * 182;
    gamepad_state.r_y = (nchuk.pitchAngle()) * 182;
    break;
  case (ExtensionType::DrumController):
    gamepad_state.l_x = (drum.joyX() - 32) * 2048;
    gamepad_state.l_y = (drum.joyY() - 32) * 2048;
    bit_write(drum.drumGreen(), gamepad_state.digital_buttons_2, XBOX_A);
    bit_write(drum.drumRed(), gamepad_state.digital_buttons_2, XBOX_B);
    bit_write(drum.cymbalYellow(), gamepad_state.digital_buttons_2, XBOX_Y);
    bit_write(drum.drumBlue(), gamepad_state.digital_buttons_2, XBOX_X);
    bit_write(drum.cymbalOrange(), gamepad_state.digital_buttons_2, XBOX_LB);
    bit_write(drum.bassPedal(), gamepad_state.digital_buttons_2, XBOX_RB);
    bit_write(drum.buttonPlus(), gamepad_state.digital_buttons_1, XBOX_START);
    bit_write(drum.buttonMinus(), gamepad_state.digital_buttons_1, XBOX_BACK);
    break;
  case (ExtensionType::GuitarController):
    gamepad_state.r_x = -(guitar.whammyBar() - 14) * 1024;
    gamepad_state.l_x = rand();
    if (guitar.whammyBar() <= 18) {
      gamepad_state.r_x = 0;
    }
    if (counter % 20 == 0) {
      double z;
      mympu_update();
      z = 32767 + (mympu.ypr[2] * (32767 / M_PI));
      if (z > 32767) {
        z = 0;
      }
      z = z * 2;
      if (z > 32767) {
        z = 65535 - z;
      }
      z = pow(z, 1.1f);
      z = constrain(z, -32767, 32767);
      gamepad_state.r_y = (int)(z);
    }
    counter++;
    bit_write(guitar.strumUp() || guitar.joyY() > 40,
              gamepad_state.digital_buttons_1, XBOX_DPAD_UP);
    bit_write(guitar.strumDown() || guitar.joyY() < 20,
              gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN);
    bit_write(guitar.joyX() < 20, gamepad_state.digital_buttons_1,
              XBOX_DPAD_LEFT);
    bit_write(guitar.joyX() > 40, gamepad_state.digital_buttons_1,
              XBOX_DPAD_RIGHT);
    bit_write(guitar.buttonPlus(), gamepad_state.digital_buttons_1, XBOX_START);
    bit_write(guitar.buttonMinus(), gamepad_state.digital_buttons_1, XBOX_BACK);
    bit_write(guitar.fretGreen(), gamepad_state.digital_buttons_2, XBOX_A);
    bit_write(guitar.fretRed(), gamepad_state.digital_buttons_2, XBOX_B);
    bit_write(guitar.fretYellow(), gamepad_state.digital_buttons_2, XBOX_Y);
    bit_write(guitar.fretBlue(), gamepad_state.digital_buttons_2, XBOX_X);
    bit_write(guitar.fretOrange(), gamepad_state.digital_buttons_2, XBOX_LB);
    break;

  case (ExtensionType::ClassicController):
    gamepad_state.l_x = (classic.leftJoyX() - 32) * 1024;
    gamepad_state.l_y = (classic.leftJoyY() - 32) * 1024;
    gamepad_state.r_x = (classic.rightJoyX() - 16) * 2048;
    gamepad_state.r_y = (classic.rightJoyY() - 16) * 2048;
    gamepad_state.lt = (classic.triggerL() - 16) * 16;
    gamepad_state.rt = (classic.triggerR() - 16) * 16;
    bit_write(classic.dpadUp(), gamepad_state.digital_buttons_1, XBOX_DPAD_UP);
    bit_write(classic.dpadDown(), gamepad_state.digital_buttons_1,
              XBOX_DPAD_DOWN);
    bit_write(classic.dpadLeft(), gamepad_state.digital_buttons_1,
              XBOX_DPAD_LEFT);
    bit_write(classic.dpadRight(), gamepad_state.digital_buttons_1,
              XBOX_DPAD_RIGHT);
    bit_write(classic.buttonStart(), gamepad_state.digital_buttons_1,
              XBOX_START);
    bit_write(classic.buttonSelect(), gamepad_state.digital_buttons_1,
              XBOX_BACK);
    bit_write(classic.buttonHome(), gamepad_state.digital_buttons_2, XBOX_HOME);
    bit_write(classic.buttonA(), gamepad_state.digital_buttons_2, XBOX_A);
    bit_write(classic.buttonB(), gamepad_state.digital_buttons_2, XBOX_B);
    bit_write(classic.buttonY(), gamepad_state.digital_buttons_2, XBOX_Y);
    bit_write(classic.buttonX(), gamepad_state.digital_buttons_2, XBOX_X);
    bit_write(classic.buttonZL(), gamepad_state.digital_buttons_2, XBOX_LB);
    bit_write(classic.buttonZR(), gamepad_state.digital_buttons_2, XBOX_RB);
    break;
  default:
    break;
  }
}
