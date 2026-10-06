#include "psx_emulation.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "hardware/sync.h"
#include <stdio.h>
#include <string.h>

PSXEmulation::~PSXEmulation()
{
    end();
#if PSX_SPI_DEBUG_LOGGING
    printf("~PSXEmulation\r\n");
#endif
}
void PSXEmulation::begin(SubType type)
{
#if PSX_SPI_DEBUG_LOGGING
    printf("PSXEmulation begin\r\n");
#endif
    if (m_reconnecting)
    {
        // already mid-swap, just reconnect as the latest subtype
        m_reconnect_type = type;
        return;
    }
    if (spi)
    {
        if (spi->type == type)
        {
            return;
        }
        // Simulate an unplug: stop acking and release the bus. The console's pad driver
        // treats missing ACKs as a disconnected controller, and on reconnect the game
        // re-runs its setup (analog mode, pressure mask, rumble mapping) for the new subtype.
        end();
        m_reconnecting = true;
        m_reconnect_type = type;
        m_reconnect_at_ms = to_ms_since_boot(get_absolute_time()) + PSX_SWAP_DISCONNECT_MS;
        return;
    }
    start(type, is_communicating());
}

void PSXEmulation::listen()
{
    if (spi || m_reconnecting || m_listening)
    {
        return;
    }
    // ACK and DAT float so the console sees an empty port. ATT gets a pull-up so a
    // disconnected cable can't float low and look like the console polling.
    gpio_init(ackPin);
    gpio_init(dat);
    gpio_set_pulls(ackPin, false, false);
    gpio_set_pulls(dat, false, false);
    gpio_init(attPin);
    gpio_set_pulls(attPin, true, false);
    gpio_acknowledge_irq(attPin, GPIO_IRQ_EDGE_FALL);
    m_att_seen = false;
    m_listening = true;
}

void PSXEmulation::acquire()
{
    m_users++;
    m_release_pending = false;
}

void PSXEmulation::release()
{
    if (m_users == 0)
    {
        return;
    }
    m_users--;
    if (m_users == 0)
    {
        m_release_pending = true;
        m_release_at_ms = to_ms_since_boot(get_absolute_time()) + PSX_RELEASE_GRACE_MS;
    }
}

void PSXEmulation::start(SubType type, bool console_present)
{
    m_listening = false;
    pio_spi_config_t config = {
        .pio_idx = 1,
        .cs_pin = attPin,
        .sck_pin = sck,
        .copi_pin = cmd,
        .cipo_pin = dat,
        .ack_pin = ackPin,
        .type = type};

    spi = pio_spi_init(&config);
    pio_spi_start(spi);
    if (console_present)
    {
        // the console is still there, so count it as communicating until the watchdog
        // says otherwise rather than flapping the activation trigger before the first poll
        spi->watchdog_active = true;
        spi->watchdog_last_activity_ms = to_ms_since_boot(get_absolute_time());
    }
}

void PSXEmulation::end()
{
#if PSX_SPI_DEBUG_LOGGING
    printf("PSXEmulation end\r\n");
#endif
    m_reconnecting = false;
    m_release_pending = false;
    m_listening = false;
    m_att_seen = false;
    if (spi)
    {
        pio_spi_stop(spi);
        pio_spi_free(spi);
        spi = nullptr;
        // hand the pins back as plain inputs so ACK and DAT float, which reads as "no controller"
        gpio_init(ackPin);
        gpio_init(dat);
        gpio_set_pulls(ackPin, false, false);
        gpio_set_pulls(dat, false, false);
    }
}

void PSXEmulation::load_state(PSXEmulation *state)
{
}
void PSXEmulation::tick()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (m_listening)
    {
        // Edge events latch in the raw INTR register even with the interrupt disabled,
        // so this catches every ATT poll between ticks without needing an IRQ handler.
        uint32_t events = (io_bank0_hw->intr[attPin / 8] >> (4 * (attPin % 8))) & 0xF;
        if (events & GPIO_IRQ_EDGE_FALL)
        {
            gpio_acknowledge_irq(attPin, GPIO_IRQ_EDGE_FALL);
            m_last_att_ms = now;
            m_att_seen = true;
        }
        else if (m_att_seen && now - m_last_att_ms >= PSX_LISTEN_TIMEOUT_MS)
        {
            m_att_seen = false;
        }
    }
    if (m_reconnecting && (int32_t)(now - m_reconnect_at_ms) >= 0)
    {
        m_reconnecting = false;
        start(m_reconnect_type, true);
    }
    if (m_release_pending && (int32_t)(now - m_release_at_ms) >= 0)
    {
        // nothing picked the controller back up, so unplug it and go back to watching ATT
        end();
        listen();
    }
    if (spi)
    {
        pio_spi_watchdog_tick(spi);
    }
}

PSXEmulation::PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin) : sck(sck), cmd(cmd), dat(dat), attPin(attPin), ackPin(ackPin)
{
#if PSX_SPI_DEBUG_LOGGING
    printf("PSXEmulation %d %d %d %d %d\r\n", sck, cmd, dat, attPin, ackPin);
#endif
}

void PSXEmulation::sendData(uint8_t len, uint8_t *data)
{
    if (!spi)
    {
        return;
    }
    // sendData supplies raw controller state only. The PSX response format and
    // length are selected later, when the next SPI transaction is prepared,
    // after all command-driven protocol state changes have been applied.
    if (len > sizeof(spi->protocol.resp_42))
        len = sizeof(spi->protocol.resp_42);

    memset((void *)spi->protocol.resp_42, 0, sizeof(spi->protocol.resp_42));
    memcpy((void *)spi->protocol.resp_42, data, len);
    memcpy((void *)spi->protocol.config_responses[0x02],
           (const void *)spi->protocol.resp_42,
           sizeof(spi->protocol.config_responses[0x02]));
    // config mode DMAs this straight to the wire, so it has to be stored active low
    spi->protocol.config_responses[0x02][0] = ~spi->protocol.config_responses[0x02][0];
    spi->protocol.config_responses[0x02][1] = ~spi->protocol.config_responses[0x02][1];

    memcpy((void *)spi->raw_report, (const void *)spi->protocol.resp_42,
           sizeof(spi->protocol.resp_42));
    sent = false;
}

#if PSX_SPI_DEBUG_LOGGING
static void dump_config_index_trace(pio_spi_t *spi)
{
    for (uint8_t i = 0; i < spi->config_index_trace_idx; ++i)
    {
        printf("\\r\\n  INDEX[%u] reg=%02X fifo=%08lX value=%02X",
               i,
               spi->config_index_trace_reg[i],
               (unsigned long)spi->config_index_trace_word[i],
               spi->config_index_trace_value[i]);
    }
}

static void dump_timing_trace(pio_spi_t *spi)
{
    printf("SPI timing trace:");
    for (uint8_t i = 0; i < spi->timing_prepare_idx; ++i)
    {
        printf("\r\n  PREP[%u] t=%lu len=%u hdr=%02X cfg=%u analog=%u",
               i,
               (unsigned long)spi->timing_prepare_us[i],
               spi->timing_prepare_len[i],
               spi->timing_prepare_header[i],
               spi->timing_prepare_config[i],
               spi->timing_prepare_analog[i]);
    }
    for (uint8_t i = 0; i < spi->timing_send_idx; ++i)
    {
        printf("\r\n  SEND[%u] t=%lu %u->%u",
               i,
               (unsigned long)spi->timing_send_us[i],
               spi->timing_send_old_len[i],
               spi->timing_send_new_len[i]);
    }
    if (spi->timing_prepare_overflow || spi->timing_send_overflow)
        printf("\r\n  TRACE OVERFLOW");
    printf("\r\n");
}

#endif

uint8_t last_lastcmd = 0;
PsxReportFormat_t PSXEmulation::getReportFormat()
{
    if (!spi)
    {
        return {false, {0, 0, 0}};
    }
    return {is_analog(spi), {spi->protocol.report_mask[0], spi->protocol.report_mask[1], spi->protocol.report_mask[2]}};
}
bool PSXEmulation::ready()
{
    return sent;
}
void PSXEmulation::get_rumble(uint8_t &small, uint8_t &large)
{
    if (spi)
    {
        small = spi->rumble_small;
        large = spi->rumble_large;
    }
    else
    {
        small = 0;
        large = 0;
    }
}