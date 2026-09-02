#include "wii_extension_emulation.hpp"
#include "wii_extension_backend.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pico/i2c_slave.h>
static wii_extension_context_t context0;
static wii_extension_context_t context1;
static void init(wii_extension_context_t *context)
{
    uint8_t extension_id = context->type == GuitarHeroGuitar ? WII_EXTENSION_GUITAR :
                           context->type == GuitarHeroDrums ? WII_EXTENSION_DRUMS :
                           context->type == DjHeroTurntable ? WII_EXTENSION_TURNTABLE :
                           WII_EXTENSION_CLASSIC;
    wii_extension_backend_init(context->twi_reg, &context->encrypted, extension_id);
}
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
            uint8_t data = i2c_read_byte_raw(i2c);
            context->mem_address = data;
            context->mem_address_written = true;
            context->transfer_len = 0;
        }
        else
        {
            // save into memory
            uint8_t data = i2c_read_byte_raw(i2c);
            wii_extension_backend_write(context->twi_reg, &context->encrypted,
                                        &context->state, context->mem_address, data);
            // Euphoria LED
            if (context->mem_address == 0xFB)
            {
                context->djhEuphoriaLedState = data;
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
        uint8_t data = wii_extension_backend_read(context->twi_reg, context->encrypted,
                                                   &context->state, context->mem_address);
        i2c_write_byte_raw(i2c, data);
        context->mem_address++;
        context->transfer_len++;
        break;
    }
    case I2C_SLAVE_FINISH:
    {
        // master has signalled Stop / Restart
        if (context->transfer_len)
        {
            if (context->mem_address == 0x50)
            {
                // generate tables once all data is loaded
                wii_extension_backend_generate_tables(context->twi_reg, &context->state,
                                                       &context->encrypted);
            }
            context->mem_address_written = false;
        }
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
    printf("WiiExtensionEmulation begin %d %d %d\r\n", sda, scl, mBlock);
    if (mBlock == 0)
    {
        context = &context0;
    }
    else
    {
        context = &context1;
    }
    context->type = type;
    init(context);
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