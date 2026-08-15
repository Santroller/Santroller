#pragma once

#include "hardware/pio.h"
#include "hardware/dma.h"
#include <stdint.h>

// Only supports sendings speeds up to ~7 MHz. Receives have been tested to about 15 MHz,
// and theoretically would support about 17 MHz.

// For all callbacks, it is recommended they all be marked __time_critical_func,
// as well as any functions they call. Callbacks are not called if they are null.

// After each transaction, both DMA buffers are cleared. This means for each
// transaction, they must be provided if required. The transaction will work
// if only 1 of the 2 is provided (Technically it will work if none are provided
// but whats the point in that).

// Called when CS goes low
// This function is when it is recommended to provide the read buffer.
// The state machines are already started, so it is recommended to provide
// the read buffer, and then do any further computations necessary (Like
// grab all data to be written after first byte.
// If write data does not depend on data received, write buffer can be provided as well.
typedef void(*pio_spi_transaction_started_func)(void* ctx);

// Called when first byte of data is received, containing value of first byte.
// Recommended place to provide the write buffer if first byte is necessary.
// This register can be received even if no read buffer is provided.
typedef void(*pio_spi_data_request_function)(void* ctx, uint8_t reg);

// Called when CS goes high. Provides the number of bytes read, the
// number of bytes written, and the number of bits that were transacted
// by the controller.
// The number of bytes written will be 1 more then the actual number of bytes
// transmitted if less bytes were requested then provided by the DMA buffer.
// This can also occur if the write fifo is delayed past the first byte.
// If read or write buffers are known, they can be provided here for the next
// transaction. If this is done, for the first transaction to be successful,
// the buffers for the first transaction would need to be provided before
// starting, as this callback will not occur before the first transaction occurs.
typedef void(*pio_spi_transaction_ended_func)(void* ctx);

// Configuration options for pio spi
// All items necessary except for callbacks and callback context
typedef struct pio_spi_config_t {
    int pio_idx; // pio index
    int cs_pin; // CS Pin (Can be any pin)
    int sck_pin; // SCK Pin (Must be 1 pin after COPI)
    int copi_pin; // COPI Pin (Can be any pin)
    int cipo_pin; // CIPO Pin (Can be any pin)
    int ack_pin; // ACK Pin (Can be any pin)
    int dbg_pin;    // Debug pin is written high at the beginning of each ISR
                    // and written low at the end. Useful for debugging timing
                    // issues between ISRs and data bytes
    int cs_sm;
    int initial_sm;
    int combined_sm;
    bool cs_active_high; // True to have CS pin be active high, false for active low
    bool trigger_on_falling; // True to trigger on the falling edge rather then rising edge.
    // Theres no variable for clock phase, however modes 1 and 3 (clock idles in triggered state)
    // are better, as the initial write DMA is only pulled when the clock is pulled away from idle
    // rather then as soon as the CS pin is triggered

    uint8_t default_write_value; // The byte value to write when there is no data available from the write FIFO.

    pio_spi_transaction_started_func transaction_started;
    pio_spi_data_request_function data_request;
    pio_spi_transaction_ended_func transaction_ended;
    void* callback_ctx; // Passed to each callback
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
    uint8_t write_buf_len;
    uint8_t read_buf_len;
    uint32_t dgb_mask;
    uint8_t header;
    uint8_t c4c_state;
    uint8_t c46_state;
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
    spi->read_buf_len = buf_bytes;
    dma_channel_transfer_to_buffer_now(spi->channel_read, buf, buf_bytes);
}

// Provide the write buffer for DMA. Must be called per transaction if write data is requested,
// but at max once per transaction.
__force_inline static void pio_spi_provide_write_buffer(pio_spi_t* spi, const volatile uint8_t* buf, uint8_t buf_bytes) {
    spi->write_buf_len = buf_bytes;

    dma_channel_transfer_from_buffer_now(spi->channel_write, buf, buf_bytes);
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

__force_inline static void pio_spi_provide_write_buffer_length(pio_spi_t* spi, uint8_t buf_bytes) {
    spi->write_buf_len = buf_bytes;
}

__force_inline static void pio_spi_provide_read_buffer_length(pio_spi_t* spi, uint8_t buf_bytes) {
    spi->read_buf_len = buf_bytes;
}

// Start the pio spi engine.
void pio_spi_start(const pio_spi_t* spi);
// Stop the pio spi engine.
void pio_spi_stop(pio_spi_t* spi);

#ifdef __cplusplus
}
#endif