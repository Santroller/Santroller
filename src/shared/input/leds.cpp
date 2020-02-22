#include "leds.h"
#define ARDUINO 1000
#include "../../../lib/Adafruit_NeoPixel_Static/Adafruit_NeoPixel_Static.h"
extern "C" {
    #include "../util.h"
#include "input_handler.h"
#include "../config/eeprom.h"
// CRGB leds[NUM_LEDS];
uint8_t pixels[2*NUM_LEDS];
Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUM_LEDS, 2, NEO_GRB + NEO_KHZ800, pixels);
void led_init(void) {
    pinMode(2, OUTPUT);
    // // We might have to do some ugly code to handle configurable pins, as the pin needs to be a constant.
    // FastLED.addLeds<NEOPIXEL, 2>(leds, NUM_LEDS);
}
void led_tick(controller_t *controller) {
    leds.setPixelColor(0, bit_check(controller->buttons, XBOX_A) ? Colors::Green : Colors::Black);
    leds.setPixelColor(1, bit_check(controller->buttons, XBOX_B) ? Colors::Red : Colors::Black);
    leds.setPixelColor(2, bit_check(controller->buttons, XBOX_Y) ? Colors::Yellow : Colors::Black);
    leds.setPixelColor(3, bit_check(controller->buttons, XBOX_X) ? Colors::Blue : Colors::Black);
    leds.setPixelColor(4, bit_check(controller->buttons, XBOX_LB) ? Colors::Orange : Colors::Black);
    leds.show();
}
}