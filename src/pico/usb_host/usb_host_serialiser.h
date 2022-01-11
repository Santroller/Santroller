#pragma once
#include "usb_host.h"

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

void sendData(uint16_t byte, int count, state_t* state, bool rev) {
    for (int i = 0; i < count; i++) {
        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
        // 1 bit is transmitted by leaving the data lines as-is.
        if (!bit_check(byte, rev ? count - 1 - i : i)) {
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
    sendData(byte, 8, state, false);
}
void sendNibble(uint8_t byte, state_t* state) {
    sendData(byte, 4, state, false);
}

void sendAddress(uint8_t byte, state_t* state) {
    sendData(byte, 7, state, false);
}
void sendPID(uint8_t byte, state_t* state) {
    sendData(byte, 8, state, false);
}

void sendCRC16(state_t* state) {
    sendData(crc_16(state->bufferCRC, state->id_crc / 8), 16, state, false);
}
void sendCRC5(state_t* state) {
    uint8_t byte;
    if (state->id_crc == 11) {
        byte = crc5_usb_11bit_input(state->bufferCRC);
    } else {
        byte = crc5_usb_19bit_input(state->bufferCRC);
    }
    sendData(byte, 5, state, false);
}

static void commit_packet(state_t* state, PacketAction_t actions, uint response_len) {
    uint8_t bufferTmp[MAX_PACKET_LEN];
    int tmpId = state->id;
    int remaining = (8 - (tmpId % 8)) / 2;
    if (response_len) {
        memcpy(bufferTmp, state->buffer, (tmpId / 8) + 1);
        reset_state(state);
        // Left pad packets as the fifo is 8 bits in size, but we also want to be able to immediately receive
        for (int i = 0; i < remaining; i++) {
            J(state);
        }
        int currentBit = 0;
        while (currentBit != tmpId) {
            bit_write(bit_check(bufferTmp[currentBit / 8], (currentBit % 8)), state->buffer[state->id / 8], (state->id % 8));
            currentBit++;
            state->id++;
        }
    } else {
        // Sending an ACK and not expecting a response, right pad.
        int remaining = (8 - (tmpId % 8)) / 2;
        for (int i = 0; i < remaining; i++) {
            J(state);
        }
    }
    // Store packet length as number of 8 bit transfers
    state->packetlens[state->current_packet] = state->id / 8;
    state->packet_actions[state->current_packet] = actions;
    // Store packet length as number of 8 bit transfers
    state->packetresplens[state->current_packet] = (response_len / 8);
    memcpy(state->packets[state->current_packet++], state->buffer, sizeof(state->buffer));
    reset_state(state);
}