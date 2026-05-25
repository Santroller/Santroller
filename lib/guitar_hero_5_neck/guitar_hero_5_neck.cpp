#include "guitar_hero_5_neck.hpp"
#include "stdio.h"
#include "utils.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    GuitarHero5Neck *inst = (GuitarHero5Neck *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}

void GuitarHero5Neck::tick()
{
    interface.tick();
};

void GuitarHero5Neck::begin()
{
    interface.dmaInit(GH5NECK_ADDR, this);
    status = GH5_NECK_CHECK_STATUS;
    processData(0, false, false, false, false);
}
void GuitarHero5Neck::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(GH5NECK_ADDR);
}
void GuitarHero5Neck::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        status = GH5_NECK_CHECK_STATUS;
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
        case GH5_NECK_CHECK_STATUS:
            if (bufferRx[1])
            {
                status = GH5_NECK_READ_DATA;
                interface.dmaWriteRead(GH5NECK_ADDR, nullptr, 0, bufferRx, 4);
                return;
            }
            break;
        case GH5_NECK_READ_DATA:
            status = GH5_NECK_CHECK_STATUS;
            green = bufferRx[0] & 1 << 4;
            red = bufferRx[0] & 1 << 5;
            yellow = bufferRx[0] & 1 << 6;
            blue = bufferRx[0] & 1 << 7;
            orange = bufferRx[0] & 1 << 0;
            tapGreen = bufferRx[3] & 1 << 4;
            tapRed = bufferRx[3] & 1 << 3;
            tapYellow = bufferRx[3] & 1 << 2;
            tapBlue = bufferRx[3] & 1 << 1;
            tapOrange = bufferRx[3] & 1 << 0;
            break;
        }

        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case GH5_NECK_CHECK_STATUS:
        bufferTx[0] = GH5NECK_BUTTONS_PTR;
        interface.dmaWriteRead(GH5NECK_ADDR, bufferTx, 1, bufferRx, 2);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}