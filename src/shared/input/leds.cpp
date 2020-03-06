#define ARDUINO 1000
#define LED_COUNT NUM_LEDS
#include "../../../lib/Adafruit_NeoPixel_Static/Adafruit_NeoPixel_Static.h"
extern "C" {
#include "leds.h"
#include "../../../lib/AVR-APA102-library/src/apa102.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "input_handler.h"
#include <avr/power.h>
uint8_t pixels[2 * NUM_LEDS];
Adafruit_NeoPixel leds =
    Adafruit_NeoPixel(NUM_LEDS, 10, NEO_GRB + NEO_KHZ800, pixels);
void led_init(void) {
  if (config.main.fret_mode == WS2812) {
    pinMode(10, OUTPUT);
  } else if (config.main.fret_mode == APA102) {
    clock_prescale_set(clock_div_1);
    apa102_init_spi();
  }
}
void led_tick(controller_t *controller) {
  if (config.main.fret_mode == WS2812) {
    leds.setPixelColor(0, bit_check(controller->buttons, XBOX_A)
                              ? Green
                              : Black);
    leds.setPixelColor(1, bit_check(controller->buttons, XBOX_B)
                              ? Red
                              : Black);
    leds.setPixelColor(2, bit_check(controller->buttons, XBOX_Y)
                              ? Yellow
                              : Black);
    leds.setPixelColor(3, bit_check(controller->buttons, XBOX_X)
                              ? Blue
                              : Black);
    leds.setPixelColor(4, bit_check(controller->buttons, XBOX_LB)
                              ? Orange
                              : Black);
    leds.show();
  } else if (config.main.fret_mode == APA102) {
    apa102_start();
    apa102_set_led(bit_check(controller->buttons, XBOX_A) ? rgb(Green)
                                                          : rgb(Black));
    apa102_set_led(bit_check(controller->buttons, XBOX_B) ? rgb(Red)
                                                          : rgb(Black));
    apa102_set_led(bit_check(controller->buttons, XBOX_Y) ? rgb(Yellow)
                                                          : rgb(Black));
    apa102_set_led(bit_check(controller->buttons, XBOX_X) ? rgb(Blue)
                                                          : rgb(Black));
    apa102_set_led(bit_check(controller->buttons, XBOX_LB)
                       ? rgb(Orange)
                       : rgb(Black));
    apa102_end();
  }
}
}