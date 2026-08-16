#pragma once

#include "hardware/pio.h"
#include "hardware/dma.h"
#include <stdint.h>

// Configuration options for pio spi
// All items necessary except for callbacks and callback context
typedef struct pio_spi_config_t {
    int pio_idx; // pio index
    int cs_pin; // CS Pin (Can be any pin)
    int sck_pin; // SCK Pin (Must be 1 pin after COPI)
    int copi_pin; // COPI Pin (Can be any pin)
    int cipo_pin; // CIPO Pin (Can be any pin)
    int ack_pin; // ACK Pin (Can be any pin)
    int cs_sm;
    int initial_sm;
    int combined_sm;
} pio_spi_config_t;

// Internal representation of PIO SPI
typedef struct pio_spi_t {
    bool allocated;
    PIO pio;
    uint offset_combined;
    uint offset_cs;
    uint channel_write;
    uint channel_read;
    uint32_t startstop_mask;
    pio_spi_config_t config;
    volatile uint8_t c4c_state;
    volatile uint8_t c46_state;
    uint8_t report_len;
    uint8_t resp_42[32];
    uint8_t resp_41[6];
    bool analog;
    volatile bool configMode;
    bool locked;
    volatile uint8_t dma_buf[32];
} pio_spi_t;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a PIO SPI object. Asserts if already allocated.
// The engine is stopped upon initialization, and must be started.
pio_spi_t* pio_spi_init(const pio_spi_config_t* config);
// Frees a PIO SPI object, stopping if its already running
void pio_spi_free(pio_spi_t* spi);

// Provide the read buffer for DMA. Must be called per transaction if read data is requested,
// but at max once per transaction.
__force_inline static void pio_spi_provide_read_buffer(pio_spi_t* spi, volatile uint8_t* buf, uint8_t buf_bytes) {
    dma_channel_transfer_to_buffer_now(spi->channel_read, buf, dma_encode_transfer_count(buf_bytes));
}

// Provide the write buffer for DMA. Must be called per transaction if write data is requested,
// but at max once per transaction.
__force_inline static void pio_spi_provide_write_buffer(pio_spi_t* spi, const volatile uint8_t* buf, uint8_t buf_bytes) {
    dma_channel_transfer_from_buffer_now(spi->channel_write, buf, dma_encode_transfer_count(buf_bytes));
}

__force_inline static dma_channel_hw_t* pio_spi_get_dma_read_channel(pio_spi_t* spi) {
    return dma_channel_hw_addr(spi->channel_read);
}

__force_inline static dma_channel_hw_t* pio_spi_get_dma_write_channel(pio_spi_t* spi) {
    return dma_channel_hw_addr(spi->channel_write);
}

__force_inline static void pio_spi_provide_write_buffer_direct(dma_channel_hw_t* hw, const volatile void* buf, uint8_t buf_bytes) {
    hw->read_addr = (uintptr_t) buf;
    hw->al1_transfer_count_trig = buf_bytes;
}

__force_inline static void pio_spi_provide_read_buffer_direct(dma_channel_hw_t* hw, volatile void* buf, uint8_t buf_bytes) {
    hw->write_addr = (uintptr_t) buf;
    hw->al1_transfer_count_trig = buf_bytes;
}
// Start the pio spi engine.
void pio_spi_start(const pio_spi_t* spi);
// Stop the pio spi engine.
void pio_spi_stop(pio_spi_t* spi);

#ifdef __cplusplus
}
#endif