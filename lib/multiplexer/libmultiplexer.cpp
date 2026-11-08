#include "libmultiplexer.hpp"
#include "devices/pins.hpp"

#define MULTIPLEXER_SLOW_SETTLE_US 50

Multiplexer::Multiplexer(uint8_t s0Pin, uint8_t s1Pin, uint8_t s2Pin, uint8_t s3Pin, uint8_t inputPin, bool sixteen_channel, bool slow) : s0Pin(s0Pin), s1Pin(s1Pin), s2Pin(s2Pin), s3Pin(s3Pin), inputPin(inputPin), sixteenChannel(sixteen_channel), slow(slow)
{
    // Unset pins (-1) arrive here as 255
    m_valid = s0Pin < NUM_BANK0_GPIOS && s1Pin < NUM_BANK0_GPIOS && s2Pin < NUM_BANK0_GPIOS &&
              (!sixteenChannel || s3Pin < NUM_BANK0_GPIOS) &&
              is_adc_pin(inputPin);
}

// 64 bit, as the select pins may be above GP31 on an RP2350B
uint64_t Multiplexer::select_mask() const
{
    uint64_t mask = 1ull << s0Pin | 1ull << s1Pin | 1ull << s2Pin;
    if (sixteenChannel)
    {
        mask |= 1ull << s3Pin;
    }
    return mask;
}

static void select_pins_init(uint64_t mask)
{
    for (uint pin = 0; pin < NUM_BANK0_GPIOS; pin++)
    {
        if (mask & (1ull << pin))
        {
            gpio_init(pin);
        }
    }
}

static void select_pins_set_dir(uint64_t mask, bool out)
{
    for (uint pin = 0; pin < NUM_BANK0_GPIOS; pin++)
    {
        if (mask & (1ull << pin))
        {
            gpio_set_dir(pin, out);
        }
    }
}

void Multiplexer::begin()
{
    if (!m_valid)
    {
        return;
    }
    select_pins_init(select_mask());
    select_pins_set_dir(select_mask(), true);
    adc_gpio_init(inputPin);
}

void Multiplexer::end()
{
    if (!m_valid)
    {
        return;
    }
    select_pins_set_dir(select_mask(), false);
    gpio_deinit(inputPin);
}

uint16_t Multiplexer::read(uint8_t channel)
{
    if (!m_valid)
    {
        return 0;
    }
    gpio_put(s0Pin, channel & 0b0001);
    gpio_put(s1Pin, channel & 0b0010);
    gpio_put(s2Pin, channel & 0b0100);
    if (sixteenChannel)
    {
        gpio_put(s3Pin, channel & 0b1000);
    }
    if (slow)
    {
        sleep_us(MULTIPLEXER_SLOW_SETTLE_US);
    }
    adc_select_input(adc_input_for_pin(inputPin));
    return adc_read() << 4;
}
