#include <stdbool.h>
#include <util/delay.h>
#include <apa102.h>
#include <apa102_simple_effects.h>

// delay the cpu by a variable number of milliseconds
static inline void variable_delay(uint16_t ms_delay) {
    for (uint16_t i = 0; i < ms_delay; i++) {
        _delay_ms(1);
    }
}

static inline void set_all_leds(const RGBColor_t *color) {
#ifdef CURRENT_PER_LED
    RGBColor_t scaled_color = *color;
    scale_color(&scaled_color);
    apa102_set_all_leds(scaled_color);
#else
    apa102_set_all_leds(*color);
#endif
}

static inline void set_led(const RGBColor_t *color) {
#ifdef CURRENT_PER_LED
    RGBColor_t scaled_color = *color;
    scale_color(&scaled_color);
    apa102_set_led(scaled_color);
#else
    apa102_set_led(*color);
#endif
}

void flash(const RGBColor_t *sequence, const uint8_t sequence_length, const uint16_t color_delay, const uint16_t blank_delay) {
    for (uint8_t i = 0; i < sequence_length; i++) {
        set_all_leds(sequence + i);
        variable_delay(color_delay);
        apa102_set_all_leds(rgb(0x000000));
        variable_delay(blank_delay);
    }
}

void ribbon(const RGBColor_t *sequence, const uint8_t sequence_length, const uint16_t color_delay) {
    for (uint8_t i = 0; i < sequence_length; i++) {
        set_all_leds(sequence + i);
        variable_delay(color_delay);
    }
}

void scroll(const RGBColor_t *sequence, const uint8_t length, const bool reverse, const uint16_t delay) {
    uint8_t modulus = length > LED_COUNT ? LED_COUNT : length;
    for (uint8_t shift = 0; shift < modulus; shift++) {
        apa102_start();
#ifdef STARTING_LED
        shift_starting_led();
#endif
        for (uint8_t led = 0; led < LED_COUNT; led++) {
            uint8_t led_shift = led;
            if (!reverse) {
                led_shift += shift;
            } else {
                led_shift += modulus - shift;
            }
            led_shift %= modulus;
            set_led(sequence + led_shift);
        }
        apa102_end();
        variable_delay(delay);
    }
}

void extend(const RGBColor_t color, const uint16_t delay) {
    uint8_t current_level = 0;
    while (current_level <= LED_COUNT) {
        apa102_start();
#ifdef STARTING_LED
        shift_starting_led();
#endif
        for (uint8_t led = 0; led < LED_COUNT; led++) {
            if (led + 1 <= current_level) {
                set_led(&color);
            } else {
                apa102_set_led(rgb(0x000000));
            }
        }
        apa102_end();
        variable_delay(delay);
        current_level++;
    }
}

void retract(const RGBColor_t color, const uint16_t delay) {
    uint8_t current_level = LED_COUNT;
    while (current_level != 255) {
        apa102_start();
#ifdef STARTING_LED
        shift_starting_led();
#endif
        for (uint8_t led = 0; led < LED_COUNT; led++) {
            if ((led + 1) <= current_level) {
                set_led(&color);
            } else {
                apa102_set_led(rgb(0x000000));
            }
        }
        apa102_end();
        variable_delay(delay);
        current_level--;
    }
}
