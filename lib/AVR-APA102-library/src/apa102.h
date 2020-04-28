#ifndef APA102_H
#define APA102_H

#include <avr/io.h>
#include <stdbool.h>


#ifndef SCK_DDR
#error SCK_DDR macro must be defined.
#endif
#ifndef SCK
#error SCK macro must be defined.
#endif

#ifndef MOSI_DDR
#error MOSI_DDR macro must be defined.
#endif
#ifndef MOSI_PORT
#error MOSI_PORT macro must be defined.
#endif
#ifndef MOSI
#error MOSI macro must be defined.
#endif

#ifndef SS_DDR
#error SS_DDR macro must be defined.
#endif
#ifndef SS_PORT
#error SS_PORT macro must be defined.
#endif
#ifndef SS
#error SS macro must be defined.
#endif

extern uint8_t led_count;
/* Defines a color for an APA102 LED */
typedef struct RGBColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBColor_t;
inline void apa102_set_led_count(uint8_t count) {
    led_count = count;
}
/* Return a RGBColor from a 32-bit hex color */
inline RGBColor_t rgb(const uint32_t color) {
    uint8_t red = color >> 16;
    uint8_t green = ((color & 0x00ff00) >> 8);
    uint8_t blue = color & 0x0000ff;
    RGBColor_t c = { red, green, blue };
    return c;
}

/* Determine if two RGBColors are equal by value */
inline bool rgbcolor_equal(const RGBColor_t *c1, const RGBColor_t *c2) {
    return c1->red == c2->red && c1->green == c2->green && c1->blue == c2->blue;
}

/* Initialize Hardware SPI for the APA102 LEDs. This runs with a clock
 * prescaler of 1/2 & takes over your MOSI, SCK, & SS pins.
 */
inline void apa102_init_spi(void) {
    MOSI_DDR |= (1 << MOSI);
    SCK_DDR |= (1 << SCK);
    SS_DDR |= (1 << SS);

    // set mosi & ss to high
    MOSI_PORT |= (1 << MOSI);
    SS_PORT |= (1 << SS);

    // enable spi
    SPCR |= (1 << SPE);

    // set as master
    SPCR |= 1 << MSTR;

    // set clock polarity/phase to mode 3
    SPCR |= (1 << CPOL) | (1 << CPHA);

    // set clock scale to 1/2
    SPSR |= 1 << SPI2X;
    SPCR |= (1 << SPR1) | (1 << SPR0);
}

/* Send out data via SPI & wait until transmission is complete */
inline void apa102_transmit_byte(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF))) {}
}

/* Send the starting sequence for a new color configuration */
inline void apa102_start(void) {
    for (uint8_t i = 0; i < 4; i++) {
        apa102_transmit_byte(0x00);
    }
}

/* Send the ending sequence for a new color configuration
 *
 * Each LED requires half a clock cycle of the stop frame. Each transmitted
 * byte creates 8 clock cycles, so we need to transmit 1 stop byte for 1-16
 * LEDs, 2 stop bytes for 17-32 LEDs, etc.
 *
 */
inline void apa102_end(void) {
#ifdef STARTING_LED
    uint8_t total_leds = led_count + STARTING_LED - 1;
#else
    uint8_t total_leds = led_count;
#endif
    uint8_t stop_bytes = (total_leds + 15) / 16;
    for (uint8_t i = 0; i < stop_bytes; i++) {
        apa102_transmit_byte(0xff);
    }
}

/* Send an LED frame with the given color at full brightness */
inline void apa102_set_led(const RGBColor_t color) {
    apa102_transmit_byte(0xff);
    apa102_transmit_byte(color.blue);
    apa102_transmit_byte(color.green);
    apa102_transmit_byte(color.red);
}

#ifdef STARTING_LED
const RGBColor_t BLANK_LED;
/* Send the required number of 0x0000000 LED frames to shift the first colored
 * LED to the `STARTING_LED` position.
 */
inline void shift_starting_led(void) {
    for (uint8_t i = 1; i < STARTING_LED; i++) {
        apa102_set_led(BLANK_LED);
    }
}
#endif

/* Set all the LEDs to the given color */
inline void apa102_set_all_leds(const RGBColor_t color) {
    apa102_start();
#ifdef STARTING_LED
    shift_starting_led();
#endif
    for (uint8_t i = 0; i < led_count; i++) {
        apa102_set_led(color);
    }
    apa102_end();
}

#ifdef CURRENT_PER_LED
/* Scale Down a Single R, G, or B Channel for an LED given the LED's total
 * current draw.
 */
inline uint8_t scale_channel(uint16_t value, uint8_t led_draw) {
    return value * CURRENT_PER_LED / led_draw;
}
/* Scale Down an RGB color if it exceeds the milliamp limit. */
inline void scale_color(RGBColor_t *color) {
    // Assuming linear current draw w/ 60mA max per LED
    uint8_t current_draw =
        (((uint16_t) color->red * 20) / 255) +
        (((uint16_t) color->green * 20) / 255) +
        (((uint16_t) color->blue * 20) / 255)
        ;

    if (current_draw > CURRENT_PER_LED) {
        // scaled channel = (limit / draw * value)
        color->red = scale_channel(color->red, current_draw);
        color->green = scale_channel(color->green, current_draw);
        color->blue = scale_channel(color->blue, current_draw);
    }
}
#endif


#endif
