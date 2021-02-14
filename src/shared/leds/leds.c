#include "leds.h"
#include "eeprom/eeprom.h"
#include "util/util.h"
// #include "input_handler.h"
// #include <avr/power.h>
#include "spi/spi.h"
void tickLEDs(Controller_t *controller) {
  // Don't do anything if the leds are disabled.
  if (config.main.fretLEDMode != APA102) return;
  int led = 0;
  for (uint8_t i = 0; i < 4; i++) { spi_transfer(0); }
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
    spi_transfer(0xff);
    spi_transfer(contLED.blue);
    spi_transfer(contLED.green);
    spi_transfer(contLED.red);
    led++;
  }
  // We need to send the correct amount of stop bytes
  for (uint8_t i=0; i<led; i+=16)
  {
    spi_transfer(0xff);  // 8 more clock cycles
  }
}