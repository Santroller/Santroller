
#define ARDUINO_MAIN
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
#include "../shared/device_comms.h"
Controller_t controller;
Controller_t previousController;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
__attribute__((section(".rfrecv"))) uint32_t rfID = 0xc2292dde;
int main(void) {
  loadConfig();
  config.rf.rfInEnabled = false;
  sei();
  Serial_Init(115200, true);
  initInputs();
  initReports();
  initRF(false, pgm_read_dword(&rfID));
  Serial_SendByte('R');
  Serial_SendByte('0'+wide_band);
  Serial_SendByte('0'+p_type);
  uint8_t val2;
  nrf24_readRegister(0x06, &val2, 1);
  Serial_SendByte(val2);
  Serial_SendByte('\n');
  while (true) {
    Serial_SendByte(nrf24_getStatus());
    if (rf_interrupt) {
      Serial_SendByte('1');
      tickRFInput(&controller);
    }
  }
}

void Serial_SendByte2(const char DataByte) {
  Serial_SendByte(DataByte);
}