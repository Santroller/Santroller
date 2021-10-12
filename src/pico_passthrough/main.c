#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pio_keepalive_high.pio.h"
#include "pio_keepalive_low.pio.h"
#include "pio_serialiser.pio.h"
#include "usb/std_descriptors.h"
#include "utils.h"

struct repeating_timer timer;
#define maxBits 512
uint32_t data_read[32];
uint32_t dma_data_read;
uint8_t buffer2[maxBits] = {0};
uint8_t buffer3[maxBits / 8] = {};
uint8_t buffer4[maxBits / 8] = {0};
int dma_chan_read;
int dma_chan_write;
int dma_chan_keepalive;
int max_packet_size;
uint sm;
PIO pio;
uint offset;
uint sm_keepalive;
uint offset_keepalive;
uint32_t currentFrame = 0;
uint32_t keepalive_delay = 0;
uint32_t jBits = 0;
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
typedef enum {
    TUSB_DIR_OUT = 0,
    TUSB_DIR_IN = 1,

    TUSB_DIR_IN_MASK = 0x80
} tusb_dir_t;

#define SETUP 0b10110100
#define DATA0 0b11000011
#define DATA1 0b11010010
#define OUT 0b10000111
#define IN 0b10010110
#define NAK 0b01011010
#define ACK 0b01001011
#define SOF 0b10100101

#define USB_DP_PIN 20
#define USB_DM_PIN 21
#define USB_FIRST_PIN 20
#if USB_DP_PIN == USB_FIRST_PIN
#define J_NUM 1
#define K_NUM 2
#else
#define J_NUM 2
#define K_NUM 1
#endif
#define USB_FIRST_PIN_READ 18
#define MAX_PACKET_LEN 40
#define MAX_PACKET_COUNT 10
float div;
bool full_speed = false;
volatile bool finishedKeepalive = false;
volatile bool waiting = false;
typedef struct {
    uint id;
    uint id_crc;
    uint oneCount;
    uint current_packet;
    uint8_t buffer[MAX_PACKET_LEN];
    uint8_t bufferCRC[MAX_PACKET_LEN];
    uint8_t packets[MAX_PACKET_COUNT][MAX_PACKET_LEN];
    uint8_t packetlens[MAX_PACKET_COUNT];
    uint8_t packetresplens[MAX_PACKET_COUNT];
    bool lastJ;
} state_t;
state_t state_ka;
state_t state_pk;
static void reset_state(state_t* state) {
    state->id_crc = 0;
    state->id = 0;
    state->oneCount = 0;
    memset(state->buffer, 0, sizeof(state->buffer));
    memset(state->bufferCRC, 0, sizeof(state->bufferCRC));
}

static void reset_crc(state_t* state) {
    memset(state->bufferCRC, 0, sizeof(state->bufferCRC));
    state->id_crc = 0;
}
static void writeBit(uint8_t dm, uint8_t dp, state_t* state) {
    // Give us the ability to swap wires if needed
    if (USB_FIRST_PIN == USB_DM_PIN) {
        state->buffer[state->id >> 3] |= dp << (state->id & 7);
        state->id++;
        state->buffer[state->id >> 3] |= dm << (state->id & 7);
        state->id++;
    } else {
        state->buffer[state->id >> 3] |= dm << (state->id & 7);
        state->id++;
        state->buffer[state->id >> 3] |= dp << (state->id & 7);
        state->id++;
    }
    // >>3 is equiv to /8 (2^3), %8 is equiv to & 7 (8-1)
}
static void J(state_t* state) {
    state->lastJ = true;
    // for full_speed=true, J is DM low, DP high
    // for full_speed=false, J is DM low, DP high
    writeBit(full_speed, !full_speed, state);
}
static void K(state_t* state) {
    state->lastJ = false;
    // for full_speed=true, K is DM high, DP low
    // for full_speed=false, K is DM high, DP low
    writeBit(!full_speed, full_speed, state);
}
static void SE0(state_t* state) {
    writeBit(0, 0, state);
}
static void SE1(state_t* state) {
    writeBit(1, 1, state);
}

static void commit_packet(state_t* state, uint readCount) {
    uint8_t bufferTmp[MAX_PACKET_LEN];
    int tmpId = state->id;
    memcpy(bufferTmp, state->buffer, (tmpId / 8) + 1);
    reset_state(state);
    // Left pad packets as the fifo is 8 bits in size, but we also want to be able to immediately receive
    int remaining = (8 - (tmpId % 8)) / 2;
    for (int i = 0; i < remaining; i++) {
        J(state);
    }
    int currentBit = 0;
    while (currentBit != tmpId) {
        bit_write(bit_check(bufferTmp[currentBit / 8], (currentBit % 8)), state->buffer[state->id / 8], (state->id % 8));
        currentBit++;
        state->id++;
    }
    // Store packet length as number of 8 bit transfers
    state->packetlens[state->current_packet] = state->id / 8;
    state->packetresplens[state->current_packet] = readCount / 8;
    // printf("Packet committed\n");
    // for (int i = 0; i < state->id; i += 2) {
    //     uint8_t bit1 = !!bit_check(state->buffer[i / 8], i % 8);
    //     uint8_t bit2 = !!bit_check(state->buffer[i / 8], (i + 1) % 8);
    //     uint8_t dm, dp;
    //     if (USB_FIRST_PIN == USB_DM_PIN) {
    //         dm = bit1;
    //         dp = bit2;
    //     } else {
    //         dp = bit1;
    //         dm = bit2;
    //     }
    //     bool j = dm == !full_speed && dp == full_speed;
    //     bool k = dm == full_speed && dp == !full_speed;
    //     bool se0 = !dm && !dp;

    //     if (k) {
    //         printf("K");
    //     } else if (j) {
    //         printf("J");
    //     } else if (se0) {
    //         printf("SE0");
    //     }
    // }
    // printf("\n");
    memcpy(state->packets[state->current_packet++], state->buffer, sizeof(state->buffer));
    reset_state(state);
}
static void sync(state_t* state) {
    K(state);
    J(state);
    K(state);
    J(state);
    K(state);
    J(state);
    K(state);
    K(state);
}
static void EOP(state_t* state) {
    SE0(state);
    SE0(state);
    J(state);
}
void sendData(uint16_t byte, int count, bool reverse, state_t* state) {
    for (int i = 0; i < count; i++) {
        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
        // 1 bit is transmitted by leaving the data lines as-is.
        if (!bit_check(byte, reverse ? (count - 1 - i) : i)) {
            // Zero
            if (state->lastJ) {
                K(state);
            } else {
                J(state);
            }
            state->oneCount = 0;
        } else {
            bit_set(state->bufferCRC[state->id_crc / 8], state->id_crc % 8);
            state->oneCount++;
            // Bit stuffing - if more than 6 one bits are set, then send an extra 0 bit, and then the 1
            if (state->oneCount == 5) {
                // Toggle lines
                if (state->lastJ) {
                    K(state);
                    K(state);
                } else {
                    J(state);
                    J(state);
                }
                state->oneCount = 0;
            } else {
                // One
                if (state->lastJ) {
                    J(state);
                } else {
                    K(state);
                }
            }
        }
        state->id_crc++;
    }
}
void sendByte(uint8_t byte, state_t* state) {
    sendData(byte, 8, false, state);
}
void sendNibble(uint8_t byte, state_t* state) {
    sendData(byte, 4, false, state);
}

void sendAddress(uint8_t byte, state_t* state) {
    sendData(byte, 7, false, state);
}
void sendPID(uint8_t byte, state_t* state) {
    sendData(byte, 8, true, state);
}

uint16_t crc_16(const void* data, size_t data_len) {
    const unsigned char* d = (const unsigned char*)data;
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

void sendCRC16(state_t* state) {
    sendData(crc_16(state->bufferCRC, state->id_crc / 8), 16, false, state);
}
void sendCRC5(state_t* state) {
    uint8_t byte;
    if (state->id_crc == 11) {
        byte = crc5_usb_11bit_input(state->bufferCRC);
    } else {
        byte = crc5_usb_19bit_input(state->bufferCRC);
    }
    sendData(byte, 5, false, state);
}
uint8_t reverse(uint8_t v) {
    unsigned int r = v;         // r will be reversed bits of v; first get LSB of v
    int s = sizeof(v) * 8 - 1;  // extra shift needed at end

    for (v >>= 1; v; v >>= 1) {
        r <<= 1;
        r |= v & 1;
        s--;
    }
    r <<= s;  // shift when v's highest bits are zero
    return r;
}
int readLongs;
void WR(state_t* state) {
    uint current_read = 0;
    finishedKeepalive = false;
    // Wait for a keepalive and schedule everything after it
    while (!finishedKeepalive) {
        tight_loop_contents();
    }
    // pio_sm_set_enabled(pio, sm_keepalive, false);
    for (int p = 0; p < state->current_packet; p++) {
        int syncCount = 0;
        int oneCount = 0;
        int bit = 0;
        bool readingData = false;
        bool valid = true;
        bool lastWasJ = true;
        bool skip = false;
        waiting = state->packetresplens[p];
        uint8_t currentLong = 0;
        readLongs = 0;
        dma_channel_transfer_from_buffer_now(dma_chan_write, state->packets[p], state->packetlens[p]);
        dma_channel_wait_for_finish_blocking(dma_chan_write);
        if (waiting) {
            while (waiting) {
                if (currentLong >= readLongs) {
                    tight_loop_contents();
                    continue;
                }
                for (int i = 0; i < 32; i += 2) {
                    bool bit1 = !!bit_check(data_read[currentLong], i);
                    bool bit2 = !!bit_check(data_read[currentLong], i + 1);
                    bool dm, dp;
                    if (USB_FIRST_PIN == USB_DM_PIN) {
                        dm = bit1;
                        dp = bit2;
                    } else {
                        dp = bit1;
                        dm = bit2;
                    }
                    // DM = 1 DP = 0 = J for full_speed
                    // for full_speed=true, J is DM low, DP high
                    // for full_speed=false, J is DM low, DP high
                    bool j = dm == !full_speed && dp == full_speed;
                    bool k = dm == full_speed && dp == !full_speed;
                    bool se0 = !dm && !dp;

                    // if (k) {
                    //     printf("K");
                    // } else if (j) {
                    //     printf("J");
                    // } else if (se0) {
                    //     printf("SE0\n");
                    // }
                    if (readingData) {
                        if (se0) {
                            waiting = false;
                            // printf("DONE\n");
                            break;
                        }
                        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
                        // 1 bit is transmitted by leaving the data lines as-is.
                        if (lastWasJ != j) {
                            if (!skip) {
                                bit_clear(buffer3[bit / 8], bit % 8);
                                bit++;
                            }
                            skip = false;
                            oneCount = 0;
                        } else {
                            bit_set(buffer3[bit / 8], bit % 8);
                            bit++;
                            skip = false;
                            oneCount++;
                            if (oneCount == 7) {
                                skip = true;
                                oneCount = 0;
                            }
                        }
                    } else {
                        if (j) {
                            // J
                            switch (syncCount) {
                                case 1:
                                case 3:
                                case 5:
                                    syncCount++;
                                    break;
                                default:
                                    valid = false;
                                    break;
                            }
                        } else if (k) {
                            // K
                            switch (syncCount) {
                                case 0:
                                case 2:
                                case 4:
                                case 6:
                                    syncCount++;
                                    break;
                                case 7:
                                    readingData = true;
                                    break;
                                default:
                                    valid = false;
                                    break;
                            }
                        }
                    }
                    lastWasJ = j;
                }
                currentLong++;
            }
            uint received = buffer3[0];
            printf("%d %d\n", p, reverse(received));
            if (received == reverse(NAK)) {
                // Device is not ready, try again.
                p--;
            } else if (received == reverse(DATA1) || received == reverse(DATA0)) {
                // Don't copy the PID
                memcpy(buffer4 + current_read, buffer3 + 1, sizeof(buffer3) - 1);
                current_read += 8;
            }
        }
    }
    reset_state(state);
    state->current_packet = 0;
}
void print(state_t* state) {
    for (int i = 0; i < 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(state->buffer[i]));
    }
    printf("\n");
}
void isr2() {
    finishedKeepalive = true;
    pio_interrupt_clear(pio, 1);
}
void isrRead() {
    // If the packet is only SE0's or J's, then its either unplugged, or it is not doing anything
    // There is a limit to how many J's can be read in a row, so this will work
    if (waiting && dma_data_read && dma_data_read != jBits) {
        // If we have a valid bit of data then write it
        data_read[readLongs++] = dma_data_read;
    }
    // dma_data_read = 0;
    dma_hw->ints0 = 1u << dma_chan_read;
    dma_channel_set_trans_count(dma_chan_read, 1, true);
}
void isr() {
    finishedKeepalive = true;
    // Increment current frame and then append its crc5
    currentFrame++;
    if (currentFrame > (1 << 11) - 1) {
        currentFrame = 0;
    }
    reset_state(&state_ka);
    sync(&state_ka);
    sendPID(SOF, &state_ka);
    reset_crc(&state_ka);
    sendData(currentFrame++, 11, false, &state_ka);
    sendCRC5(&state_ka);
    EOP(&state_ka);
    int remaining = (32 - (state_ka.id % 32)) / 2;
    for (int i = 0; i < remaining; i++) {
        J(&state_ka);
    }
    dma_channel_transfer_from_buffer_now(dma_chan_keepalive, state_ka.buffer, state_ka.id / 32);
    pio_interrupt_clear(pio, 1);
}

void initKeepAlive() {
    keepalive_delay = ((clock_get_hz(clk_sys) / div) / 10000);
    keepalive_delay *= 1.4f;
    if (full_speed) {
        dma_chan_keepalive = dma_claim_unused_channel(true);
        offset_keepalive = pio_add_program(pio, &pio_keepalive_high_program);
        pio_keepalive_high_program_init(pio, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
        irq_set_exclusive_handler(PIO0_IRQ_1, isr);
        dma_channel_config cka = dma_channel_get_default_config(dma_chan_keepalive);
        channel_config_set_transfer_data_size(&cka, DMA_SIZE_32);
        channel_config_set_write_increment(&cka, false);
        channel_config_set_read_increment(&cka, true);
        channel_config_set_dreq(&cka, pio_get_dreq(pio, sm_keepalive, true));
        dma_channel_configure(
            dma_chan_keepalive,
            &cka,
            &pio->txf[sm_keepalive],  // Write address (only need to set this once)
            state_ka.buffer,
            3,
            false);
        isr();
    } else {
        offset_keepalive = pio_add_program(pio, &pio_keepalive_low_program);
        pio_keepalive_low_program_init(pio, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
        irq_set_exclusive_handler(PIO0_IRQ_1, isr2);
    }
    pio_set_irq1_source_enabled(pio, pis_interrupt1, true);
    irq_set_priority(PIO0_IRQ_1, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(PIO0_IRQ_1, true);
}
void sendSetup(uint8_t address, uint8_t endpoint, tusb_control_request_t request, uint8_t* d) {
    memset(buffer4, 0, sizeof(buffer4));
    // printf("Token\n");
    sync(&state_pk);
    sendPID(SETUP, &state_pk);
    reset_crc(&state_pk);
    sendAddress(address, &state_pk);
    sendNibble(endpoint, &state_pk);
    sendCRC5(&state_pk);
    EOP(&state_pk);
    J(&state_pk);
    sync(&state_pk);
    sendPID(DATA0, &state_pk);
    reset_crc(&state_pk);
    uint8_t* data = (uint8_t*)&request;
    for (int i = 0; i < sizeof(tusb_control_request_t); i++) {
        sendByte(*data++, &state_pk);
    }
    sendCRC16(&state_pk);
    EOP(&state_pk);
    commit_packet(&state_pk, 16);
    if (request.bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if (request.wLength) {
            sync(&state_pk);
            sendPID(OUT, &state_pk);
            reset_crc(&state_pk);
            sendAddress(address, &state_pk);
            sendNibble(endpoint, &state_pk);
            sendCRC5(&state_pk);
            EOP(&state_pk);
            sync(&state_pk);
            sendPID(DATA1, &state_pk);
            reset_crc(&state_pk);
            uint8_t* data = (uint8_t*)&d;
            for (int i = 0; i < request.wLength; i++) {
                sendByte(*data++, &state_pk);
            }
            sendCRC16(&state_pk);
            EOP(&state_pk);
            commit_packet(&state_pk, 40);
        }
        sync(&state_pk);
        sendPID(IN, &state_pk);
        reset_crc(&state_pk);
        sendAddress(address, &state_pk);
        sendNibble(endpoint, &state_pk);
        sendCRC5(&state_pk);
        EOP(&state_pk);
        commit_packet(&state_pk, 16);
        sync(&state_pk);
        sendPID(ACK, &state_pk);
        EOP(&state_pk);
        commit_packet(&state_pk, 0);
    } else {
        if (request.wLength) {
            for (int i = 0; i < (request.wLength / 8) + 1; i++) {
                sync(&state_pk);
                sendPID(IN, &state_pk);
                reset_crc(&state_pk);
                sendAddress(address, &state_pk);
                sendNibble(endpoint, &state_pk);
                sendCRC5(&state_pk);
                EOP(&state_pk);
                commit_packet(&state_pk, request.wLength + 8);
                sync(&state_pk);
                sendPID(ACK, &state_pk);
                EOP(&state_pk);
                commit_packet(&state_pk, 0);
            }
        }
        sync(&state_pk);
        sendPID(OUT, &state_pk);
        reset_crc(&state_pk);
        sendAddress(address, &state_pk);
        sendNibble(endpoint, &state_pk);
        sendCRC5(&state_pk);
        EOP(&state_pk);
        sync(&state_pk);
        sendPID(DATA1, &state_pk);
        reset_crc(&state_pk);
        sendCRC16(&state_pk);
        EOP(&state_pk);
        commit_packet(&state_pk, 40);
    }
    WR(&state_pk);
    if (request.bmRequestType_bit.direction == TUSB_DIR_IN && request.wLength) {
        memcpy(d, buffer4, request.wLength);
    }
}

void initPIO() {
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    sm_keepalive = pio_claim_unused_sm(pio, true);

    offset = pio_add_program(pio, &pio_serialiser_program);

    dma_chan_write = dma_claim_unused_channel(true);
    dma_chan_read = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dma_chan_write);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));

    dma_channel_configure(
        dma_chan_write,
        &c,
        &pio->txf[sm],
        state_pk.buffer,
        0,
        false);

    dma_channel_config cr = dma_channel_get_default_config(dma_chan_read);
    channel_config_set_transfer_data_size(&cr, DMA_SIZE_32);
    channel_config_set_write_increment(&cr, false);
    channel_config_set_read_increment(&cr, false);
    channel_config_set_dreq(&cr, pio_get_dreq(pio, sm, false));
    dma_channel_configure(
        dma_chan_read,
        &cr,
        &dma_data_read,
        &pio->rxf[sm],
        1,
        false);
    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dma_chan_read, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, isrRead);
    // irq_set_priority(DMA_IRQ_0, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(DMA_IRQ_0, true);
    isrRead();
}
void reset() {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_set_enabled(pio, sm_keepalive, false);
    gpio_init(USB_DM_PIN);
    gpio_init(USB_DP_PIN);
    gpio_set_dir_out_masked(_BV(USB_DM_PIN) | _BV(USB_DP_PIN));
    gpio_put_masked(_BV(USB_DM_PIN) | _BV(USB_DP_PIN), 0);
    sleep_ms(1000);
    pio_serialiser_program_init(pio, sm, offset, USB_FIRST_PIN, div);
    if (full_speed) {
        pio_keepalive_high_program_init(pio, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
    } else {
        pio_keepalive_low_program_init(pio, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
    }
}
/*------------- MAIN -------------*/
int main(void) {
    set_sys_clock_khz(126000, true);
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
        if (gpio_get(USB_DM_PIN)) {
            div = (clock_get_hz(clk_sys) / ((1500000.0f))) / 7;
            full_speed = false;
            jBits = 0xaaaaaaaa;
            break;
        } else if (gpio_get(USB_DP_PIN)) {
            div = (clock_get_hz(clk_sys) / ((12000000.0f))) / 7;
            full_speed = true;
            jBits = 0x55555555;
            break;
        }
    }
    printf("Speed: %d %f %d\n", full_speed, div, clock_get_hz(clk_sys));
    initPIO();
    // Reset
    initKeepAlive();
    reset();

    sleep_ms(1000);
    // TUSB_Descriptor_Device_t deviceDescriptor;
    // tusb_control_request_t req = {.bmRequestType = {0x80}, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = 8};
    // sendSetup(0x00, 0x00, req, (uint8_t*)&deviceDescriptor);

    // for (int i = 0; i < sizeof(buffer4); i++) {
    //     printf("%x, ", buffer4[i]);
    // }
    // reset();
    // sleep_ms(1000);
    // printf("\nreq2\n");
    tusb_control_request_t req2 = {.bmRequestType = {0x00}, .bRequest = 0x05, .wValue = 13};
    sendSetup(0x00, 0x00, req2, NULL);
    // printf("req4\n");
    // sleep_ms(1000);
    // tusb_control_request_t req4 = {.bmRequestType = {0x80}, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = 0x0012};
    // sendSetup(13, 0x00, req4, (uint8_t*)&deviceDescriptor);
    // for (int i = 0; i < sizeof(buffer4); i++) {
    //     printf("%x, ", buffer4[i]);
    // }
    // printReceived();
    // printReceivedP();
    // printf("\nWR Done!\n");
    // sleep_ms(100);
    // printf("WR2 TX!\n");
    // printf("WR Done!\n");
    while (true) {
    }
    return 0;
}