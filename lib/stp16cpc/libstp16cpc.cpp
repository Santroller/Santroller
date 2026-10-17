#include "libstp16cpc.hpp"
#include "utils.h"
#include <stdio.h>
STP16CPC::STP16CPC(uint8_t block, uint8_t mosi, uint8_t sck, uint8_t oe, uint8_t le, uint8_t count) : interface(block, SPI_CPHA_0, SPI_CPOL_0, sck, mosi, -1, true, 12000000),  m_oe(oe), m_le(le), m_count(count)
{
    m_bytes = 0;
    // Unset pins (-1) arrive here as 255
    m_valid = mosi < NUM_BANK0_GPIOS && sck < NUM_BANK0_GPIOS && oe < NUM_BANK0_GPIOS && le < NUM_BANK0_GPIOS;
    if (!m_valid)
    {
        printf("stp16cpc pins invalid, aborting\r\n");
        return;
    }
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
    if (!m_valid)
    {
        return;
    }
    for (size_t i = 0; i < m_bytes; i++) {
        interface.transfer(leds[i]);
    }
    gpio_put(m_le, true);
    sleep_us(10);
    gpio_put(m_le, false);
}