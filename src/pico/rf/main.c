#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "bootloader/bootloader.h"
#include "bsp/board.h"
#include "config/defines.h"
#include "controller/guitar_includes.h"
#include "eeprom/eeprom.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "output/control_requests.h"
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
#include <pico/sleep.h>
#include <pico/unique_id.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((section(".rfrecv"))) uint32_t rftxID = 0xDEADBEEF;
__attribute__((section(".rfrecv"))) uint32_t rfrxID = 0xDEADBEEF;
Controller_t controller;
Controller_t prevCtrl;
long lastPoll = 0;
int validAnalog = 0;
uint8_t pollRate;
uint8_t inputType;
bool typeIsGuitar;
bool typeIsDrum;
bool typeIsDJ;
bool isRF = false;
void stopReading(void) {}

void initialise(void) {
  board_init();
  Configuration_t config;
  loadConfig(&config);
  config.rf.rfInEnabled = false;
  fullDeviceType = fullDeviceType;
  deviceType = fullDeviceType;
  pollRate = config.main.pollRate;
  inputType = config.main.inputType;
  typeIsDrum = isDrum(fullDeviceType);
  typeIsGuitar = isGuitar(fullDeviceType);
  initInputs(&config);
  initLEDs(&config);
}
int main(void) {
  initRF(true, rftxID, rfrxID);
  initialise();
  long lastChange = millis();
  long lastButtons = 0;
  while (true) {
    if (millis() - lastChange > 600000) {
      lastChange = millis();
      sleep_run_from_xosc();
      sleep_goto_dormant_until_edge_high(PIN_WAKEUP);
    }
    if (millis() - lastPoll > pollRate) {
      tickInputs(&controller);
      tickLEDs(&controller);
      // Since we receive data via acks, we need to make sure data is always
      // being sent, so we send data every 100ms regardless.
      if (memcmp(&controller, &prevCtrl, sizeof(Controller_t)) != 0 ||
          millis() - lastPoll > 100) {
        lastPoll = millis();

        uint8_t data[12];
        if (tickRFTX((uint8_t *)&controller, data, sizeof(XInput_Data_t))) {
          uint8_t cmd = data[0];
          bool isRead = data[1];
          if (isRead) {
            processHIDReadFeatureReport(cmd, 0, NULL);
          } else {
            if (cmd == COMMAND_WRITE_CONFIG) {
              // 2 bytes for rf header
              processHIDWriteFeatureReport(cmd, 30, data + 2);
            } else {
              handleCommand(cmd);
            }
          }
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
void writeToUSB(const void *const Buffer, uint8_t Length, uint8_t report,
                const void *request) {
  uint8_t data[32];
  tickRFTX((uint8_t *)Buffer, data, Length);
}