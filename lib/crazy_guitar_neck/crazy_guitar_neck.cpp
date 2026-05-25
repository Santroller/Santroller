#include "crazy_guitar_neck.hpp"
#include "stdio.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    CrazyGuitarNeck *inst = (CrazyGuitarNeck *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}
void CrazyGuitarNeck::tick()
{
    interface.tick();
};

void CrazyGuitarNeck::begin()
{
    interface.dmaInit(CLONE_ADDR, this);
    status = CLONE_NECK_CHECK_STATUS;
    processData(0, false, false, false, false);
}
void CrazyGuitarNeck::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(CLONE_ADDR);
}
void CrazyGuitarNeck::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        status = CLONE_NECK_CHECK_STATUS;
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
        case CLONE_NECK_CHECK_STATUS:
            status = CLONE_NECK_READ_DATA;
            break;
        case CLONE_NECK_READ_DATA:
            reading = false;
            green = bufferRx[2] & 0x40;
            red = bufferRx[2] & 0x01;
            yellow = bufferRx[2] & 0x02;
            blue = bufferRx[2] & 0x10;
            orange = bufferRx[2] & 0x20;
            soloGreen = bufferRx[1] & 0x08;
            soloRed = bufferRx[1] & 0x04;
            soloYellow = bufferRx[1] & 0x02;
            soloBlue = bufferRx[1] & 0x01;
            soloOrange = bufferRx[2] & 0x80;
            break;
        }
        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case CLONE_NECK_CHECK_STATUS:
        bufferTx[0] = 0x53;
        bufferTx[1] = 0x10;
        bufferTx[2] = 0x00;
        bufferTx[3] = 0x01;
        interface.dmaWriteRead(CLONE_ADDR, bufferTx, 1, nullptr, 0);
        break;
    case CLONE_NECK_READ_DATA:
        interface.dmaWriteRead(CLONE_ADDR, nullptr, 0, bufferRx, 4);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}