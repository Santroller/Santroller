#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include "device_comms.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/rf.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
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

int main(void) {
  loadConfig();

  sei();
  initInputs();
  initReports();
  Serial_Init(115200, true);
  // rf_interrupt = true;
  initRF();
  while (true) {
    if (millis() - lastPoll > config.main.pollRate) {
      tickInputs(&controller);
      tickLEDs(&controller);
      controller.l_x = rand();
      if (memcmp(&controller, &previousController, sizeof(Controller_t)) != 0 &&
          !rf_interrupt) {
        lastPoll = millis();
        tickRFTX(&controller);
        memcpy(&previousController, &controller, sizeof(Controller_t));
        Serial_SendByte('0' + rf_interrupt);
        tickRFTXIRQ();
      }
    }
  }
}