#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "pins/pins.h"
void spi_begin(uint32_t clock, bool cpol, bool cpha, bool lsbfirst);
uint8_t spi_transfer(uint8_t data);
void spi_high(void);
void init_ack(Pin_t ack);
extern volatile bool spi_acknowledged;
