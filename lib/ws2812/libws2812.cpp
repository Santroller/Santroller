#include "libws2812.hpp"
#include <stdio.h>
#include "hardware/dma.h"
#include "pico/time.h"
void WS2812::putLeds(uint32_t *leds, uint8_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        uint8_t r = leds[i] & 0xff;
        uint8_t g = (leds[i] >> 8) & 0xff;
        uint8_t b = (leds[i] >> 16) & 0xff;
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
        led_state[i] = ((uint32_t)(x) << 24) |
                       ((uint32_t)(y) << 16) |
                       ((uint32_t)(z) << 8) |
                       ((uint32_t)(w));
    }
}

static int chan;
static uint32_t *data;
static alarm_id_t reset_delay_alarm_id;
int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data)
{

    dma_channel_set_read_addr(chan, data, true);
    return 0;
}
void __isr dma_complete_handler()
{
    // DMA done. Start a timer as the leds need an off period between led writes
    if (dma_hw->ints0 & (1 << chan))
    {
        // clear IRQ
        dma_hw->ints0 = (1 << chan);
        if (reset_delay_alarm_id)
            cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
    }
}

WS2812::WS2812(uint8_t pin, uint8_t count, WS2812Type type) : m_pin(pin), hasW(type >= Ws2812Rgbw), m_type(type)
{
    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &ws2812Pio, &ws2812Sm, &ws2812Offset, m_pin, 1, true);
    ws2812_program_init(ws2812Pio, ws2812Sm, ws2812Offset, m_pin, 800000, hasW);
    chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(ws2812Pio, ws2812Sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    data = led_state;

    dma_channel_configure(
        chan,                      // Channel to be configured
        &c,                        // The configuration we just created
        &ws2812Pio->txf[ws2812Sm], // The initial write address
        led_state,                 // The initial read address
        count,                     // Number of transfers; in this case each is 1 byte.
        true                       // Start immediately.
    );
    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(chan, true);
    irq_set_enabled(DMA_IRQ_0, true);
}

WS2812::~WS2812()
{
    pio_remove_program_and_unclaim_sm(&ws2812_program, ws2812Pio, ws2812Sm, ws2812Offset);
    irq_set_enabled(DMA_IRQ_0, false);
    dma_channel_cleanup(chan);
    dma_channel_unclaim(chan);
}

static void pio_irq_handler()
{
}