#include "world_tour_drum.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "utils.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    WorldTourDrum *inst = (WorldTourDrum *)user_data;
    if (inst)
    {
        inst->processData();
    }
    return 0;
}
WorldTourDrum::WorldTourDrum(MidiDevice *midiDevice, int8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, int8_t csPin)
    : mInterface(block, SPI_CPHA_1, SPI_CPOL_0, sck, mosi, miso, false, clock), mCsPin(csPin), m_device(midiDevice)
{   printf("wt drum: %d\r\n", csPin);
    if (csPin != -1 && sck != -1 && mosi != -1 && miso != -1)
    {
        gpio_init(csPin);
        gpio_set_dir(csPin, true);
        gpio_set_pulls(csPin, false, false);
    } else {
        printf("wt spi invalid, aborting\r\n");
        finished = true;
    }
};
void WorldTourDrum::begin()
{
    printf("wt begin! %d\r\n", finished);
    if (finished) {
        return;
    }
    processData();
}
void WorldTourDrum::end()
{
    cancel_alarm(restart_alarm_id);
    finished = true;
    printf("wt end\r\n");
}
void WorldTourDrum::processData()
{
    if (finished) {
        return;
    }
    if (status == WT_DRUM_REQUEST_STATUS)
    {
        gpio_put(mCsPin, false);
        status = WT_DRUM_REQUEST_STATUS;
        restart_alarm_id = add_alarm_in_us(50, restart_handler, this, true);
        return;
    }
    if (status == WT_DRUM_CHECK_STATUS)
    {
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
            restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
            status = WT_DRUM_REQUEST_STATUS;
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
        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        status = WT_DRUM_REQUEST_STATUS;
        return;
    }
}
void WorldTourDrum::tick()
{
};