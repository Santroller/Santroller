#include "band_hero_drum.hpp"
#include "hardware/sync.h"
#include "utils.h"

// The drum wants a gap between the status read and reading the packets it reported
#define BH_DRUM_DATA_DELAY_US 200
#define BH_DRUM_POLL_INTERVAL_US 500

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    BandHeroDrum *inst = (BandHeroDrum *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}
void BandHeroDrum::tick()
{
    interface.tick();
    if (lastPoll && !restart_alarm_id && to_ms_since_boot(get_absolute_time()) - lastPoll > 1000)
    {
        // Masked so the I2C interrupt can't schedule a poll between this cancelling the old alarm and
        // storing the new one, which would leave an alarm that end() can't cancel
        uint32_t irq_status = save_and_disable_interrupts();
        status = BH_DRUM_CHECK_STATUS;
        process_data(0, false, false, false, false);
        restore_interrupts(irq_status);
    }
}
void BandHeroDrum::begin()
{
    m_stopped = false;
    interface.dmaInit(DRUM_ADDR, this);
    status = BH_DRUM_CHECK_STATUS;
    process_data(DRUM_ADDR, false, false, false, false);
}
void BandHeroDrum::end()
{
    // Stop polls scheduling more before cancelling the pending one, or one finishing in between
    // would leave an alarm that fires after this is freed
    m_stopped = true;
    interface.dmaDeinit(DRUM_ADDR);
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
}
void BandHeroDrum::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (m_stopped)
    {
        return;
    }
    lastPoll = to_ms_since_boot(get_absolute_time());
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    if (timeout || abort_detected)
    {
        status = BH_DRUM_CHECK_STATUS;
        failCount++;
        if (failCount > 10)
        {
            connected = false;
        }
        // A one-off NAK shouldn't stall a connected drum, only back off once it's gone
        alarm_id_t id = connected ? add_alarm_in_us(BH_DRUM_POLL_INTERVAL_US, restart_handler, this, true)
                                  : add_alarm_in_ms(500, restart_handler, this, true);
        restart_alarm_id = (id > 0) ? id : 0;
        return;
    }
    if (stop_detected)
    {
        connected = true;
        failCount = 0;
        switch (status)
        {
        case BH_DRUM_CHECK_STATUS:
            numPackets = bufferRx[1] >> 4;
            if (numPackets)
            {
                status = BH_DRUM_READ_DATA;
                alarm_id_t id = add_alarm_in_us(BH_DRUM_DATA_DELAY_US, restart_handler, this, true);
                restart_alarm_id = (id > 0) ? id : 0;
                return;
            }
            break;
        case BH_DRUM_READ_DATA:
            m_device->process_midi_data(bufferRx, PACKET_SIZE);
            if (--numPackets)
            {
                interface.dmaWriteRead(DRUM_ADDR, nullptr, 0, bufferRx, PACKET_SIZE);
                return;
            }
            status = BH_DRUM_CHECK_STATUS;
            break;
        }

        alarm_id_t id = add_alarm_in_us(BH_DRUM_POLL_INTERVAL_US, restart_handler, this, true);
        restart_alarm_id = (id > 0) ? id : 0;
        return;
    }
    switch (status)
    {
    case BH_DRUM_CHECK_STATUS:
        bufferTx[0] = BH_DRUM_PTR;
        interface.dmaWriteRead(DRUM_ADDR, bufferTx, 1, bufferRx, 2);
        break;
    case BH_DRUM_READ_DATA:
        // continues on from the status, at the packets. Read them one at a time, the drums lock up
        // when more than one is read at once
        interface.dmaWriteRead(DRUM_ADDR, nullptr, 0, bufferRx, PACKET_SIZE);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}