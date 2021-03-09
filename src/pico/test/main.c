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
Controller_t controller;
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
uint8_t size;

int main() {
  stdio_init_all();
  loadConfig();
  config.main.inputType = WII;
  initInputs();
  while (1) {
    uint32_t m = micros();
    uint8_t data[8];
    twi_readFromPointerSlow(0x52, 0x00, 8, data);
    for (int i = 0; i < 8; i++) { printf("%02x", data[i]); }
    printf("\n");
    // twi_readFromPointerSlow(0x52, 0x00, 8, data);
    // for (int i = 0; i < 8; i++) { printf("%x", data[i]); }
    // printf("\n");
    if (data[0] == 0xff) {
      twi_writeSingleToPointer(0x52, 0xF0, 0x55);
      _delay_us(10);
      twi_writeSingleToPointer(0x52, 0xFB, 0x00);
      _delay_us(10);
    }
    m = micros() - m;
    printf("%d\n", m);
  }
}
void stopReading(void) {}