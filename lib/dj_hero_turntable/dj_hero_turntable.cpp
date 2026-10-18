#include "dj_hero_turntable.hpp"
#include "hardware/sync.h"
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
// How long to wait before asking a connected platter again after a NAK
#define DJH_RETRY_US 500
void DJHeroTurntable::tick()
{
    interface.tick();
    if (lastPoll && !restart_alarm_id && to_ms_since_boot(get_absolute_time()) - lastPoll > 1000)
    {
        // Masked so the I2C interrupt can't schedule a poll between this cancelling the old alarm and
        // storing the new one, which would leave an alarm that end() can't cancel
        uint32_t irq_status = save_and_disable_interrupts();
        process_data(0, false, false, false, false);
        restore_interrupts(irq_status);
    }
}
void DJHeroTurntable::begin()
{
    m_stopped = false;
    interface.dmaInit(address, this);
    process_data(0, false, false, false, false);
}
void DJHeroTurntable::end()
{
    // Stop polls scheduling more before cancelling the pending one, or one finishing in between
    // would leave an alarm that fires after this is freed
    m_stopped = true;
    interface.dmaDeinit(address);
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
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
    alarm_id_t id = add_alarm_in_us(delay_us, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}
void DJHeroTurntable::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
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
        failCount++;
        if (failCount > 10)
        {
            connected = false;
            clear();
        }
        // A one-off NAK shouldn't stall a connected platter, only back off once it's gone
        if (connected)
        {
            schedule(DJH_RETRY_US);
        }
        else
        {
            alarm_id_t id = add_alarm_in_ms(500, restart_handler, this, true);
            restart_alarm_id = (id > 0) ? id : 0;
        }
        return;
    }
    if (stop_detected)
    {
        connected = true;
        failCount = 0;
        velocity = (int8_t)bufferRx[2];
        green = bufferRx[0] & (1 << 4);
        red = bufferRx[0] & (1 << 5);
        blue = bufferRx[0] & (1 << 6);
        // the reading is movement since the last one, so space reads out by the poll interval
        schedule(pollIntervalUs);
        return;
    }
    // The platter has a "new data" status at 0x11, but like the GH5 neck there's no point checking it,
    // so read the buttons and platter straight from 0x12
    bufferTx[0] = DJ_BUTTONS_PTR;
    interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 3);
}
