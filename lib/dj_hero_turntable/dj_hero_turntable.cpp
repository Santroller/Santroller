#include "dj_hero_turntable.hpp"
#include <stdio.h>
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    DJHeroTurntable *inst = (DJHeroTurntable *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}
// How long to wait before asking the platter again when it had nothing new
#define DJH_STATUS_RETRY_US 500
void DJHeroTurntable::tick()
{
    interface.tick();
    if (lastPoll && to_ms_since_boot(get_absolute_time()) - lastPoll > 500)
    {
        process_data(address, false, false, false, false);
    }
}
void DJHeroTurntable::begin()
{
    interface.dmaInit(address, this);
    status = DJH_CHECK_STATUS;
    process_data(0, false, false, false, false);
}
void DJHeroTurntable::end()
{
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    interface.dmaDeinit(address);
    connected = false;
    clear();
}
void DJHeroTurntable::clear()
{
    velocity = 0;
    green = red = blue = false;
}
void DJHeroTurntable::schedule(uint32_t delay_us)
{
    restart_alarm_id = add_alarm_in_us(delay_us, restart_handler, this, true);
}
void DJHeroTurntable::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    lastPoll = to_ms_since_boot(get_absolute_time());
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    if (timeout || abort_detected)
    {
        status = DJH_CHECK_STATUS;
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        failCount++;
        if (failCount > 10)
        {
            connected = false;
            clear();
        }
        return;
    }
    if (stop_detected)
    {
        connected = true;
        failCount = 0;
        switch (status)
        {
        case DJH_CHECK_STATUS:
            // the platter says whether it has a new reading, which follows on from the status
            if (bufferRx[1])
            {
                status = DJH_READ_DATA;
                interface.dmaWriteRead(address, nullptr, 0, bufferRx, 3);
                return;
            }
            break;
        case DJH_READ_DATA:
        {
            status = DJH_CHECK_STATUS;
            velocity = (int8_t)bufferRx[2];
            green = bufferRx[0] & (1 << 4);
            red = bufferRx[0] & (1 << 5);
            blue = bufferRx[0] & (1 << 6);
            // the reading is movement since the last one, so space reads out by the poll interval
            schedule(pollIntervalUs);
            return;
        }
        }
        schedule(DJH_STATUS_RETRY_US);
        return;
    }
    switch (status)
    {
    case DJH_CHECK_STATUS:
        bufferTx[0] = DJ_BUTTONS_PTR;
        interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 2);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}
