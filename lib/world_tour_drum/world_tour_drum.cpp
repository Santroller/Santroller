#include "world_tour_drum.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "hardware/sync.h"
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
// What the original firmware waited after each byte
#define WT_DRUM_BYTE_GAP_US 50
// Data bytes and anything this doesn't know the length of are read on their own
static uint8_t midi_message_length(uint8_t status)
{
    switch (status & 0xF0)
    {
    case 0x80:
    case 0x90:
    case 0xA0:
    case 0xB0:
    case 0xE0:
        return 3;
    case 0xC0:
    case 0xD0:
        return 2;
    default:
        return 1;
    }
}
WorldTourDrum::WorldTourDrum(MidiDevice *midiDevice, int8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, int8_t csPin)
    : mInterface(block, SPI_CPHA_1, SPI_CPOL_0, sck, mosi, miso, true, clock), mCsPin(csPin), m_device(midiDevice)
{   printf("wt drum: %d\r\n", csPin);
    if (csPin != -1 && sck != -1 && mosi != -1 && miso != -1)
    {
        // gpio_init leaves the output low. Deselect the brain before driving the pin, it may still be booting
        gpio_init(csPin);
        gpio_put(csPin, true);
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
    // Stop the alarm rescheduling itself before cancelling it, or it could fire in between and
    // leave a new one that runs after this is freed
    finished = true;
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
    if (mCsPin != -1)
    {
        gpio_put(mCsPin, true);
    }
    printf("wt end\r\n");
}
void WorldTourDrum::schedule(uint32_t delay_us)
{
    // A failed add leaves this 0, and tick() restarts polling
    alarm_id_t id = add_alarm_in_us(delay_us, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}
void WorldTourDrum::process_data()
{
    if (finished) {
        return;
    }
    // Whatever alarm got us here has fired
    restart_alarm_id = 0;
    lastPoll = to_ms_since_boot(get_absolute_time());
    if (status == WT_DRUM_REQUEST_STATUS)
    {
        gpio_put(mCsPin, false);
        status = WT_DRUM_CHECK_STATUS;
        schedule(WT_DRUM_BYTE_GAP_US);
        return;
    }
    // The brain needs a gap after every byte, so each one goes out from its own alarm
    if (status == WT_DRUM_CHECK_STATUS)
    {
        // 1: Send 0xAA, acknowledged with 0xAA
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
            schedule(500);
            return;
        }
        connected = true;
        missing = 0;
        status = WT_DRUM_READ_COUNT;
        schedule(WT_DRUM_BYTE_GAP_US);
        return;
    }
    if (status == WT_DRUM_READ_COUNT)
    {
        // 2: Send 0x55, response: packet count in buffer
        if (!mInterface.transfer(0x55))
        {
            gpio_put(mCsPin, true);
            status = WT_DRUM_REQUEST_STATUS;
            schedule(500);
            return;
        }
        m_pos = 0;
        m_len = 1;
        status = WT_DRUM_READ_DATA;
        schedule(WT_DRUM_BYTE_GAP_US);
        return;
    }
    if (status == WT_DRUM_READ_DATA)
    {
        // 3: Read one packet, a MIDI message sized by its status byte. Any left are picked up by the next polls
        uint8_t data = mInterface.transfer(0x00);
        if (m_pos == 0)
        {
            m_len = midi_message_length(data);
        }
        m_rxBuf[m_pos++] = data;
        if (m_pos < m_len)
        {
            schedule(WT_DRUM_BYTE_GAP_US);
            return;
        }
        if (m_len > 1 || data & 0x80)
        {
            m_device->process_midi_data(m_rxBuf, m_len);
        }
        // The original firmware held the brain selected a little longer after the last byte. Too short
        // to be worth another alarm
        busy_wait_us_32(10);
        gpio_put(mCsPin, true);
        status = WT_DRUM_REQUEST_STATUS;
        schedule(500);
        return;
    }
}
void WorldTourDrum::tick()
{
    // Scheduling the next poll failed. Masked so a poll can't run while this restarts from scratch
    uint32_t irq_status = save_and_disable_interrupts();
    if (!finished && !restart_alarm_id && lastPoll && to_ms_since_boot(get_absolute_time()) - lastPoll > 1000)
    {
        gpio_put(mCsPin, true);
        status = WT_DRUM_REQUEST_STATUS;
        process_data();
    }
    restore_interrupts(irq_status);
}
