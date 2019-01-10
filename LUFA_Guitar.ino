extern "C" {
  #include "src/lufa/XInputPad.h"
}
#include "util.h"
#include "bootloader.h"
#include "Wire.h"
#include "WiiExtension.h"
#include <EEPROM.h>
#include "src/mpu6050/mpu.h"
int ret;
WiiExtension controller;
void setup()
{
  Wire.begin();
  Wire.setClock(350000);
  mympu_open(300);
  xbox_init(true);
  // Disable JTAG
  bit_set(MCUCR, 1 << JTD);
  bit_set(MCUCR, 1 << JTD);
  controller.setup();
}
void loop()
{
  ret = mympu_update();
  xbox_reset_watchdog();
  if (!controller.valid) {
      controller.setup();
  }
  if (controller.read_controller((WiiController*)&gamepad_state, mympu.ypr)) {
      bootloader();
  }
  xbox_send_pad_state();
}
