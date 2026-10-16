#include "guitar_hero_5_neck.hpp"
#include "hardware/sync.h"
#include "stdio.h"
#include "utils.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    GuitarHero5Neck *inst = (GuitarHero5Neck *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}

void GuitarHero5Neck::tick()
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

void GuitarHero5Neck::begin()
{
    printf("GuitarHero5Neck begin\n");
    m_stopped = false;
    interface.dmaInit(GH5NECK_ADDR, this);
    failCount = 0;
    connected = false;
    clear_buttons();
    alarm_id_t id = add_alarm_in_ms(GH5_STARTUP_MS, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}

void GuitarHero5Neck::end()
{
    printf("GuitarHero5Neck end\n");
    // Stop polls scheduling more before cancelling the pending one, or one finishing in between
    // would leave an alarm that fires after this is freed
    m_stopped = true;
    interface.dmaDeinit(GH5NECK_ADDR);
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
    connected = false;
    clear_buttons();
}

void GuitarHero5Neck::clear_buttons()
{
    green = red = yellow = blue = orange = false;
    tapGreen = tapRed = tapYellow = tapBlue = tapOrange = false;
}

void GuitarHero5Neck::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
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
            clear_buttons();
        }
        alarm_id_t id = add_alarm_in_ms(500, restart_handler, this, true);
        restart_alarm_id = (id > 0) ? id : 0;
        return;
    }
    if (stop_detected)
    {
        connected = true;
        failCount = 0;
        green = bufferRx[0] & (1 << 4);
        red = bufferRx[0] & (1 << 5);
        yellow = bufferRx[0] & (1 << 7);
        blue = bufferRx[0] & (1 << 6);
        orange = bufferRx[0] & (1 << 0);
        tapGreen = bufferRx[3] & (1 << 4);
        tapRed = bufferRx[3] & (1 << 3);
        tapYellow = bufferRx[3] & (1 << 2);
        tapBlue = bufferRx[3] & (1 << 1);
        tapOrange = bufferRx[3] & (1 << 0);

        alarm_id_t id = add_alarm_in_us(GH5_POLL_INTERVAL_US, restart_handler, this, true);
        restart_alarm_id = (id > 0) ? id : 0;
        return;
    }

    bufferTx[0] = GH5NECK_BUTTONS_PTR;
    interface.dmaWriteRead(GH5NECK_ADDR, bufferTx, 1, bufferRx, 4);
}