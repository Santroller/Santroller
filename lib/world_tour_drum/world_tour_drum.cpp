#include "world_tour_drum.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "utils.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    WorldTourDrum *inst = (WorldTourDrum *)user_data;
    if (inst)
    {
        inst->process_data();
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
    process_data();
}
void WorldTourDrum::end()
{
    cancel_alarm(restart_alarm_id);
    mInterface.transfer_dma_abort();
    finished = true;
    printf("wt end\r\n");
}
void WorldTourDrum::process_data()
{
    if (finished) {
        return;
    }
    if (status == WT_DRUM_REQUEST_STATUS)
    {
        gpio_put(mCsPin, false);
        status = WT_DRUM_CHECK_STATUS;
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
            status = WT_DRUM_REQUEST_STATUS;
            restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
            return;
        }
        connected = true;
        // 2: Send 0x55, response: packet count in buffer
        resp = mInterface.transfer(0x55);
        if (!resp)
        {
            // no packets in buffer
            gpio_put(mCsPin, true);
            status = WT_DRUM_REQUEST_STATUS;
            restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
            return;
        }
        // 3: Stream in data with 50us inter-byte pacing using PWM-paced DMA
        m_bytesToRead = resp > sizeof(m_rxBuf) ? sizeof(m_rxBuf) : resp;
        if (mInterface.transfer_dma_start_paced(nullptr, m_rxBuf, m_bytesToRead, 50))
        {
            status = WT_DRUM_READ_DATA;
            restart_alarm_id = add_alarm_in_us(m_bytesToRead * 50 + 10, restart_handler, this, true);
            return;
        }
        else
        {
            gpio_put(mCsPin, true);
            status = WT_DRUM_REQUEST_STATUS;
            restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
            return;
        }
    }
    if (status == WT_DRUM_READ_DATA)
    {
        if (mInterface.transfer_dma_busy())
        {
            restart_alarm_id = add_alarm_in_us(50, restart_handler, this, true);
            return;
        }
        mInterface.transfer_dma_finish(m_rxBuf, m_bytesToRead);
        m_device->process_midi_data(m_rxBuf, m_bytesToRead);

        gpio_put(mCsPin, true);
        status = WT_DRUM_REQUEST_STATUS;
        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        return;
    }
}
void WorldTourDrum::tick()
{
};