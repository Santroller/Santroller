#include "usb_host.h"

#include <stdio.h>
#include <string.h>
#include <tusb.h>

#include "constants.h"
#include "crc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "lib_main.h"
#include "pico/stdlib.h"
#include "pio_keepalive_high.pio.h"
#include "pio_keepalive_low.pio.h"
#include "pio_serialiser.pio.h"
#include "usb.h"
#include "usb/std_descriptors.h"

#define maxBits 512
uint32_t data_read[128];
uint32_t dma_data_read;
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
uint offset_keepalive_high;
uint offset_keepalive_low;
uint32_t currentFrame = 0;
uint32_t keepalive_delay = 0;
uint32_t jBits = 0;
uint8_t maxPacket = 8;
uint8_t jNum;
uint8_t kNum;
volatile bool initialised = false;
TUSB_Descriptor_Device_t hostDescriptor;
typedef enum {
    STANDARD_ACK,
    NEXT_ACK_100,
    NEXT_ACK_DATA,
} PacketAction_t;

#define USB_DP_PIN 20
#define USB_DM_PIN 21
#define USB_FIRST_PIN 20

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
    uint8_t packet_actions[MAX_PACKET_COUNT];
    bool lastJ;
} state_t;
state_t state_ka;
state_t state_pk;
void reset_state(state_t* state) {
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

static void commit_packet(state_t* state, PacketAction_t actions, uint response_len) {
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
    state->packet_actions[state->current_packet] = actions;
    // Store packet length as number of 8 bit transfers (noting that we actually transmit two sets of 4 bits)
    state->packetresplens[state->current_packet] = (response_len / 4) + 3;
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
void sendData(uint16_t byte, int count, state_t* state) {
    for (int i = 0; i < count; i++) {
        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
        // 1 bit is transmitted by leaving the data lines as-is.
        if (!bit_check(byte, i)) {
            // Zero
            if (state->lastJ) {
                K(state);
            } else {
                J(state);
            }
            state->oneCount = 0;
        } else {
            bit_set(state->bufferCRC[state->id_crc / 8], state->id_crc % 8);
            // One
            if (state->lastJ) {
                J(state);
            } else {
                K(state);
            }
            state->oneCount++;
            // Bit stuffing - if 6 one bits are set, then send an extra 0 bit
            if (state->oneCount == 6) {
                // Toggle lines
                if (state->lastJ) {
                    K(state);
                } else {
                    J(state);
                }
                state->oneCount = 0;
            }
        }
        state->id_crc++;
    }
}
void sendByte(uint8_t byte, state_t* state) {
    sendData(byte, 8, state);
}
void sendNibble(uint8_t byte, state_t* state) {
    sendData(byte, 4, state);
}

void sendAddress(uint8_t byte, state_t* state) {
    sendData(byte, 7, state);
}
void sendPID(uint8_t byte, state_t* state) {
    sendByte(byte, state);
}

void sendCRC16(state_t* state) {
    sendData(crc_16(state->bufferCRC, state->id_crc / 8), 16, state);
}
void sendCRC5(state_t* state) {
    uint8_t byte;
    if (state->id_crc == 11) {
        byte = crc5_usb_11bit_input(state->bufferCRC);
    } else {
        byte = crc5_usb_19bit_input(state->bufferCRC);
    }
    sendData(byte, 5, state);
}
int readLongs;

#pragma GCC push_options
#pragma GCC optimize("-O3")
void WR(state_t* state) {
    uint current_read = 0;
    finishedKeepalive = false;
    // Wait for a keepalive and schedule everything after it
    while (!finishedKeepalive) {
        tight_loop_contents();
    }
    int ackCount = 100;
    bool wasData0 = true;
    for (int p = 0; p < state->current_packet;) {
        if (!initialised) return;
        int syncCount = 0;
        int oneCount = 0;
        int bit = 0;
        bool readingData = false;
        bool valid = true;
        bool lastWasJ = true;
        bool skip = false;
        PacketAction_t action = state->packet_actions[p];
        waiting = action == STANDARD_ACK;
        uint8_t currentLong = 0;
        readLongs = 0;
        buffer3[0] = 0;
        uint8_t wait_amt;
        // TODO: maxPacket can be either 8, 16, 32 or 64, so we still need to code in 16 and 32
        // I suspect for offical xbox controllers maxPacket will usually be 8
        // TODO: Low speed doesn't work, but im also not sure i care
        if (maxPacket == 64) {
            wait_amt = full_speed ? 3 : 70;
            if (action == NEXT_ACK_DATA) {
                wait_amt = full_speed ? 15 : 70;
            }
        } else if (maxPacket == 8) {
            wait_amt = full_speed ? 3 : 70;
            if (action == NEXT_ACK_DATA) {
                wait_amt = full_speed ? 5 : 70;
            }
        }
        uint read_pkt = state->packetresplens[p];
        dma_channel_transfer_from_buffer_now(dma_chan_write, state->packets[p], state->packetlens[p]);
        uint8_t t = pio_sm_get_rx_fifo_level(pio, sm);
        while (t--) {
            pio_sm_get_blocking(pio, sm);
        }
        uint32_t pid_nibble = 0;
        dma_channel_transfer_to_buffer_now(dma_chan_read, data_read, read_pkt);
        dma_channel_wait_for_finish_blocking(dma_chan_read);
        if (!waiting) {
            // TODO: currently, we are ACKing even if the data fails to read. This can cause us to go to the next packet even if we aren't ready for it
            // We need some way to read the PID here to work out if we were previously successful.
            // Interestingly, with O3, the parsing code is almost fast enough to run, but we need to make it only parse a PID and nothing else.
            dma_channel_transfer_from_buffer_now(dma_chan_write, state->packets[p + 1], state->packetlens[p + 1]);
            dma_channel_wait_for_finish_blocking(dma_chan_write);
            if (action == NEXT_ACK_100) {
                if (ackCount--) {
                    // sleep_ms(1);
                    continue;
                } else {
                    ackCount = 100;
                    // Skip the ACK
                    p += 2;
                    continue;
                }
            }
        }
        uint32_t test;
        uint8_t test2;
        bool timeout = false;
        uint32_t last = time_us_64();
        int bits = 0;
        for (int i2 = 0; i2 < read_pkt; i2++) {
            if (!initialised) return;
            test = data_read[i2] >> 24;
            // printf("\nNew:");
            for (int i = 0; i < 4; i++) {
                test2 = (test & 3);
                bool se0 = !test2;
                bool j = test2 == jNum;
                bool k = test2 == kNum;
                // if (k) {
                //     printf("k");
                // }
                // if (j) {
                //     printf("j");
                // }
                // if (se0) {
                //     printf("SE0");
                // }
                test >>= 2;
                // }
                // printf("\nOld:");
                // for (int i = 24; i < 32; i += 2) {
                bits++;
                // bool bit1 = !!bit_check(data_read[i2], i);
                // bool bit2 = !!bit_check(data_read[i2], i + 1);
                // bool dm, dp;
                // if (USB_FIRST_PIN == USB_DM_PIN) {
                //     dm = bit1;
                //     dp = bit2;
                // } else {
                //     dp = bit1;
                //     dm = bit2;
                // }
                // // DM = 1 DP = 0 = J for full_speed
                // // for full_speed=true, J is DM low, DP high
                // // for full_speed=false, J is DM low, DP high
                // bool j = dm == !full_speed && dp == full_speed;
                // bool k = dm == full_speed && dp == !full_speed;
                // bool se0 = !dm && !dp;
                // if (k) {
                //     printf("k");
                // }
                // if (j) {
                //     printf("j");
                // }
                // if (se0) {
                //     printf("SE0");
                // }
                if (readingData) {
                    if (se0) {
                        waiting = false;
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
                        if (oneCount == 6) {
                            skip = true;
                            oneCount = 0;
                        }
                    }
                } else if (j) {
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
                lastWasJ = j;
            }
            currentLong++;
        }
        // printf("\n");
        uint pid = buffer3[0];
        printf("%d %d %d\n", p, pid, wasData0);
        printf("%d\n", state->packetresplens[p]);
        if (pid == NAK || timeout) {
            // Device is not ready, try again.
            sleep_us(50);
            continue;
        } else if (pid == STALL) {
            // Stalled, return immediately.
            return;
        } else if (pid == DATA1 || pid == DATA0) {
            if (action != STANDARD_ACK) {
                // When reading, alternating packets will have different DATAx pids, so thats how we know we are on the next packet
                if ((pid == DATA1 && wasData0) || (pid == DATA0 && !wasData0)) {
                    // Don't copy the PID
                    memcpy(buffer4 + current_read, buffer3 + 1, maxPacket);
                    current_read += maxPacket;
                    wasData0 = !wasData0;
                    p += 2;  //Skip ACK packet as it has already been transmitted.
                    // sleep_ms(1);
                    // sleep_us(50);
                }
            } else {
                // For reading maxPacketLength, we actually just read the first 8 bytes and then do a reset, so we don't care about finishing up the read
                // Don't copy the PID
                memcpy(buffer4 + current_read, buffer3 + 1, sizeof(buffer3) - 1);
                current_read += maxPacket;
                p++;
            }
        } else if (action == STANDARD_ACK && pid == ACK) {
            p++;
        }
        sleep_ms(10);
    }

    reset_state(state);
    state->current_packet = 0;
}
#pragma GCC pop_options
// void isrRead() {
//     // If the packet is only SE0's or J's, then its either unplugged, or it is not doing anything
//     // There is a limit to how many J's can be read in a row, so this will work
//     if (waiting && dma_data_read && dma_data_read != jBits) {
//         // If we have a valid bit of data then write it
//         data_read[readLongs++] = dma_data_read;
//     }
//     if (!dma_data_read) {
//         pio_sm_set_enabled(pio, sm, false);
//         pio_sm_set_enabled(pio, sm_keepalive, false);
//         initialised = false;
//         dma_hw->ints0 = 1u << dma_chan_read;
//         return;
//     }
//     // dma_data_read = 0;
//     dma_hw->ints0 = 1u << dma_chan_read;
//     dma_channel_set_trans_count(dma_chan_read, 1, true);
// }
void isrKeepAlive() {
    if (!initialised) {
        return;
    }
    finishedKeepalive = true;
    if (full_speed) {
        // Increment current frame and then append its crc5
        currentFrame++;
        if (currentFrame > (1 << 11) - 1) {
            currentFrame = 0;
        }
        reset_state(&state_ka);
        sync(&state_ka);
        sendPID(SOF, &state_ka);
        reset_crc(&state_ka);
        sendData(currentFrame++, 11, &state_ka);
        sendCRC5(&state_ka);
        EOP(&state_ka);
        int remaining = (32 - (state_ka.id % 32)) / 2;
        for (int i = 0; i < remaining; i++) {
            J(&state_ka);
        }
        dma_channel_transfer_from_buffer_now(dma_chan_keepalive, state_ka.buffer, state_ka.id / 32);
    }
    pio_interrupt_clear(pio, 1);
}

void send_control_request(uint8_t address, uint8_t endpoint, const tusb_control_request_t request, bool terminateEarly, uint8_t* d) {
    memset(buffer4, 0, sizeof(buffer4));
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
    commit_packet(&state_pk, STANDARD_ACK, 19);
    if (request.bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if (request.wLength) {
            uint len = request.wLength / maxPacket;
            if (request.wLength % maxPacket) {
                len++;
            }
            bool data1 = true;
            uint8_t* data = (uint8_t*)d;
            for (int i = 0; i < len; i++) {
                sync(&state_pk);
                sendPID(OUT, &state_pk);
                reset_crc(&state_pk);
                sendAddress(address, &state_pk);
                sendNibble(endpoint, &state_pk);
                sendCRC5(&state_pk);
                EOP(&state_pk);
                J(&state_pk);
                J(&state_pk);
                J(&state_pk);
                sync(&state_pk);
                sendPID(data1 ? DATA1 : DATA0, &state_pk);
                reset_crc(&state_pk);
                for (int i2 = 0; i2 < maxPacket; i2++) {
                    if ((i * maxPacket) + i2 < request.wLength) {
                        sendByte(*data++, &state_pk);
                    }
                }
                sendCRC16(&state_pk);
                EOP(&state_pk);
                commit_packet(&state_pk, STANDARD_ACK, 19);
                data1 = !data1;
            }
        }
        sync(&state_pk);
        sendPID(IN, &state_pk);
        reset_crc(&state_pk);
        sendAddress(address, &state_pk);
        sendNibble(endpoint, &state_pk);
        sendCRC5(&state_pk);
        EOP(&state_pk);
        commit_packet(&state_pk, NEXT_ACK_100, 8 + 8 + 16 + 3);
        sync(&state_pk);
        sendPID(ACK, &state_pk);
        EOP(&state_pk);
        commit_packet(&state_pk, STANDARD_ACK, 0);
    } else {
        if (request.wLength) {
            uint len = request.wLength / maxPacket;
            if (request.wLength % maxPacket) {
                len++;
            }
            for (int i = 0; i < len; i++) {
                sync(&state_pk);
                sendPID(IN, &state_pk);
                reset_crc(&state_pk);
                sendAddress(address, &state_pk);
                sendNibble(endpoint, &state_pk);
                sendCRC5(&state_pk);
                EOP(&state_pk);
                // maxPacket is expressed in bytes not bits
                uint l = maxPacket * 8;
                if (i == len - 1) {
                    l = (request.wLength % maxPacket) * 8;
                }
                printf("Pkt: %d, l: %d, total: %d, len: %d\n", (i * 2) + 1, l, 3 + 8 + 8 + 16 + l, len);
                l = 3 + 8 + 8 + 16 + l;
                if (terminateEarly) {
                    commit_packet(&state_pk, STANDARD_ACK, l);
                } else {
                    commit_packet(&state_pk, NEXT_ACK_DATA, l);
                    sync(&state_pk);
                    sendPID(ACK, &state_pk);
                    EOP(&state_pk);
                    commit_packet(&state_pk, STANDARD_ACK, 0);
                }
            }
        }
        // If we are working out what size maxPacket is, we need to skip ACKs as we don't know how long to wait and are just gonna reset anyways
        if (!terminateEarly) {
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
            commit_packet(&state_pk, STANDARD_ACK, 19);
        }
    }
    WR(&state_pk);
    if (request.bmRequestType_bit.direction == TUSB_DIR_IN && request.wLength) {
        memcpy(d, buffer4, request.wLength);
    }
}
void sendOut(uint8_t address, uint8_t endpoint, uint len, uint8_t* d) {
    sync(&state_pk);
    sendPID(OUT, &state_pk);
    reset_crc(&state_pk);
    sendAddress(address, &state_pk);
    sendNibble(endpoint, &state_pk);
    sendCRC5(&state_pk);
    EOP(&state_pk);
    J(&state_pk);
    J(&state_pk);
    sync(&state_pk);
    sendPID(DATA0, &state_pk);
    reset_crc(&state_pk);
    uint8_t* data = (uint8_t*)d;
    for (int i = 0; i < len; i++) {
        sendByte(*data++, &state_pk);
    }
    for (int i = len; i < 8; i++) {
        sendByte(0, &state_pk);
    }
    sendCRC16(&state_pk);
    EOP(&state_pk);
    commit_packet(&state_pk, STANDARD_ACK, 19);
    WR(&state_pk);
}
void initKeepAlive() {
    keepalive_delay = ((clock_get_hz(clk_sys) / div) / 10000);
    keepalive_delay *= 1.4f;
    if (full_speed) {
        pio_keepalive_high_program_init(pio, sm_keepalive, offset_keepalive_high, USB_FIRST_PIN, div, keepalive_delay);
        isrKeepAlive();
    } else {
        pio_keepalive_low_program_init(pio, sm_keepalive, offset_keepalive_low, USB_FIRST_PIN, div, keepalive_delay);
    }
}
void initPIO() {
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    sm_keepalive = pio_claim_unused_sm(pio, true);

    offset = pio_add_program(pio, &pio_serialiser_program);

    dma_chan_write = dma_claim_unused_channel(true);
    dma_chan_read = dma_claim_unused_channel(true);
    dma_chan_keepalive = dma_claim_unused_channel(true);

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
    channel_config_set_write_increment(&cr, true);
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
    // dma_channel_set_irq0_enabled(dma_chan_read, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    // irq_set_exclusive_handler(DMA_IRQ_0, isrRead);
    // irq_set_priority(DMA_IRQ_0, PICO_HIGHEST_IRQ_PRIORITY);
    // irq_set_enabled(DMA_IRQ_0, true);

    irq_set_exclusive_handler(PIO0_IRQ_1, isrKeepAlive);
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
    pio_set_irq1_source_enabled(pio, pis_interrupt1, true);
    irq_set_priority(PIO0_IRQ_1, PICO_HIGHEST_IRQ_PRIORITY);
    irq_set_enabled(PIO0_IRQ_1, true);
    offset_keepalive_high = pio_add_program(pio, &pio_keepalive_high_program);
    offset_keepalive_low = pio_add_program(pio, &pio_keepalive_low_program);
}
void host_reset() {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_set_enabled(pio, sm_keepalive, false);
    gpio_init(USB_DM_PIN);
    gpio_init(USB_DP_PIN);
    gpio_set_dir_out_masked(_BV(USB_DM_PIN) | _BV(USB_DP_PIN));
    gpio_put_masked(_BV(USB_DM_PIN) | _BV(USB_DP_PIN), 0);
    sleep_ms(20);
    pio_serialiser_program_init(pio, sm, offset, USB_FIRST_PIN, div);
    if (full_speed) {
        pio_keepalive_high_program_init(pio, sm_keepalive, offset_keepalive_high, USB_FIRST_PIN, div, keepalive_delay);
    } else {
        pio_keepalive_low_program_init(pio, sm_keepalive, offset_keepalive_low, USB_FIRST_PIN, div, keepalive_delay);
    }
}
void initialise_device(void) {
    tusb_control_request_t req = {.bmRequestType = {0x80}, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = sizeof(hostDescriptor)};
    send_control_request(0x00, 0x00, req, true, (uint8_t*)&hostDescriptor);
    maxPacket = hostDescriptor.Endpoint0Size;
    printf("Max Packet: %d\n", maxPacket);
    host_reset();
    while (true) {
        tusb_control_request_t req = {.bmRequestType = {0x80}, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = sizeof(hostDescriptor)};
        send_control_request(0x00, 0x00, req, false, (uint8_t*)&hostDescriptor);
    }
    tusb_control_request_t req2 = {.bmRequestType = {0x00}, .bRequest = 0x05, .wValue = 13};
    send_control_request(0x00, 0x00, req2, false, NULL);
    tusb_control_request_t req3 = {.bmRequestType = {0x00}, .bRequest = 0x09, .wValue = 01};
    send_control_request(13, 0x00, req3, false, NULL);
    tusb_control_request_t req4 = {.bmRequestType = {0x80}, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = sizeof(hostDescriptor)};
    send_control_request(13, 0x00, req4, false, (uint8_t*)&hostDescriptor);
}
void set_xinput_led(int led) {
    uint8_t data2[] = {0x01, 0x03, led};
    sendOut(13, 0x02, sizeof(data2), data2);
}
bool is_xinput(void) {
    return hostDescriptor.Class == 0xFF && hostDescriptor.Protocol == 0xFF && hostDescriptor.SubClass == 0xFF;
}
void tick_usb_host(void) {
    if (!initialised) {
        gpio_init(USB_DM_PIN);
        gpio_init(USB_DP_PIN);
        if (!gpio_get(USB_DM_PIN) && !gpio_get(USB_DP_PIN)) {
            return;
        }
        sleep_ms(2);
        if (gpio_get(USB_DM_PIN)) {
            div = (clock_get_hz(clk_sys) / ((1500000.0f))) / 7;
            full_speed = false;
            // First 16 bits, JJJJ low speed
            jBits = 0xaa000000;
        } else if (gpio_get(USB_DP_PIN)) {
            div = (clock_get_hz(clk_sys) / ((12000000.0f))) / 7;
            full_speed = true;
            // First 16 bits, JJJJ full speed
            jBits = 0x55000000;
        }
        // for full_speed=true, J is DM low, DP high
        // for full_speed=false, J is DM high, DP low
        if (USB_FIRST_PIN == USB_DM_PIN) {
            if (full_speed) {
                jNum = 0b10;
                kNum = 0b01;
            } else {
                jNum = 0b01;
                kNum = 0b10;
            }
        } else {
            if (full_speed) {
                jNum = 0b01;
                kNum = 0b10;
            } else {
                jNum = 0b10;
                kNum = 0b01;
            }
        }
        initialised = true;
        printf("Speed: %d %f %d\n", full_speed, div, clock_get_hz(clk_sys));
        initKeepAlive();
        // Reset
        host_reset();

        initialise_device();
        if (is_xinput()) {
            set_xinput_led(0x0A);
        }
        tud_disconnect();
        tud_connect();
    }
}
void init_usb_host(void) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_set_enabled(pio, sm_keepalive, false);
    initPIO();
}
TUSB_Descriptor_Device_t getHostDescriptor(void) {
    return hostDescriptor;
}