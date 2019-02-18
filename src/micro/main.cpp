#include "../shared/Controller.h"
#include "../shared/twi/I2Cdev.h"
#include "../shared/util.h"
#include "../shared/wii/WiiExtension.h"
#include "../shared/bootloader/bootloader.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include "../shared/controller/XInputPad.h"

WiiExtension controller;

int main() {
  xbox_init(true);
  sei();
  controller.init();
  for (;;) {
    controller.read_controller(&gamepad_state);
    if (bit_check(gamepad_state.digital_buttons_1, XBOX_BACK) &&
        bit_check(gamepad_state.digital_buttons_1, XBOX_START)) {
      bootloader();
    }
    xbox_send_pad_state();
    xbox_reset_watchdog();
  }
}