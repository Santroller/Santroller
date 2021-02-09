
#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include "device_comms.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/rf.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
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
Controller_t prevCtrl;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
__attribute__((section(".rfrecv"))) uint32_t rftxID = 0xDEADBEEF;
__attribute__((section(".rfrecv"))) uint32_t rfrxID = 0xDEADBEEF;
int main(void) {
  loadConfig();
  sei();
  setupMicrosTimer();
  config.rf.rfInEnabled = false;
  initInputs();
  initReports();
  initRF(true, pgm_read_dword(&rftxID), pgm_read_dword(&rfrxID));
  long lastChange = millis();
  long lastButtons = 0;
  while (true) {
    if (millis() - lastChange > 600000) {

    }
    if (millis() - lastPoll > config.main.pollRate) {
      tickInputs(&controller);
      // Since we receive data via acks, we need to make sure data is always
      // being sent, so we send data every 4ms regardless.
      if ((memcmp(&controller, &prevCtrl, sizeof(Controller_t)) != 0 ||
           millis() - lastPoll > 4)) {
        lastPoll = millis();
        uint8_t data[32];
        if (tickRFTX((uint8_t *)&controller, data, sizeof(XInput_Data_t))) {
          uint8_t cmd = data[0];
          bool isRead = data[1];
          if (isRead) {
            processHIDReadFeatureReport(cmd);
          } else {
            if (cmd == COMMAND_WRITE_CONFIG) {
              // 2 bytes for rf header
              processHIDWriteFeatureReport(cmd, 30, data + 2);
            } else {
              handleCommand(cmd);
            }
          }
          // for (int i = 0; i < 32; i++) { Serial_SendByte(data[i]); }
        }
        memcpy(&prevCtrl, &controller, sizeof(Controller_t));
        if (lastButtons != controller.buttons) {
          lastButtons = controller.buttons;
          lastChange = millis();
        }
      }
    }
  }
}

void writeToUSB(const void *const Buffer, uint8_t Length) {
  uint8_t data[32];
  tickRFTX((uint8_t *)Buffer, data, Length);
}