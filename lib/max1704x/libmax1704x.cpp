#include "libmax1704x.hpp"
#include "stdio.h"
#include "hardware/sync.h"
#include "utils.h"
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
    // Polls are a second apart, so anything this long without one means a transfer never came back
    if (lastPoll && !restart_alarm_id && to_ms_since_boot(get_absolute_time()) - lastPoll > 3000)
    {
        // Masked so the I2C interrupt can't schedule a poll between process_data cancelling the old
        // alarm and storing the new one, which would leave an alarm that end() can't cancel
        uint32_t irq_status = save_and_disable_interrupts();
        process_data(MAX710X_I2C_ADDRESS, false, false, false, false);
        restore_interrupts(irq_status);
    }
}
void Max1704X::begin()
{
    m_stopped = false;
    interface.dmaInit(MAX710X_I2C_ADDRESS, this);
    status = MAX710X_DETECT;
    failCount = 0;
    process_data(MAX710X_I2C_ADDRESS, false, false, false, false);
}
void Max1704X::end()
{
    // Stop polls scheduling more before cancelling the pending one, or one finishing in between
    // would leave an alarm that fires after this is freed
    m_stopped = true;
    interface.dmaDeinit(MAX710X_I2C_ADDRESS);
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
    connected = false;
}
void Max1704X::schedule(uint32_t ms)
{
    alarm_id_t id = add_alarm_in_ms(ms, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}
void Max1704X::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
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
        // Ride out the occasional NAK, only drop it once it stops answering
        if (connected && ++failCount <= 10)
        {
            schedule(10);
            return;
        }
        // look for it again shortly
        connected = false;
        failCount = 0;
        status = MAX710X_DETECT;
        schedule(500);
        return;
    }
    if (stop_detected)
    {
        failCount = 0;
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
            unsigned percent = bufferRx[0] + (bufferRx[1] >= 128 ? 1 : 0);
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
        break;
    case MAX710X_POLL:
        bufferTx[0] = REGISTER_SOC;
        break;
    }
    if (!interface.dmaWriteRead(MAX710X_I2C_ADDRESS, bufferTx, 1, bufferRx, 2))
    {
        schedule(500);
    }
}
