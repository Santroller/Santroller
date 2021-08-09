#pragma once
#include "spi_impl.h"
#include <stdint.h>
#include <stdbool.h>
void spi_begin(uint32_t clock, bool cpol, bool cpha, bool lsbfirst);
uint8_t spi_transfer(uint8_t data);
void spi_high(void);
void spi_low(void);