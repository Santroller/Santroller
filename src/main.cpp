#include "Controller.h"
#include "twi/I2Cdev.h"
#include "util.h"
#include "util/delay.h"
#include "wii/WiiExtension.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include "bootloader.h"
extern "C"
{
#include "lufa/XInputPad.h"
}

WiiExtension controller;

int main()
{
  xbox_init(true);
  sei();
  controller.init();
  for (;;)
  {
    controller.read_controller();
    if (bit_is_set(gamepad_state.digital_buttons_1, XBOX_BACK) && bit_is_set(gamepad_state.digital_buttons_1, XBOX_START)) {
      bootloader();
    }
    xbox_send_pad_state();
    xbox_reset_watchdog();
  }
}
