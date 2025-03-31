#include <stdint.h>

#include "Arduino.h"
#include "io.h"
#include "shared_main.h"
#ifdef WT_DRUM_SPI_PORT
static long lastTick = 0;
static int missing = 0;
bool wt_drum_found = false;
void tickWtDrum() {
    // WT Drum format: 0xAA <remaining packet count> <midi packet>
    if (micros() - lastTick > 500) {
        lastTick = micros();
        WT_DRUM_CS_CLEAR();
        delayMicroseconds(50);
        // We send 0xAA, it is acknowledged with a 0xAA
        uint8_t resp = spi_transfer(WT_DRUM_SPI_PORT, 0xAA);
        delayMicroseconds(50);
        if (resp != 0xAA) {
            missing++;
            if (missing > 10) {
                wt_drum_found = false;
                missing = 0;
            }
            WT_DRUM_CS_SET();
            return;
        }
        wt_drum_found = true;
        // We send 0x55, we get the number of packets waiting in the queue
        resp = spi_transfer(WT_DRUM_SPI_PORT, 0x55);
        delayMicroseconds(50);
        if (!resp) {
            WT_DRUM_CS_SET();
            return;
        }
        uint8_t status = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
        delayMicroseconds(50);
        uint8_t type = (status & 0xf0);
        uint8_t channel = status & 0x0f;
        // TODO: CC and stuff, can the midi lib parse this all for us?
        if (type == 0x90) {
            uint8_t note = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
            delayMicroseconds(50);
            uint8_t velocity = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
            onNote(channel, note, velocity);
        } else if (type == 0x80) {
            uint8_t note = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
            delayMicroseconds(50);
            uint8_t velocity = spi_transfer(WT_DRUM_SPI_PORT, 0x00);
            offNote(channel, note, velocity);
        }
        delayMicroseconds(10);
        WT_DRUM_CS_SET();
    }
}
#endif