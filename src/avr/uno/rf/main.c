
#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "eeprom/eeprom.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "rf/rf.h"
#include "timer/timer.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sfr_defs.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
// Sleep pin: 3
Controller_t controller;
Controller_t prevCtrl;
Configuration_t newConfig;
long lastPoll = 0;
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
      lastChange = millis();
      // disable ADC
      ADCSRA = 0;
      // Turn off RF
      nrf24_powerDown();
      // turn off various modules
      power_all_disable();

      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      cli(); // timed sequence follows
      EIFR = _BV(INTF1);
      EICRA |= _BV(ISC11);
      EIMSK |= _BV(INT1);
      sleep_enable();
      sei();       // guarantees next instruction executed
      sleep_cpu(); // sleep within 3 clock cycles of above
    }
    if (millis() - lastPoll > config.main.pollRate) {
      tickInputs(&controller);
      // Since we receive data via acks, we need to make sure data is always
      // being sent, so we send data every 100ms regardless.
      if ((memcmp(&controller, &prevCtrl, sizeof(Controller_t)) != 0 ||
           millis() - lastPoll > 100)) {
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
ISR(INT1_vect) {
  sleep_disable();
  power_all_enable();
  setupADC();
  initRF(true, pgm_read_dword(&rftxID), pgm_read_dword(&rfrxID));
  EICRA &= ~(_BV(ISC11));
  EIMSK &= ~(_BV(INT1));
}