#ifndef APA102_SIMPLE_EFFECTS_H
#define APA102_SIMPLE_EFFECTS_H

#include <stdbool.h>
#include "apa102.h"


/* Flash all LEDs through a sequence of colors with variable color & blank delays. */
void flash(const RGBColor_t *sequence, const uint8_t sequence_length, const uint16_t color_delay, const uint16_t blank_delay);

/* Show a sequence of uninterrupted colors with a fixed delay between each */
void ribbon(const RGBColor_t *sequence, const uint8_t sequence_length, const uint16_t color_delay);

/* Scroll a sequence of colors up or down the LED strip
 *
 * If there are more LEDs than colors, the colors will repeat.
 */
void scroll(const RGBColor_t *sequence, const uint8_t length, const bool reverse, const uint16_t delay);

/* Sequentially turn each LED to a specific color, creating an "extending" strip */
void extend(const RGBColor_t color, const uint16_t delay);

/* Sequentially turn off each LED from a specific color, creating a "retracting" strip */
void retract(const RGBColor_t color, const uint16_t delay);

#endif
