
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
Controller_t previousController;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
// eefdc362
__attribute__((section(".rfrecv"))) uint32_t rftxID = 0x8581f888;
__attribute__((section(".rfrecv"))) uint32_t rfrxID = 0xc2292dde;
// TODO: we can use millis here to work out if a controller has been out of use
// for several minutes, and to then go into a sleep mode
int main(void) {
  loadConfig();
  sei();
  setupMicrosTimer();
  config.rf.rfInEnabled = false;
  config.main.pollRate = 0;
  initInputs();
  initReports();
  // Serial_Init(115200, true);
  // id = generate_crc32();
  // initRF(true, pgm_read_dword(&rftxID), pgm_read_dword(&rfrxID));
  initRF(true, 0x8581f888, 0xc2292dde);
  while (true) {
    if (millis() - lastPoll > config.main.pollRate) {
      tickInputs(&controller);
      // Since we receive data via acks, we need to make sure data is always being sent, so we send data every 100ms regardless.
      if ((memcmp(&controller, &previousController, sizeof(Controller_t)) !=
               0 ||
           millis() - lastPoll > 100)) {
        uint8_t data[32];
        uint8_t ret =
            tickRFTX((uint8_t *)&controller, data, sizeof(XInput_Data_t));
        if (ret == 1) {
          uint8_t cmd = data[0];
          uint8_t offset = 29 * data[1];
          bool isRead = data[2];
          if (isRead) {
            processHIDReadFeatureReport(cmd);
          } else {
            if (cmd == COMMAND_WRITE_CONFIG) {
              // The first byte of COMMAND_WRITE_CONFIG is an offset.
              // Since rf has its own offset, we can just combine both to get a
              // result offset
              data[3] += offset;
              // 3 bytes for rf header
              processHIDWriteFeatureReport(cmd, 29, data + 3);
            } else {
              handleCommand(cmd);
            }
            // for (int i = 0; i < 32; i++) { Serial_SendByte(data[i]); }
          }
        }
        if (ret) {
          memcpy(&previousController, &controller, sizeof(Controller_t));
          lastPoll = millis();
        }
      }
    }
  }
}

void writeToUSB(const void *const Buffer, uint8_t Length) {
  uint8_t data[32];
  tickRFTX((uint8_t *)Buffer, data, Length);
}