#include "wii_extension_emulation.hpp"
#include "wii_extension_backend.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <hardware/sync.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pico/i2c_slave.h>
static wii_extension_context_t context_0;
static wii_extension_context_t context_1;
static uint8_t extension_id_for_subtype(SubType type)
{
    return type == GuitarHeroGuitar ? WII_EXTENSION_GUITAR :
           type == GuitarHeroDrums ? WII_EXTENSION_DRUMS :
           type == DjHeroTurntable ? WII_EXTENSION_TURNTABLE :
           type == Taiko ? WII_EXTENSION_TAIKO :
           WII_EXTENSION_CLASSIC;
}
static void init(wii_extension_context_t *context)
{
    wii_extension_backend_init(context->registers, &context->encrypted,
                               extension_id_for_subtype(context->type));
}
static void i2c_slave_handler(i2c_inst_t *i2c, wii_extension_context_t *context, i2c_slave_event_t event)
{
    if (event == I2C_SLAVE_RECEIVE || event == I2C_SLAVE_REQUEST)
    {
        context->last_activity_ms = to_ms_since_boot(get_absolute_time());
    }

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
    if (m_reconnecting)
    {
        // already mid-swap, just reconnect as the latest subtype
        m_reconnect_type = type;
        return;
    }
    if (m_context && m_context->type == type)
    {
        return;
    }
    if (m_context && m_detect >= 0)
    {
        // Simulate an unplug: drop detect and stop answering on I2C. When detect comes
        // back the Wiimote re-runs extension init and reads the new ID.
        end();
        m_reconnecting = true;
        m_reconnect_type = type;
        m_reconnect_at_ms = to_ms_since_boot(get_absolute_time()) + WII_SWAP_DISCONNECT_MS;
        return;
    }
    if (m_context)
    {
        uint8_t registers[256];
        bool encrypted = false;
        wii_extension_backend_init(registers, &encrypted, extension_id_for_subtype(type));

        // Leave I2C and the Wiimote's encryption session intact while changing
        // the advertised extension and calibration data.
        uint32_t irq_state = save_and_disable_interrupts();
        memcpy(&m_context->registers[0x20], &registers[0x20], 0x20);
        memcpy(&m_context->registers[0xFA], &registers[0xFA], 6);
        m_context->type = type;
        restore_interrupts(irq_state);
        return;
    }
    start(type);
}

void WiiExtensionEmulation::start(SubType type)
{
    if (m_block == 0)
    {
        m_context = &context_0;
    }
    else
    {
        m_context = &context_1;
    }
    m_context->type = type;
    m_context->mem_address_written = false;
    m_context->transfer_length = 0;
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
    if (m_detect >= 0)
    {
        // only raise detect once I2C is ready, as the Wiimote starts talking straight away
        gpio_put(m_detect, true);
    }
}

void WiiExtensionEmulation::acquire()
{
    m_users++;
    m_release_pending = false;
}

void WiiExtensionEmulation::release(SubType idle_type)
{
    if (m_users == 0)
    {
        return;
    }
    m_users--;
    if (m_users == 0)
    {
        m_release_pending = true;
        m_idle_type = idle_type;
        m_release_at_ms = to_ms_since_boot(get_absolute_time()) + WII_RELEASE_GRACE_MS;
    }
}

WiiExtensionEmulation::WiiExtensionEmulation(uint8_t block, uint8_t sda, uint8_t scl, int8_t detect) : m_block(block), m_sda(sda), m_scl(scl), m_detect(detect), m_context(nullptr)
{
    if (m_detect >= 0)
    {
        // no extension until begin()
        gpio_init(m_detect);
        gpio_set_dir(m_detect, GPIO_OUT);
        gpio_put(m_detect, false);
    }
}
WiiExtensionEmulation::~WiiExtensionEmulation()
{
    end();
}
void WiiExtensionEmulation::set_inputs(uint8_t *inputs, uint8_t len)
{
    if (!m_context)
    {
        return;
    }
    memcpy(m_context->registers, inputs, len);
}

uint8_t WiiExtensionEmulation::wii_data_format()
{
    if (!m_context)
    {
        return 0;
    }
    return m_context->registers[0xFE];
}

void WiiExtensionEmulation::end()
{
    m_reconnecting = false;
    m_release_pending = false;
    if (m_detect >= 0)
    {
        gpio_put(m_detect, false);
    }
    if (m_context)
    {
        i2c_slave_deinit(m_block == 0 ? i2c0 : i2c1);
        m_context = nullptr;
    }
}

void WiiExtensionEmulation::update()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (m_reconnecting && (int32_t)(now - m_reconnect_at_ms) >= 0)
    {
        m_reconnecting = false;
        start(m_reconnect_type);
        // the Wiimote is still there, so count it as communicating until it actually
        // goes quiet rather than flapping the activation trigger before its first read
        m_context->last_activity_ms = now;
    }
    if (m_release_pending && (int32_t)(now - m_release_at_ms) >= 0)
    {
        // nothing picked the extension back up, so go back to the idle subtype
        m_release_pending = false;
        begin(m_idle_type);
    }
}

bool WiiExtensionEmulation::is_communicating() const
{
    // Stay "communicating" through a deliberate swap disconnect, otherwise the Wii
    // profile's activation trigger drops out and nothing would bring us back.
    if (m_reconnecting)
    {
        return true;
    }
    if (!m_context)
    {
        return false;
    }
    // Read the IRQ-owned timestamp before now, and compare signed: an I2C transfer
    // landing in between can leave last ahead of now, and the unsigned difference
    // would wrap and report the Wiimote as gone.
    uint32_t last = m_context->last_activity_ms;
    if (last == 0)
    {
        return false;
    }
    uint32_t now = to_ms_since_boot(get_absolute_time());
    return (int32_t)(now - last) < 1000;
}