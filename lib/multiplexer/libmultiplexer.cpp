#include "libmultiplexer.hpp"

#define MULTIPLEXER_SLOW_SETTLE_US 50

Multiplexer::Multiplexer(uint8_t s0Pin, uint8_t s1Pin, uint8_t s2Pin, uint8_t s3Pin, uint8_t inputPin, bool sixteen_channel, bool slow) : s0Pin(s0Pin), s1Pin(s1Pin), s2Pin(s2Pin), s3Pin(s3Pin), inputPin(inputPin), sixteenChannel(sixteen_channel), slow(slow)
{
}

uint32_t Multiplexer::select_mask() const
{
    uint32_t mask = 1 << s0Pin | 1 << s1Pin | 1 << s2Pin;
    if (sixteenChannel)
    {
        mask |= 1 << s3Pin;
    }
    return mask;
}

void Multiplexer::begin()
{
    gpio_init_mask(select_mask());
    gpio_set_dir_out_masked(select_mask());
    adc_gpio_init(inputPin);
}

void Multiplexer::end()
{
    gpio_set_dir_in_masked(select_mask());
    gpio_deinit(inputPin);
}

uint16_t Multiplexer::read(uint8_t channel)
{
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
    // the ADC is numbered by channel, starting from GPIO 26
    adc_select_input(inputPin - 26);
    return adc_read() << 4;
}
