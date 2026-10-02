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
    if (status == INIT_SOFT_RESET)
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

    status = INIT_SOFT_RESET;
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
    case CHECK_HANDSHAKE:
        resp = send_cmd(VTECH_REG_HANDSHAKE_READ, 0x00);
        // If init was successful, this final command responds with 0x5A
        if (resp == 0x5A)
        {
            status = POLL_INPUTS;
            connected = true;
        }
        else
        {
            status = INIT_SOFT_RESET;
            connected = false;
        }
        break;
    case POLL_INPUTS:
        button_data = send_cmd(VTECH_REG_POLL_INPUTS, 0x0E);
        status = UPDATE_LEDS;
        break;
    case UPDATE_LEDS:
        send_cmd(VTECH_REG_P0_OUT, led_data);
        status = CHECK_HANDSHAKE;
        break;
    case INIT_SOFT_RESET:
        send_cmd(VTECH_REG_SOFT_RESET, 0x00);
        status = INIT_UNLOCK_KEY1;
        break;
    case INIT_UNLOCK_KEY1:
        send_cmd(VTECH_REG_KEY1, 0xA5);
        status = INIT_UNLOCK_KEY2;
        break;
    case INIT_UNLOCK_KEY2:
        send_cmd(VTECH_REG_KEY2, 0x5A);
        status = INIT_SET_P0_DIR_IN;
        break;
    case INIT_SET_P0_DIR_IN:
        send_cmd(VTECH_REG_P0_DIR, 0xFF);
        status = INIT_SET_P0_PULLUP;
        break;
    case INIT_SET_P0_PULLUP:
        send_cmd(VTECH_REG_P0_PULLUP, 0xFF);
        status = INIT_SET_P1_DIR_IN;
        break;
    case INIT_SET_P1_DIR_IN:
        send_cmd(VTECH_REG_P1_DIR, 0xFF);
        status = INIT_SET_P0_OUT_CLEAR;
        break;
    case INIT_SET_P0_OUT_CLEAR:
        send_cmd(VTECH_REG_P0_OUT, 0x00);
        status = INIT_SET_P1_PULLUP;
        break;
    case INIT_SET_P1_PULLUP:
        send_cmd(VTECH_REG_P1_PULLUP, 0xFF);
        status = INIT_SET_P2_DIR_OUT;
        break;
    case INIT_SET_P2_DIR_OUT:
        send_cmd(VTECH_REG_P2_DIR, 0x00);
        status = INIT_SET_P1_OUT_OFF;
        break;
    case INIT_SET_P1_OUT_OFF:
        send_cmd(VTECH_REG_P1_OUT, 0xFF);
        status = CHECK_HANDSHAKE;
        break;
    }
}