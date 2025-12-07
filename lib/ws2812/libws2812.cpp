#include "libws2812.hpp"
void WS2812::putLed(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t w = 0;
    if (hasW)
    {
        w = MIN(MIN(r, g), b);
        r -= w;
        g -= w;
        b -= w;
    }
    uint8_t x = r, y = g, z = b;
    switch (m_type)
    {
    case Ws2812Rgbw:
    case Ws2812Rgb:
        x = r;
        y = g;
        z = b;
        break;
    case Ws2812Rbgw:
    case Ws2812Rbg:
        x = r;
        y = b;
        z = g;
        break;
    case Ws2812Grbw:
    case Ws2812Grb:
        x = g;
        y = r;
        z = b;
        break;
    case Ws2812Gbrw:
    case Ws2812Gbr:
        x = g;
        y = b;
        z = r;
        break;
    case Ws2812Brgw:
    case Ws2812Brg:
        x = b;
        y = r;
        z = g;
        break;
    case Ws2812Bgrw:
    case Ws2812Bgr:
        x = b;
        y = g;
        z = r;
        break;
    }
    pio_sm_put_blocking(ws2812Pio, ws2812Sm,
                        ((uint32_t)(x) << 24) |
                            ((uint32_t)(y) << 16) |
                            ((uint32_t)(z) << 8) |
                            ((uint32_t)(w)));
}

WS2812::WS2812(uint8_t pin, WS2812Type type) : pin(pin), hasW(type >= Ws2812Rgbw), m_type(type)
{

    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &ws2812Pio, &ws2812Sm, &ws2812Offset, pin, 1, true);
    ws2812_program_init(ws2812Pio, ws2812Sm, ws2812Offset, pin, 800000, hasW);
}