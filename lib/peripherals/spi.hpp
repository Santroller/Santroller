#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
class SPIMasterInterface
{
public:
    SPIMasterInterface(uint8_t block, spi_cpha_t cpha, spi_cpol_t cpol, int8_t sck, int8_t mosi, int8_t miso, bool msbfirst, uint32_t clock);
    uint8_t transfer(uint8_t data);

private:
    spi_inst_t *spi;
    spi_inst_t *_hardwareBlocks[NUM_I2CS] = {spi0, spi1};
    bool m_msbfirst;
};