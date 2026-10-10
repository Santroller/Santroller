#include "joybus_controller.hpp"
#include <string.h>
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "Controller.pio.h"

// How often to poll a controller, and to probe for one while nothing is plugged in
#define JOYBUS_POLL_INTERVAL_US 1000
#define JOYBUS_PROBE_INTERVAL_US 10000
// Missed polls in a row before the controller counts as unplugged
#define JOYBUS_MAX_FAILURES 3

JoybusController::JoybusController(uint8_t pin) : m_pin(pin)
{
}

JoybusController::~JoybusController()
{
    end();
}

bool JoybusController::begin()
{
    if (m_started)
    {
        return true;
    }
    if (!pio_claim_free_sm_and_add_program(&controller_program, &m_pio, &m_sm, &m_offset))
    {
        m_pio = nullptr;
        return false;
    }
    m_dma_tx = dma_claim_unused_channel(false);
    m_dma_rx = dma_claim_unused_channel(false);
    if (m_dma_tx < 0 || m_dma_rx < 0)
    {
        if (m_dma_tx >= 0)
            dma_channel_unclaim(m_dma_tx);
        if (m_dma_rx >= 0)
            dma_channel_unclaim(m_dma_rx);
        m_dma_tx = m_dma_rx = -1;
        pio_remove_program_and_unclaim_sm(&controller_program, m_pio, m_sm, m_offset);
        m_pio = nullptr;
        return false;
    }
    m_started = true;
    restart_sm();
    disconnected();
    // probe straight away
    m_transfer_start_us = time_us_32() - JOYBUS_PROBE_INTERVAL_US;
    return true;
}

void JoybusController::end()
{
    if (!m_started)
    {
        return;
    }
    dma_channel_abort(m_dma_rx);
    dma_channel_abort(m_dma_tx);
    dma_channel_unclaim(m_dma_rx);
    dma_channel_unclaim(m_dma_tx);
    m_dma_tx = m_dma_rx = -1;
    pio_sm_set_enabled(m_pio, m_sm, false);
    pio_remove_program_and_unclaim_sm(&controller_program, m_pio, m_sm, m_offset);
    m_pio = nullptr;
    gpio_init(m_pin);
    m_busy = false;
    m_started = false;
    disconnected();
}

void JoybusController::restart_sm()
{
    // Also used to recover from a missing reply, where the program is stuck waiting on the line
    pio_sm_set_enabled(m_pio, m_sm, false);
    pio_sm_config c = controller_program_get_default_config(m_offset);
    controller_program_init(m_pio, m_sm, m_offset, m_pin, &c);
    // We are the console here, so nothing else pulls the line up. The internal pull-up is weak enough that
    // the line rises slowly, so an external ~1k pull-up to 3.3V is recommended for reliable reads
    gpio_pull_up(m_pin);
}

void JoybusController::disconnected()
{
    m_kind = JoybusControllerKind::None;
    m_step = Step::Probe;
    m_failures = 0;
    m_rumble_sent = false;
    memset(m_state, 0, sizeof(m_state));
}

void JoybusController::set_rumble(bool rumble)
{
    m_rumble = rumble;
}

void JoybusController::start_transfer(const uint8_t *request, uint8_t request_len, uint8_t response_len)
{
    // The program takes the response length first, then the request one byte per word
    m_tx[0] = (uint32_t)((response_len - 1) & 0x1F) << 24;
    for (uint8_t i = 0; i < request_len; i++)
    {
        m_tx[i + 1] = (uint32_t)request[i] << 24;
    }
    m_response_len = response_len;
    // One extra word, pushed once the controller's stop bit arrives
    uint8_t rx_words = response_len + 1;

    dma_channel_config rx = dma_channel_get_default_config(m_dma_rx);
    channel_config_set_transfer_data_size(&rx, DMA_SIZE_32);
    channel_config_set_read_increment(&rx, false);
    channel_config_set_write_increment(&rx, true);
    channel_config_set_dreq(&rx, pio_get_dreq(m_pio, m_sm, false));
    dma_channel_configure(m_dma_rx, &rx, m_rx, &m_pio->rxf[m_sm], rx_words, true);

    dma_channel_config tx = dma_channel_get_default_config(m_dma_tx);
    channel_config_set_transfer_data_size(&tx, DMA_SIZE_32);
    channel_config_set_read_increment(&tx, true);
    channel_config_set_write_increment(&tx, false);
    channel_config_set_dreq(&tx, pio_get_dreq(m_pio, m_sm, true));
    dma_channel_configure(m_dma_tx, &tx, &m_pio->txf[m_sm], m_tx, request_len + 1, true);

    m_busy = true;
    m_transfer_start_us = time_us_32();
    // 4us a bit, plus some slack for the controller to start replying
    m_transfer_timeout_us = (request_len + response_len) * 40 + 300;
}

void JoybusController::finish_transfer(bool ok)
{
    m_busy = false;
    if (!ok)
    {
        dma_channel_abort(m_dma_rx);
        dma_channel_abort(m_dma_tx);
        restart_sm();
        switch (m_step)
        {
        case Step::N64PakInit:
        case Step::N64Rumble:
            // Not every controller answers controller pak commands, that's no reason to drop it
            m_rumble_sent = m_rumble;
            m_step = Step::N64Poll;
            return;
        case Step::Probe:
            return;
        default:
            if (++m_failures >= JOYBUS_MAX_FAILURES)
            {
                disconnected();
            }
            return;
        }
    }
    m_failures = 0;
    uint8_t response[10];
    for (uint8_t i = 0; i < m_response_len; i++)
    {
        response[i] = m_rx[i] & 0xFF;
    }
    switch (m_step)
    {
    case Step::Probe:
        if (response[0] == 0x05)
        {
            m_kind = JoybusControllerKind::N64;
            m_step = Step::N64PakInit;
        }
        else if (response[0] & 0x08)
        {
            // 0x09 for a wired controller, the WaveBird receiver reports other bits as well
            m_kind = JoybusControllerKind::GameCube;
            m_step = Step::GcOrigin;
        }
        break;
    case Step::GcOrigin:
        m_step = Step::GcPoll;
        break;
    case Step::GcPoll:
        memcpy(m_state, response, 8);
        break;
    case Step::N64PakInit:
        m_step = Step::N64Poll;
        break;
    case Step::N64Poll:
        memcpy(m_state, response, 4);
        break;
    case Step::N64Rumble:
        m_step = Step::N64Poll;
        break;
    }
}

void JoybusController::tick()
{
    if (!m_started)
    {
        return;
    }
    uint32_t now = time_us_32();
    if (m_busy)
    {
        if (!dma_channel_is_busy(m_dma_rx))
        {
            finish_transfer(true);
        }
        else if (now - m_transfer_start_us > m_transfer_timeout_us)
        {
            finish_transfer(false);
        }
        return;
    }
    uint32_t interval = m_kind == JoybusControllerKind::None ? JOYBUS_PROBE_INTERVAL_US : JOYBUS_POLL_INTERVAL_US;
    if (now - m_transfer_start_us < interval)
    {
        return;
    }
    switch (m_step)
    {
    case Step::Probe:
    {
        const uint8_t request[] = {0x00};
        start_transfer(request, sizeof(request), 3);
        break;
    }
    case Step::GcOrigin:
    {
        const uint8_t request[] = {0x41};
        start_transfer(request, sizeof(request), 10);
        break;
    }
    case Step::GcPoll:
    {
        const uint8_t request[] = {0x40, 0x03, (uint8_t)(m_rumble ? 0x01 : 0x00)};
        start_transfer(request, sizeof(request), 8);
        break;
    }
    case Step::N64PakInit:
    {
        // Writing 0x80 to 0x8000 switches on a rumble pak (0x01 is the address CRC)
        uint8_t request[35] = {0x03, 0x80, 0x01};
        memset(request + 3, 0x80, 32);
        start_transfer(request, sizeof(request), 1);
        break;
    }
    case Step::N64Poll:
    case Step::N64Rumble:
    {
        if (m_rumble != m_rumble_sent)
        {
            // The rumble pak motor is driven by writing to 0xC000 (0x1B is the address CRC)
            m_step = Step::N64Rumble;
            m_rumble_sent = m_rumble;
            uint8_t request[35] = {0x03, 0xC0, 0x1B};
            memset(request + 3, m_rumble ? 0x01 : 0x00, 32);
            start_transfer(request, sizeof(request), 1);
            break;
        }
        m_step = Step::N64Poll;
        const uint8_t request[] = {0x01};
        start_transfer(request, sizeof(request), 4);
        break;
    }
    }
}
