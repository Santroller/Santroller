#include "i2c.hpp"

#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

inline void process_dma(i2c_dma_t *i2c_dma)
{
    if (i2c_dma->running)
    {
        i2c_dma->running = false;
        cancel_alarm(i2c_dma->timeout_alarm_id);
        if (i2c_dma->abort_detected || !i2c_dma->stop_detected)
        {
            dma_channel_abort(i2c_dma->tx_chan);
            if (i2c_dma->reading)
            {
                dma_channel_abort(i2c_dma->rx_chan);
            }
        }
    }
    if (i2c_dma->process_data)
    {
        i2c_dma->process_data(i2c_dma->running, i2c_dma->timeout, i2c_dma->abort_detected, i2c_dma->stop_detected);
    }
    i2c_dma->timeout = false;
    i2c_dma->abort_detected = false;
    i2c_dma->stop_detected = false;
}

static i2c_dma_t i2c_dma_list[2];
static void i2c_dma_irq_handler(i2c_dma_t *i2c_dma)
{
    const uint32_t status = i2c_get_hw(i2c_dma->i2c)->intr_stat;

    // If there is an abort, normally there is an abort interrupt followed by a
    // stop interrupt. On the rare occasion, for example, if the first I2C
    // transaction after reset is aborted, the abort and stop interrupt flags
    // appear to be set at the same instant or almost the same instant.
    if (status & I2C_IC_INTR_STAT_R_TX_ABRT_BITS)
    {
        // Transfer aborted.
        i2c_get_hw(i2c_dma->i2c)->clr_tx_abrt;
        i2c_dma->abort_detected = true;
    }

    if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS)
    {
        // Transfer complete.
        i2c_get_hw(i2c_dma->i2c)->clr_stop_det;
        i2c_dma->stop_detected = true;
        process_dma(i2c_dma);
    }
}

static void i2c0_dma_irq_handler(void)
{
    i2c_dma_irq_handler(&i2c_dma_list[0]);
}

static void i2c_dma_pin_open_drain(uint gpio)
{
    gpio_set_function(gpio, GPIO_FUNC_SIO);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_put(gpio, 0);
}
static void i2c1_dma_irq_handler(void)
{
    i2c_dma_irq_handler(&i2c_dma_list[1]);
}
static void i2c_dma_pin_od_low(uint gpio)
{
    gpio_set_dir(gpio, GPIO_OUT);
}

static void i2c_dma_pin_od_high(uint gpio)
{
    gpio_set_dir(gpio, GPIO_IN);
}
static void i2c_dma_unblock(i2c_dma_t *i2c_dma)
{
    i2c_dma_pin_open_drain(i2c_dma->sda_gpio);
    i2c_dma_pin_open_drain(i2c_dma->scl_gpio);

    bool sda_high;
    int max_tries = 9;

    // Make sure the frequency of the bit-bannged I2C clock is at most 100KHz.
    const uint32_t f_clk_sys_khz =
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    const uint32_t i2c_delay = f_clk_sys_khz / 100 / 2;

    do
    {
        i2c_dma_pin_od_low(i2c_dma->scl_gpio);
        for (int i = i2c_delay; i > 0; i -= 1)
        {
            __asm__("nop");
        }

        i2c_dma_pin_od_high(i2c_dma->scl_gpio);
        for (int i = i2c_delay; i > 0; i -= 1)
        {
            __asm__("nop");
        }

        max_tries -= 1;
        sda_high = gpio_get(i2c_dma->sda_gpio);
    } while (!sda_high && max_tries > 0);
}
static bool i2c_dma_is_blocked(i2c_dma_t *i2c_dma)
{
    i2c_dma_pin_open_drain(i2c_dma->sda_gpio);
    i2c_dma_pin_open_drain(i2c_dma->scl_gpio);

    const bool sda_high = gpio_get(i2c_dma->sda_gpio);
    const bool scl_high = gpio_get(i2c_dma->scl_gpio);

    return !sda_high || !scl_high;
}
static int i2c_dma_init_intern(i2c_dma_t *i2c_dma)
{
    irq_set_enabled(i2c_dma->irq_num, false);

    i2c_dma->stop_detected = false;
    i2c_dma->abort_detected = false;
    // Attempt to unblock a blocked bus. If it can't be unblocked, continue
    // anyway.
    if (i2c_dma_is_blocked(i2c_dma))
    {
        i2c_dma_unblock(i2c_dma);
    }

    i2c_init(i2c_dma->i2c, i2c_dma->baudrate);

    gpio_set_function(i2c_dma->sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(i2c_dma->scl_gpio, GPIO_FUNC_I2C);
    gpio_pull_up(i2c_dma->sda_gpio);
    gpio_pull_up(i2c_dma->scl_gpio);

    i2c_get_hw(i2c_dma->i2c)->intr_mask =
        I2C_IC_INTR_MASK_M_STOP_DET_BITS |
        I2C_IC_INTR_MASK_M_TX_ABRT_BITS;

    irq_set_exclusive_handler(i2c_dma->irq_num, i2c_dma->irq_handler);
    irq_set_enabled(i2c_dma->irq_num, true);
    i2c_dma->tx_chan = dma_claim_unused_channel(false);
    i2c_dma->rx_chan = dma_claim_unused_channel(false);

    return PICO_OK;
}

static void i2c_dma_set_target_addr(i2c_inst_t *i2c, uint8_t addr)
{
    i2c_get_hw(i2c)->enable = 0;
    i2c_get_hw(i2c)->tar = addr;
    i2c_get_hw(i2c)->enable = 1;
}

static void i2c_dma_tx_channel_configure(
    i2c_inst_t *i2c, int tx_channel, const uint16_t *tx_buf, size_t len)
{
    dma_channel_config tx_config = dma_channel_get_default_config(tx_channel);
    channel_config_set_read_increment(&tx_config, true);
    channel_config_set_write_increment(&tx_config, false);
    channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_16);
    channel_config_set_dreq(&tx_config, i2c_get_dreq(i2c, true));
    dma_channel_configure(
        tx_channel, &tx_config, &i2c_get_hw(i2c)->data_cmd, tx_buf, len, true);
}

static void i2c_dma_rx_channel_configure(
    i2c_inst_t *i2c, int rx_channel, uint8_t *rx_buf, size_t len)
{
    dma_channel_config rx_config = dma_channel_get_default_config(rx_channel);
    channel_config_set_read_increment(&rx_config, false);
    channel_config_set_write_increment(&rx_config, true);
    channel_config_set_transfer_data_size(&rx_config, DMA_SIZE_8);
    channel_config_set_dreq(&rx_config, i2c_get_dreq(i2c, false));
    dma_channel_configure(
        rx_channel, &rx_config, rx_buf, &i2c_get_hw(i2c)->data_cmd, len, true);
}

int64_t timeout_handler(__unused alarm_id_t id, void *user_data)
{
    i2c_dma_t *i2c_dma = (i2c_dma_t *)user_data;
    i2c_dma->timeout = true;
    process_dma(i2c_dma);
    return 0;
}
static void i2c_dma_write_read_internal(
    i2c_dma_t *i2c_dma,
    uint8_t addr,
    const uint8_t *wbuf,
    size_t wbuf_len,
    uint8_t *rbuf,
    size_t rbuf_len)
{
    if (
        (wbuf_len > 0 && wbuf == NULL) ||
        (rbuf_len > 0 && rbuf == NULL) ||
        (wbuf_len == 0 && rbuf_len == 0) ||
        (wbuf_len + rbuf_len > I2C_MAX_TRANSFER_SIZE))
    {
        return;
    }

    i2c_dma->writing = (wbuf_len > 0);
    i2c_dma->reading = (rbuf_len > 0);

    if (i2c_dma->writing)
    {
        // Setup commands for each byte to write to the I2C bus.
        for (size_t i = 0; i != wbuf_len; ++i)
        {
            i2c_dma->data_cmds[i] = wbuf[i];
        }

        // The first byte written must be preceded by a start.
        i2c_dma->data_cmds[0] |= I2C_IC_DATA_CMD_RESTART_BITS;
    }

    if (i2c_dma->reading)
    {
        // Setup commands for each byte to read from the I2C bus.
        for (size_t i = 0; i != rbuf_len; ++i)
        {
            i2c_dma->data_cmds[wbuf_len + i] = I2C_IC_DATA_CMD_CMD_BITS;
        }

        // The first byte read must be preceded by a start/restart.
        i2c_dma->data_cmds[wbuf_len] |= I2C_IC_DATA_CMD_RESTART_BITS;
    }

    // The last byte transfered must be followed by a stop.
    i2c_dma->data_cmds[wbuf_len + rbuf_len - 1] |= I2C_IC_DATA_CMD_STOP_BITS;

    // Tell the I2C peripheral the adderss of the device for the transfer.
    i2c_dma_set_target_addr(i2c_dma->i2c, addr);

    i2c_dma->stop_detected = false;
    i2c_dma->abort_detected = false;
    i2c_dma->timeout = false;
    i2c_dma->running = true;

    // Start the I2C transfer on required DMA channels.
    if (i2c_dma->reading)
    {
        i2c_dma_rx_channel_configure(i2c_dma->i2c, i2c_dma->rx_chan, rbuf, rbuf_len);
    }
    i2c_dma_tx_channel_configure(
        i2c_dma->i2c, i2c_dma->tx_chan, i2c_dma->data_cmds, wbuf_len + rbuf_len);
    i2c_dma->timeout_alarm_id = add_alarm_in_us(I2C_TRANSFER_TIMEOUT_MS, timeout_handler, i2c_dma, true);
}
I2CMasterInterface::I2CMasterInterface(uint8_t block, int8_t sda, int8_t scl, uint32_t clock, void (*process_data)(bool running, bool timeout, bool abort_detected, bool stop_detected))
{
    if (sda == -1 || scl == -1)
    {
        i2c = nullptr;
        return;
    }
    i2c = _hardwareBlocks[block];
    printf("i2c: %d %d %d %d\r\n", sda, scl, block, clock);
    i2c_init(i2c, clock);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);

    gpio_pull_up(sda);
    gpio_pull_up(scl);
    if (process_data)
    {
        if (i2c == i2c0)
        {
            i2c_dma = &i2c_dma_list[0];
            i2c_dma->i2c = i2c0;
            i2c_dma->irq_num = I2C0_IRQ;
            i2c_dma->irq_handler = i2c0_dma_irq_handler;
            i2c_dma->process_data = process_data;
        }

        if (i2c == i2c1)
        {
            i2c_dma = &i2c_dma_list[1];
            i2c_dma->i2c = i2c1;
            i2c_dma->irq_num = I2C1_IRQ;
            i2c_dma->irq_handler = i2c1_dma_irq_handler;
            i2c_dma->process_data = process_data;
        }
        i2c_dma->baudrate = clock;
        i2c_dma->sda_gpio = sda;
        i2c_dma->scl_gpio = scl;
        i2c_dma->timeout = false;
        i2c_dma->abort_detected = false;
        i2c_dma->stop_detected = false;
        i2c_dma_init_intern(i2c_dma);
    }
}
I2CMasterInterface::~I2CMasterInterface() {
    irq_set_enabled(i2c_dma->irq_num, false);

    cancel_alarm(i2c_dma->timeout_alarm_id);
    if (i2c_dma->abort_detected || !i2c_dma->stop_detected)
    {
        dma_channel_abort(i2c_dma->tx_chan);
        if (i2c_dma->reading)
        {
            dma_channel_abort(i2c_dma->rx_chan);
        }
    }

    // Free the DMA channels.
    dma_channel_unclaim(i2c_dma->tx_chan);
    if (i2c_dma->reading)
    {
        dma_channel_unclaim(i2c_dma->rx_chan);
    }
    i2c_dma->stop_detected = false;
    i2c_dma->abort_detected = false;
    i2c_dma->process_data = nullptr;
}
void I2CMasterInterface::dmaWriteRead(uint8_t addr,
                                      const uint8_t *wbuf,
                                      size_t wbuf_len,
                                      uint8_t *rbuf,
                                      size_t rbuf_len)
{
    i2c_dma_write_read_internal(i2c_dma, addr, wbuf, wbuf_len, rbuf, rbuf_len);
}
bool I2CMasterInterface::readRegister(uint8_t address, uint8_t pointer, uint8_t length,
                                      uint8_t *data)
{
    return writeTo(address, &pointer, 1, true, true) &&
           readFrom(address, data, length, true);
}
bool I2CMasterInterface::readRegisterRepeatedStart(uint8_t address, uint8_t pointer, uint8_t length,
                                                   uint8_t *data)
{
    return writeTo(address, &pointer, 1, true, false) &&
           readFrom(address, data, length, true);
}
bool I2CMasterInterface::writeRegister(uint8_t address, uint8_t pointer, uint8_t data)
{
    return writeRegister(address, pointer, 1, &data);
}
bool I2CMasterInterface::writeRegister(uint8_t address, uint8_t pointer, uint8_t length,
                                       uint8_t *data)
{
    if (!i2c)
    {
        return false;
    }
    uint8_t data2[length + 1];
    data2[0] = pointer;
    memcpy(data2 + 1, data, length);

    return writeTo(address, data2, length + 1, true, true);
}

bool I2CMasterInterface::readFrom(uint8_t address, uint8_t *data, uint8_t length,
                                  uint8_t sendStop)
{
    if (!i2c)
    {
        return false;
    }
    int ret =
        i2c_read_timeout_per_char_us(i2c, address, data, length, !sendStop, 5000);
    return ret > 0;
}

bool I2CMasterInterface::writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                                 uint8_t sendStop)
{
    if (!i2c)
    {
        return false;
    }
    int ret =
        i2c_write_timeout_per_char_us(i2c, address, data, length, !sendStop, 1000);
    if (ret < 0)
        ret = i2c_write_timeout_per_char_us(i2c, address, data, length, !sendStop, 1000);
    return ret > 0;
}