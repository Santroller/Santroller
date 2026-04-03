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
    // 3: Stream in all data we receive straight to the midi parser
    uint8_t data;
    for (size_t i = 0; i < resp; i++) {
        data = mInterface->transfer(0x00);
        midiInterface.parsePacket(&data, 1);
        sleep_us(50);
    }
    gpio_put(mCsPin, true);
};