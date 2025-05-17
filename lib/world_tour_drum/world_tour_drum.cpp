#include "world_tour_drum.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>

void WorldTourDrum::tick() {
    gpio_put(mCsPin, false);
    sleep_us(50);
    // 1: Send 0xAA, resp 0xAA
    uint8_t resp = mInterface->transfer(0xAA);
    if (resp != 0xAA) {
        missing++;
        if (missing > 10) {
            connected = false;
            missing = 0;
        }
        gpio_put(mCsPin, true);
        return;
    }
    connected = true;
    // 2: Send 0x55, response: packet count in buffer
    resp = mInterface->transfer(0x55);
    sleep_us(50);
    if (!resp) {
        // no packets in buffer
        gpio_put(mCsPin, true);
        return;
    }
    // 3: read the rest of the packet
    // TODO: check if we can handle packets longer than 3?
    uint8_t data[3];
    for (int i = 0; i < sizeof(data); i++) {
        data[i] = mInterface->transfer(0x00);
        sleep_us(50);
    }
    gpio_put(mCsPin, true);
};