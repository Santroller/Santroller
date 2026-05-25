#include "dj_hero_turntable.hpp"
#include <stdio.h>
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    DJHeroTurntable *inst = (DJHeroTurntable *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}
void DJHeroTurntable::tick()
{
    interface.tick();
}
void DJHeroTurntable::begin() {
    interface.dmaInit(address, this);
    status = DJH_CHECK_STATUS;
    processData(0, false, false, false, false);
}
void DJHeroTurntable::end() {
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(address);
}
void DJHeroTurntable::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        status = DJH_CHECK_STATUS;
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        failCount++;
        if (failCount > 10) {
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
        case DJH_CHECK_STATUS:
            if (bufferRx[1])
            {
                status = DJH_READ_DATA;
                interface.dmaWriteRead(address, nullptr, 0, bufferRx, 3);
                return;
            }
            break;
        case DJH_READ_DATA:
            status = DJH_CHECK_STATUS;
            velocity = (int8_t)bufferRx[2];
            green = bufferRx[0] & (1 << 4);
            red = bufferRx[0] & (1 << 5);
            blue = bufferRx[0] & (1 << 6);
            break;
        }
        // TODO: for DJH, we do need to make the configurable as the poll rate dictates the range
        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
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