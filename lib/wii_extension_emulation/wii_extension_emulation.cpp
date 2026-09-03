#include "wii_extension_emulation.hpp"
#include "wii_extension_backend.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pico/i2c_slave.h>
static wii_extension_context_t context_0;
static wii_extension_context_t context_1;
static void init(wii_extension_context_t *context)
{
    uint8_t extension_id = context->type == GuitarHeroGuitar ? WII_EXTENSION_GUITAR :
                           context->type == GuitarHeroDrums ? WII_EXTENSION_DRUMS :
                           context->type == DjHeroTurntable ? WII_EXTENSION_TURNTABLE :
                           WII_EXTENSION_CLASSIC;
    wii_extension_backend_init(context->registers, &context->encrypted, extension_id);
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
            context->transfer_length = 0;
        }
        else
        {
            // save into memory
            uint8_t data = i2c_read_byte_raw(i2c);
            wii_extension_backend_write(context->registers, &context->encrypted,
                                        &context->state, context->mem_address, data);
            // Euphoria LED
            if (context->mem_address == 0xFB)
            {
                context->djh_euphoria_led_state = data;
            }

            context->transfer_length++;
            context->mem_address++;
        }
        break;
    }
    case I2C_SLAVE_REQUEST:
    {
        // master is requesting data
        // load from memory
        uint8_t data = wii_extension_backend_read(context->registers, context->encrypted,
                                                   &context->state, context->mem_address);
        i2c_write_byte_raw(i2c, data);
        context->mem_address++;
        context->transfer_length++;
        break;
    }
    case I2C_SLAVE_FINISH:
    {
        // master has signalled Stop / Restart
        if (context->transfer_length)
        {
            if (context->mem_address == 0x50)
            {
                // generate tables once all data is loaded
                wii_extension_backend_generate_tables(context->registers, &context->state,
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
    i2c_slave_handler(i2c, &context_0, event);
}
static void i2c_slave_handler1(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    i2c_slave_handler(i2c, &context_1, event);
}
void WiiExtensionEmulation::begin(SubType type)
{
    printf("WiiExtensionEmulation begin %d %d %d\r\n", m_sda, m_scl, m_block);
    if (m_block == 0)
    {
        m_context = &context_0;
    }
    else
    {
        m_context = &context_1;
    }
    m_context->type = type;
    init(m_context);
    gpio_init(m_sda);
    gpio_set_function(m_sda, GPIO_FUNC_I2C);
    gpio_pull_up(m_sda);

    gpio_init(m_scl);
    gpio_set_function(m_scl, GPIO_FUNC_I2C);
    gpio_pull_up(m_scl);
    if (m_block == 0)
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
WiiExtensionEmulation::WiiExtensionEmulation(uint8_t block, uint8_t sda, uint8_t scl) : m_block(block), m_sda(sda), m_scl(scl)
{
}
void WiiExtensionEmulation::set_inputs(uint8_t *inputs, uint8_t len)
{
    memcpy(m_context->registers, inputs, len);
}

uint8_t WiiExtensionEmulation::wii_data_format()
{
    return m_context->registers[0xFE];
}