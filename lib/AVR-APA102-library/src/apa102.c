#include "apa102.h"
uint8_t led_count = 0;
extern inline RGBColor_t rgb(const uint32_t color);
extern inline bool rgbcolor_equal(const RGBColor_t *c1, const RGBColor_t *c2);

extern inline void apa102_init_spi(void);
extern inline void apa102_transmit_byte(uint8_t data);
extern inline void apa102_start(void);
extern inline void apa102_end(void);
extern inline void apa102_set_led(const RGBColor_t color);
#ifdef STARTING_LED
const RGBColor_t BLANK_LED = { 0x00, 0x00, 0x00 };
extern inline void shift_starting_led(void);
#endif
extern inline void apa102_set_all_leds(const RGBColor_t color);
#ifdef CURRENT_PER_LED
extern inline uint8_t scale_channel(uint16_t value, uint8_t led_draw);
extern inline void scale_color(RGBColor_t *color);
#endif
