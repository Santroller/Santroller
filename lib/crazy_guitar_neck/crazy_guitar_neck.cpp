#include "crazy_guitar_neck.hpp"
#include "stdio.h"
#include <string.h>
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    CrazyGuitarNeck *inst = (CrazyGuitarNeck *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}
void CrazyGuitarNeck::tick()
{
    interface.tick();
    if (lastPoll && to_ms_since_boot(get_absolute_time()) - lastPoll > 500)
    {
        process_data(CLONE_ADDR, false, false, false, false);
    }
};

// The necks don't like being polled too quickly, so leave this long between each transfer
#define CLONE_POLL_INTERVAL_US 4000
// and give them time to start up after power on, or after losing them
#define CLONE_STARTUP_MS 350
static const uint8_t clone_request[] = {0x53, 0x10, 0x00, 0x01};

void CrazyGuitarNeck::begin()
{
    interface.dmaInit(CLONE_ADDR, this);
    status = CLONE_NECK_CHECK_STATUS;
    restart_alarm_id = add_alarm_in_ms(CLONE_STARTUP_MS, restart_handler, this, true);
}
void CrazyGuitarNeck::end()
{
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    interface.dmaDeinit(CLONE_ADDR);
    connected = false;
    clear_buttons();
}
void CrazyGuitarNeck::clear_buttons()
{
    green = red = yellow = blue = orange = false;
    soloGreen = soloRed = soloYellow = soloBlue = soloOrange = false;
}
void CrazyGuitarNeck::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    lastPoll = to_ms_since_boot(get_absolute_time());
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    if (timeout || abort_detected)
    {
        status = CLONE_NECK_CHECK_STATUS;
        restart_alarm_id = add_alarm_in_ms(CLONE_STARTUP_MS, restart_handler, this, true);
        failCount++;
        if (failCount > 10)
        {
            connected = false;
            clear_buttons();
        }
        return;
    }
    if (stop_detected)
    {
        switch (status)
        {
        case CLONE_NECK_CHECK_STATUS:
            status = CLONE_NECK_READ_DATA;
            break;
        case CLONE_NECK_READ_DATA:
            // every read is preceded by the request
            status = CLONE_NECK_CHECK_STATUS;
            if (bufferRx[0] != CLONE_VALID_PACKET)
            {
                break;
            }
            connected = true;
            failCount = 0;
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
        restart_alarm_id = add_alarm_in_us(CLONE_POLL_INTERVAL_US, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case CLONE_NECK_CHECK_STATUS:
        memcpy(bufferTx, clone_request, sizeof(clone_request));
        interface.dmaWriteRead(CLONE_ADDR, bufferTx, sizeof(clone_request), nullptr, 0);
        break;
    case CLONE_NECK_READ_DATA:
        interface.dmaWriteRead(CLONE_ADDR, nullptr, 0, bufferRx, 4);
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}
