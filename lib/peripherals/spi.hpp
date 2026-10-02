#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include "pico/time.h"

#define SPI_MAX_TRANSFER_SIZE 4096
#define SPI_TRANSFER_TIMEOUT_MS 10000
#define SPI_TAKE_MUTEX_TIMEOUT_MS 10000



class SPIMasterInterface
{
public:
    SPIMasterInterface(uint8_t block, spi_cpha_t cpha, spi_cpol_t cpol, int8_t sck, int8_t mosi, int8_t miso, bool msbfirst, uint32_t clock);
    ~SPIMasterInterface();
    uint8_t transfer(uint8_t data);
    void transfer(const uint8_t *tx, uint8_t *rx, size_t len);

    bool transfer_dma_start(const uint8_t *tx, uint8_t *rx, size_t len);
    bool transfer_dma_start_paced(const uint8_t *tx, uint8_t *rx, size_t len, uint32_t interval_us);
    bool transfer_dma_busy();
    void transfer_dma_finish(uint8_t *rx, size_t len);
    void transfer_dma_abort();

private:
    spi_inst_t *spi;
    spi_inst_t *_hardwareBlocks[NUM_I2CS] = {spi0, spi1};
    bool m_msbfirst;
    bool m_valid;
    int m_dma_tx = -1;
    int m_dma_rx = -1;
    int m_dma_timer = -1;
    uint8_t m_dma_tx_buf[64] = {0};
    uint8_t m_dma_rx_buf[64] = {0};
};