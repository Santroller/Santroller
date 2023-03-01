#include <stdint.h>
#include "Arduino.h"
#include "esp32-hal-spi.h"
#define NUM_TOTAL_PINS 34
#define DIGITAL_BITMASK_TYPE uint16_t
extern spi_t* spi_0;
extern spi_t* spi_1;
#define PIN_A0 0
#define TWI_0 0
#define TWI_1 1
#define SPI_0 spi_0
#define SPI_1 spi_1
#define TWI_BLOCK uint8_t
#define SPI_BLOCK spi_t*