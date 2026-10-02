#include "vtech_guitar_ioexpander.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdio.h>

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    VTechGuitarIOExpander *inst = (VTechGuitarIOExpander *)user_data;
    inst->process_data(false, true);
    return 0;
}
void VTechGuitarIOExpander::no_attention(void)
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
        timeout_alarm_id = add_alarm_in_us(CS_DELAY, restart_handler, this, true);
    }
}
bool VTechGuitarIOExpander::read_button(uint8_t pin)
{
    return (~button_data) & (1 << pin);
}
void VTechGuitarIOExpander::signal_attention(void)
{
    attention = true;
    gpio_put(mCsPin, false);
    timeout_alarm_id = add_alarm_in_us(CS_DELAY, restart_handler, this, true);
}
void VTechGuitarIOExpander::tick() {
};
void VTechGuitarIOExpander::set_led(uint8_t i, uint8_t val)
{
    if (val)
    {
        led_data |= (1 << i);
    }
    else
    {
        led_data &= ~(1 << i);
    }
}
void VTechGuitarIOExpander::begin()
{

    status = INIT_POWER_ON;
    connected = false;
    attention = false;
    process_data(false, false);
};
VTechGuitarIOExpander::VTechGuitarIOExpander(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t csPin) : mInterface(block, SPI_CPHA_0, SPI_CPOL_0, sck, mosi, miso, true, clock), mCsPin(csPin)
{
    printf("vtech expander init!\r\n");
    gpio_init(csPin);
    gpio_set_dir(csPin, true);
}
void VTechGuitarIOExpander::end()
{
    cancel_alarm(timeout_alarm_id);
};
void VTechGuitarIOExpander::process_data(bool ack, bool timeout)
{
    uint8_t resp = 0;
    if (!attention)
    {
        signal_attention();
        return;
    }

    auto send_cmd = [this](uint8_t cmd, uint8_t data) -> uint8_t {
        uint8_t tx[2] = {cmd, data};
        uint8_t rx[2] = {0};
        mInterface.transfer(tx, rx, 2);
        no_attention();
        return rx[1];
    };

    switch (status)
    {
    case CHECK:
        resp = send_cmd(0x00, 0x00);
        // If init was successful, this final command responds with 0x5A
        if (resp == 0x5A)
        {
            status = POLL;
            connected = true;
        }
        else
        {
            status = INIT_POWER_ON;
            connected = false;
        }
        break;
    case POLL:
        button_data = send_cmd(0x0E, 0x0E);
        status = UPDATE_LED;
        break;
    case UPDATE_LED:
        send_cmd(0x81, led_data);
        status = CHECK;
        break;
    case INIT_POWER_ON:
        send_cmd(0xFF, 0x00);
        status = INIT_2;
        break;
    case INIT_2:
        send_cmd(0x88, 0xA5);
        status = INIT_3;
        break;
    case INIT_3:
        send_cmd(0x80, 0x5A);
        status = INIT_4;
        break;
    case INIT_4:
        send_cmd(0x84, 0xFF);
        status = INIT_5;
        break;
    case INIT_5:
        send_cmd(0x89, 0xFF);
        status = INIT_6;
        break;
    case INIT_6:
        send_cmd(0x85, 0xFF);
        status = INIT_7;
        break;
    case INIT_7:
        send_cmd(0x81, 0x00);
        status = INIT_8;
        break;
    case INIT_8:
        send_cmd(0x8A, 0xFF);
        status = INIT_9;
        break;
    case INIT_9:
        send_cmd(0x86, 0x00);
        status = INIT_10;
        break;
    case INIT_10:
        send_cmd(0x82, 0xFF);
        status = CHECK;
        break;
    }
}