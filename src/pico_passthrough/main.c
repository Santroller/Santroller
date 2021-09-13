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

#define USB_DP_PIN 20
#define USB_DM_PIN 21
#define USB_FIRST_PIN USB_DP_PIN
float div;

bool full_speed = false;
uint8_t buffer[100];
uint32_t id = 0;
int fifoSize = 32;
int fifoSizeIdx = DMA_SIZE_32;

bool lastJ = false;
static void writeBit(uint8_t dm, uint8_t dp) {
    // Give us the ability to swap wires if needed
    if (USB_FIRST_PIN == USB_DP_PIN) {
        buffer[id >> 3] |= dp << (id & 7);
        id++;
        buffer[id >> 3] |= dm << (id & 7);
        id++;
    } else {
        buffer[id >> 3] |= dm << (id & 7);
        id++;
        buffer[id >> 3] |= dp << (id & 7);
        id++;
    }
    // >>3 is equiv to /8 (2^3), %8 is equiv to & 7 (8-1)
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
    // Make sure we start on a J
    J();

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
    int remaining = (fifoSize - (id % fifoSize)) / 2;
    for (int i = 0; i < remaining; i++) {
        J();
    }
}

int oneCount = 0;
void sendData(uint8_t byte, int count, bool reverse) {
    for (int i = 0; i < count; i++) {
        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
        // 1 bit is transmitted by leaving the data lines as-is.
        if (!bit_check(byte, reverse ? (count - 1 - i) : i)) {
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
    sendData(byte, 8, false);
}
void sendNibble(uint8_t byte) {
    sendData(byte, 4, false);
}
void sendCRC5(uint8_t byte) {
    sendData(byte, 5, false);
}
void sendAddress(uint8_t byte) {
    sendData(byte, 7, false);
}
void sendControl(uint8_t byte) {
    sendData(byte, 8, true);
}
void setAddress1() {
    sync();
    sendControl(SETUP);

    sendAddress(0x00);

    sendNibble(0x0);

    sendCRC5(0x02);
    EOP();
}
void setAddress2() {
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

    sendByte(0xEA);
    sendByte(0x38);
    EOP();
}
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
#define readLen 4096
uint8_t buffer2[readLen] = {0};
int dma_chan_read;
int dma_chan_write;
uint sm, smRead;
PIO pio, pioRead;
void initPIO() {
    pioRead = pio1;
    dma_chan_read = dma_claim_unused_channel(true);
    dma_channel_config cr = dma_channel_get_default_config(dma_chan_read);
    channel_config_set_transfer_data_size(&cr, DMA_SIZE_32);
    channel_config_set_write_increment(&cr, true);
    channel_config_set_read_increment(&cr, false);
    channel_config_set_dreq(&cr, DREQ_PIO1_RX0);
    dma_channel_configure(
        dma_chan_read,
        &cr,
        buffer2,  // Write address (only need to set this once)
        &pioRead->rxf[0],
        readLen / 32,
        false);

    smRead = pio_claim_unused_sm(pioRead, true);
    uint offsetRead = pio_add_program(pioRead, &pio_serialiser_read_program);
    pio = pio0;

    pio_serialiser_read_program_init(pioRead, smRead, offsetRead, USB_FIRST_PIN, div);
    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember this location!

    uint offsetWrite = pio_add_program(pio, &pio_serialiser_program);

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    sm = pio_claim_unused_sm(pio, true);
    pio_serialiser_program_init(pio, sm, offsetWrite, USB_FIRST_PIN, div);

    dma_chan_write = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dma_chan_write);
    channel_config_set_transfer_data_size(&c, fifoSizeIdx);
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan_write,
        &c,
        &pio->txf[0],  // Write address (only need to set this once)
        buffer,
        0,
        false);
}
void WR() {
    printf("About to write:\n");
    printf("bits: %d\n", id);
    printf("transfers: %d\n", id / fifoSize);
    for (int i = 0; i < id / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer[i]));
    }

    dma_channel_set_read_addr(dma_chan_write, buffer, false);
    dma_channel_set_trans_count(dma_chan_write, id / fifoSize, true);
    pio_sm_set_consecutive_pindirs(pio, sm, USB_FIRST_PIN, 2, true);
    pio_sm_set_consecutive_pindirs(pioRead, smRead, USB_FIRST_PIN, 2, true);
    pio_sm_set_enabled(pio, sm, true);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    while (!pio_sm_is_tx_fifo_empty(pio, sm)) {
    }
    sleep_us(2);
    pio_sm_set_enabled(pio, sm, false);
    // while (true) {

    // }
    dma_channel_set_write_addr(dma_chan_read, buffer2, false);
    dma_channel_set_trans_count(dma_chan_read, readLen / 32, true);
    pio_sm_set_consecutive_pindirs(pioRead, smRead, USB_FIRST_PIN, 2, false);
    pio_sm_set_enabled(pioRead, smRead, true);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
    pio_sm_set_enabled(pioRead, smRead, false);
    printf("\n");
    for (int i = 0; i < readLen / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer2[i]));
    }
    printf("\n");
    printf("WR Done!");
    id = 0;
    memset(buffer, 0, sizeof(buffer));
    memset(buffer2, 0, sizeof(buffer2));
}
/*------------- MAIN -------------*/
int main(void) {
    stdio_init_all();
    div = clock_get_hz(clk_sys) / 1500000.0f;
    initPIO();
    printf("Address 0\n");
    setAddress1();
    WR();
    printf("Address 1\n");
    setAddress2();
    WR();

    while (true) {
    }
    return 0;
}