#include "libmax1704x.hpp"
#include "stdio.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    Max1704X *inst = (Max1704X *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}
bool Max1704X::init()
{
    return true;
}
void Max1704X::tick()
{
    interface.tick();
}
void Max1704X::begin()
{
    interface.dmaInit(MAX710X_I2C_ADDRESS, this);
    status = MAX710X_RESET;
    processData(0, false, false, false, false);
}
void Max1704X::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(MAX710X_I2C_ADDRESS);
}
void Max1704X::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        failCount++;
        if (failCount > 10)
        {
            status = MAX710X_RESET;
        }
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        return;
    }
    if (stop_detected)
    {
        failCount = 0;
        switch (status)
        {
        case MAX710X_RESET:
            status = MAX710X_CHECK_CONFIG;
            bufferTx[0] = REGISTER_CONFIG;
            interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 1, bufferRx, 2);
            break;
        case MAX710X_CHECK_CONFIG:
            if ((bufferRx[0] << 8 | bufferRx[1]) == 0x1C97)
            {
                // module needs 10ms after reset before it can be used
                status = MAX710X_QUICKSTART;
                restart_alarm_id = add_alarm_in_ms(10, restart_handler, this, true);
            }
            else
            {
                // config is wrong
                status = MAX710X_RESET;
                restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
            }
            return;
        case MAX710X_POLL:
            batteryLevel = bufferRx[0];
            break;
        }

        restart_alarm_id = add_alarm_in_us(500, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case MAX710X_POLL:
        bufferTx[0] = REGISTER_SOC;
        interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 1, bufferRx, 1);
    case MAX710X_RESET:
        bufferTx[0] = REGISTER_COMMAND;
        bufferTx[1] = RESET_COMMAND >> 8;
        bufferTx[2] = RESET_COMMAND & 0xFF;
        interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 3, nullptr, 0);
        break;
    case MAX710X_QUICKSTART:
        status = MAX710X_POLL;
        bufferTx[0] = REGISTER_MODE;
        bufferTx[1] = QUICKSTART_MODE >> 8;
        bufferTx[2] = QUICKSTART_MODE & 0xFF;
        interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 3, nullptr, 0);
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}