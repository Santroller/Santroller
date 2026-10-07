#include "libxbox360_rf.hpp"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "xbox360_rf.pio.h"

// Commands are 10 bits, MSB first
#define RF_FAT_LED_INIT 0x084
#define RF_SLIM_START 0x012
#define RF_BOOT_ANIM 0x085
#define RF_SYNC 0x004
#define RF_COMMAND_BITS 10
// The slim module needs this long after each falling edge before the data changes
#define RF_SLIM_HOLD_US 1000
// The module needs time to come up after power on before it accepts commands
#define RF_POWER_ON_DELAY_MS 1000

Xbox360Rf::Xbox360Rf(uint8_t data_pin, uint8_t clock_pin, bool slim) : m_data_pin(data_pin), m_clock_pin(clock_pin), m_slim(slim)
{
}

bool Xbox360Rf::begin()
{
    uint base = m_data_pin < m_clock_pin ? m_data_pin : m_clock_pin;
    uint count = (m_data_pin < m_clock_pin ? m_clock_pin - m_data_pin : m_data_pin - m_clock_pin) + 1;
    if (!pio_claim_free_sm_and_add_program_for_gpio_range(&xbox360_rf_program, &m_pio, &m_sm, &m_offset, base, count, true))
    {
        printf("xbox360 rf: no free PIO state machine\r\n");
        m_pio = nullptr;
        return false;
    }
    xbox360_rf_program_init(m_pio, m_sm, m_offset, m_data_pin, m_clock_pin, m_slim ? RF_SLIM_HOLD_US : 0);
    m_head = 0;
    m_count = 0;
    m_sending = false;
    m_next_at = to_ms_since_boot(get_absolute_time()) + RF_POWER_ON_DELAY_MS;
    send_init();
    return true;
}

void Xbox360Rf::end()
{
    if (!m_pio)
    {
        return;
    }
    pio_sm_set_enabled(m_pio, m_sm, false);
    pio_remove_program_and_unclaim_sm(&xbox360_rf_program, m_pio, m_sm, m_offset);
    gpio_init(m_data_pin);
    gpio_init(m_clock_pin);
    m_pio = nullptr;
}

void Xbox360Rf::send_init()
{
    queue(m_slim ? RF_SLIM_START : RF_FAT_LED_INIT, RF_COMMAND_BITS);
    queue(RF_BOOT_ANIM, RF_COMMAND_BITS);
}

void Xbox360Rf::send_sync()
{
    for (uint8_t i = 0; i < m_count; i++)
    {
        if (m_queue[(m_head + i) % QUEUE_SIZE].bits == RF_SYNC)
        {
            return;
        }
    }
    queue(RF_SYNC, RF_COMMAND_BITS);
}

void Xbox360Rf::queue(uint16_t bits, uint8_t count)
{
    if (m_count == QUEUE_SIZE)
    {
        return;
    }
    m_queue[(m_head + m_count) % QUEUE_SIZE] = {bits, count};
    m_count++;
}

bool Xbox360Rf::idle()
{
    // back at the first pull with nothing left to send
    return pio_sm_is_tx_fifo_empty(m_pio, m_sm) && pio_sm_get_pc(m_pio, m_sm) == m_offset;
}

void Xbox360Rf::reset_sm()
{
    pio_sm_set_enabled(m_pio, m_sm, false);
    pio_sm_clear_fifos(m_pio, m_sm);
    pio_sm_restart(m_pio, m_sm);
    pio_sm_exec(m_pio, m_sm, pio_encode_set(pio_pins, 1));
    pio_sm_exec(m_pio, m_sm, pio_encode_set(pio_pindirs, 0));
    // restore the hold time stashed in ISR
    pio_sm_put(m_pio, m_sm, m_slim ? RF_SLIM_HOLD_US : 0);
    pio_sm_exec(m_pio, m_sm, pio_encode_pull(false, true));
    pio_sm_exec(m_pio, m_sm, pio_encode_mov(pio_isr, pio_osr));
    pio_sm_exec(m_pio, m_sm, pio_encode_jmp(m_offset));
    pio_sm_set_enabled(m_pio, m_sm, true);
}

void Xbox360Rf::tick()
{
    if (!m_pio)
    {
        return;
    }
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (m_sending)
    {
        if (idle())
        {
            m_sending = false;
            m_next_at = now + COMMAND_GAP_MS;
        }
        else if (now - m_sent_at > COMMAND_TIMEOUT_MS)
        {
            printf("xbox360 rf: module never clocked the command, is it connected?\r\n");
            reset_sm();
            m_sending = false;
            m_next_at = now + COMMAND_GAP_MS;
        }
        return;
    }
    if (!m_count || (int32_t)(now - m_next_at) < 0)
    {
        return;
    }
    Command cmd = m_queue[m_head];
    m_head = (m_head + 1) % QUEUE_SIZE;
    m_count--;
    uint32_t bits = cmd.bits;
    uint8_t count = cmd.count;
    if (m_slim && cmd.bits == RF_SYNC)
    {
        // the slim module needs data held high for one more clock after sync
        bits = (bits << 1) | 1;
        count++;
    }
    pio_sm_put(m_pio, m_sm, count - 1);
    pio_sm_put(m_pio, m_sm, bits << (32 - count));
    m_sending = true;
    m_sent_at = now;
}
