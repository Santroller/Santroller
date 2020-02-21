#include "leds.h"
#include "../../../lib/FastLED/FastLED.h"
extern "C" {
    #include "../util.h"
#include "input_handler.h"
#include "../config/eeprom.h"
CRGB leds[NUM_LEDS];
void led_init(void) {
    // We might have to do some ugly code to handle configurable pins, as the pin needs to be a constant.
    FastLED.addLeds<NEOPIXEL, 2>(leds, NUM_LEDS);
}
void led_tick(controller_t *controller) {
    leds[0] = bit_check(controller->buttons, XBOX_A) ? CRGB::Green : CRGB::Black;
    leds[1] = bit_check(controller->buttons, XBOX_B) ? CRGB::Red : CRGB::Black;
    leds[2] = bit_check(controller->buttons, XBOX_Y) ? CRGB::Yellow : CRGB::Black;
    leds[3] = bit_check(controller->buttons, XBOX_X) ? CRGB::Blue : CRGB::Black;
    leds[4] = bit_check(controller->buttons, XBOX_LB) ? CRGB::Orange : CRGB::Black;
    FastLED.show();
}
}