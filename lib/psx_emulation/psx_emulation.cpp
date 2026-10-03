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
    if (spi)
    {
        end();
    }
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
}

void PSXEmulation::end()
{
    #if PSX_SPI_DEBUG_LOGGING
    printf("PSXEmulation end\r\n");
    #endif
    if (spi)
    {
        pio_spi_stop(spi);
        pio_spi_free(spi);
        spi = nullptr;
    }
}

void PSXEmulation::load_state(PSXEmulation *state)
{
}
void PSXEmulation::tick()
{
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