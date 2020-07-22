#include "leds.h"
#include "../config/eeprom.h"
#include "arduino_pins.h"
#include "util/util.h"
// #include "input_handler.h"
#include <avr/power.h>
/* Send out data via SPI & wait until transmission is complete */
void transmitSPIByte(uint8_t data) {
  SPDR = data;
  while (!(SPSR & _BV(SPIF)))
    ;
}
void tickLEDs(Controller_t *controller) {
  // Don't do anything if the leds are disabled.
  if (config.main.fretLEDMode != APA102) return;
  int led = 0;
  for (uint8_t i = 0; i < 4; i++) { transmitSPIByte(0); }
  Led_t configLED;
  Led_t contLED;
  // Loop until either config.leds runs out, or controller->leds runs out. This
  // is due to the fact that controller->leds can contain more leds if a config
  // is in the process of being made.
  while (true) {
    configLED = config.leds[led];
    contLED = controller->leds[led];
    if (!configLED.pin && !contLED.pin) break;
    // Only bind pins to buttons if we know what pin to map, and the computer
    // has not sent a new pin
    if (!contLED.blue && !contLED.red && !contLED.green && configLED.pin) {
      if (getVelocity(controller, configLED.pin - 1)) { contLED = configLED; }
    }
    // Write an leds colours
    transmitSPIByte(0xff);
    transmitSPIByte(contLED.blue);
    transmitSPIByte(contLED.green);
    transmitSPIByte(contLED.red);
    led++;
  }
  // We need to send the correct amount of stop bytes
  for (uint8_t i=0; i<led; i+=16)
  {
    transmitSPIByte(0xff);  // 8 more clock cycles
  }
}