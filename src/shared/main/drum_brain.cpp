#include "drum_brain.h"

#include <stdint.h>

#include "Arduino.h"
#include "config.h"
#include "io.h"
uint8_t drum_brain_velocities[DRUM_BRAIN_PAD_COUNT] = {0};
bool drum_brain_found = false;
#if defined(WT_DRUM_SPI_PORT) || defined(BH_DRUM_TWI_PORT)
// The drum brains send out midi, using these notes for each pad
#define DRUM_BRAIN_NOTE_KICK 36
#define DRUM_BRAIN_NOTE_RED 38
#define DRUM_BRAIN_NOTE_GREEN 45
#define DRUM_BRAIN_NOTE_YELLOW 46
#define DRUM_BRAIN_NOTE_BLUE 48
#define DRUM_BRAIN_NOTE_ORANGE 49
#define DRUM_BRAIN_NOTE_KICK2 100
static void drum_brain_midi(uint8_t status, uint8_t note, uint8_t velocity) {
    uint8_t type = status & 0xf0;
    if (type != 0x90 && type != 0x80) return;
    uint8_t pad;
    switch (note) {
        case DRUM_BRAIN_NOTE_KICK:
        case DRUM_BRAIN_NOTE_KICK2:
            pad = DRUM_BRAIN_KICK;
            break;
        case DRUM_BRAIN_NOTE_RED:
            pad = DRUM_BRAIN_RED;
            break;
        case DRUM_BRAIN_NOTE_YELLOW:
            pad = DRUM_BRAIN_YELLOW;
            break;
        case DRUM_BRAIN_NOTE_BLUE:
            pad = DRUM_BRAIN_BLUE;
            break;
        case DRUM_BRAIN_NOTE_ORANGE:
            pad = DRUM_BRAIN_ORANGE;
            break;
        case DRUM_BRAIN_NOTE_GREEN:
            pad = DRUM_BRAIN_GREEN;
            break;
        default:
            return;
    }
    // velocities are 7 bit, a note on with a velocity of 0 is a note off
    drum_brain_velocities[pad] = type == 0x90 ? velocity << 1 : 0;
}
#endif
#ifdef WT_DRUM_SPI_PORT
static unsigned long lastWtTick = 0;
static uint8_t wtMissing = 0;
void tick_wt_drum(void) {
    // WT Drum format: 0xAA <remaining packet count> <midi packet>
    if (micros() - lastWtTick < 500) return;
    lastWtTick = micros();
    WT_DRUM_CS_CLEAR();
    delayMicroseconds(50);
    // We send 0xAA, it is acknowledged with a 0xAA
    uint8_t resp = spi_transfer(WT_DRUM_SPI_PORT, 0xAA);
    delayMicroseconds(50);
    if (resp != 0xAA) {
        if (++wtMissing > 10) {
            drum_brain_found = false;
            wtMissing = 0;
        }
        WT_DRUM_CS_SET();
        return;
    }
    wtMissing = 0;
    drum_brain_found = true;
    // We send 0x55, we get the number of packets waiting in the queue
    resp = spi_transfer(WT_DRUM_SPI_PORT, 0x55);
    delayMicroseconds(50);
    if (resp) {
        uint8_t status = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
        delayMicroseconds(50);
        uint8_t type = status & 0xf0;
        if (type == 0x90 || type == 0x80) {
            uint8_t note = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
            delayMicroseconds(50);
            uint8_t velocity = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
            drum_brain_midi(status, note, velocity);
        }
        delayMicroseconds(10);
    }
    WT_DRUM_CS_SET();
}
#endif
#ifdef BH_DRUM_TWI_PORT
#define BH_DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
void tick_bh_drum(void) {
    // Drum packet starts with some counter, and then the number of packets in the buffer
    uint8_t header[2];
    uint8_t data[3];
    drum_brain_found = twi_readFromPointerRepeatedStart(BH_DRUM_TWI_PORT, BH_DRUM_ADDR, BH_DRUM_PTR, sizeof(header), header);
    if (!drum_brain_found) return;
    // Stream out the rest of the packets one at a time, as the drums lock up otherwise.
    uint8_t count = header[1] >> 4;
    for (uint8_t i = 0; i < count; i++) {
        if (!twi_readFrom(BH_DRUM_TWI_PORT, BH_DRUM_ADDR, data, sizeof(data), true)) {
            drum_brain_found = false;
            return;
        }
        drum_brain_midi(data[0], data[1], data[2]);
    }
}
#endif
