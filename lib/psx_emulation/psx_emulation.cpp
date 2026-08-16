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
const uint8_t resp_46_00[] = {0x00, 0x00, 0x01, 0x02, 0x00, 0x0A};
const uint8_t resp_46_01[] = {0x00, 0x00, 0x01, 0x01, 0x01, 0x14};

const uint8_t resp_47[] = {0x00, 0x00, 0x02, 0x00, 0x01, 0x00};
const uint8_t resp_4c_00[] = {0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
const uint8_t resp_4c_01[] = {0x00, 0x00, 0x00, 0x07, 0x00, 0x00};
const uint8_t resp_4d[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t resp_4f[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x5a};
static void __time_critical_func(handle_transaction_started)(void *ctx, pio_spi_t* spi)
{
    pio_spi_provide_read_buffer(spi, spi->dma_buf, dma_encode_transfer_count(32));
    // When not in config mode, the response is always the same so we don't need to wait to know the command
    if (!spi->configMode) {
        pio_spi_provide_write_buffer(spi, spi->resp_42, dma_encode_transfer_count(spi->report_len));
    }
}

static void __time_critical_func(handle_data_request)(void *ctx, uint8_t cmd, pio_spi_t* spi)
{
    switch (cmd)
    {
    case 0x43:
        pio_spi_provide_write_buffer(spi, resp_43, dma_encode_transfer_count(sizeof(resp_43)));
        break;
    case 0x42:
        pio_spi_provide_write_buffer(spi, spi->resp_42, dma_encode_transfer_count(sizeof(spi->resp_42)));
        break;
    case 0x40:
        pio_spi_provide_write_buffer(spi, resp_40, dma_encode_transfer_count(sizeof(resp_40)));
        break;
    case 0x41:
        pio_spi_provide_write_buffer(spi, spi->resp_41, dma_encode_transfer_count(sizeof(spi->resp_41)));
        break;
    case 0x44:
        pio_spi_provide_write_buffer(spi, resp_44, dma_encode_transfer_count(sizeof(resp_44)));
        break;
    case 0x45:
        pio_spi_provide_write_buffer(spi, resp_45_ds2, dma_encode_transfer_count(sizeof(resp_45_ds2)));
        break;
    case 0x46:
        pio_spi_provide_write_buffer(spi, spi->c46_state ? resp_46_01 : resp_46_00, dma_encode_transfer_count(sizeof(resp_46_00)));
        break;
    case 0x47:
        pio_spi_provide_write_buffer(spi, resp_47, dma_encode_transfer_count(sizeof(resp_47)));
        break;
    case 0x4c:
        pio_spi_provide_write_buffer(spi, spi->c4c_state ? resp_4c_01 : resp_4c_00, dma_encode_transfer_count(sizeof(resp_4c_00)));
        break;
    case 0x4d:
        pio_spi_provide_write_buffer(spi, resp_4d, dma_encode_transfer_count(sizeof(resp_4d)));
        break;
    case 0x4f:
        pio_spi_provide_write_buffer(spi, resp_4f, dma_encode_transfer_count(sizeof(resp_4f)));
        break;
    }
}

static void __time_critical_func(handle_transaction_ended)(void *ctx, pio_spi_t* spi)
{
    switch (spi->dma_buf[1])
    {
    case 0x43:
        spi->c46_state = 0;
        spi->c4c_state = 0;
        spi->configMode = spi->dma_buf[3];
        break;
    case 0x44:
        spi->analog = spi->dma_buf[3];
        spi->locked = spi->dma_buf[4];
        memset(spi->resp_41, 0, sizeof(spi->resp_41));
        if (spi->analog)
        {
            // Analog mode, default is 2 digial bytes + 4 analog bytes
            spi->resp_41[0] = 0b111111;
        }
        break;
    case 0x4c:
        spi->c4c_state = spi->dma_buf[3] == 0x00 ? 0x01 : 0x00;
        break;
    case 0x46:
        spi->c46_state = spi->dma_buf[3] == 0x00 ? 0x01 : 0x00;
        break;
    }
}


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
        .cs_active_high = false,
        .trigger_on_falling = false,
        .default_write_value = 0xFF,
        .transaction_started = handle_transaction_started,
        .data_request = handle_data_request,
        .transaction_ended = handle_transaction_ended,
        .callback_ctx = this};

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
}

PSXEmulation::PSXEmulation(int8_t sck, int8_t cmd, int8_t dat, uint8_t attPin, uint8_t ackPin) : sck(sck), cmd(cmd), dat(dat), attPin(attPin), ackPin(ackPin)
{
    printf("PSXEmulation %d %d %d %d %d\r\n", sck, cmd, dat, attPin, ackPin);
}

void PSXEmulation::sendData(uint8_t len, uint8_t *data)
{
    memcpy(spi->resp_42, data, len);
    spi->report_len = len;
    sent = false;
}
PsxReportFormat_t PSXEmulation::getReportFormat()
{
    return {spi->analog, {spi->resp_41[0], spi->resp_41[1], spi->resp_41[2]}};
}
bool PSXEmulation::ready()
{
    return sent;
}