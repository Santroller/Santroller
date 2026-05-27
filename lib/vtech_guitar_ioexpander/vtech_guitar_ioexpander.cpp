#include "vtech_guitar_ioexpander.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdio.h>

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    VTechGuitarIOExpander *inst = (VTechGuitarIOExpander *)user_data;
    inst->processData(false, true);
    return 0;
}
void VTechGuitarIOExpander::noAttention(void)
{
    attention = false;
    gpio_put(mCsPin, true);
    if (status == INIT_POWER_ON)
    {
        // first command needs a delay
        timeout_alarm_id = add_alarm_in_ms(30, restart_handler, this, true);
    }
    else
    {
        timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
    }
}
bool VTechGuitarIOExpander::read_button(uint8_t pin) {
    return button_data & (1 << pin);
}
void VTechGuitarIOExpander::signalAttention(void)
{
    attention = true;
    gpio_put(mCsPin, false);
    timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
}
void VTechGuitarIOExpander::tick() {
};
void VTechGuitarIOExpander::begin()
{
    status = INIT_POWER_ON;
    bool attention = false;
    processData(false, false);
};
void VTechGuitarIOExpander::end()
{
    cancel_alarm(timeout_alarm_id);
};
void VTechGuitarIOExpander::processData(bool ack, bool timeout)
{
    uint8_t resp;
    if (!attention)
    {
        signalAttention();
        return;
    }
    switch (status)
    {
    case CHECK:
        mInterface.transfer(0x00);
        resp = mInterface.transfer(0x00);
        noAttention();
        // If init was successful, this final command responds with 0x5A
        if (resp == 0x5A)
        {
            status = POLL;
        }
        else
        {
            status = INIT_POWER_ON;
        }
        break;
    case POLL:
        mInterface.transfer(0x0E);
        resp = mInterface.transfer(0x0E);
        noAttention();
        timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
        button_data = resp;
        status = UPDATE_LED;
        break;
    case UPDATE_LED:
        mInterface.transfer(0x81);
        mInterface.transfer(led_data);
        noAttention();
        timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
        status = CHECK;
        break;
    case INIT_POWER_ON:
        mInterface.transfer(0xFF);
        mInterface.transfer(0x00);
        noAttention();
        status = INIT_2;
        break;
    case INIT_2:
        mInterface.transfer(0x88);
        mInterface.transfer(0xA5);
        noAttention();
        status = INIT_3;
        break;
    case INIT_3:
        mInterface.transfer(0x80);
        mInterface.transfer(0x5A);
        noAttention();
        status = INIT_4;
        break;
    case INIT_4:
        mInterface.transfer(0x84);
        mInterface.transfer(0xFF);
        noAttention();
        status = INIT_5;
        break;
    case INIT_5:
        mInterface.transfer(0x89);
        mInterface.transfer(0xFF);
        noAttention();
        status = INIT_6;
        break;
    case INIT_6:
        mInterface.transfer(0x85);
        mInterface.transfer(0xFF);
        noAttention();
        status = INIT_7;
        break;
    case INIT_7:
        mInterface.transfer(0x81);
        mInterface.transfer(0x00);
        noAttention();
        status = INIT_8;
        break;
    case INIT_8:
        mInterface.transfer(0x8A);
        mInterface.transfer(0xFF);
        noAttention();
        status = INIT_9;
        break;
    case INIT_9:
        mInterface.transfer(0x86);
        mInterface.transfer(0x00);
        noAttention();
        status = INIT_10;
        break;
    case INIT_10:
        mInterface.transfer(0x82);
        mInterface.transfer(0xFF);
        noAttention();
        break;
    }
}