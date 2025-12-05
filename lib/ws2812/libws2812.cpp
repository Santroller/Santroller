#include "libws2812.hpp"
#include "stdio.h"
void WS2812::putLed(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t w = 0;
    if (hasW) {
        w = MIN(MIN(a, b), c);
        a -= w;
        b -= w;
        c -= w;
    }
    pio_sm_put_blocking(ws2812Pio, ws2812Sm,
                        ((uint32_t)(a) << 24) |
                            ((uint32_t)(b) << 16) |
                            ((uint32_t)(c) << 8) |
                            ((uint32_t)(w)));
}

WS2812::WS2812(uint8_t pin, bool hasW): pin(pin), hasW(hasW) {

    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &ws2812Pio, &ws2812Sm, &ws2812Offset, pin, 1, true);
    ws2812_program_init(ws2812Pio, ws2812Sm, ws2812Offset, pin, 800000, hasW);
}