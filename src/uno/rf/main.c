
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
Controller_t controller;
Controller_t previousController;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
int main(void) {
  loadConfig();
  config.rf.rfInEnabled = false;
  sei();
  // Serial_Init(115200, true);
  initInputs();
  initReports();
  initRF(true);
  while (true) {
    if (millis() - lastPoll > config.main.pollRate && rf_interrupt) {
      tickInputs(&controller);
      tickLEDs(&controller);
      if (memcmp(&controller, &previousController, sizeof(Controller_t)) != 0) {
        lastPoll = millis();

        uint8_t data[12];
        if (tickRFTX(&controller, data)) {
          // for (int i = 0; i < sizeof(data); i++) { Serial_SendByte(data[i]);
          // }
        }
        memcpy(&previousController, &controller, sizeof(Controller_t));
      }
    }
  }
}