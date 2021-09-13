#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pio_serialiser.pio.h"
#include "pio_serialiser_read.pio.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
// #define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_write(c, p, m) ((p) = (p & ~(_BV(m))) | ((!!(c)) << (m)));
#define bit_check(value, bit) (value & _BV(bit))

#define SETUP 0b10110100
#define DATA0 0b11000011
#define DATA1 0b11010010
#define OUT 0b10000111
#define IN 0b10010110
#define NAK 0b01011010
#define ACK 0b01001011

#define USB_DM_PIN 20
#define USB_DP_PIN 21
#define PRINTF_BINARY_PATTERN_INT8 "%c%c, %c%c, %c%c, %c%c, "
#define PRINTF_BYTE_TO_BINARY_INT8(i) \
    (((i)&0x01ll) ? '1' : '0'),       \
        (((i)&0x02ll) ? '1' : '0'),   \
        (((i)&0x04ll) ? '1' : '0'),   \
        (((i)&0x08ll) ? '1' : '0'),   \
        (((i)&0x10ll) ? '1' : '0'),   \
        (((i)&0x20ll) ? '1' : '0'),   \
        (((i)&0x40ll) ? '1' : '0'),   \
        (((i)&0x80ll) ? '1' : '0')
bool full_speed = false;
uint8_t buffer[100];
uint32_t id = 0;
bool lastJ = false;
float div = 10.41f;
static void writeBit(uint8_t dm, uint8_t dp) {
    // >>3 is equiv to /8 (2^3), %8 is equiv to & 7 (8-1)
    buffer[id >> 3] |= dm << (id & 7);
    id++;
    buffer[id >> 3] |= dp << (id & 7);
    id++;
}
static void J() {
    lastJ = true;
    // for full_speed=true, J is DM low, DP high
    // for full_speed=false, J is DM low, DP high
    writeBit(full_speed, !full_speed);
}
static void K() {
    lastJ = false;
    // for full_speed=true, K is DM high, DP low
    // for full_speed=false, K is DM high, DP low
    writeBit(!full_speed, full_speed);
}
static void SE0() {
    writeBit(0, 0);
}
static void sync() {
    K();
    J();
    K();
    J();
    K();
    J();
    K();
    K();
}
static void EOP() {
    SE0();
    SE0();
    J();
}
int oneCount = 0;
void sendData(uint8_t byte, int count) {
    for (int i = 0; i < count; i++) {
        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
        // 1 bit is transmitted by leaving the data lines as-is.
        if (bit_check(byte, i)) {
            // Zero
            if (lastJ) {
                K();
            } else {
                J();
            }
            oneCount = 0;
        } else {
            oneCount++;
            // Bit stuffing - if more than 6 one bits are set, then send an extra 0 bit
            if (oneCount == 7) {
                // Toggle lines
                if (lastJ) {
                    K();
                } else {
                    J();
                }
                oneCount = 0;
            }
            // One
            if (lastJ) {
                J();
            } else {
                K();
            }
        }
    }
}
void sendByte(uint8_t byte) {
    sendData(byte, 8);
}
void sendNibble(uint8_t byte) {
    sendData(byte, 4);
}
int dma_chan_read;
int dma_chan_write;
#define readLen 256
uint8_t buffer2[readLen] = {0};
void initPIO() {
    // Simultaneously read and wrote from pio0
    uint offsetWrite = pio_add_program(pio0, &pio_serialiser_program);
    pio_serialiser_program_init(pio0, 0, offsetWrite, USB_DM_PIN, div);
    uint offsetRead = pio_add_program(pio1, &pio_serialiser_read_program);
    pio_serialiser_read_program_init(pio1, 0, offsetRead, USB_DM_PIN, div);
    // pio_sm_set_enabled(pio0, 0, false);
    // pio_sm_set_enabled(pio1, 0, false);
    dma_chan_read = dma_claim_unused_channel(true);
    dma_chan_write = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan_read);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_write_increment(&c, true);
    channel_config_set_read_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO1_RX0);
    dma_channel_configure(
        dma_chan_read,
        &c,
        buffer2,  // Write address (only need to set this once)
        &pio1_hw->rxf[0],
        readLen / 32,
        false);

    c = dma_channel_get_default_config(dma_chan_write);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan_write,
        &c,
        &pio0_hw->txf[0],  // Write address (only need to set this once)
        buffer,
        (id / 32) + 1,
        false);
}
void RW() {
    // Put a scope against this and work out what its writing and if its malformed.
    printf("About to write:\n");
    printf("bits: %d\n", id);
    for (int i = 0; i < (id / 8) + 1; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer[i]));
    }
    printf("\n\n\n");
    dma_channel_set_read_addr(dma_chan_write, buffer, false);
    dma_channel_set_write_addr(dma_chan_read, buffer2, false);
    dma_channel_set_trans_count(dma_chan_write, (id / 32) + 1, true);
    dma_channel_set_trans_count(dma_chan_read, readLen / 32, true);
    pio_sm_set_consecutive_pindirs(pio0, 0, USB_DM_PIN, 2, true);
    pio_sm_set_enabled(pio0, 0, true);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_set_enabled(pio1, 0, true);
    pio_sm_set_consecutive_pindirs(pio1, 0, USB_DM_PIN, 2, false);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
    pio_sm_set_enabled(pio1, 0, false);
    for (int i = 0; i < readLen / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer2[i]));
    }
    printf("\n");
    id = 0;
    memset(buffer, 0, sizeof(buffer));
    memset(buffer2, 0, sizeof(buffer2));
}
void W() {
    // Put a scope against this and work out what its writing and if its malformed.
    // printf("About to write:\n");
    // printf("bits: %d\n", id);
    // for (int i = 0; i < (id / 8) + 1; i++) {
    //     if (i % 4 == 0) {
    //         printf("\n");
    //     }
    //     printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer[i]));
    // }
    // printf("\n\n\n");
    dma_channel_set_read_addr(dma_chan_write, buffer, false);
    dma_channel_set_trans_count(dma_chan_write, (id / 32) + 1, true);
    // dma_channel_set_trans_count(dma_chan_read, readLen / 32, true);
    // dma_channel_set_write_addr(dma_chan_read, buffer2, false);
    pio_sm_set_consecutive_pindirs(pio0, 0, USB_DM_PIN, 2, true);
    pio_sm_set_enabled(pio0, 0, true);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    pio_sm_set_enabled(pio0, 0, false);
    // pio_sm_set_enabled(pio1, 0, true);
    // pio_sm_set_consecutive_pindirs(pio1, 0, USB_DM_PIN, 2, false);
    // dma_channel_wait_for_finish_blocking(dma_chan_read);
    // pio_sm_set_enabled(pio1, 0, false);
    // for (int i = 0; i < readLen / 8; i++) {
    //     if (i % 4 == 0) {
    //         printf("\n");
    //     }
    //     printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer2[i]));
    // }
    // printf("\n");
    // id = 0;
    // memset(buffer, 0, sizeof(buffer));
    // memset(buffer2, 0, sizeof(buffer2));
}
/*------------- MAIN -------------*/
int main(void) {
    stdio_init_all();
    // gpio_init(USB_DM_PIN);
    // gpio_init(USB_DP_PIN);
    // while (1) {
    //     // D- pull up = Low, D+ pull up = full
    //     if (gpio_get(USB_DM_PIN)) {
            div = clock_get_hz(clk_sys) / ((1.5 * 1000000));
    //         break;
    //     } else if (gpio_get(USB_DP_PIN)) {
    //         div = clock_get_hz(clk_sys) / ((12 * 1000000));
    //         full_speed = true;
    //         break;
    //     }
    // }
    initPIO();
    printf("\nFull Speed: %d, clkdiv: %f, usb: %f\n", full_speed, div);
    sync();
    sendByte(SETUP);

    sendByte(0x00);

    sendNibble(0x0);

    sendByte(0x02);
    EOP();

    sync();
    sendByte(DATA0);

    sendByte(0x00);
    sendByte(0x05);
    sendByte(0x0C);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);

    sendByte(0x38);
    sendByte(0xEA);
    EOP();

    // RW();
     printf("About to write:\n");
    printf("bits: %d\n", id);
    for (int i = 0; i < (id / 8) + 1; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer[i]));
    }
    printf("\n\n\n");
    while (true) {
        W();
    }

    // sync();
    // sendByte(SETUP);

    // sendByte(0x0C);

    // sendNibble(0x0);

    // sendByte(0x0B);
    // EOP();

    // sync();
    // sendByte(DATA0);

    // sendByte(0x80);
    // sendByte(0x06);
    // sendByte(0x00);
    // sendByte(0x01);
    // sendByte(0x00);
    // sendByte(0x00);
    // sendByte(0x12);
    // sendByte(0x00);

    // sendByte(0xF4);
    // sendByte(0xE0);
    // EOP();
    // RW();

    while (1) {
    }
    return 0;
}
