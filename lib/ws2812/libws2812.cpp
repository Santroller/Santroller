#include "libws2812.hpp"
#include <stdio.h>
#include "hardware/dma.h"
#include "pico/time.h"
#include <cstdlib>
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

WS2812* WS2812::s_dma_owners[NUM_DMA_CHANNELS] = {};

int64_t WS2812::reset_delay_complete(__unused alarm_id_t id, void *user_data)
{
    auto* instance = static_cast<WS2812*>(user_data);
    dma_channel_set_read_addr(instance->m_dma_channel, instance->led_state, true);
    instance->m_reset_delay_alarm_id = 0;
    return 0;
}

void __isr WS2812::dma_complete_handler()
{
    uint32_t owned_channels = 0;
    for (uint channel = 0; channel < NUM_DMA_CHANNELS; channel++)
    {
        if (s_dma_owners[channel])
            owned_channels |= 1u << channel;
    }

    const uint32_t completed_channels = dma_hw->ints0 & owned_channels;
    dma_hw->ints0 = completed_channels;

    for (uint channel = 0; channel < NUM_DMA_CHANNELS; channel++)
    {
        WS2812* instance = s_dma_owners[channel];
        if (instance && (completed_channels & (1u << channel)))
        {
            if (instance->m_reset_delay_alarm_id)
                cancel_alarm(instance->m_reset_delay_alarm_id);
            instance->m_reset_delay_alarm_id = add_alarm_in_us(
                400, reset_delay_complete, instance, true);
        }
    }
}

WS2812::WS2812(uint8_t pin, uint8_t count, WS2812Type type) : m_pin(pin), hasW(type >= Ws2812Rgbw), m_type(type), m_count(count)
{
}
void WS2812::begin()
{
    printf("ws2812 begin %d!\r\n", m_count);
    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &ws2812Pio, &ws2812Sm, &ws2812Offset, m_pin, 1, true);
    ws2812_program_init(ws2812Pio, ws2812Sm, ws2812Offset, m_pin, 800000, hasW);
    m_dma_channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(m_dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(ws2812Pio, ws2812Sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    led_state = new uint32_t[m_count];
    memset(led_state, 0, sizeof(uint32_t) * m_count);
    s_dma_owners[m_dma_channel] = this;
    putLeds(led_state, m_count);
    dma_channel_configure(
        m_dma_channel,             // Channel to be configured
        &c,                        // The configuration we just created
        &ws2812Pio->txf[ws2812Sm], // The initial write address
        led_state,                 // The initial read address
        m_count,                   // Number of transfers
        true                       // Start immediately.
    );
    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(m_dma_channel, true);
    irq_set_enabled(DMA_IRQ_0, true);
}
void WS2812::end()
{
    if (m_dma_channel == UINT_MAX)
        return;

    if (m_reset_delay_alarm_id)
    {
        cancel_alarm(m_reset_delay_alarm_id);
        m_reset_delay_alarm_id = 0;
    }
    dma_channel_set_irq0_enabled(m_dma_channel, false);
    s_dma_owners[m_dma_channel] = nullptr;
    pio_remove_program_and_unclaim_sm(&ws2812_program, ws2812Pio, ws2812Sm, ws2812Offset);
    dma_channel_cleanup(m_dma_channel);
    dma_channel_unclaim(m_dma_channel);
    m_dma_channel = UINT_MAX;
    delete [] led_state;
    led_state = nullptr;
    printf("ws2812 end!\r\n");
}

WS2812::~WS2812()
{
    printf("~WS2812\r\n");
}