#include "leds.h"
#include "pin.h"
#include "spi.h"
void tickLEDs(Input_t** leds) {
  // Don't do anything if the leds are disabled.
  int led = 0;
  for (uint8_t i = 0; i < 4; i++) { spi_transfer(0); }
  Input_t** current = leds;
  uint8_t r,g,b;
  while (current) {
    Input_t* c = *current;
    if (c->forceColor || c->digitalRead(c)) {
      r = c->red;
      g = c->green;
      b = c->blue;
    } else {
      r = g = b = 0;
    }
    // Write an leds colours
    spi_transfer(0xff);
    spi_transfer(b);
    spi_transfer(g);
    spi_transfer(r);
    current++;
  }
  // We need to send the correct amount of stop bytes
  for (uint8_t i=0; i<led; i+=16)
  {
    spi_transfer(0xff);  // 8 more clock cycles
  }
}