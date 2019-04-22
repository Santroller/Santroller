#include "WiiExtensionInput.h"

WiiExtension::WiiExtension()
    : nchuk(port), classic(port), dj(port), guitar(port), drum(port)
{
}
void WiiExtension::init()
{
  I2Cdev::TWIInit();
}
void WiiExtension::read_controller(Controller* data)
{
  if (!port.update())
  {
    port.connect();
    return;
  }
  switch (port.getControllerType())
  {
  case (ExtensionType::DJTurntableController):
    break;
  case (ExtensionType::Nunchuk):
    bit_write(nchuk.buttonC(), data->buttons, GREEN);
    bit_write(nchuk.buttonZ(), data->buttons, RED);
    data->l_x = (nchuk.joyX() - 128) * 256;
    data->l_y = (nchuk.joyY() - 128) * 256;
    data->r_x = (nchuk.rollAngle()) * 182;
    data->r_y = (nchuk.pitchAngle()) * 182;
    break;
  case (ExtensionType::DrumController):
    data->l_x = (drum.joyX() - 32) * 2048;
    data->l_y = (drum.joyY() - 32) * 2048;
    bit_write(drum.drumGreen(), data->buttons, GREEN);
    bit_write(drum.drumRed(), data->buttons, RED);
    bit_write(drum.cymbalYellow(), data->buttons, YELLOW);
    bit_write(drum.drumBlue(), data->buttons, BLUE);
    bit_write(drum.cymbalOrange(), data->buttons, ORANGE);
    bit_write(drum.buttonPlus(), data->buttons, START);
    bit_write(drum.buttonMinus(), data->buttons, SELECT);
    bit_write(drum.bassPedal(), data->buttons, RB);
    break;
  case (ExtensionType::GuitarController):
    data->r_x = -(guitar.whammyBar() - 14) * 1024;
    if (guitar.whammyBar() <= 18)
    {
      data->r_x = 0;
    }
    bit_write(guitar.fretGreen(), data->buttons, GREEN);
    bit_write(guitar.fretRed(), data->buttons, RED);
    bit_write(guitar.fretYellow(), data->buttons, YELLOW);
    bit_write(guitar.fretBlue(), data->buttons, BLUE);
    bit_write(guitar.fretOrange(), data->buttons, ORANGE);
    bit_write(guitar.strumUp() || guitar.joyY() > 40, data->buttons, UP);
    bit_write(guitar.strumDown() || guitar.joyY() < 20, data->buttons, DOWN);
    bit_write(guitar.joyX() < 20, data->buttons, LEFT);
    bit_write(guitar.joyX() > 40, data->buttons, RIGHT);
    bit_write(guitar.buttonPlus(), data->buttons, START);
    bit_write(guitar.buttonMinus(), data->buttons, SELECT);
    break;

  case (ExtensionType::ClassicController):
    data->l_x = (classic.leftJoyX() - 32) * 1024;
    data->l_y = (classic.leftJoyY() - 32) * 1024;
    data->r_x = (classic.rightJoyX() - 16) * 2048;
    data->r_y = (classic.rightJoyY() - 16) * 2048;
    data->lt = (classic.triggerL() - 16) * 16;
    data->rt = (classic.triggerR() - 16) * 16;
    bit_write(classic.dpadUp(), data->buttons, UP);
    bit_write(classic.dpadDown(), data->buttons, DOWN);
    bit_write(classic.dpadLeft(), data->buttons, LEFT);
    bit_write(classic.dpadRight(), data->buttons, RIGHT);
    bit_write(classic.buttonPlus(), data->buttons, START);
    bit_write(classic.buttonMinus(), data->buttons, SELECT);
    bit_write(classic.buttonHome(), data->buttons, HOME);
    bit_write(classic.buttonA(), data->buttons, GREEN);
    bit_write(classic.buttonB(), data->buttons, RED);
    bit_write(classic.buttonY(), data->buttons, YELLOW);
    bit_write(classic.buttonX(), data->buttons, BLUE);
    bit_write(classic.buttonZL(), data->buttons, ORANGE);
    bit_write(classic.buttonZR(), data->buttons, RB);
    break;
  default:
    break;
  }
}
