#include "libstp16cpc.hpp"
#include "utils.h"
STP16CPC::STP16CPC(uint8_t block, uint8_t mosi, uint8_t sck, uint8_t oe, uint8_t le, uint8_t count) : interface(block, SPI_CPHA_0, SPI_CPOL_0, sck, mosi, -1, true, 12000000), m_count(count), m_oe(oe), m_le(le)
{
    gpio_init(oe);
    gpio_init(le);
    gpio_set_dir(oe, true);
    gpio_set_dir(le, true);
    m_bytes = count / 8;
    if ((count % 8) != 0) {
        m_bytes++;
    }
    gpio_put(m_le, false);
    gpio_put(m_oe, false);
}
void STP16CPC::putLeds(uint8_t* leds)
{
    for (size_t i = 0; i < m_bytes; i+=8) {
        interface.transfer(leds[i]);
    }
    gpio_put(m_le, true);
    sleep_us(10);
    gpio_put(m_le, false);
}