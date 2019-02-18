#include "WiiExtension.h"
#include "../util.h"
#include <util/delay.h>
#include "../Controller.h"
#define constrain(amt, low, high) \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

WiiExtension::WiiExtension()
    : nchuk(port), classic(port), dj(port), guitar(port), drum(port)
{
}
void WiiExtension::init()
{
  I2Cdev::TWIInit();
  mympu_open(15);
}
void WiiExtension::read_controller(USB_JoystickReport_Data_t* data)
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
    bit_write(nchuk.buttonC(), data->digital_buttons_2, XBOX_A);
    bit_write(nchuk.buttonZ(), data->digital_buttons_2, XBOX_B);
    data->l_x = (nchuk.joyX() - 128) * 256;
    data->l_y = (nchuk.joyY() - 128) * 256;
    data->r_x = (nchuk.rollAngle()) * 182;
    data->r_y = (nchuk.pitchAngle()) * 182;
    break;
  case (ExtensionType::DrumController):
    data->l_x = (drum.joyX() - 32) * 2048;
    data->l_y = (drum.joyY() - 32) * 2048;
    bit_write(drum.drumGreen(), data->digital_buttons_2, XBOX_A);
    bit_write(drum.drumRed(), data->digital_buttons_2, XBOX_B);
    bit_write(drum.cymbalYellow(), data->digital_buttons_2, XBOX_Y);
    bit_write(drum.drumBlue(), data->digital_buttons_2, XBOX_X);
    bit_write(drum.cymbalOrange(), data->digital_buttons_2, XBOX_LB);
    bit_write(drum.bassPedal(), data->digital_buttons_2, XBOX_RB);
    bit_write(drum.buttonPlus(), data->digital_buttons_1, XBOX_START);
    bit_write(drum.buttonMinus(), data->digital_buttons_1, XBOX_BACK);
    break;
  case (ExtensionType::GuitarController):
    // data->l_x = rand();
    data->r_x = -(guitar.whammyBar() - 14) * 1024;
    if (guitar.whammyBar() <= 18)
    {
      data->r_x = 0;
    }
    if (counter % 20 == 0)
    {
      double z;
      mympu_update();
      z = 32767 + (mympu.ypr[2] * (32767 / M_PI));
      if (z > 32767)
      {
        z = 0;
      }
      z = z * 2;
      if (z > 32767)
      {
        z = 65535 - z;
      }
      z = pow(z, 1.1f);
      z = constrain(z, -32767, 32767);
      data->r_y = (int)(z);
    }
    counter++;
    bit_write(guitar.strumUp() || guitar.joyY() > 40,
              data->digital_buttons_1, XBOX_DPAD_UP);
    bit_write(guitar.strumDown() || guitar.joyY() < 20,
              data->digital_buttons_1, XBOX_DPAD_DOWN);
    bit_write(guitar.joyX() < 20, data->digital_buttons_1,
              XBOX_DPAD_LEFT);
    bit_write(guitar.joyX() > 40, data->digital_buttons_1,
              XBOX_DPAD_RIGHT);
    bit_write(guitar.buttonPlus(), data->digital_buttons_1, XBOX_START);
    bit_write(guitar.buttonMinus(), data->digital_buttons_1, XBOX_BACK);
    bit_write(guitar.fretGreen(), data->digital_buttons_2, XBOX_A);
    bit_write(guitar.fretRed(), data->digital_buttons_2, XBOX_B);
    bit_write(guitar.fretYellow(), data->digital_buttons_2, XBOX_Y);
    bit_write(guitar.fretBlue(), data->digital_buttons_2, XBOX_X);
    bit_write(guitar.fretOrange(), data->digital_buttons_2, XBOX_LB);
    break;

  case (ExtensionType::ClassicController):
    data->l_x = (classic.leftJoyX() - 32) * 1024;
    data->l_y = (classic.leftJoyY() - 32) * 1024;
    data->r_x = (classic.rightJoyX() - 16) * 2048;
    data->r_y = (classic.rightJoyY() - 16) * 2048;
    data->lt = (classic.triggerL() - 16) * 16;
    data->rt = (classic.triggerR() - 16) * 16;
    bit_write(classic.dpadUp(), data->digital_buttons_1, XBOX_DPAD_UP);
    bit_write(classic.dpadDown(), data->digital_buttons_1,
              XBOX_DPAD_DOWN);
    bit_write(classic.dpadLeft(), data->digital_buttons_1,
              XBOX_DPAD_LEFT);
    bit_write(classic.dpadRight(), data->digital_buttons_1,
              XBOX_DPAD_RIGHT);
    bit_write(classic.buttonStart(), data->digital_buttons_1,
              XBOX_START);
    bit_write(classic.buttonSelect(), data->digital_buttons_1,
              XBOX_BACK);
    bit_write(classic.buttonHome(), data->digital_buttons_2, XBOX_HOME);
    bit_write(classic.buttonA(), data->digital_buttons_2, XBOX_A);
    bit_write(classic.buttonB(), data->digital_buttons_2, XBOX_B);
    bit_write(classic.buttonY(), data->digital_buttons_2, XBOX_Y);
    bit_write(classic.buttonX(), data->digital_buttons_2, XBOX_X);
    bit_write(classic.buttonZL(), data->digital_buttons_2, XBOX_LB);
    bit_write(classic.buttonZR(), data->digital_buttons_2, XBOX_RB);
    break;
  default:
    break;
  }
}
