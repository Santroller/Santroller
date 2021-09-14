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
#define T_ATTR_PACKED __attribute__((packed))
typedef struct T_ATTR_PACKED {
    union {
        struct T_ATTR_PACKED {
            uint8_t recipient : 5;  ///< Recipient type tusb_request_recipient_t.
            uint8_t type : 2;       ///< Request type tusb_request_type_t.
            uint8_t direction : 1;  ///< Direction type. tusb_dir_t
        } bmRequestType_bit;

        uint8_t bmRequestType;
    };

    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} tusb_control_request_t;
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
#define USB_FIRST_PIN 20
float div;

bool full_speed = false;
uint8_t bufferTmp[100];
uint8_t buffer[100];
uint8_t bufferOut[100];
uint32_t id3 = 0;
uint32_t id = 0;

bool lastJ = false;
static void writeBit(uint8_t dm, uint8_t dp) {
    // Give us the ability to swap wires if needed
    if (USB_FIRST_PIN == USB_DM_PIN) {
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
    J();
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
    // Now that we know what we are writing, left pad so we can write to the 32 bit fifo buffer
    int tmpId = id;
    id = 0;
    memcpy(bufferTmp, buffer, (tmpId / 8) + 1);
    memset(buffer, 0, (tmpId / 8) + 1);
    int remaining = (32 - (tmpId % 32)) / 2;
    for (int i = 0; i < remaining; i++) {
        J();
    }
    int currentBit = 0;
    while (currentBit != tmpId) {
        bit_write(bit_check(bufferTmp[currentBit / 8], (currentBit % 8)), buffer[id / 8], (id % 8));
        currentBit++;
        id++;
    }
    memcpy(bufferOut + id3, buffer, (tmpId / 8) + 1);
    id3 += (tmpId / 8) + 1;
}
uint8_t packetBuffer[100];
uint8_t id2 = 0;
int oneCount = 0;
void sendData(uint16_t byte, int count, bool reverse) {
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
            bit_set(packetBuffer[id2 / 8], id2 % 8);
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
        id2++;
    }
}
void sendByte(uint8_t byte) {
    sendData(byte, 8, false);
}
void sendNibble(uint8_t byte) {
    sendData(byte, 4, false);
}

static uint8_t crc5_usb_bits(const void *data, int vl, uint8_t ival) {
    const unsigned char *d = (const unsigned char *)data;
    /* This function is based on code posted by John Sullivan to Wireshark-dev
     * mailing list on Jul 21, 2019.
     *
     * "One of the properties of LFSRs is that a 1 bit in the input toggles a
     *  completely predictable set of register bits *at any point in the
     *  future*. This isn't often useful for most CRC caculations on variable
     *  sized input, as the cost of working out which those bits are vastly
     *  outweighs most other methods."
     *
     * In USB 2.0, the CRC5 is calculated on either 11 or 19 bits inputs,
     * and thus this approach is viable.
     */
    uint8_t rv = ival;
    static const uint8_t bvals[19] = {
        0x1e, 0x15, 0x03, 0x06, 0x0c, 0x18, 0x19, 0x1b,
        0x1f, 0x17, 0x07, 0x0e, 0x1c, 0x11, 0x0b, 0x16,
        0x05, 0x0a, 0x14};

    for (int i = 0; i < vl; i++) {
        uint8_t t = i & 7;
        if (bit_check(*d, t)) {
            rv ^= bvals[19 - vl + i];
        }
        if (t == 7) {
            d++;
        }
    }
    return rv;
}

uint8_t crc5_usb_11bit_input(const void *input) {
    return crc5_usb_bits(input, 11, 0x02);
}

uint8_t crc5_usb_19bit_input(const void *input) {
    return crc5_usb_bits(input, 19, 0x1d);
}
uint16_t crc_update(const void *data, size_t data_len) {
    const unsigned char *d = (const unsigned char *)data;
    unsigned int i;
    bool bit;
    unsigned char c;
    uint16_t crc = 0xffff;
    while (data_len--) {
        c = *d++;
        for (i = 0x01; i & 0xff; i <<= 1) {
            bit = crc & 0x8000;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x8005;
            }
        }
        crc &= 0xffff;
    }

    uint16_t ret = crc & 0x01;
    for (i = 1; i < 16; i++) {
        crc >>= 1;
        ret = (ret << 1) | (crc & 0x01);
    }
    return ret ^ 0xffff;
}
void sendCRC5() {
    uint8_t byte;
    if (id2 == 11) {
        byte = crc5_usb_11bit_input(packetBuffer);
    } else {
        byte = crc5_usb_19bit_input(packetBuffer);
    }
    sendData(byte, 5, false);
}
void sendCRC16() {
    sendData(crc_update(packetBuffer, id2 / 8), 16, false);
}
void sendAddress(uint8_t byte) {
    sendData(byte, 7, false);
}
void sendPID(uint8_t byte) {
    sendData(byte, 8, true);
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

#define readLen 128
uint8_t buffer2[readLen] = {0};
int dma_chan_read;
int dma_chan_write;
uint sm, smRead;
PIO pio, pioRead;
static uint32_t SM_STALL_MASK = 1u << (PIO_FDEBUG_TXSTALL_LSB + 0);
void WR() {
    // printf("About to write:\n");
    // printf("bits: %d\n", id);
    // printf("transfers: %d\n", id / fifoSize);
    // for (int i = 0; i < id / 8; i++) {
    //     if (i % 4 == 0) {
    //         printf("\n");
    //     }
    //     printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer[i]));
    // }

    dma_channel_set_read_addr(dma_chan_write, bufferOut, false);
    dma_channel_set_trans_count(dma_chan_write, (id3 * 8) / 32, true);
    pio_sm_set_consecutive_pindirs(pio, sm, USB_FIRST_PIN, 2, true);
    pio_sm_set_consecutive_pindirs(pioRead, smRead, USB_FIRST_PIN, 2, true);
    pio_sm_set_enabled(pio, sm, true);
    dma_channel_wait_for_finish_blocking(dma_chan_write);
    // Wait until the PIO processor stalls (aka there is no data left to write)
    pio->fdebug = SM_STALL_MASK;
    while (!(pio->fdebug & SM_STALL_MASK)) {
    }
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_set_consecutive_pindirs(pio, sm, USB_FIRST_PIN, 2, false);
    dma_channel_set_write_addr(dma_chan_read, buffer2, false);
    dma_channel_set_trans_count(dma_chan_read, readLen / 32, true);
    pio_sm_set_consecutive_pindirs(pioRead, smRead, USB_FIRST_PIN, 2, false);
    pio_sm_set_enabled(pioRead, smRead, true);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
    pio_sm_set_enabled(pioRead, smRead, false);
    // printf("\n");
    // for (int i = 0; i < readLen / 8; i++) {
    //     if (i % 4 == 0) {
    //         printf("\n");
    //     }
    //     printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer2[i]));
    // }
    // printf("\n");
    // printf("WR Done!\n");
    id = 0;
    memset(buffer, 0, sizeof(buffer));
    memset(buffer2, 0, sizeof(buffer2));
}
void sendOut(uint8_t address, uint8_t endpoint) {
    sync();
    sendPID(IN);
    memset(packetBuffer, 0, sizeof(packetBuffer));
    id2 = 0;
    sendAddress(address);
    sendNibble(endpoint);
    sendCRC5();
    EOP();
    WR();
}
void sendIn(uint8_t address, uint8_t endpoint) {
    sync();
    sendPID(IN);
    memset(packetBuffer, 0, sizeof(packetBuffer));
    id2 = 0;
    sendAddress(address);
    sendNibble(endpoint);
    sendCRC5();
    EOP();
    WR();
}

void sendSetup(uint8_t address, uint8_t endpoint, tusb_control_request_t request) {
    sync();
    sendPID(SETUP);
    memset(packetBuffer, 0, sizeof(packetBuffer));
    id2 = 0;
    sendAddress(address);
    sendNibble(endpoint);
    sendCRC5();
    EOP();
    
    sync();
    sendPID(DATA0);
    memset(packetBuffer, 0, sizeof(packetBuffer));
    id2 = 0;
    uint8_t *data = (uint8_t *)&request;
    for (int i = 0; i < sizeof(tusb_control_request_t); i++) {
        sendByte(*data++);
    }
    sendCRC16();
    EOP();
    WR();
}
void sendRequestData(tusb_control_request_t request) {
    sync();
    sendPID(DATA0);
    memset(packetBuffer, 0, sizeof(packetBuffer));
    id2 = 0;
    uint8_t *data = (uint8_t *)&request;
    for (int i = 0; i < sizeof(tusb_control_request_t); i++) {
        sendByte(*data++);
    }
    sendCRC16();
    EOP();
    WR();
}
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
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
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
/*------------- MAIN -------------*/
int main(void) {
    // If its needed, 120mhz would give us perfectly even clock dividers.
    set_sys_clock_khz(120000, true);
    stdio_init_all();
    gpio_init(USB_DM_PIN);
    gpio_init(USB_DP_PIN);
    while (1) {
        // D- pull up = Low, D+ pull up = full
        if (gpio_get(USB_DM_PIN)) {
            break;
        } else if (gpio_get(USB_DP_PIN)) {
            break;
        }
    }
    sleep_ms(2);
    while (1) {
        // D- pull up = Low, D+ pull up = full
        // Could we set the processor clock so it evenly divides into usb
        if (gpio_get(USB_DM_PIN)) {
            div = clock_get_hz(clk_sys) / ((1500000.0f));
            full_speed = false;
            break;
        } else if (gpio_get(USB_DP_PIN)) {
            div = clock_get_hz(clk_sys) / ((12000000.0f));
            full_speed = true;
            break;
        }
    }
    printf("Speed: %d %f %d\n", full_speed, div, clock_get_hz(clk_sys));
    initPIO();
    pio_sm_set_pins(pio, sm, _BV(USB_DM_PIN));
    sleep_ms(10);
    tusb_control_request_t req = {.bmRequestType = {0x00}, .bRequest = 0x05, .wValue = 0x0C};
    sendSetup(0x00, 0x00, req);

    while (true) {
    }
    return 0;
}