#include "../config/config.h"
#include "../shared/bootloader/Bootloader.h"
#include "../shared/controller/ControllerProcessor.h"
#include "../shared/controller/output/OutputProcessor.h"
#include "../shared/twi/I2Cdev.h"
#include "../shared/util.h"
#include "../shared/wii/WiiExtension.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include "pins.h"

ControllerProcessor controller;
OutputProcessor out;

int main() {
  out.init();
  controller.init();
  while (true) {
    controller.process();
    out.process(&controller.controller);
  }
}