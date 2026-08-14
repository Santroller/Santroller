#include "psx_emulation.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include "hardware/sync.h"
#include <stdio.h>
#include <string.h>

static PSXEmulation *controller;

static pio_spi_t *spi;
static volatile uint8_t dma_buf[256];
static volatile uint8_t write_buf[] = {1, 2, 3, 4, 5, 6};
static volatile uint8_t test_buf[] = {0xff, 0xff};
uint8_t resp_42[] = {0xff, 0xff};
uint8_t resp_43[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t resp_40[] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x5A};
uint8_t resp_41_digital[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t resp_41_analog[] = {0xFF, 0xFF, 0x03, 0x00, 0x00, 0x5A};
uint8_t resp_44[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t resp_45_ds2[] = {0x03, 0x02, 0x00, 0x02, 0x01, 0x00};
uint8_t resp_45_gh[] = {0x01, 0x02, 0x00, 0x02, 0x01, 0x00};
uint8_t resp_46_01_ds2[] = {0x00, 0x00, 0x00, 0x02, 0x00, 0x0A};
uint8_t resp_46_02_ds2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x14};
uint8_t resp_46_01_gh[] = {0x00, 0x00, 0x01, 0x02, 0x00, 0x0A};
uint8_t resp_46_02_gh[] = {0x00, 0x00, 0x01, 0x01, 0x01, 0x14};

uint8_t resp_47_ds2[] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
uint8_t resp_47_gh[] = {0x00, 0x00, 0x02, 0x00, 0x01, 0x00};
uint8_t resp_4c_01_ds2[] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
uint8_t resp_4c_02_ds2[] = {0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
uint8_t resp_4c_01_gh[] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
uint8_t resp_4c_02_gh[] = {0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
uint8_t resp_4d[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t resp_4f[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x5a};
static dma_channel_hw_t *write_dma;
extern "C"
{
    static void __time_critical_func(transaction_started)(void *ctx)
    {
        (void)ctx;
    }

    static void __time_critical_func(data_request)(void *ctx, uint8_t reg)
    {
        (void)ctx;
        switch (reg)
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
            pio_spi_provide_write_buffer(spi, resp_41_digital, dma_encode_transfer_count(sizeof(resp_41_digital)));
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

    static void __time_critical_func(transaction_ended)(void *ctx)
    {
        (void)ctx;
        pio_spi_provide_read_buffer(spi, dma_buf, 255);
        pio_spi_provide_write_buffer_length(spi, 9);
        switch (dma_buf[1])
        {
        case 0x43:
            spi->c46_state = 0;
            spi->c4c_state = 0;
            if (dma_buf[3] == 0x01)
            {
                spi->header = 0xF3;
            }

            if (dma_buf[3] == 0x00)
            {
                spi->header = 0x41;
            }
            break;
        case 0x4c:
            spi->c4c_state = dma_buf[3] == 0x00 ? 0x01 : 0x00;
            break;
        case 0x46:
            spi->c46_state = dma_buf[3] == 0x00 ? 0x01 : 0x00;
            break;
        }
    }
}

PSXEmulation::~PSXEmulation()
{
    printf("~PSXEmulation\r\n");
}
void PSXEmulation::begin()
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
        .transaction_started = transaction_started,
        .data_request = data_request,
        .transaction_ended = transaction_ended,
        .callback_ctx = NULL};

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
    controller = this;
    printf("PSXEmulation %d %d %d %d %d\r\n", sck, cmd, dat, attPin, ackPin);
}