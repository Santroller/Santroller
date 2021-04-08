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
Controller_t controller = {0};
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
uint8_t size;
uint8_t fullDeviceType;
bool xinputEnabled = false;
bool isRF = false;
bool typeIsGuitar;
bool typeIsDrum;
static bool usingI2C;
static bool usingSPI;
static bool lastDigitalValue[NUM_DIGITAL_PINS];
bool shouldSkipPin2(uint8_t i) {
  // On the 328p, due to an inline LED, it isn't possible to check pin 13
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) ||              \
    defined(__AVR_ATmega328P__)
  if (i == 13 || i == 0 || i == 1) return true;
#endif
  // Skip sda + scl when using peripherials utilising I2C
  if (usingI2C && (i == PIN_WIRE_SDA || i == PIN_WIRE_SCL)) { return true; }
  // Skip RF related pins (such as spi) when using an RF transmitter
#ifdef RF_TX
#  if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) ||            \
      defined(__AVR_ATmega328P__)
  if (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
      i == PIN_SPI_SS || i == 8 || i == 2)
    return true;
#  else
  if (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
      i == PIN_SPI_SS || i == 0 || i == 1)
    return true;
#  endif
#endif
  // Skip SPI pins when using peripherials that utilise SPI
  if (usingSPI && (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
                   i == PIN_SPI_SS)) {
    return true;
  }
  return false;
}
int main() {
  stdio_init_all();
  Configuration_t config = loadConfig();
  // config.main.inputType = PS2;
  usingI2C =
      (config.main.tiltType == MPU_6050 || config.main.inputType == WII);
  usingSPI =
      (config.main.fretLEDMode == APA102) || config.main.inputType == PS2;
  fullDeviceType = config.main.subType;
  initInputs(&config);
  Controller_t c;
#define I2C_ADDR 0x52
  for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
    if (!shouldSkipPin2(i)) {
      pinMode(i, INPUT_PULLUP);
      _delay_us(100);
      lastDigitalValue[i] = digitalRead(i);
    }
  }
  while (1) {
    for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
      if (!shouldSkipPin2(i)) {

          // printf("%d %d %d\n", i, digitalRead(i), lastDigitalValue[i]);
        if (digitalRead(i) != lastDigitalValue[i]) {
          printf("%d\n", detectedPin);
        }
      }
    }
    // uint32_t m = micros();
    // tickInputs(&c);
    // m = micros() - m;
    // printf("%d %d %d\n", m, ps2CtrlType, c.buttons);
    // uint8_t data[6] = {0};
    // twi_readFromPointerSlow(I2C_ADDR, 0xFA, 6, data);
    // m = micros() - m;
    // for (int i = 0; i < 6; i++) {
    //   printf("%d ", data[i]);
    // }
    // printf("%d\n", m);
  }
}
void stopReading(void) {}