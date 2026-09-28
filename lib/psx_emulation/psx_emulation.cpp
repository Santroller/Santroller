#include "psx_emulation.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "hardware/sync.h"
#include <stdio.h>
#include <string.h>

PSXEmulation::~PSXEmulation()
{
    end();
    printf("~PSXEmulation\r\n");
}
void PSXEmulation::begin(SubType type)
{
    printf("PSXEmulation begin\r\n");
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
    printf("PSXEmulation end\r\n");
    pio_spi_stop(spi);
    pio_spi_free(spi);
}

void PSXEmulation::load_state(PSXEmulation *state)
{
}
void PSXEmulation::tick()
{
    pio_spi_watchdog_tick(spi);
}

PSXEmulation::PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin) : sck(sck), cmd(cmd), dat(dat), attPin(attPin), ackPin(ackPin)
{
    printf("PSXEmulation %d %d %d %d %d\r\n", sck, cmd, dat, attPin, ackPin);
}

void PSXEmulation::sendData(uint8_t len, uint8_t *data)
{
    uint8_t ts = spi->timing_send_idx;
    uint8_t tn = (uint8_t)((ts + 1) & 0x0F);
    if (tn == spi->timing_prepare_idx)
    {
        spi->timing_send_overflow = true;
    }
    else
    {
        spi->timing_send_us[ts] = time_us_32();
        spi->timing_send_old_len[ts] = spi->protocol.report_len;
        spi->timing_send_new_len[ts] = len;
        spi->timing_send_idx = tn;
    }

    memcpy(spi->protocol.resp_42, data, len);
    memcpy(spi->protocol.config_responses[0x02], data, sizeof(spi->protocol.config_responses[0x02]));
    spi->protocol.report_len = len;
    sent = false;
}
static void dump_timing_trace(pio_spi_t *spi)
{
    while (spi->timing_send_idx != spi->timing_prepare_idx)
        break;

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

uint8_t last_lastcmd = 0;
PsxReportFormat_t PSXEmulation::getReportFormat()
{
    if (spi->timing_prepare_idx || spi->timing_send_idx)
        dump_timing_trace(spi);

    while (spi->transaction_idx_read != spi->transaction_idx_write)
    {
        uint8_t idx = spi->transaction_idx_read;
        printf("SPI transaction: TX ");
        for (int i = 0; i < 8; i++)
            printf("%02X ", spi->transaction_test[idx][i]);
        printf(" RX ");
        for (int i = 0; i < spi->transaction_test_rx_len[idx]; i++)
            printf("%02X ", spi->transaction_test_rx[idx][i]);
        printf(" | state: config=%d analog=%d len=%u mask=%02X %02X %02X\r\n",
               spi->protocol.configMode,
               spi->protocol.config_responses[0x05][2],
               spi->protocol.report_len,
               spi->protocol.report_mask[0],
               spi->protocol.report_mask[1],
               spi->protocol.report_mask[2]);
        spi->transaction_idx_read = (spi->transaction_idx_read + 1) & 0x07;
    }
    if (spi->read_idx_read != spi->read_idx_write)
    {
        spi->read_idx_read = (spi->read_idx_read + 1) & 0x07; // Assuming a buffer size of 8
        printf("New command received: %d: ", spi->protocol.cmd_id);
        for (int i = 0; i < 8; i++)
        {
            printf("%02X ", spi->dma_buf_test[spi->read_idx_read][i]);
        }
        printf("\r\n");
        printf("  RX: ");
        for (int i = 0; i < spi->response_test_len[spi->read_idx_read]; i++)
            printf("%02X ", spi->response_test[spi->read_idx_read][i]);
        printf(" | state: config=%d analog=%d len=%u mask=%02X %02X %02X\r\n",
               spi->protocol.configMode,
               spi->protocol.config_responses[0x05][2],
               spi->protocol.report_len,
               spi->protocol.report_mask[0],
               spi->protocol.report_mask[1],
               spi->protocol.report_mask[2]);
    }
    if (spi->write_idx_read != spi->write_idx_write)
    {
        spi->write_idx_read = (spi->write_idx_read + 1) & 0x07; // Assuming a buffer size of 8
        printf("New write queued: %d, cmd: %02X: ", spi->write_id, spi->dma_buf_test2[spi->write_idx_read][0]);
        for (int i = 1; i < 7; i++)
        {
            printf("%02X ", spi->dma_buf_test2[spi->write_idx_read][i]);
        }
        printf("\r\n");
    }
    return {spi->protocol.config_responses[0x05][2] == 0x01, {spi->protocol.report_mask[0], spi->protocol.report_mask[1], spi->protocol.report_mask[2]}};
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