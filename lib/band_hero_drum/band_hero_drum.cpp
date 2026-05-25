#include "band_hero_drum.hpp"
#include "utils.h"

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    BandHeroDrum *inst = (BandHeroDrum *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}
void BandHeroDrum::tick()
{
    interface.tick();
}
void BandHeroDrum::begin()
{
    interface.dmaInit(DRUM_ADDR, this);
    status = BH_DRUM_CHECK_STATUS;
    processData(0, false, false, false, false);
}
void BandHeroDrum::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(DRUM_ADDR);
}
void BandHeroDrum::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        status = BH_DRUM_CHECK_STATUS;
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        failCount++;
        if (failCount > 10)
        {
            connected = false;
        }
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
                interface.dmaWriteRead(DRUM_ADDR, nullptr, 0, bufferRx, numPackets * PACKET_SIZE);
                return;
            }
            break;
        case BH_DRUM_READ_DATA:
            status = BH_DRUM_CHECK_STATUS;
            m_device->processMidiData(bufferRx, numPackets * PACKET_SIZE);
            break;
        }

        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case BH_DRUM_CHECK_STATUS:
        bufferTx[0] = BH_DRUM_PTR;
        interface.dmaWriteRead(DRUM_ADDR, bufferTx, 1, bufferRx, 2);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}