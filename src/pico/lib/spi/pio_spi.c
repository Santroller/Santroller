/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pio_spi.h"
#include "hardware/gpio.h"
#include "pins_arduino.h"
#include <pico/time.h>

// Just 8 bit functions provided here. The PIO program supports any frame size
// 1...32, but the software to do the necessary FIFO shuffling is left as an
// exercise for the reader :)
//
// Likewise we only provide MSB-first here. To do LSB-first, you need to
// - Do shifts when reading from the FIFO, for general case n != 8, 16, 32
// - Do a narrow read at a one halfword or 3 byte offset for n == 16, 8
// in order to get the read data correctly justified.

void __time_critical_func(pio_spi_write8_blocking)(const pio_spi_inst_t *spi,
                                                   const uint8_t *src,
                                                   size_t len) {
  size_t tx_remain = len, rx_remain = len;
  // Do 8 bit accesses on FIFO, so that write data is byte-replicated. This
  // gets us the left-justification for free (for MSB-first shift-out)
  io_rw_8 *txfifo = (io_rw_8 *)&spi->pio->txf[spi->sm];
  io_rw_8 *rxfifo = (io_rw_8 *)&spi->pio->rxf[spi->sm];
  while (tx_remain || rx_remain) {
    if (tx_remain && !pio_sm_is_tx_fifo_full(spi->pio, spi->sm)) {
      *txfifo = *src++;
      --tx_remain;
    }
    if (rx_remain && !pio_sm_is_rx_fifo_empty(spi->pio, spi->sm)) {
      (void)*rxfifo;
      --rx_remain;
    }
  }
}

void __time_critical_func(pio_spi_read8_blocking)(const pio_spi_inst_t *spi,
                                                  uint8_t *dst, size_t len) {
  size_t tx_remain = len, rx_remain = len;
  io_rw_8 *txfifo = (io_rw_8 *)&spi->pio->txf[spi->sm];
  io_rw_8 *rxfifo = (io_rw_8 *)&spi->pio->rxf[spi->sm];
  while (tx_remain || rx_remain) {
    if (tx_remain && !pio_sm_is_tx_fifo_full(spi->pio, spi->sm)) {
      *txfifo = 0;
      --tx_remain;
    }
    if (rx_remain && !pio_sm_is_rx_fifo_empty(spi->pio, spi->sm)) {
      *dst++ = *rxfifo;
      --rx_remain;
    }
  }
}

uint8_t revbits2(uint8_t x) {
  x = (((x & 0xaaU) >> 1) | ((x & 0x55U) << 1));
  x = (((x & 0xccU) >> 2) | ((x & 0x33U) << 2));
  x = (((x & 0xf0U) >> 4) | ((x & 0x0fU) << 4));
  return x;
}
/** \brief Attention Delay
 *
 * Time between attention being issued to the controller and the first clock
 * edge (us).
 */
#define ATTN_DELAY 15
void __time_critical_func(pio_spi_write8_read8_blocking)(
    const pio_spi_inst_t *spi, const uint8_t *src, uint8_t *dst, size_t len) {
  const uint8_t *srcStart = src;
  uint8_t cnt = 0;
  gpio_put(PIN_PS2_ATT, 0);
  pio_sm_set_pins_with_mask(spi->pio, spi->sm,
                            (1u << PIN_SPI_SCK) | (0u << PIN_SPI_MOSI),
                            (1u << PIN_SPI_SCK) | (1u << PIN_SPI_MOSI));
  busy_wait_us_32(ATTN_DELAY);
  size_t tx_remain = len, rx_remain = len;
  io_rw_8 *txfifo = (io_rw_8 *)&spi->pio->txf[spi->sm];
  io_rw_8 *rxfifo = (io_rw_8 *)&spi->pio->rxf[spi->sm];
  while (tx_remain || rx_remain) {
    if (tx_remain && !pio_sm_is_tx_fifo_full(spi->pio, spi->sm)) {
      *txfifo = revbits2(*src++);
      --tx_remain;
    }
    if (rx_remain && !pio_sm_is_rx_fifo_empty(spi->pio, spi->sm)) {
      *dst++ = revbits2(*rxfifo);
      if (cnt == 1) {
        uint8_t lenNew = (srcStart[1] & 0x0F) * 2;
        tx_remain = len-tx_remain+lenNew;
        rx_remain = len-rx_remain+lenNew;
      }
      --rx_remain;
      cnt++;
    }
  }
  pio_sm_set_pins_with_mask(spi->pio, spi->sm,
                            (0u << PIN_SPI_SCK) | (1u << PIN_SPI_MOSI),
                            (1u << PIN_SPI_SCK) | (1u << PIN_SPI_MOSI));
  gpio_put(PIN_PS2_ATT, 1);
  busy_wait_us_32(ATTN_DELAY);
}
