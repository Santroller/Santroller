
#define ARDUINO_MAIN
#include "../shared/device_comms.h"
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/rf.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "pins_arduino.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
Controller_t controller;
Controller_t previousController;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
__attribute__((section(".rfrecv"))) uint32_t rfID = 0xc2292dde;
void print_reg(char *name, uint8_t reg, uint8_t size) {
  int32_t val2;
  nrf24_readRegister(reg, (uint8_t *)&val2, size);
  // Bytes to bits
  uint8_t len = strlen(name);
  for (int i = 0; i < len; i++) { Serial_SendByte(name[i]); }
  Serial_SendByte(':');
  Serial_SendByte(' ');
  for (int i = 0; i < size * 8; i++) {
    Serial_SendByte('0' + (val2 & 1));
    val2 >>= 1;
  }
  Serial_SendByte('\n');
}
int main(void) {
  loadConfig();
  config.rf.rfInEnabled = false;
  sei();
  Serial_Init(115200, true);
  initInputs();
  initReports();
  initRF(false, pgm_read_dword(&rfID));
  uint8_t data[96];
  uint8_t* ptr = data;
  memcpy(ptr, &config.main, sizeof(config.main));
  ptr += sizeof(config.main);
  memcpy(ptr, &config.pins, sizeof(config.pins));
  ptr += sizeof(config.pins);
  memcpy(ptr, &config.axis, sizeof(config.axis));
  ptr += sizeof(config.axis);
  memcpy(ptr, &config.leds, sizeof(config.leds));
  // pins, main, axis, leds 
  writeRFConfig(data, 32);
  writeRFConfig(data + 32, 32);
  writeRFConfig(data + 64, 32);
  // for (int i = 0; i < 32*3; i++) { Serial_SendByte(((uint8_t *)&config)[i]);
  // }
  while (true) {
    if (rf_interrupt) {
      tickRFInput(&controller);
    }
  }
}

void Serial_SendByte2(const char DataByte) { Serial_SendByte(DataByte); }
