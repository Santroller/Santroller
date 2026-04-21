#include "wii_extension.hpp"

#include <string.h>

#include "devices/midi.hpp"
#include "main.hpp"
#include <cmath>
#include "utils.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

bool WiiExtension::verifyData(const uint8_t *dataIn, uint8_t dataSize)
{
    uint8_t orCheck = 0x00;  // Check if data is zeroed (bad connection)
    uint8_t andCheck = 0xFF; // Check if data is maxed (bad init)

    for (int i = 0; i < dataSize; i++)
    {
        orCheck |= dataIn[i];
        andCheck &= dataIn[i];
    }

    if (orCheck == 0x00 || andCheck == 0xFF)
    {
        return false; // No data or bad data
    }

    return true;
}
int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    i2c_dma_t *i2c_dma = (i2c_dma_t *)user_data;
    if (i2c_dma->process_data)
    {
        i2c_dma->process_data();
    }
    return 0;
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
        if (i2c_dma->running)
        {
            i2c_dma->abort_detected = true;
        }
    }

    if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS)
    {
        // Transfer complete.
        i2c_get_hw(i2c_dma->i2c)->clr_stop_det;
        if (i2c_dma->running)
        {
            i2c_dma->stop_detected = true;
            if (i2c_dma->process_data)
            {
                i2c_dma->process_data();
            }
        }
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
    if (i2c_dma->process_data && i2c_dma->running)
    {
        i2c_dma->process_data();
    }
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
static WiiExtension *wiiinstances[2];
void process_data_0()
{
    if (wiiinstances[0])
    {
        wiiinstances[0]->processData();
    }
}
void process_data_1()
{
    if (wiiinstances[1])
    {
        wiiinstances[1]->processData();
    }
}

void WiiExtension::processData()
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
    if (i2c_dma->timeout || i2c_dma->abort_detected)
    {
        // printf("connection lost %d %d %d!\r\n", status, i2c_dma->timeout, i2c_dma->abort_detected);
        status = WII_INIT_FINISH_ENC;
        mType = WiiExtType::WiiNoExtension;
        i2c_dma->timeout = false;
        i2c_dma->abort_detected = false;
        i2c_dma->stop_detected = false;
        i2c_dma->restart_alarm_id = add_alarm_in_ms(500, restart_handler, i2c_dma, true);
        return;
    }
    // printf("status: %d %d %d %d\r\n", status, i2c_dma->abort_detected, i2c_dma->stop_detected, i2c_dma->timeout);
    if (i2c_dma->stop_detected)
    {
        switch (status)
        {
        case WII_INIT_FINISH_ENC:
            status = WII_INIT_FB_0;
            break;
        case WII_INIT_FB_0:
            status = WII_INIT_READ_ID_WRITE_PTR;
            break;
        case WII_INIT_READ_ID_WRITE_PTR:
            status = WII_INIT_READ_ID_READ;
            break;
        case WII_INIT_READ_ID_READ:
            if (verifyData(bufferRx, WII_ID_LEN))
            {
                mType = static_cast<WiiExtType>(bufferRx[0] << 8 | bufferRx[5]);
                printf("found wii ext: %d\r\n", mType);
                wiiPointer = 0;
                wiiBytes = 6;
                hiRes = false;
                s_box = 0;
                if (mType == WiiUbisoftDrawsomeTablet)
                {
                    status = WII_INIT_DRAWSOME;
                }
                else if (mType == WiiExtType::WiiClassicController ||
                         mType == WiiExtType::WiiClassicControllerPro)
                {
                    status = WII_INIT_CLASSIC_0;
                }
                else
                {
                    status = WII_INIT_READ_DATA_WRITE_PTR;
                }
                if (mType == WiiExtType::WiiTaikoNoTatsujinController)
                {
                    // We can cheat a little with these controllers, as most of the bytes that
                    // get read back are constant. Hence we start at 0x5 instead of 0x0.
                    wiiPointer = 5;
                    wiiBytes = 1;
                }
            }
            break;
        case WII_INIT_DRAWSOME:
            status = WII_INIT_READ_DATA_WRITE_PTR;
            break;
        case WII_INIT_CLASSIC_0:
            status = WII_INIT_CLASSIC_1;
            break;
        case WII_INIT_CLASSIC_1:
            status = WII_INIT_CLASSIC_2;
            break;
        case WII_INIT_CLASSIC_2:
            status = WII_INIT_CLASSIC_READ_ID_WRITE_PTR;
            break;
        case WII_INIT_CLASSIC_READ_ID_WRITE_PTR:
            status = WII_INIT_CLASSIC_READ_ID_READ;
            break;
        case WII_INIT_CLASSIC_READ_ID_READ:
            if (bufferRx[4] == WII_HIGHRES_MODE)
            {
                hiRes = true;
                wiiBytes = 8;
            }
            else
            {
                hiRes = false;
            }
            status = WII_INIT_READ_DATA_WRITE_PTR;
            break;
        case WII_INIT_READ_DATA_WRITE_PTR:
            status = WII_INIT_READ_DATA_READ;
            break;
        case WII_INIT_READ_DATA_READ:
        {
            uint8_t orCheck = 0x00;
            for (int i = 0; i < wiiBytes; i++)
            {
                orCheck |= bufferRx[i];
            }
            if (orCheck == 0)
            {
                status = WII_INIT_ENABLE_ENC_0;
            }
            else
            {
                status = WII_INPUTS_WRITE_PTR;
            }
            break;
        }
        case WII_INIT_ENABLE_ENC_0:
            status = WII_INIT_ENABLE_ENC_1;
            break;
        case WII_INIT_ENABLE_ENC_1:
            status = WII_INIT_ENABLE_ENC_2;
            break;
        case WII_INIT_ENABLE_ENC_2:
            status = WII_INIT_ENABLE_ENC_3;
            break;
        case WII_INIT_ENABLE_ENC_3:
            status = WII_INIT_ENC_READ_ID_WRITE_PTR;
            break;
        case WII_INIT_ENC_READ_ID_WRITE_PTR:
            status = WII_INIT_ENC_READ_ID_READ;
            break;
        case WII_INIT_ENC_READ_ID_READ:
        {
            s_box = FIRST_PARTY_SBOX;
            if (bufferRx[3] != 0xFF)
            {
                s_box = THIRD_PARTY_SBOX;
            }
            status = WII_INPUTS_WRITE_PTR;
            break;
        }
        case WII_INPUTS_WRITE_PTR:
            status = WII_INPUTS_READ;
            break;
        case WII_INPUTS_READ:
            status = WII_INPUTS_WRITE_PTR;
            if (verifyData(bufferRx, wiiBytes))
            {
                memcpy(mBuffer, bufferRx, wiiBytes);
            }
            break;
        }
    }
    if (delayNext)
    {
        delayNext = false;
        i2c_dma->restart_alarm_id = add_alarm_in_us(170, restart_handler, i2c_dma, true);
        return;
    }
    delayNext = true;
    switch (status)
    {
    case WII_INIT_FINISH_ENC:
        bufferTx[0] = WII_ENCRYPTION_STATE_ID;
        bufferTx[1] = WII_ENCRYPTION_FINISH_ID;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_FB_0:
        bufferTx[0] = 0xFB;
        bufferTx[1] = 0x00;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_ID_READ:
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INIT_DRAWSOME:
        bufferTx[0] = 0xFB;
        bufferTx[1] = 0x01;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_0:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_1:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_2:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_READ_ID_READ:
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INIT_READ_DATA_WRITE_PTR:
        bufferTx[0] = wiiPointer;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_DATA_READ:
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, nullptr, 0, bufferRx, wiiBytes);
        break;
    case WII_INIT_ENABLE_ENC_0:
        bufferTx[0] = WII_ENCRYPTION_STATE_ID;
        bufferTx[1] = WII_ENCRYPTION_ENABLE_ID;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_1:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID;
        memset(bufferTx + 1, 0, 6);
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 7, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_2:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID_2;
        memset(bufferTx + 1, 0, 6);
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 7, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_3:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID_3;
        memset(bufferTx + 1, 0, 4);
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 5, nullptr, 0);
        break;
    case WII_INIT_ENC_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_ENC_READ_ID_READ:
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INPUTS_WRITE_PTR:
        bufferTx[0] = wiiPointer;
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INPUTS_READ:
        i2c_dma_write_read_internal(i2c_dma, WII_ADDR, nullptr, 0, bufferRx, wiiBytes);
        break;
    }
}

WiiExtension::WiiExtension(MidiDevice *midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock) : mInterface(block, sda, scl, clock), mFound(false), m_device(midiDevice)
{
    printf("wiiext init\r\n");
    if (sda == -1 || scl == -1)
    {
        i2c = nullptr;
        return;
    }
    i2c = _hardwareBlocks[block];

    if (i2c == i2c0)
    {
        i2c_dma = &i2c_dma_list[0];
        i2c_dma->i2c = i2c0;
        i2c_dma->irq_num = I2C0_IRQ;
        i2c_dma->irq_handler = i2c0_dma_irq_handler;
        i2c_dma->process_data = process_data_0;
        wiiinstances[0] = this;
    }

    if (i2c == i2c1)
    {
        i2c_dma = &i2c_dma_list[1];
        i2c_dma->i2c = i2c1;
        i2c_dma->irq_num = I2C1_IRQ;
        i2c_dma->irq_handler = i2c1_dma_irq_handler;
        i2c_dma->process_data = process_data_1;
        wiiinstances[1] = this;
    }
    i2c_dma->status = I2C_NONE;
    i2c_dma->baudrate = clock;
    i2c_dma->sda_gpio = sda;
    i2c_dma->scl_gpio = scl;
    i2c_dma->timeout = false;
    i2c_dma->abort_detected = false;
    i2c_dma->stop_detected = false;
    i2c_dma_init_intern(i2c_dma);
    delayNext = false;
    processData();
}
WiiExtension::~WiiExtension()
{
    printf("wiiext deinit\r\n");
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
    if (i2c == i2c0)
    {
        wiiinstances[0] = nullptr;
    }
    if (i2c == i2c1)
    {
        wiiinstances[1] = nullptr;
    }
}

void WiiExtension::setEuphoriaLed(bool state)
{
    nextEuphoriaLedState = state;
    ledUpdated = true;
}
void WiiExtension::tick()
{
    // return;
    // if (micros() - lastTick > 750)
    // {
    //     lastTick = micros();
    // }
    // else
    // {
    //     return;
    // }
    // static uint8_t wiiData[8];
    // memset(wiiData, 0, sizeof(wiiData));
    // if (mType == WiiExtType::WiiNotInitialised ||
    //     mType == WiiExtType::WiiNoExtension ||
    //     !mInterface.readRegisterSlow(WII_ADDR, wiiPointer, wiiBytes, wiiData) ||
    //     !verifyData(wiiData, wiiBytes))
    // {
    //     if (mFound)
    //     {
    //         packetIssueCount++;
    //         if (packetIssueCount < 10)
    //         {
    //             return;
    //         }
    //     }
    //     packetIssueCount = 0;
    //     mFound = false;
    //     initWiiExt();
    //     return;
    // }
    // packetIssueCount = 0;
    // // decrypt if encryption is enabled
    // if (s_box)
    // {
    //     for (int i = 0; i < 8; i++)
    //     {
    //         wiiData[i] = (uint8_t)(((wiiData[i] ^ s_box) + s_box) & 0xFF);
    //     }
    // }
    // mFound = true;
    // // Update the led if it changes
    // if (mType == WiiExtType::WiiDjHeroTurntable && ledUpdated)
    // {
    //     ledUpdated = false;
    //     // encrypt if encryption is enabled
    //     uint8_t state = nextEuphoriaLedState ? 1 : 0;
    //     if (s_box)
    //     {
    //         state = (state - s_box) ^ s_box;
    //     }
    //     mInterface.writeRegister(WII_ADDR, WII_DJ_EUPHORIA, state);
    // }
    // if (mType == WiiExtType::WiiGuitarHeroDrums)
    // {
    //     // https://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_World_Tour_(Wii)_Drums
    //     uint8_t velocity = ((wiiData[4] & 0b00000001) |
    //                         ((wiiData[4] & 0b10000000) >> 6) |
    //                         ((wiiData[3] & 0b00000001) << 2) |
    //                         ((wiiData[2] & 0b00000001) << 3) |
    //                         ((wiiData[3] & (0b11100000)) >> 1));
    //     uint8_t note = (wiiData[2] >> 1) & 0x7f;
    //     uint8_t channel = ((~wiiData[3]) >> 1) & 0xF;
    //     velocity = 0x7F - velocity;
    //     note = 0x7F - note;
    //     if (velocity || note)
    //     {
    //         // Sadly the wii drums don't include the status byte, so we have to make one up.
    //         uint8_t packet[] = {0, MIDI_CIN_NOTE_ON << 4 | channel, note, velocity};
    //         m_device->processMidiData(packet, sizeof(packet));
    //     }
    // }
    // if (mType == WiiExtType::WiiGuitarHeroGuitar)
    // {
    //     auto lastTapWii = (wiiData[2] & 0x1f);

    //     // GH3 guitars set this bit, while WT and GH5 guitars do not
    //     if (!hasTapBar)
    //     {
    //         if (lastTapWii == 0x0F)
    //         {
    //             hasTapBar = true;
    //         }
    //         lastTapWii = 0;
    //     }
    // }
    // memcpy(mBuffer, wiiData, sizeof(wiiData));
}

uint16_t atanAxis(uint16_t y, uint16_t x)
{
    float theta = std::atan2(y, x);
    if (theta < 0)
    {
        theta += M_PI * 2;
    }
    return theta * 65535 / M_PI;
}

uint16_t WiiExtension::readAxis(proto_WiiAxisType type)
{
    switch (mType)
    {
    case WiiExtType::WiiClassicControllerPro:
    case WiiExtType::WiiClassicController:
    {
        if (hiRes)
        {
            switch (type)
            {
            case WiiAxisType::WiiAxisClassicLeftStickX:
                return (mBuffer[0]) << 8;
            case WiiAxisType::WiiAxisClassicLeftStickY:
                return ((mBuffer[2]) << 8);
            case WiiAxisType::WiiAxisClassicRightStickX:
                return (mBuffer[1]) << 8;
            case WiiAxisType::WiiAxisClassicRightStickY:
                return ((mBuffer[3]) << 8);
            case WiiAxisType::WiiAxisClassicLeftTrigger:
                // compared to a conventional controller, zl is where the trigger is so we need to swap
                if (mType == WiiClassicControllerPro)
                {

                    return readButton(WiiButtonClassicZl) ? 65535 : 0;
                }
                return mBuffer[4] << 8;
            case WiiAxisType::WiiAxisClassicRightTrigger:
                if (mType == WiiClassicControllerPro)
                {

                    return readButton(WiiButtonClassicZr) ? 65535 : 0;
                }
                return mBuffer[5] << 8;
            default:
                return 0;
            }
        }
        else
        {

            switch (type)
            {
            case WiiAxisType::WiiAxisClassicLeftStickX:
                return ((mBuffer[0] & 0x3f)) << 10;
            case WiiAxisType::WiiAxisClassicLeftStickY:
                return (((mBuffer[1] & 0x3f)) << 10);
            case WiiAxisType::WiiAxisClassicRightStickX:
                return ((((mBuffer[0] & 0xc0) >> 3) | ((mBuffer[1] & 0xc0) >> 5) | (mBuffer[2] >> 7))) << 11;
            case WiiAxisType::WiiAxisClassicRightStickY:
                return (((mBuffer[2] & 0x1f)) << 11);
            case WiiAxisType::WiiAxisClassicLeftTrigger:
                if (mType == WiiClassicControllerPro)
                {

                    return readButton(WiiButtonClassicZl) ? 65535 : 0;
                }
                return (((mBuffer[3] & 0xE0) >> 5 | (mBuffer[2] & 0x60) >> 2)) << 11;
            case WiiAxisType::WiiAxisClassicRightTrigger:
                if (mType == WiiClassicControllerPro)
                {

                    return readButton(WiiButtonClassicZr) ? 65535 : 0;
                }
                return (mBuffer[3] & 0x1f) << 11;
            default:
                return 0;
            }
        }
        break;
    }
    case WiiExtType::WiiDjHeroTurntable:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisDjCrossfadeSlider:
            return ((mBuffer[2] & 0x1E) >> 1) << 12;
        case WiiAxisType::WiiAxisDjEffectDial:
            return (((mBuffer[3] & 0xE0) >> 5 | (mBuffer[2] & 0x60) >> 2)) << 11;
        case WiiAxisType::WiiAxisDjStickX:
            return ((mBuffer[0] & 0x3F)) << 10;
        case WiiAxisType::WiiAxisDjStickY:
            return ((mBuffer[1] & 0x3F)) << 10;
        case WiiAxisType::WiiAxisDjTurntableLeft:
            return ((mBuffer[4] & 1) ? 32 + (0x1F - (mBuffer[3] & 0x1F)) : 32 - (mBuffer[3] & 0x1F)) << 10;
        case WiiAxisType::WiiAxisDjTurntableRight:
        {
            uint8_t rtt = (mBuffer[2] & 0x80) >> 7 | (mBuffer[1] & 0xC0) >> 5 | (mBuffer[0] & 0xC0) >> 3;
            return ((mBuffer[2] & 1) ? 32 + (0x1F - rtt) : 32 - rtt);
        }
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiUbisoftDrawsomeTablet:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisDrawsomePenPressure:
            return (mBuffer[4] | (mBuffer[5] & 0x0f) << 8);
        case WiiAxisType::WiiAxisDrawsomePenX:
            return (mBuffer[0] | mBuffer[1] << 8);
        case WiiAxisType::WiiAxisDrawsomePenY:
            return (mBuffer[2] | mBuffer[3] << 8);
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiThqUdrawTablet:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisUDrawPenPressure:
            return (mBuffer[3]);
        case WiiAxisType::WiiAxisUDrawPenX:
            return ((mBuffer[2] & 0x0f) << 8) | mBuffer[0];
        case WiiAxisType::WiiAxisUDrawPenY:
            return ((mBuffer[2] & 0xf0) << 4) | mBuffer[1];
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiGuitarHeroGuitar:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisGuitarJoystickX:
            return ((mBuffer[0] & 0x3f)) << 10;
        case WiiAxisType::WiiAxisGuitarJoystickY:
            return ((mBuffer[1] & 0x3f)) << 10;
        case WiiAxisType::WiiAxisGuitarWhammy:
            return (mBuffer[3] & 0x1f) << 11;
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiGuitarHeroDrums:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisDrumJoystickX:
            return ((mBuffer[0] & 0x3f)) << 10;
        case WiiAxisType::WiiAxisDrumJoystickY:
            return ((mBuffer[1] & 0x3f)) << 10;
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiNunchuk:
    {
        switch (type)
        {
        case WiiAxisType::WiiAxisNunchukAccelerationX:
            return ((mBuffer[2] << 2) | ((mBuffer[5] & 0xC0) >> 6)) << 6;
        case WiiAxisType::WiiAxisNunchukAccelerationY:
            return ((mBuffer[3] << 2) | ((mBuffer[5] & 0x30) >> 4)) << 6;
        case WiiAxisType::WiiAxisNunchukAccelerationZ:
            return ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)) << 6;
        case WiiAxisType::WiiAxisNunchukRotationPitch:
            return (std::atan2(((mBuffer[3] << 2) | ((mBuffer[5] & 0x30) >> 4)) - 511.0, ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)) - 511.0) * 32767 / M_PI) + 32767;
        case WiiAxisType::WiiAxisNunchukRotationRoll:
            return (-std::atan2(((mBuffer[2] << 2) | ((mBuffer[5] & 0xC0) >> 6)) - 511.0, ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)) - 511.0) * 32767 / M_PI) + 32767;
        case WiiAxisType::WiiAxisNunchukStickX:
            return (mBuffer[0]) << 8;
        case WiiAxisType::WiiAxisNunchukStickY:
            return (mBuffer[1]) << 8;
        default:
            return 0;
        }
    }
    default:
        return 0;
    }
    return 0;
}
bool WiiExtension::readButton(proto_WiiButtonType type)
{
    auto lastTap = hasTapBar ? (mBuffer[2] & 0x1f) : 0x0F;
    auto wiiButtonsLow = ~mBuffer[4];
    auto wiiButtonsHigh = ~mBuffer[5];
    if (hiRes)
    {
        wiiButtonsLow = ~mBuffer[6];
        wiiButtonsHigh = ~mBuffer[7];
    }
    switch (mType)
    {
    case WiiExtType::WiiClassicControllerPro:
    case WiiExtType::WiiClassicController:
    {
        switch (type)
        {
        case WiiButtonClassicRt:
            return ((wiiButtonsLow) & (1 << 1));
        case WiiButtonClassicPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonClassicHome:
            return ((wiiButtonsLow) & (1 << 3));
        case WiiButtonClassicMinus:
            return ((wiiButtonsLow) & (1 << 4));
        case WiiButtonClassicLt:
            return ((wiiButtonsLow) & (1 << 5));
        case WiiButtonClassicDPadDown:
            return ((wiiButtonsLow) & (1 << 6));
        case WiiButtonClassicDPadRight:
            return ((wiiButtonsLow) & (1 << 7));
        case WiiButtonClassicDPadUp:
            return ((wiiButtonsHigh) & (1 << 0));
        case WiiButtonClassicDPadLeft:
            return ((wiiButtonsHigh) & (1 << 1));
        case WiiButtonClassicZr:
            // compared to a conventional controller, zr is where the trigger is so we need to swap
            if (mType == WiiClassicControllerPro)
            {
                return ((wiiButtonsLow) & (1 << 1));
            }
            return ((wiiButtonsHigh) & (1 << 2));
        case WiiButtonClassicX:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonClassicA:
            return ((wiiButtonsHigh) & (1 << 4));
        case WiiButtonClassicY:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonClassicB:
            return ((wiiButtonsHigh) & (1 << 6));
        case WiiButtonClassicZl:
            if (mType == WiiClassicControllerPro)
            {
                return ((wiiButtonsLow) & (1 << 5));
            }
            return ((wiiButtonsHigh) & (1 << 7));
        default:
            return 0;
        }
        break;
    }

    case WiiExtType::WiiDjHeroTurntable:
    {
        switch (type)
        {
        case WiiButtonDjHeroPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonDjHeroMinus:
            return ((wiiButtonsLow) & (1 << 4));
        case WiiButtonDjHeroLeftBlue:
            return ((wiiButtonsHigh) & (1 << 7));
        case WiiButtonDjHeroLeftRed:
            return ((wiiButtonsLow) & (1 << 5));
        case WiiButtonDjHeroLeftGreen:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonDjHeroRightGreen:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonDjHeroRightRed:
            return ((wiiButtonsLow) & (1 << 1));
        case WiiButtonDjHeroRightBlue:
            return ((wiiButtonsHigh) & (1 << 2));
        case WiiButtonDjHeroEuphoria:
            return ((wiiButtonsHigh) & (1 << 4));
        default:
            return 0;
        }
        break;
    }

    case WiiExtType::WiiGuitarHeroDrums:
    {
        switch (type)
        {
        case WiiButtonDrumPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonDrumMinus:
            return ((wiiButtonsLow) & (1 << 4));
        default:
            return 0;
        }
        break;
    }

    case WiiExtType::WiiGuitarHeroGuitar:
    {
        switch (type)
        {
        case WiiButtonGuitarPlus:
            return ((wiiButtonsLow) & (1 << 2));
        case WiiButtonGuitarMinus:
            return ((wiiButtonsLow) & (1 << 4));
        case WiiButtonGuitarStrumDown:
            return ((wiiButtonsLow) & (1 << 6));
        case WiiButtonGuitarStrumUp:
            return ((wiiButtonsHigh) & (1 << 0));
        case WiiButtonGuitarGreen:
            return ((wiiButtonsHigh) & (1 << 4));
        case WiiButtonGuitarRed:
            return ((wiiButtonsHigh) & (1 << 6));
        case WiiButtonGuitarYellow:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonGuitarBlue:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonGuitarOrange:
            return ((wiiButtonsHigh) & (1 << 7));
        case WiiButtonGuitarTapGreen:
            return lastTap < 0x0A;
        case WiiButtonGuitarTapRed:
            return lastTap != 0x0F && lastTap < 0x12 && lastTap >= 0x0A;
        case WiiButtonGuitarTapYellow:
            return lastTap < 0x17 && lastTap >= 0x12;
        case WiiButtonGuitarTapBlue:
            return lastTap < 0x1F && lastTap >= 0x17;
        case WiiButtonGuitarTapOrange:
            return lastTap >= 0x1F;
        case WiiButtonGuitarPedal:
            return ((wiiButtonsHigh) & (1 << 2));
        default:
            return 0;
        }
        break;
    }
    case WiiExtType::WiiNunchuk:
    {
        switch (type)
        {
        case WiiButtonNunchukC:
            return ((wiiButtonsHigh) & (1 << 1));
        case WiiButtonNunchukZ:
            return ((wiiButtonsHigh) & (1 << 0));
        default:
            return 0;
        }
        break;
    }

    case WiiExtType::WiiTaikoNoTatsujinController:
    {
        switch (type)
        {
        case WiiButtonTaTaConRightDrumRim:
            return ((~mBuffer[0]) & (1 << 3));
        case WiiButtonTaTaConRightDrumCenter:
            return ((~mBuffer[0]) & (1 << 4));
        case WiiButtonTaTaConLeftDrumRim:
            return ((~mBuffer[0]) & (1 << 5));
        case WiiButtonTaTaConLeftDrumCenter:
            return ((~mBuffer[0]) & (1 << 6));
        default:
            return 0;
        }
        break;
    }

    case WiiExtType::WiiUbisoftDrawsomeTablet:
    {
        switch (type)
        {
        case WiiButtonUDrawPenButton1:
            return ((wiiButtonsHigh) & (1 << 0));
        case WiiButtonUDrawPenButton2:
            return ((wiiButtonsHigh) & (1 << 1));
        case WiiButtonUDrawPenClick:
            return ((~wiiButtonsHigh) & (1 << 2));
        default:
            return 0;
        }
        break;
    }
    default:
        return 0;
    }
    return false;
}