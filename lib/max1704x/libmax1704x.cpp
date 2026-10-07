#include "libmax1704x.hpp"
#include "stdio.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    Max1704X *inst = (Max1704X *)user_data;
    if (inst)
    {
        inst->process_data(MAX710X_I2C_ADDRESS, false, false, false, false);
    }
    return 0;
}
void Max1704X::tick()
{
    interface.tick();
}
void Max1704X::begin()
{
    interface.dmaInit(MAX710X_I2C_ADDRESS, this);
    status = MAX710X_DETECT;
    process_data(MAX710X_I2C_ADDRESS, false, false, false, false);
}
void Max1704X::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(MAX710X_I2C_ADDRESS);
}
void Max1704X::schedule(uint32_t ms)
{
    restart_alarm_id = add_alarm_in_ms(ms, restart_handler, this, true);
}
void Max1704X::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        // look for it again shortly
        connected = false;
        status = MAX710X_DETECT;
        schedule(500);
        return;
    }
    if (stop_detected)
    {
        switch (status)
        {
        case MAX710X_DETECT:
            // anything answering at this address is taken as the gauge
            connected = true;
            status = MAX710X_POLL;
            schedule(1);
            return;
        case MAX710X_POLL:
        {
            // the high byte of SOC is whole percent, the low byte 1/256ths
            uint8_t percent = bufferRx[0] + (bufferRx[1] >= 128 ? 1 : 0);
            batteryLevel = percent > 100 ? 100 : percent;
            // the gauge only updates its estimate every second or so
            schedule(1000);
            return;
        }
        }
        return;
    }
    // a scheduled step
    switch (status)
    {
    case MAX710X_DETECT:
        bufferTx[0] = REGISTER_VERSION;
        interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 1, bufferRx, 2);
        break;
    case MAX710X_POLL:
        bufferTx[0] = REGISTER_SOC;
        interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 1, bufferRx, 2);
        break;
    }
}
