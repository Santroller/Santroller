
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
#include "pins_arduino.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
#include "output/serial_handler.h"
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
  Serial_Init(115200, true);
  config.main.inputType = DIRECT;
  config.main.fretLEDMode = APA102;
  config.main.tiltType = NO_TILT;
  config.pins.a = 19;
  initInputs();
  initReports();
  // id = generate_crc32();
  // initRF(true, pgm_read_dword(&rftxID), pgm_read_dword(&rfrxID));
  initRF(true, 0x8581f888, 0xc2292dde);
  while (true) {
    if (millis() - lastPoll > config.main.pollRate) {
      tickInputs(&controller);
      controller.l_x = rand();
      if (memcmp(&controller, &previousController, sizeof(Controller_t)) != 0) {
        lastPoll = millis();
        uint8_t data[32];
        if (tickRFTX(&controller, data)) {
          uint8_t cmd = data[0];
          uint8_t offset = 32 * data[1];
          // The first byte of COMMAND_WRITE_CONFIG is an offset.
          // Since rf has its own offset, we can just combine both to get a result offset
          if (cmd == COMMAND_WRITE_CONFIG) {
            data[3] += offset;
          }
          handleCommand(cmd);
          // 2 bytes for rf header
          processHIDWriteFeatureReport(cmd, 30, data+2);
        }
        memcpy(&previousController, &controller, sizeof(Controller_t));
      }
    }
  }
}