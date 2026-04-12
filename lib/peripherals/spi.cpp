#include "spi.hpp"

#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include "utils.h"
#include <stdio.h>
SPIMasterInterface::SPIMasterInterface(uint8_t block, spi_cpha_t cpha, spi_cpol_t cpol, int8_t sck, int8_t mosi, int8_t miso, bool msbfirst, uint32_t clock): m_msbfirst(msbfirst)
{
    spi = _hardwareBlocks[block];
    printf("spi: %d %d %d %d %d\r\n", sck, mosi, miso, block, clock);
    spi_init(spi, clock);
    spi_set_format(spi, 8, cpol, cpha, SPI_MSB_FIRST);
    if (sck != -1) {
        gpio_set_function(sck, GPIO_FUNC_SPI);
    }
    if (mosi != -1)
    {
        gpio_set_function(mosi, GPIO_FUNC_SPI);
    }
    if (miso != -1)
    {
        gpio_set_function(miso, GPIO_FUNC_SPI);
        gpio_set_pulls(miso, true, false);
    }
    m_valid = sck != -1 && (mosi != -1 || miso != -1);
}
uint8_t SPIMasterInterface::transfer(uint8_t data)
{
    if (!m_valid) {
        return 0;
    }
    if (!m_msbfirst) {
        data = revbits(data);
    }
    uint8_t ret = 0;
    spi_write_read_blocking(spi, &data, &ret, 1);
    if (!m_msbfirst) {
        ret = revbits(ret);
    }
    return ret;
}