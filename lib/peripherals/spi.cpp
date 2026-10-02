#include "spi.hpp"
#include <hardware/dma.h>
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
    m_valid = sck != -1 || mosi != -1 || miso != -1;
    if (m_valid) {
        m_dma_tx = dma_claim_unused_channel(false);
        m_dma_rx = dma_claim_unused_channel(false);
    }
}

SPIMasterInterface::~SPIMasterInterface()
{
    if (m_dma_tx >= 0) {
        dma_channel_unclaim(m_dma_tx);
    }
    if (m_dma_rx >= 0) {
        dma_channel_unclaim(m_dma_rx);
    }
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

void SPIMasterInterface::transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (!m_valid || len == 0) {
        return;
    }
    if (m_msbfirst) {
        spi_write_read_blocking(spi, tx, rx, len);
    } else {
        for (size_t i = 0; i < len; i++) {
            uint8_t data = revbits(tx[i]);
            uint8_t ret = 0;
            spi_write_read_blocking(spi, &data, &ret, 1);
            rx[i] = revbits(ret);
        }
    }
}

bool SPIMasterInterface::transfer_dma_start(const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (!m_valid || m_dma_tx < 0 || m_dma_rx < 0 || len == 0 || len > sizeof(m_dma_tx_buf)) {
        return false;
    }

    if (m_msbfirst) {
        memcpy(m_dma_tx_buf, tx, len);
    } else {
        for (size_t i = 0; i < len; i++) {
            m_dma_tx_buf[i] = revbits(tx[i]);
        }
    }

    dma_channel_config rx_config = dma_channel_get_default_config(m_dma_rx);
    channel_config_set_read_increment(&rx_config, false);
    channel_config_set_write_increment(&rx_config, true);
    channel_config_set_transfer_data_size(&rx_config, DMA_SIZE_8);
    channel_config_set_dreq(&rx_config, spi_get_dreq(spi, false));
    dma_channel_configure(
        m_dma_rx, &rx_config, m_dma_rx_buf, &spi_get_hw(spi)->dr, len, false);

    dma_channel_config tx_config = dma_channel_get_default_config(m_dma_tx);
    channel_config_set_read_increment(&tx_config, true);
    channel_config_set_write_increment(&tx_config, false);
    channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_8);
    channel_config_set_dreq(&tx_config, spi_get_dreq(spi, true));
    dma_channel_configure(
        m_dma_tx, &tx_config, &spi_get_hw(spi)->dr, m_dma_tx_buf, len, false);

    while (spi_is_readable(spi)) {
        (void)spi_get_hw(spi)->dr;
    }

    dma_start_channel_mask((1u << m_dma_rx) | (1u << m_dma_tx));
    return true;
}

bool SPIMasterInterface::transfer_dma_busy()
{
    if (m_dma_rx < 0 || m_dma_tx < 0) return false;
    return dma_channel_is_busy(m_dma_rx) || dma_channel_is_busy(m_dma_tx);
}

void SPIMasterInterface::transfer_dma_finish(uint8_t *rx, size_t len)
{
    if (m_dma_rx >= 0 && dma_channel_is_busy(m_dma_rx)) {
        dma_channel_wait_for_finish_blocking(m_dma_rx);
    }
    if (m_dma_tx >= 0 && dma_channel_is_busy(m_dma_tx)) {
        dma_channel_wait_for_finish_blocking(m_dma_tx);
    }

    if (!rx || len == 0) return;

    if (m_msbfirst) {
        memcpy(rx, m_dma_rx_buf, len);
    } else {
        for (size_t i = 0; i < len; i++) {
            rx[i] = revbits(m_dma_rx_buf[i]);
        }
    }
}

void SPIMasterInterface::transfer_dma_abort()
{
    if (m_dma_tx >= 0 && dma_channel_is_busy(m_dma_tx)) {
        dma_channel_abort(m_dma_tx);
    }
    if (m_dma_rx >= 0 && dma_channel_is_busy(m_dma_rx)) {
        dma_channel_abort(m_dma_rx);
    }
}