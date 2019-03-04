#include "../config/config.h"
#include "../shared/bootloader/Bootloader.h"
#include "../shared/controller/ControllerProcessor.h"
#include "../shared/controller/XInputPad.h"
#include "../shared/twi/I2Cdev.h"
#include "../shared/util.h"
#include "../shared/wii/WiiExtension.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include "pins.h"

ControllerProcessor controller;

int main() {
#if DEVICE_TYPE == XINPUT
  xbox_init(true);
#endif
  sei();
  controller.init();
  while (true) {
    controller.process();
#if DEVICE_TYPE == XINPUT
    xbox_send_pad_state();
    xbox_reset_watchdog();
#endif DEVICE_TYPE == XINPUT
  }
}