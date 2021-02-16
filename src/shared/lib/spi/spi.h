#pragma once
#include <stdint.h>
#include <stdbool.h>
void spi_begin(uint32_t clock, bool cpol, bool cpha);
uint8_t spi_transfer(uint8_t data);