#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "bootloader/bootloader.h"
#include "config/defines.h"
#include "controller/guitar_includes.h"
#include "eeprom/eeprom.h"
#include "i2c/i2c.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "output/descriptors.h"
#include "output/reports.h"
#include "output/serial_handler.h"
#include "pico/stdlib.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "rf/rf.h"
#include "stdbool.h"
#include "timer/timer.h"
#include "util/util.h"
#include <hardware/sync.h>
#include <pico/unique_id.h>
#include <stdio.h>
#include <stdlib.h>
int validAnalog = 0;
Controller_t controller = {0};
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
uint8_t size;

int main() {
  stdio_init_all();
  loadConfig();
  config.main.inputType = PS2;
  initInputs();
  Controller_t c;
  #define I2C_ADDR 0x52
  while (1) {
    // uint32_t m = micros();
    tickInputs(&c);
    // m = micros() - m;
    // printf("%d %d %d\n", m, ps2CtrlType, c.buttons);
    // uint8_t data[6] = {0};
    // twi_readFromPointerSlow(I2C_ADDR, 0xFA, 6, data);
    // m = micros() - m;
    // for (int i = 0; i < 6; i++) {
    //   printf("%d ", data[i]);
    // }
    // printf("%d\n", m);
  }
}
void stopReading(void) {}