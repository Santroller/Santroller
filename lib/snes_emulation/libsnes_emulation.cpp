#include "libsnes_emulation.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "snes_device.pio.h"

SnesEmulation::SnesEmulation(uint8_t clock_pin, uint8_t latch_pin, uint8_t data_pin, bool nes)
    : m_clock_pin(clock_pin), m_latch_pin(latch_pin), m_data_pin(data_pin), m_nes(nes)
{
}

SnesEmulation::~SnesEmulation()
{
    end();
}

void SnesEmulation::begin()
{
    if (m_listening)
    {
        return;
    }
    gpio_init(m_clock_pin);
    gpio_init(m_latch_pin);
    gpio_init(m_data_pin);
    // latch gets a pull-down so a disconnected cable can't look like the console polling
    gpio_set_pulls(m_latch_pin, false, true);
    gpio_set_pulls(m_clock_pin, true, false);
    gpio_acknowledge_irq(m_latch_pin, GPIO_IRQ_EDGE_RISE);
    m_seen_latch = false;
    m_listening = true;
}

void SnesEmulation::end()
{
    stop();
    m_listening = false;
    m_users = 0;
    m_release_pending = false;
    m_seen_latch = false;
}

bool SnesEmulation::start()
{
    if (m_pio)
    {
        return true;
    }
    if (!pio_claim_free_sm_and_add_program(&snes_device_program, &m_pio, &m_sm, &m_offset))
    {
        m_pio = nullptr;
        return false;
    }
    pio_gpio_init(m_pio, m_data_pin);
    pio_sm_set_consecutive_pindirs(m_pio, m_sm, m_data_pin, 1, true);
    pio_sm_set_consecutive_pindirs(m_pio, m_sm, m_clock_pin, 1, false);
    pio_sm_set_consecutive_pindirs(m_pio, m_sm, m_latch_pin, 1, false);
    pio_sm_config c = snes_device_program_get_default_config(m_offset);
    sm_config_set_in_pins(&c, m_clock_pin);
    sm_config_set_jmp_pin(&c, m_latch_pin);
    sm_config_set_out_pins(&c, m_data_pin, 1);
    sm_config_set_set_pins(&c, m_data_pin, 1);
    sm_config_set_out_shift(&c, true, false, m_nes ? 8 : 16);
    sm_config_set_in_shift(&c, false, false, 32);
    pio_sm_init(m_pio, m_sm, m_offset + snes_device_offset_idle, &c);
    // seed X with nothing pressed (active low), so the first latch isn't all buttons held
    pio_sm_put(m_pio, m_sm, (uint16_t)~m_buttons);
    pio_sm_set_enabled(m_pio, m_sm, true);
    return true;
}

void SnesEmulation::stop()
{
    if (!m_pio)
    {
        return;
    }
    pio_sm_set_enabled(m_pio, m_sm, false);
    pio_remove_program_and_unclaim_sm(&snes_device_program, m_pio, m_sm, m_offset);
    m_pio = nullptr;
    // release the data line so the port reads as empty
    gpio_init(m_data_pin);
}

void SnesEmulation::acquire()
{
    m_users++;
    m_release_pending = false;
    if (m_listening)
    {
        start();
    }
}

void SnesEmulation::release()
{
    if (m_users == 0)
    {
        return;
    }
    m_users--;
    if (m_users == 0)
    {
        m_release_pending = true;
        m_release_at_ms = to_ms_since_boot(get_absolute_time()) + SNES_RELEASE_GRACE_MS;
    }
}

void SnesEmulation::tick()
{
    if (!m_listening)
    {
        return;
    }
    uint32_t now = to_ms_since_boot(get_absolute_time());
    // Edge events latch in the raw INTR register even with the interrupt disabled (and with
    // the PIO owning the pin), so this catches every latch between ticks without an IRQ handler.
    uint32_t events = (io_bank0_hw->intr[m_latch_pin / 8] >> (4 * (m_latch_pin % 8))) & 0xF;
    if (events & GPIO_IRQ_EDGE_RISE)
    {
        gpio_acknowledge_irq(m_latch_pin, GPIO_IRQ_EDGE_RISE);
        m_last_latch_ms = now;
        m_seen_latch = true;
    }
    if (m_release_pending && (int32_t)(now - m_release_at_ms) >= 0)
    {
        // nothing picked the controller back up, so unplug it
        m_release_pending = false;
        stop();
    }
}

bool SnesEmulation::is_communicating() const
{
    return m_seen_latch && (int32_t)(to_ms_since_boot(get_absolute_time()) - m_last_latch_ms) < SNES_LISTEN_TIMEOUT_MS;
}

void SnesEmulation::set_buttons(uint16_t buttons)
{
    m_buttons = buttons;
    // The PIO drains the FIFO down to the newest entry between latches, so if it is full
    // the state it is about to use is at most a few updates old
    if (m_pio && !pio_sm_is_tx_fifo_full(m_pio, m_sm))
    {
        pio_sm_put(m_pio, m_sm, (uint16_t)~buttons);
    }
}
