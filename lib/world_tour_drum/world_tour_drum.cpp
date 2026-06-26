#include "world_tour_drum.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "utils.h"

WorldTourDrum::WorldTourDrum(MidiDevice *midiDevice, int8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t csPin)
    : mInterface(block, SPI_CPHA_1, SPI_CPOL_0, sck, mosi, miso, false, clock), mCsPin(csPin), m_device(midiDevice)
{
    gpio_init(csPin);
    gpio_set_dir(csPin, true);
    gpio_set_pulls(csPin, false, false);
};

void WorldTourDrum::tick()
{
    if (micros() - last > 500)
    {
        last = micros();
        gpio_put(mCsPin, false);
        sleep_us(50);
        // 1: Send 0xAA, resp 0xAA
        uint8_t resp = mInterface.transfer(0xAA);
        if (resp != 0xAA)
        {
            missing++;
            if (missing > 10)
            {
                connected = false;
                missing = 0;
            }
            gpio_put(mCsPin, true);
            return;
        }
        connected = true;
        // 2: Send 0x55, response: packet count in buffer
        resp = mInterface.transfer(0x55);
        sleep_us(50);
        if (!resp)
        {
            // no packets in buffer
            gpio_put(mCsPin, true);
            return;
        }
        // 3: Stream in all data we receive straight to the midi parser
        uint8_t data;
        for (size_t i = 0; i < resp; i++)
        {
            data = mInterface.transfer(0x00);
            m_device->processMidiData(&data, 1);
            sleep_us(50);
        }
        gpio_put(mCsPin, true);
    }
};