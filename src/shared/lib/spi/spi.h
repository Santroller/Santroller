#pragma once
#include <stdint.h>
void spi_begin(uint32_t clock, uint8_t config);
uint8_t spi_transfer(uint8_t data);