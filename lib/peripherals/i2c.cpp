#include "i2c.hpp"

#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "secondary_pico.hpp"
static void i2c_dma_next_queued(
    i2c_dma_t *i2c_dma);

static i2c_dma_slot_t *__not_in_flash_func(find_slot)(i2c_dma_t *i2c_dma, uint8_t addr)
{
    for (size_t i = 0; i < I2C_MAX_DEVICES; i++)
    {
        if (i2c_dma->slots[i].in_use && i2c_dma->slots[i].addr == addr)
        {
            return &i2c_dma->slots[i];
        }
    }
    return nullptr;
}

static inline i2c_dma_slot_t *alloc_slot(i2c_dma_t *i2c_dma, uint8_t addr)
{
    i2c_dma_slot_t *free_slot = nullptr;
    for (size_t i = 0; i < I2C_MAX_DEVICES; i++)
    {
        if (i2c_dma->slots[i].in_use && i2c_dma->slots[i].addr == addr)
        {
            return &i2c_dma->slots[i];
        }
        if (!i2c_dma->slots[i].in_use && !free_slot)
        {
            free_slot = &i2c_dma->slots[i];
        }
    }
    if (free_slot)
    {
        free_slot->in_use = true;
        free_slot->addr = addr;
        free_slot->dmaInterface = nullptr;
        free_slot->owner = nullptr;
        free_slot->queuedBy = nullptr;
        free_slot->hasWaitingTransfer = false;
        free_slot->clock = 0;
        memset((void *)&free_slot->waitingTransfer, 0, sizeof(free_slot->waitingTransfer));
        return free_slot;
    }
    return nullptr;
}

// Everything a transfer completing runs is kept in RAM, as these interrupts are frequent and would
// otherwise mostly run from cold flash
static void __not_in_flash_func(process_dma)(i2c_dma_t *i2c_dma)
{
    i2c_dma->processing = true;
    cancel_alarm(i2c_dma->timeout_alarm_id);
    i2c_dma->timeout_alarm_id = 0;
    if (i2c_dma->running)
    {
        i2c_dma->running = false;
        if (i2c_dma->abort_detected || !i2c_dma->stop_detected)
        {
            dma_channel_abort(i2c_dma->tx_chan);
            if (i2c_dma->reading)
            {
                dma_channel_abort(i2c_dma->rx_chan);
            }
        }
    }
    if (i2c_dma->currentSlot >= 0 && i2c_dma->currentSlot < (int)I2C_MAX_DEVICES)
    {
        i2c_dma_slot_t &slot = i2c_dma->slots[i2c_dma->currentSlot];
        if (slot.in_use && slot.dmaInterface)
        {
            slot.dmaInterface->process_data(slot.addr, i2c_dma->running, i2c_dma->timeout, i2c_dma->abort_detected, i2c_dma->stop_detected);
        }
    }
    i2c_dma->timeout = false;
    i2c_dma->abort_detected = false;
    i2c_dma->stop_detected = false;
    i2c_dma->processing = false;
    i2c_dma_next_queued(i2c_dma);
}

static i2c_dma_t i2c_dma_list[2] = {0};
static void __not_in_flash_func(i2c_dma_irq_handler)(i2c_dma_t *i2c_dma)
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

static void __not_in_flash_func(i2c0_dma_irq_handler)(void)
{
    i2c_dma_irq_handler(&i2c_dma_list[0]);
}

static void i2c_dma_pin_open_drain(uint gpio)
{
    gpio_set_function(gpio, GPIO_FUNC_SIO);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_put(gpio, 0);
}
static void __not_in_flash_func(i2c1_dma_irq_handler)(void)
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
    // if the bus is already initialised, then just increase the device count
    if (i2c_dma->device_count)
    {
        i2c_dma->device_count++;
        return PICO_OK;
    }
    for (size_t i = 0; i < I2C_MAX_DEVICES; i++)
    {
        i2c_dma->slots[i].hasWaitingTransfer = false;
        memset((void *)&i2c_dma->slots[i].waitingTransfer, 0, sizeof(i2c_dma->slots[i].waitingTransfer));
    }
    i2c_dma->currentSlot = -1;
    irq_set_enabled(i2c_dma->irq_num, false);

    i2c_dma->stop_detected = false;
    i2c_dma->abort_detected = false;
    // Attempt to unblock a blocked bus. If it can't be unblocked, continue
    // anyway.
    if (i2c_dma_is_blocked(i2c_dma))
    {
        i2c_dma_unblock(i2c_dma);
    }

    if (is_secondary_pico_mode())
    {
        if (i2c_dma->i2c == get_secondary_pico_slave_i2c() || (uint)i2c_dma->sda_gpio == get_secondary_pico_slave_sda() || (uint)i2c_dma->scl_gpio == get_secondary_pico_slave_scl())
        {
            // The bus belongs to the secondary pico link. Count the device so deinit stays balanced,
            // but leave it without DMA channels so nothing is ever sent on it
            i2c_dma->tx_chan = -1;
            i2c_dma->rx_chan = -1;
            i2c_dma->device_count = 1;
            return PICO_ERROR_GENERIC;
        }
    }

    i2c_init(i2c_dma->i2c, i2c_dma->baudrate);

    i2c_dma->baud_pending = false;

    gpio_set_function(i2c_dma->sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(i2c_dma->scl_gpio, GPIO_FUNC_I2C);
    gpio_pull_up(i2c_dma->sda_gpio);
    gpio_pull_up(i2c_dma->scl_gpio);
    i2c_dma->pins_claimed = true;

    i2c_get_hw(i2c_dma->i2c)->intr_mask =
        I2C_IC_INTR_MASK_M_STOP_DET_BITS |
        I2C_IC_INTR_MASK_M_TX_ABRT_BITS;

    irq_set_exclusive_handler(i2c_dma->irq_num, i2c_dma->irq_handler);
    irq_set_enabled(i2c_dma->irq_num, true);
    i2c_dma->tx_chan = dma_claim_unused_channel(false);
    i2c_dma->rx_chan = dma_claim_unused_channel(false);
    i2c_dma->device_count = 1;
    if (i2c_dma->tx_chan < 0 || i2c_dma->rx_chan < 0)
    {
        // Out of channels. Keep the device counted, transfers just won't run without both channels
        printf("i2c: no free DMA channels\r\n");
        if (i2c_dma->tx_chan >= 0)
        {
            dma_channel_unclaim(i2c_dma->tx_chan);
        }
        if (i2c_dma->rx_chan >= 0)
        {
            dma_channel_unclaim(i2c_dma->rx_chan);
        }
        i2c_dma->tx_chan = -1;
        i2c_dma->rx_chan = -1;
        return PICO_ERROR_GENERIC;
    }

    return PICO_OK;
}

// Points the bus at the transfer's device, and moves it to a new clock if one is waiting, as both
// need the block disabled. Matches i2c_set_baudrate, with its sums done beforehand in
// i2c_dma_update_baudrate as this runs from interrupts
static void __not_in_flash_func(i2c_dma_set_target_addr)(i2c_dma_t *i2c_dma, uint8_t addr)
{
    i2c_hw_t *hw = i2c_get_hw(i2c_dma->i2c);
    hw->enable = 0;
    if (i2c_dma->baud_pending)
    {
        i2c_dma->baud_pending = false;
        hw_write_masked(&hw->con, I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB, I2C_IC_CON_SPEED_BITS);
        hw->fs_scl_hcnt = i2c_dma->fs_scl_hcnt;
        hw->fs_scl_lcnt = i2c_dma->fs_scl_lcnt;
        hw->fs_spklen = i2c_dma->fs_spklen;
        hw_write_masked(&hw->sda_hold, i2c_dma->sda_tx_hold << I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_LSB,
                        I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_BITS);
    }
    hw->tar = addr;
    hw->enable = 1;
}

// Moves the bus to the slowest clock its devices asked for, as one that can't keep up with a faster
// clock may answer to the wrong address or hold SDA in the middle of another device's transfer
static void i2c_dma_update_baudrate(i2c_dma_t *i2c_dma)
{
    uint32_t clock = 0;
    for (size_t i = 0; i < I2C_MAX_DEVICES; i++)
    {
        const i2c_dma_slot_t &slot = i2c_dma->slots[i];
        if (slot.in_use && slot.clock && (!clock || slot.clock < clock))
        {
            clock = slot.clock;
        }
    }
    if (!clock || clock == i2c_dma->baudrate)
    {
        return;
    }
    const uint32_t freq_in = clock_get_hz(clk_sys);
    const uint32_t period = (freq_in + clock / 2) / clock;
    const uint32_t lcnt = period * 3 / 5;
    const uint32_t hcnt = period - lcnt;
    const uint32_t hold = clock < 1000000 ? ((freq_in * 3) / 10000000) + 1 : ((freq_in * 3) / 25000000) + 1;
    printf("i2c: bus clock %u -> %u\r\n", (unsigned)i2c_dma->baudrate, (unsigned)clock);
    const uint32_t irq_status = save_and_disable_interrupts();
    i2c_dma->fs_scl_hcnt = hcnt;
    i2c_dma->fs_scl_lcnt = lcnt;
    i2c_dma->fs_spklen = lcnt < 16 ? 1 : lcnt / 16;
    i2c_dma->sda_tx_hold = hold;
    i2c_dma->baudrate = clock;
    i2c_dma->baud_pending = true;
    restore_interrupts(irq_status);
}

// Hands the bus pins back once nothing is on the bus, so a config whose gates picked a device on other
// pins doesn't leave them on the I2C block too. Pins something else has taken over are left alone
static void i2c_dma_release_pins(i2c_dma_t *i2c_dma)
{
    if (!i2c_dma->pins_claimed)
    {
        return;
    }
    i2c_dma->pins_claimed = false;
    const uint pins[] = {i2c_dma->sda_gpio, i2c_dma->scl_gpio};
    for (uint gpio : pins)
    {
        if (gpio_get_function(gpio) == GPIO_FUNC_I2C)
        {
            gpio_disable_pulls(gpio);
            gpio_set_function(gpio, GPIO_FUNC_NULL);
        }
    }
}

static void __not_in_flash_func(i2c_dma_tx_channel_configure)(
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

static void __not_in_flash_func(i2c_dma_rx_channel_configure)(
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

int64_t __not_in_flash_func(timeout_handler)(alarm_id_t, void *user_data)
{
    i2c_dma_t *i2c_dma = (i2c_dma_t *)user_data;
    i2c_dma->timeout_alarm_id = 0;
    i2c_dma->timeout = true;
    process_dma(i2c_dma);
    return 0;
}
static void __not_in_flash_func(i2c_dma_next_queued)(
    i2c_dma_t *i2c_dma)
{
    i2c_dma->queuing = true;
    if (i2c_dma->running || i2c_dma->processing || i2c_dma->tx_chan < 0 || i2c_dma->rx_chan < 0)
    {
        return;
    }
    volatile i2c_dma_transfer_t *transfer = nullptr;
    int next_slot = -1;
    int start_slot = (i2c_dma->currentSlot >= 0) ? (i2c_dma->currentSlot + 1) : 0;
    for (size_t count = 0; count < I2C_MAX_DEVICES; count++)
    {
        int idx = (start_slot + count) % I2C_MAX_DEVICES;
        if (i2c_dma->running || i2c_dma->processing)
        {
            return;
        }
        if (i2c_dma->slots[idx].in_use && i2c_dma->slots[idx].hasWaitingTransfer)
        {
            i2c_dma->running = true;
            next_slot = idx;
            transfer = &i2c_dma->slots[idx].waitingTransfer;
            i2c_dma->runningFor = i2c_dma->slots[idx].queuedBy;
            i2c_dma->slots[idx].hasWaitingTransfer = false;
            break;
        }
    }
    if (next_slot < 0 || !transfer)
    {
        return;
    }
    i2c_dma->currentSlot = next_slot;
    uint8_t addr = transfer->addr;
    const uint8_t *wbuf = transfer->wbuf;
    size_t wbuf_len = transfer->wbuf_len;
    uint8_t *rbuf = transfer->rbuf;
    size_t rbuf_len = transfer->rbuf_len;
    uint16_t *data_cmds = transfer->cmds ? transfer->cmds : i2c_dma->data_cmds;
    i2c_dma->writing = (wbuf_len > 0);
    i2c_dma->reading = (rbuf_len > 0);

    if (i2c_dma->writing)
    {
        // Setup commands for each byte to write to the I2C bus.
        for (size_t i = 0; i != wbuf_len; ++i)
        {
            data_cmds[i] = wbuf[i];
        }

        // The first byte written must be preceded by a start.
        data_cmds[0] |= I2C_IC_DATA_CMD_RESTART_BITS;
    }

    if (i2c_dma->reading)
    {
        // Setup commands for each byte to read from the I2C bus.
        for (size_t i = 0; i != rbuf_len; ++i)
        {
            data_cmds[wbuf_len + i] = I2C_IC_DATA_CMD_CMD_BITS;
        }

        // The first byte read must be preceded by a start/restart.
        data_cmds[wbuf_len] |= I2C_IC_DATA_CMD_RESTART_BITS;
    }

    // The last byte transfered must be followed by a stop.
    data_cmds[wbuf_len + rbuf_len - 1] |= I2C_IC_DATA_CMD_STOP_BITS;

    // Tell the I2C peripheral the adderss of the device for the transfer.
    i2c_dma_set_target_addr(i2c_dma, addr);

    i2c_dma->stop_detected = false;
    i2c_dma->abort_detected = false;
    i2c_dma->timeout = false;

    // Start the I2C transfer on required DMA channels.
    if (i2c_dma->reading)
    {
        i2c_dma_rx_channel_configure(i2c_dma->i2c, i2c_dma->rx_chan, rbuf, rbuf_len);
    }
    i2c_dma_tx_channel_configure(
        i2c_dma->i2c, i2c_dma->tx_chan, data_cmds, wbuf_len + rbuf_len);
    const uint32_t baudrate = i2c_dma->baudrate ? i2c_dma->baudrate : 100000;
    const uint64_t byte_us = (9ull * 1000000ull + baudrate - 1) / baudrate + I2C_TRANSFER_STRETCH_US_PER_BYTE;
    const uint64_t timeout_us = 2 * byte_us * (wbuf_len + rbuf_len) + I2C_TRANSFER_TIMEOUT_MARGIN_US;
    i2c_dma->timeout_alarm_id = add_alarm_in_us(timeout_us, timeout_handler, i2c_dma, true);
}

void I2CMasterInterface::tick()
{
    if (!i2c_dma)
    {
        return;
    }

    if (i2c_dma->running || i2c_dma->processing)
    {
        return;
    }
}

void I2CMasterInterface::dmaInit(uint8_t addr, I2CDMAInterface *dmaInterface)
{
    if (i2c == nullptr)
    {
        return;
    }
    if (i2c == i2c0)
    {
        i2c_dma = &i2c_dma_list[0];
        i2c_dma->i2c = i2c0;
        i2c_dma->irq_num = I2C0_IRQ;
        i2c_dma->irq_handler = i2c0_dma_irq_handler;
    }

    if (i2c == i2c1)
    {
        i2c_dma = &i2c_dma_list[1];
        i2c_dma->i2c = i2c1;
        i2c_dma->irq_num = I2C1_IRQ;
        i2c_dma->irq_handler = i2c1_dma_irq_handler;
    }
    // The block can only be on one set of pins. Taking it over would cut off the devices already on it
    if (i2c_dma->device_count && (i2c_dma->sda_gpio != m_sda || i2c_dma->scl_gpio != m_scl))
    {
        printf("i2c: %02x wants GP%d / GP%d, but the bus is already on GP%d / GP%d\r\n", addr, m_sda, m_scl,
               i2c_dma->sda_gpio, i2c_dma->scl_gpio);
        i2c_dma = nullptr;
        return;
    }
    // device_count counts slots, so a deinit only gives back what an init took. An address that
    // already has a slot just gets a new owner
    i2c_dma_slot_t *slot = find_slot(i2c_dma, addr);
    const bool new_slot = !slot;
    if (!slot)
    {
        slot = alloc_slot(i2c_dma, addr);
    }
    if (!slot)
    {
        printf("i2c: no free slot for %02x\r\n", addr);
        return;
    }
    slot->dmaInterface = dmaInterface;
    slot->owner = this;
    slot->clock = m_clock;
    if (!new_slot)
    {
        i2c_dma_update_baudrate(i2c_dma);
        return;
    }
    if (i2c_dma->device_count)
    {
        i2c_dma_init_intern(i2c_dma);
        i2c_dma_update_baudrate(i2c_dma);
        return;
    }
    // The first device on the bus. Only set while the bus is idle, these are live state while it runs
    i2c_dma->baudrate = m_clock;
    i2c_dma->sda_gpio = m_sda;
    i2c_dma->scl_gpio = m_scl;
    i2c_dma->timeout = false;
    i2c_dma->abort_detected = false;
    i2c_dma->stop_detected = false;
    i2c_dma_init_intern(i2c_dma);
}
I2CMasterInterface::I2CMasterInterface(uint8_t block, int8_t sda, int8_t scl, uint32_t clock) : m_sda(sda), m_scl(scl), m_clock(clock)
{
    if (sda == -1 || scl == -1)
    {
        i2c = nullptr;
        return;
    }
    i2c = _hardwareBlocks[block];
    printf("i2c: %d %d %d %d\r\n", sda, scl, block, clock);
    if (is_secondary_pico_mode())
    {
        if (i2c == get_secondary_pico_slave_i2c() || (sda >= 0 && (uint)sda == get_secondary_pico_slave_sda()) || (scl >= 0 && (uint)scl == get_secondary_pico_slave_scl()))
        {
            return;
        }
    }
    // The bus is set up by the first device's dmaInit. Setting the block up here would put it on every
    // device's pins, before dmaInit can refuse the ones on different pins to the bus
}
I2CMasterInterface::~I2CMasterInterface()
{
}

// A transfer still running for a slot keeps its DMA writing into the device's buffers, which are freed
// once the device's end() returns. Let it finish, or stop it if it doesn't finish promptly
#define I2C_DEINIT_WAIT_US 5000
static void i2c_dma_drain(i2c_dma_t *i2c_dma, const void *master)
{
    const uint32_t start = time_us_32();
    while (i2c_dma->running && i2c_dma->runningFor == master && time_us_32() - start < I2C_DEINIT_WAIT_US)
    {
        tight_loop_contents();
    }
    uint32_t irq_status = save_and_disable_interrupts();
    if (i2c_dma->running && i2c_dma->runningFor == master)
    {
        printf("i2c: stopping a transfer that didn't finish\r\n");
        cancel_alarm(i2c_dma->timeout_alarm_id);
        i2c_dma->timeout_alarm_id = 0;
        dma_channel_abort(i2c_dma->tx_chan);
        dma_channel_abort(i2c_dma->rx_chan);
        i2c_hw_t *hw = i2c_get_hw(i2c_dma->i2c);
        hw->enable |= I2C_IC_ENABLE_ABORT_BITS;
        const uint32_t abort_start = time_us_32();
        while ((hw->enable & I2C_IC_ENABLE_ABORT_BITS) && time_us_32() - abort_start < 1000)
        {
            tight_loop_contents();
        }
        // Drop the abort and stop it raised, they belong to the transfer being thrown away
        (void)hw->clr_intr;
        i2c_dma->running = false;
        i2c_dma->stop_detected = false;
        i2c_dma->abort_detected = false;
        i2c_dma->timeout = false;
        i2c_dma->currentSlot = -1;
        i2c_dma->runningFor = nullptr;
    }
    restore_interrupts(irq_status);
}

void I2CMasterInterface::dmaDeinit(uint8_t addr)
{
    if (!i2c_dma)
    {
        return;
    }
    i2c_dma_slot_t *slot = find_slot(i2c_dma, addr);
    if (!slot)
    {
        return;
    }
    // A transfer this queued still points at its buffers, drop it
    if (slot->queuedBy == this)
    {
        slot->hasWaitingTransfer = false;
    }
    const bool owner = slot->owner == this;
    if (owner)
    {
        // Stop this slot's completion reaching the device, and anything more being queued for it
        slot->dmaInterface = nullptr;
        slot->hasWaitingTransfer = false;
    }
    i2c_dma_drain(i2c_dma, this);
    if (!owner)
    {
        // A replacement device has taken the address over, the slot is its now
        return;
    }
    slot->in_use = false;
    memset((void *)&slot->waitingTransfer, 0, sizeof(slot->waitingTransfer));
    if (i2c_dma->device_count)
    {
        i2c_dma->device_count--;
    }
    if (i2c_dma->device_count)
    {
        // The device going may have been the slowest
        i2c_dma_update_baudrate(i2c_dma);
        // Another device may have queued a transfer while this one was draining
        i2c_dma_next_queued(i2c_dma);
    }
    else
    {
        irq_set_enabled(i2c_dma->irq_num, false);

        cancel_alarm(i2c_dma->timeout_alarm_id);
        i2c_dma->timeout_alarm_id = 0;
        // Both channels are claimed together, so free both whatever the last transfer used
        if (i2c_dma->tx_chan >= 0)
        {
            dma_channel_abort(i2c_dma->tx_chan);
            dma_channel_unclaim(i2c_dma->tx_chan);
        }
        if (i2c_dma->rx_chan >= 0)
        {
            dma_channel_abort(i2c_dma->rx_chan);
            dma_channel_unclaim(i2c_dma->rx_chan);
        }
        i2c_dma->tx_chan = -1;
        i2c_dma->rx_chan = -1;
        i2c_dma->stop_detected = false;
        i2c_dma->abort_detected = false;
        i2c_dma->running = false;
        i2c_dma->processing = false;
        i2c_dma->timeout = false;
        i2c_dma->currentSlot = -1;
        i2c_dma->baud_pending = false;
        i2c_dma_release_pins(i2c_dma);
    }
}
bool __not_in_flash_func(I2CMasterInterface::dmaWriteRead)(uint8_t addr,
                                      const uint8_t *wbuf,
                                      size_t wbuf_len,
                                      uint8_t *rbuf,
                                      size_t rbuf_len)
{
    return dmaWriteRead(addr, wbuf, wbuf_len, rbuf, rbuf_len, nullptr);
}

bool __not_in_flash_func(I2CMasterInterface::dmaWriteRead)(uint8_t addr,
                                      const uint8_t *wbuf,
                                      size_t wbuf_len,
                                      uint8_t *rbuf,
                                      size_t rbuf_len,
                                      uint16_t *cmds)
{
    if (!i2c_dma || (!cmds && wbuf_len + rbuf_len > I2C_MAX_TRANSFER_SIZE))
    {
        return false;
    }
    // Without DMA channels (the bus is the secondary pico link, or there were none free) nothing queued
    // would ever run
    if (i2c_dma->tx_chan < 0 || i2c_dma->rx_chan < 0)
    {
        return false;
    }
    i2c_dma_slot_t *slot = find_slot(i2c_dma, addr);
    if (!slot)
    {
        return false;
    }
    slot->waitingTransfer.cmds = cmds;
    slot->waitingTransfer.addr = addr;
    slot->waitingTransfer.wbuf_len = wbuf_len;
    slot->waitingTransfer.rbuf_len = rbuf_len;
    slot->waitingTransfer.wbuf = wbuf;
    slot->waitingTransfer.rbuf = rbuf;
    slot->queuedBy = this;
    slot->hasWaitingTransfer = true;
    i2c_dma_next_queued(i2c_dma);
    return true;
}