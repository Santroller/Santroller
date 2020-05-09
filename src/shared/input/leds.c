#include "leds.h"
#include "../../../lib/AVR-APA102-library/src/apa102.h"
#include "../config/eeprom.h"
#include "../util.h"
// #include "input_handler.h"
#include <avr/power.h>
void led_init(void) {
  if (config.main.fret_mode != APA102) return;
  apa102_set_led_count(NUM_LEDS);
  clock_prescale_set(clock_div_1);
  apa102_init_spi();
}
uint8_t gh[XBOX_BTN_COUNT + XBOX_AXIS_COUNT] = {
    [XBOX_A] = 1, [XBOX_B] = 2, [XBOX_Y] = 3, [XBOX_X] = 4, [XBOX_LB] = 5};
void led_tick(controller_t *controller) {
  if (config.main.fret_mode != APA102) return;
  apa102_start();
  for (int i = 0; config.new_items.leds.pins[i]; i++) {
    uint32_t col = controller->leds.gui;
    if (col == Black) {
      uint8_t button = config.new_items.leds.pins[i] - 1;
      uint8_t ghBtn = gh[button];
      if (ghBtn) {
        uint8_t ghIdx = controller->leds.leds[ghBtn - 1];
        if (ghIdx == 1) col = config.new_items.leds.colours[i];
        if (ghIdx >= 2) col = config.new_items.leds.ghColours[ghIdx - 2];
      }

      if ((col == Black && bit_check(controller->buttons, button))) {
        col = config.new_items.leds.colours[i];
      }
    }
    apa102_set_led(rgb(col));
  }
  apa102_end();
}