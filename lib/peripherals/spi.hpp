#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include "pico/time.h"

#define SPI_MAX_TRANSFER_SIZE 1056
// A transfer timeout of 1000ms will allow a 10000 bit transfer to complete
// successfully without timeouts at baudrates as low as 10000 baud.
#define SPI_TRANSFER_TIMEOUT_MS 10000
#define SPI_TAKE_MUTEX_TIMEOUT_MS 10000
typedef struct spi_dma_s
{
    spi_inst_t *spi;

    uint irq_num;
    irq_handler_t irq_handler;

    uint baudrate;
    uint sda_gpio;
    uint scl_gpio;
    int tx_chan;
    int rx_chan;
    bool reading;
    bool writing;

    volatile bool stop_detected;
    volatile bool abort_detected;
    volatile bool timeout;
    volatile bool running;
    alarm_id_t timeout_alarm_id;

    uint16_t data_cmds[SPI_MAX_TRANSFER_SIZE];
    void (*process_data)(bool running, bool timeout, bool abort_detected, bool stop_detected);
} spi_dma_t;
class SPIMasterInterface
{
public:
    SPIMasterInterface(uint8_t block, spi_cpha_t cpha, spi_cpol_t cpol, int8_t sck, int8_t mosi, int8_t miso, bool msbfirst, uint32_t clock);
    uint8_t transfer(uint8_t data);

private:
    spi_inst_t *spi;
    spi_inst_t *_hardwareBlocks[NUM_I2CS] = {spi0, spi1};
    bool m_msbfirst;
    bool m_valid;
};