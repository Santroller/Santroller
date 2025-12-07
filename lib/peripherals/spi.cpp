#include "spi.hpp"

#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"
SPIMasterInterface::SPIMasterInterface(uint8_t block, spi_cpha_t cpha, spi_cpol_t cpol, int8_t sck, int8_t mosi, int8_t miso, bool msbfirst, uint32_t clock): m_msbfirst(msbfirst)
{
    spi = _hardwareBlocks[block];
    spi_init(spi, clock);
    spi_set_format(spi, 8, cpol, cpha, SPI_MSB_FIRST);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    if (mosi != -1)
    {
        gpio_set_function(mosi, GPIO_FUNC_SPI);
    }
    if (miso != -1)
    {
        gpio_set_function(miso, GPIO_FUNC_SPI);
        gpio_set_pulls(miso, true, false);
    }
}
uint8_t SPIMasterInterface::transfer(uint8_t data)
{
    if (!m_msbfirst) {
        data = revbits(data);
    }
    spi_write_read_blocking(spi, &data, &data, 1);
    if (!m_msbfirst) {
        data = revbits(data);
    }
    return data;
}