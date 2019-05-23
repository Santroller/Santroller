#include "WiiExtensionInput.h"

WiiExtension::WiiExtension()
    : nchuk(port), classic(port), dj(port), guitar(port), drum(port) {}
void WiiExtension::init() {  }
void WiiExtension::read_controller(Controller *data) {
  if (!port.update()) {
    port.connect();
    return;
  }
  switch (port.getControllerType()) {
  case (ExtensionType::DJTurntableController):
    break;
  case (ExtensionType::Nunchuk):
    bit_write(nchuk.buttonC(), data->buttons, XBOX_A);
    bit_write(nchuk.buttonZ(), data->buttons, XBOX_B);
    data->l_x = (nchuk.joyX() - 128) * 256;
    data->l_y = (nchuk.joyY() - 128) * 256;
    data->r_x = (nchuk.rollAngle()) * 182;
    data->r_y = (nchuk.pitchAngle()) * 182;
    break;
  case (ExtensionType::DrumController):
    data->l_x = (drum.joyX() - 32) * 2048;
    data->l_y = (drum.joyY() - 32) * 2048;
    bit_write(drum.drumGreen(), data->buttons, XBOX_A);
    bit_write(drum.drumRed(), data->buttons, XBOX_B);
    bit_write(drum.cymbalYellow(), data->buttons, XBOX_Y);
    bit_write(drum.drumBlue(), data->buttons, XBOX_X);
    bit_write(drum.cymbalOrange(), data->buttons, XBOX_LB);
    bit_write(drum.buttonPlus(), data->buttons, XBOX_START);
    bit_write(drum.buttonMinus(), data->buttons, XBOX_BACK);
    bit_write(drum.bassPedal(), data->buttons, XBOX_RB);
    break;
  case (ExtensionType::GuitarController):
    data->r_x = (guitar.whammyBar() - 14) * 2048;
    if (guitar.whammyBar() <= 18) {
      data->r_x = 0;
    }
    bit_write(guitar.fretGreen(), data->buttons, XBOX_A);
    bit_write(guitar.fretRed(), data->buttons, XBOX_B);
    bit_write(guitar.fretYellow(), data->buttons, XBOX_Y);
    bit_write(guitar.fretBlue(), data->buttons, XBOX_X);
    bit_write(guitar.fretOrange(), data->buttons, XBOX_LB);
    bit_write(guitar.strumUp() || guitar.joyY() > 40, data->buttons, XBOX_DPAD_UP);
    bit_write(guitar.strumDown() || guitar.joyY() < 20, data->buttons, XBOX_DPAD_DOWN);
    bit_write(guitar.joyX() < 20, data->buttons, XBOX_DPAD_LEFT);
    bit_write(guitar.joyX() > 40, data->buttons, XBOX_DPAD_RIGHT);
    bit_write(guitar.buttonPlus(), data->buttons, XBOX_START);
    bit_write(guitar.buttonMinus(), data->buttons, XBOX_BACK);
    break;

  case (ExtensionType::ClassicController):
    data->l_x = (classic.leftJoyX() - 32) * 1024;
    data->l_y = (classic.leftJoyY() - 32) * 1024;
    data->r_x = (classic.rightJoyX() - 16) * 2048;
    data->r_y = (classic.rightJoyY() - 16) * 2048;
    data->lt = (classic.triggerL() - 16) * 16;
    data->rt = (classic.triggerR() - 16) * 16;
    bit_write(classic.dpadUp(), data->buttons, XBOX_DPAD_UP);
    bit_write(classic.dpadDown(), data->buttons, XBOX_DPAD_DOWN);
    bit_write(classic.dpadLeft(), data->buttons, XBOX_DPAD_LEFT);
    bit_write(classic.dpadRight(), data->buttons, XBOX_DPAD_RIGHT);
    bit_write(classic.buttonPlus(), data->buttons, XBOX_START);
    bit_write(classic.buttonMinus(), data->buttons, XBOX_BACK);
    bit_write(classic.buttonHome(), data->buttons, XBOX_HOME);
    bit_write(classic.buttonA(), data->buttons, XBOX_A);
    bit_write(classic.buttonB(), data->buttons, XBOX_B);
    bit_write(classic.buttonY(), data->buttons, XBOX_Y);
    bit_write(classic.buttonX(), data->buttons, XBOX_X);
    bit_write(classic.buttonZL(), data->buttons, XBOX_LB);
    bit_write(classic.buttonZR(), data->buttons, XBOX_RB);
    break;
  default:
    break;
  }
}
