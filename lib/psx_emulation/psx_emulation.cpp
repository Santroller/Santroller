#include "psx_emulation.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "hardware/sync.h"
#include <stdio.h>
#include <string.h>

const uint8_t resp_43[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t resp_40[] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x5A};
const uint8_t resp_44[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t resp_45_ds2[] = {0x03, 0x02, 0x00, 0x02, 0x01, 0x00};
const uint8_t resp_45_gh[] = {0x01, 0x02, 0x00, 0x02, 0x01, 0x00};
const uint8_t resp_46_01_ds2[] = {0x00, 0x00, 0x00, 0x02, 0x00, 0x0A};
const uint8_t resp_46_02_ds2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x14};
const uint8_t resp_46_01_gh[] = {0x00, 0x00, 0x01, 0x02, 0x00, 0x0A};
const uint8_t resp_46_02_gh[] = {0x00, 0x00, 0x01, 0x01, 0x01, 0x14};

const uint8_t resp_47_ds2[] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
const uint8_t resp_47_gh[] = {0x00, 0x00, 0x02, 0x00, 0x01, 0x00};
const uint8_t resp_4c_01_ds2[] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
const uint8_t resp_4c_02_ds2[] = {0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
const uint8_t resp_4c_01_gh[] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
const uint8_t resp_4c_02_gh[] = {0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
const uint8_t resp_4d[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t resp_4f[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x5a};
static void __time_critical_func(handle_transaction_started)(void *ctx)
{
    PSXEmulation *controller = (PSXEmulation *)ctx;
    controller->transaction_started();
}

static void __time_critical_func(handle_data_request)(void *ctx, uint8_t reg)
{
    PSXEmulation *controller = (PSXEmulation *)ctx;
    controller->data_request(reg);
}

static void __time_critical_func(handle_transaction_ended)(void *ctx)
{
    PSXEmulation *controller = (PSXEmulation *)ctx;
    controller->transaction_ended();
}

void PSXEmulation::transaction_started()
{
}
void PSXEmulation::transaction_ended()
{
    pio_spi_provide_read_buffer(spi, dma_buf, 255);
    pio_spi_provide_write_buffer_length(spi, 32);
    switch (dma_buf[1])
    {
    case 0x43:
        spi->c46_state = 0;
        spi->c4c_state = 0;
        config = dma_buf[3];
        break;
    case 0x44:
        analog = dma_buf[3];
        locked = dma_buf[4];
        memset(resp_41, 0, sizeof(resp_41));
        if (analog)
        {
            // Analog mode, default is 2 digial bytes + 4 analog bytes
            resp_41[0] = 0b111111;
        }
        break;
    case 0x4c:
        spi->c4c_state = dma_buf[3] == 0x00 ? 0x01 : 0x00;
        break;
    case 0x46:
        spi->c46_state = dma_buf[3] == 0x00 ? 0x01 : 0x00;
        break;
    }
    if (config)
    {
        spi->header = 0xF3;
    }
    else
    {
        spi->header = 0x40 | (report_len / 2);
    }
    sent = true;
}

void PSXEmulation::data_request(uint8_t cmd)
{
    switch (cmd)
    {
    case 0x43:
        pio_spi_provide_write_buffer(spi, spi->header == 0xF3 ? resp_43 : resp_42, dma_encode_transfer_count(sizeof(spi->header == 0xF3 ? resp_43 : resp_42)));
        break;
    case 0x42:
        pio_spi_provide_write_buffer(spi, resp_42, dma_encode_transfer_count(sizeof(resp_42)));
        break;
    case 0x40:
        pio_spi_provide_write_buffer(spi, resp_40, dma_encode_transfer_count(sizeof(resp_40)));
        break;
    case 0x41:
        pio_spi_provide_write_buffer(spi, resp_41, dma_encode_transfer_count(sizeof(resp_41)));
        break;
    case 0x44:
        pio_spi_provide_write_buffer(spi, resp_44, dma_encode_transfer_count(sizeof(resp_44)));
        break;
    case 0x45:
        pio_spi_provide_write_buffer(spi, resp_45_gh, dma_encode_transfer_count(sizeof(resp_45_gh)));
        break;
    case 0x46:
        pio_spi_provide_write_buffer(spi, spi->c46_state ? resp_46_02_gh : resp_46_01_gh, dma_encode_transfer_count(sizeof(resp_46_01_gh)));
        break;
    case 0x47:
        pio_spi_provide_write_buffer(spi, resp_47_gh, dma_encode_transfer_count(sizeof(resp_47_gh)));
        break;
    case 0x4c:
        pio_spi_provide_write_buffer(spi, spi->c4c_state ? resp_4c_02_gh : resp_4c_01_gh, dma_encode_transfer_count(sizeof(resp_4c_01_gh)));
        break;
    case 0x4d:
        pio_spi_provide_write_buffer(spi, resp_4d, dma_encode_transfer_count(sizeof(resp_4d)));
        break;
    case 0x4f:
        pio_spi_provide_write_buffer(spi, resp_4f, dma_encode_transfer_count(sizeof(resp_4f)));
        break;
    }
}

PSXEmulation::~PSXEmulation()
{
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
        .dbg_pin = -1,
        .cs_active_high = false,
        .trigger_on_falling = false,
        .default_write_value = 0xFF,
        .transaction_started = handle_transaction_started,
        .data_request = handle_data_request,
        .transaction_ended = handle_transaction_ended,
        .callback_ctx = this};

    spi = pio_spi_init(&config);
    write_dma = pio_spi_get_dma_write_channel(spi);
    pio_spi_provide_read_buffer(spi, dma_buf, 255);
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
}

PSXEmulation::PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin) : sck(sck), cmd(cmd), dat(dat), attPin(attPin), ackPin(ackPin)
{
    printf("PSXEmulation %d %d %d %d %d\r\n", sck, cmd, dat, attPin, ackPin);
}

void PSXEmulation::sendData(uint8_t len, uint8_t *data)
{
    memcpy(resp_42, data, len);
    report_len = len;
    sent = false;
}
PsxReportFormat_t PSXEmulation::getReportFormat()
{
    return {analog, {resp_41[0], resp_41[1], resp_41[2]}};
}
bool PSXEmulation::ready()
{
    return sent;
}