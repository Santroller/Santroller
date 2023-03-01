#include "io.h"

#include <math.h>
#include <stdio.h>

#include "Arduino.h"
#include "config.h"
#include "esp_system.h"
#include "pins_define.h"
#if SPI_0_CPHA==0 && SPI_0_CPOL==0
#define SPI_0_MODE 0
#elif SPI_0_CPHA==0 && SPI_0_CPOL==1
#define SPI_0_MODE 1
#elif SPI_0_CPHA==1 && SPI_0_CPOL==0
#define SPI_0_MODE 2
#elif SPI_0_CPHA==1 && SPI_0_CPOL==1
#define SPI_0_MODE 3
#endif
#if SPI_1_CPHA==0 && SPI_1_CPOL==0
#define SPI_1_MODE 0
#elif SPI_1_CPHA==0 && SPI_1_CPOL==1
#define SPI_0_MODE 1
#elif SPI_1_CPHA==1 && SPI_1_CPOL==0
#define SPI_0_MODE 2
#elif SPI_1_CPHA==1 && SPI_1_CPOL==1
#define SPI_0_MODE 3
#endif
#if SPI_0_MSBFIRST==0
    #define SPI_0_BITORDER SPI_MSBFIRST
#else
    #define SPI_0_BITORDER SPI_LSBFIRST
#endif
volatile bool spi_acknowledged = false;
spi_t* spi_0;
spi_t* spi_1;
void spi_begin() {
#ifdef SPI_0_MOSI
    spiStartBus(0, spiFrequencyToClockDiv(SPI_0_CLOCK), SPI_0_MODE, SPI_0_BITORDER);
    spiAttachSCK(SPI_0, SPI_0_SCK);
    spiAttachMISO(SPI_0, SPI_0_MISO);
    spiAttachMOSI(SPI_0, SPI_0_MOSI);
#endif
#ifdef SPI_1_MOSI
    spiStartBus(1, spiFrequencyToClockDiv(SPI_1_CLOCK), SPI_1_MODE, SPI_1_BITORDER);
    spiAttachSCK(SPI_1, SPI_1_SCK);
    spiAttachMISO(SPI_1, SPI_1_MISO);
    spiAttachMOSI(SPI_1, SPI_1_MOSI);
#endif
}
// SINCE LSB_FIRST isn't supported, we need to invert bits ourselves when its set
uint8_t spi_transfer(SPI_BLOCK block, uint8_t data) {
    return spiTransferByte(block, data);
}
void spi_high(SPI_BLOCK block) {}
void twi_init() {
#ifdef TWI_0_CLOCK
    i2cInit(0, TWI_0_SDA, TWI_0_SCL, TWI_0_CLOCK);
#endif
#ifdef TWI_1_CLOCK
    i2cInit(1, TWI_1_SDA, TWI_1_SCL, TWI_1_CLOCK);
#endif
}
bool twi_readFromPointerSlow(TWI_BLOCK block, uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data) {
    if (!i2cWrite(block, address, &pointer, 1, 1000)) return false;
    delayMicroseconds(170);
    size_t count;
    return i2cRead(block, address, data, length, 1000, &count);
}
bool twi_readFrom(TWI_BLOCK block, uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop) {
    size_t count;
    int ret =
        i2cRead(block, address, data, length, 1000, &count);
    return ret > 0 ? ret : 0;
}

bool twi_writeTo(TWI_BLOCK block, uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) {
    int ret = i2cWrite(block, address, data, length, 1000);
    return ret > 0;
}

#ifdef PS2_ACK
void callback() {
    spi_acknowledged = true;
}
void init_ack() {
    attachInterrupt(PS2_ACK, &callback, RISING);
}
#endif

void read_serial(uint8_t *id, uint8_t len) {
    uint8_t retrieved_id[8];
    esp_efuse_mac_get_default(retrieved_id);
    size_t i;
    // Generate hex one nibble at a time
    for (i = 0; (i < len - 1) && (i < 8 * 2); i++) {
        int nibble = (retrieved_id[i / 2] >> (4 - 4 * (i & 1))) & 0xf;
        id[i] = (char)(nibble < 10 ? nibble + '0' : nibble + 'A' - 10);
    }
    id[i] = 0;
}