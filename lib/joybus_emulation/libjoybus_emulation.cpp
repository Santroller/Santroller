#include "libjoybus_emulation.hpp"
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "joybus.pio.h"

#define MAX_INSTANCES (NUM_PIOS * NUM_PIO_STATE_MACHINES)
static JoybusEmulation *s_instances[MAX_INSTANCES];
static uint8_t s_handler_users[NUM_PIOS];

// Turn bytes into the (value, enable) bit pairs the PIO program shifts out, followed by
// the stop bit. Returns the number of words.
static uint8_t encode(const uint8_t *data, uint8_t len, uint32_t *result)
{
    uint8_t count = len / 2 + 1;
    memset(result, 0, count * sizeof(uint32_t));
    for (int i = 0; i < len; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            int bit = 2 * (8 * (i % 2) + j);
            result[i / 2] |= 1u << (bit + 1);
            result[i / 2] |= (uint32_t)(!!(data[i] & (0x80u >> j))) << bit;
        }
    }
    result[len / 2] |= 3u << (2 * (8 * (len % 2)));
    return count;
}

JoybusEmulation::JoybusEmulation(uint8_t pin, bool n64) : m_pin(pin), m_n64(n64)
{
    // GameCube reports its rumble motor is supported, N64 reports no controller pak
    static const uint8_t gc_probe[] = {0x09, 0x00, 0x03};
    static const uint8_t n64_probe[] = {0x05, 0x00, 0x02};
    m_probe_count = encode(n64 ? n64_probe : gc_probe, 3, m_probe_words);
    // Centered sticks and released triggers, which is what the console calibrates against
    static const uint8_t origin[] = {0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00};
    m_origin_count = encode(origin, sizeof(origin), m_origin_words);
    uint8_t report[GC_REPORT_SIZE] = {0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00};
    if (n64)
    {
        memset(report, 0, sizeof(report));
    }
    m_report_count = encode(report, n64 ? N64_REPORT_SIZE : GC_REPORT_SIZE, m_report_words);
}

JoybusEmulation::~JoybusEmulation()
{
    end();
}

bool JoybusEmulation::begin()
{
    if (m_started)
    {
        return true;
    }
    // The PS2 emulation owns PIO1's interrupts outright, so prefer the other blocks
    static const uint8_t pio_order[] = {0, 2, 1};
    for (uint8_t idx : pio_order)
    {
        if (idx >= NUM_PIOS)
        {
            continue;
        }
        PIO pio = pio_get_instance(idx);
        if (irq_get_exclusive_handler(pio_get_irq_num(pio, 0)) || !pio_can_add_program(pio, &joybus_program))
        {
            continue;
        }
        int sm = pio_claim_unused_sm(pio, false);
        if (sm < 0)
        {
            continue;
        }
        m_pio = pio;
        m_sm = sm;
        m_offset = pio_add_program(pio, &joybus_program);
        break;
    }
    if (!m_pio)
    {
        return false;
    }
    m_dma = dma_claim_unused_channel(false);
    if (m_dma < 0)
    {
        pio_remove_program(m_pio, &joybus_program, m_offset);
        pio_sm_unclaim(m_pio, m_sm);
        m_pio = nullptr;
        return false;
    }

    gpio_init(m_pin);
    gpio_set_dir(m_pin, GPIO_IN);
    gpio_pull_up(m_pin);
    pio_gpio_init(m_pio, m_pin);

    pio_sm_config config = joybus_program_get_default_config(m_offset);
    sm_config_set_in_pins(&config, m_pin);
    sm_config_set_jmp_pin(&config, m_pin);
    sm_config_set_out_pins(&config, m_pin, 1);
    sm_config_set_set_pins(&config, m_pin, 1);
    sm_config_set_clkdiv(&config, (float)clock_get_hz(clk_sys) / 25e6f);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_shift(&config, false, true, 8);
    pio_sm_init(m_pio, m_sm, m_offset + joybus_offset_inmode, &config);

    for (auto &instance : s_instances)
    {
        if (!instance)
        {
            instance = this;
            break;
        }
    }
    uint pio_idx = pio_get_index(m_pio);
    uint irq = pio_get_irq_num(m_pio, 0);
    if (s_handler_users[pio_idx]++ == 0)
    {
        irq_add_shared_handler(irq, irq_handler, PICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY);
        irq_set_enabled(irq, true);
    }
    m_command_len = 0;
    m_replied = false;
    m_started = true;
    pio_set_irqn_source_enabled(m_pio, 0, pio_get_rx_fifo_not_empty_interrupt_source(m_sm), true);
    pio_sm_set_enabled(m_pio, m_sm, true);
    return true;
}

void JoybusEmulation::end()
{
    if (!m_started)
    {
        return;
    }
    pio_set_irqn_source_enabled(m_pio, 0, pio_get_rx_fifo_not_empty_interrupt_source(m_sm), false);
    pio_sm_set_enabled(m_pio, m_sm, false);
    dma_channel_abort(m_dma);
    dma_channel_unclaim(m_dma);
    m_dma = -1;
    uint pio_idx = pio_get_index(m_pio);
    uint irq = pio_get_irq_num(m_pio, 0);
    if (--s_handler_users[pio_idx] == 0)
    {
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, irq_handler);
    }
    for (auto &instance : s_instances)
    {
        if (instance == this)
        {
            instance = nullptr;
        }
    }
    pio_remove_program(m_pio, &joybus_program, m_offset);
    pio_sm_unclaim(m_pio, m_sm);
    m_pio = nullptr;
    // Hand the pin back as an input so the port reads as empty
    gpio_init(m_pin);
    gpio_set_dir(m_pin, GPIO_IN);
    m_active = false;
    m_users = 0;
    m_release_pending = false;
    m_seen_command = false;
    m_started = false;
}

void JoybusEmulation::acquire()
{
    m_users++;
    m_release_pending = false;
    m_active = true;
}

void JoybusEmulation::release()
{
    if (m_users == 0)
    {
        return;
    }
    m_users--;
    if (m_users == 0)
    {
        m_release_pending = true;
        m_release_at_ms = to_ms_since_boot(get_absolute_time()) + JOYBUS_RELEASE_GRACE_MS;
    }
}

void JoybusEmulation::tick()
{
    if (m_release_pending && (int32_t)(to_ms_since_boot(get_absolute_time()) - m_release_at_ms) >= 0)
    {
        // nothing picked the controller back up, so stop answering the console
        m_release_pending = false;
        m_active = false;
        m_rumble = false;
    }
}

bool JoybusEmulation::is_communicating() const
{
    if (!m_seen_command)
    {
        return false;
    }
    uint32_t last = m_last_command_ms;
    return (int32_t)(to_ms_since_boot(get_absolute_time()) - last) < JOYBUS_LISTEN_TIMEOUT_MS;
}

void JoybusEmulation::set_report(const uint8_t *report)
{
    uint32_t words[5];
    uint8_t count = encode(report, m_n64 ? N64_REPORT_SIZE : GC_REPORT_SIZE, words);
    uint32_t irq_state = save_and_disable_interrupts();
    memcpy(m_report_words, words, sizeof(words));
    m_report_count = count;
    restore_interrupts(irq_state);
}

uint8_t JoybusEmulation::command_length(uint8_t command) const
{
    switch (command)
    {
    case 0x00: // probe
    case 0xFF: // reset
        return 1;
    }
    if (m_n64)
    {
        switch (command)
        {
        case 0x01: // poll
            return 1;
        case 0x02: // controller pak read
            return 3;
        case 0x03: // controller pak write
            return 35;
        }
        return 0;
    }
    switch (command)
    {
    case 0x40: // poll
    case 0x42: // calibrate
        return 3;
    case 0x41: // origin
        return 1;
    }
    return 0;
}

void JoybusEmulation::irq_handler()
{
    for (auto *instance : s_instances)
    {
        if (instance)
        {
            instance->handle_rx();
        }
    }
}

// The PIO pushes each byte of a command, then 0xFFFFFFFF once the line has gone idle
#define JOYBUS_END_OF_COMMAND 0xFFFFFFFF

void __time_critical_func(JoybusEmulation::handle_rx)()
{
    while (!pio_sm_is_rx_fifo_empty(m_pio, m_sm))
    {
        uint32_t data = pio_sm_get(m_pio, m_sm);
        if (data == JOYBUS_END_OF_COMMAND)
        {
            if (m_command_len)
            {
                m_last_command_ms = to_ms_since_boot(get_absolute_time());
                m_seen_command = true;
            }
            // The PIO waits for a reply after every command, so tell it when there isn't one
            if (!m_replied)
            {
                pio_sm_put(m_pio, m_sm, 0);
            }
            m_command_len = 0;
            m_replied = false;
            continue;
        }
        if (m_command_len >= sizeof(m_command))
        {
            continue;
        }
        m_command[m_command_len++] = data;
        // Queue the reply as soon as the command is complete, so it is ready to go out as
        // soon as the PIO sees the stop bit finish
        if (!m_replied && m_command_len == command_length(m_command[0]))
        {
            handle_command();
        }
    }
}

void __time_critical_func(JoybusEmulation::handle_command)()
{
    if (!m_active)
    {
        return;
    }
    uint8_t command = m_command[0];
    switch (command)
    {
    case 0x00:
    case 0xFF:
        reply(m_probe_words, m_probe_count);
        return;
    }
    if (m_n64)
    {
        // No controller pak, so pak reads and writes go unanswered
        if (command == 0x01)
        {
            memcpy(m_tx_words, m_report_words, m_report_count * sizeof(uint32_t));
            reply(m_tx_words, m_report_count);
        }
        return;
    }
    switch (command)
    {
    case 0x41:
    case 0x42:
        reply(m_origin_words, m_origin_count);
        return;
    case 0x40:
        // the third byte holds the rumble motor state (1 = on, 2 = brake)
        m_rumble = (m_command[2] & 0x03) == 0x01;
        memcpy(m_tx_words, m_report_words, m_report_count * sizeof(uint32_t));
        reply(m_tx_words, m_report_count);
        return;
    }
}

void __time_critical_func(JoybusEmulation::reply)(const uint32_t *words, uint8_t count)
{
    // The previous reply went out before the PIO started listening again, so the DMA is idle
    dma_channel_config c = dma_channel_get_default_config(m_dma);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(m_pio, m_sm, true));
    dma_channel_configure(m_dma, &c, &m_pio->txf[m_sm], words, count, true);
    m_replied = true;
}
