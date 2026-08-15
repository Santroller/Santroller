#include "wii_extension_emulation.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <pico/i2c_slave.h>
// calibration data
const unsigned char cal_data[32] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
const uint8_t id_guitar[6] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x03};
const uint8_t id_drum[6] = {0x01, 0x00, 0xA4, 0x20, 0x01, 0x03};
const uint8_t id_turntable[6] = {0x03, 0x00, 0xA4, 0x20, 0x01, 0x03};
const uint8_t id_classic[6] = {0x01, 0x00, 0xA4, 0x20, 0x01, 0x01};
static wii_extension_context_t context0;
static wii_extension_context_t context1;
static void i2c_slave_handler(i2c_inst_t *i2c, wii_extension_context_t *context, i2c_slave_event_t event)
{
    switch (event)
    {
    case I2C_SLAVE_RECEIVE:
    {
        // master has written some data
        if (!context->mem_address_written)
        {
            // writes always start with the memory address
            uint8_t by = i2c_read_byte_raw(i2c);
            context->mem_address = by;
            context->mem_address_written = true;
            context->transfer_len = 0;
        }
        else
        {
            // save into memory
            uint8_t by = i2c_read_byte_raw(i2c);
            if (context->twi_reg[0xF0] == 0xAA && context->mem_address != 0xF0) // if encryption is on
            {
                // decrypt
                ext_decrypt_bytes(&context->state, &by, 0, 1);
            }
            context->twi_reg[context->mem_address] = by;
            // Euphoria LED
            if (context->mem_address == 0xFB)
            {
                context->djhEuphoriaLedState = context->twi_reg[context->mem_address];
            }

            context->transfer_len++;
            context->mem_address++;
        }
        break;
    }
    case I2C_SLAVE_REQUEST:
    {
        // master is requesting data
        // load from memory
        uint8_t d = context->twi_reg[context->mem_address];
        if (context->twi_reg[0xF0] == 0xAA) // encryption is on
        {
            // encrypt
            ext_encrypt_bytes(&context->state, &d, 0, 1);
        }
        i2c_write_byte_raw(i2c, d);
        context->mem_address++;
        context->transfer_len++;
        break;
    }
    case I2C_SLAVE_FINISH:
    {
        // master has signalled Stop / Restart
        if (context->mem_address >= 0x4C && context->mem_address < 0x50)
        {
            if (context->mem_address + context->transfer_len == 0x50)
            {
                // generate tables once all data is loaded
                ext_generate_tables(&context->state, &context->twi_reg[0x40]);
            }
        }
        context->mem_address_written = false;
        break;
    }
    default:
        break;
    }
}
static void i2c_slave_handler0(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    i2c_slave_handler(i2c, &context0, event);
}
static void i2c_slave_handler1(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    i2c_slave_handler(i2c, &context1, event);
}
void WiiExtensionEmulation::begin(SubType type)
{
    if (mBlock == 0)
    {
        context = &context0;
    }
    else
    {
        context = &context1;
    }
    memset(context->twi_reg, 0, sizeof(context->twi_reg));
    context->twi_reg[0xF0] = 0; // disable encryption
    // set id
    for (unsigned int i = 0, j = 0xFA; i < 6; i++, j++)
    {
        switch (type)
        {
        case GuitarHeroGuitar:
            context->twi_reg[j] = id_guitar[i];
            break;
        case GuitarHeroDrums:
            context->twi_reg[j] = id_drum[i];
            break;
        case DjHeroTurntable:
            context->twi_reg[j] = id_turntable[i];
            break;
        default:
            context->twi_reg[j] = id_classic[i];
            break;
        }
    }

    // set calibration data
    for (unsigned int i = 0, j = 0x20; i < 6; i++, j++)
    {
        context->twi_reg[j] = cal_data[i];
    }
    gpio_init(sda);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_pull_up(sda);

    gpio_init(scl);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(scl);
    if (mBlock == 0)
    {
        i2c_init(i2c0, 100000);
        // configure I2C0 for slave mode
        i2c_slave_init(i2c0, WII_ADDR, &i2c_slave_handler0);
    }
    else
    {
        i2c_init(i2c1, 100000);
        // configure I2C1 for slave mode
        i2c_slave_init(i2c1, WII_ADDR, &i2c_slave_handler1);
    }
}
WiiExtensionEmulation::WiiExtensionEmulation(uint8_t block, uint8_t sda, uint8_t scl) : mBlock(block), sda(sda), scl(scl)
{
}
void WiiExtensionEmulation::setInputs(uint8_t *inputs, uint8_t len)
{
    memcpy(context->twi_reg, inputs, len);
}

uint8_t WiiExtensionEmulation::wii_data_format()
{
    return context->twi_reg[0xFE];
}