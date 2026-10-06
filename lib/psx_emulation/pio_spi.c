#include "pio_spi.h"
#include "psx_spi_protocol.h"
#include "pio_spi.pio.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include <string.h>
#include <stdio.h>
#include <pico/time.h>

uint fixPio(PIO pio, pio_program_t program, int sck_pin)
{
    // Santroller 1 let us put clock on any pin
    // because of this, we need to encode the pin into the pio instructions
    // which is easiest done by just replacing the instructions
    uint16_t data[32];
    memcpy(data, program.instructions, program.length * sizeof(uint16_t));
    uint16_t prevClk0 = pio_encode_wait_gpio(false, PIN_SCK_PLACEHOLDER);
    uint16_t prevClk1 = pio_encode_wait_gpio(true, PIN_SCK_PLACEHOLDER);
    uint16_t newClk0 = pio_encode_wait_gpio(false, sck_pin);
    uint16_t newClk1 = pio_encode_wait_gpio(true, sck_pin);
    for (int i = 0; i < program.length; i++)
    {
        if (program.instructions[i] == prevClk0)
        {
            data[i] = newClk0;
        }
        if (program.instructions[i] == prevClk1)
        {
            data[i] = newClk1;
        }
    }
    program.instructions = data;
    return pio_add_program(pio, &program);
}

pio_spi_t pio_spi[2];

#define PSX_SPI_WATCHDOG_TIMEOUT_MS 1000

static void inline __attribute__((always_inline)) psx_spi_watchdog_touch(pio_spi_t *spi)
{
    spi->watchdog_last_activity_ms = to_ms_since_boot(get_absolute_time());
    spi->watchdog_active = true;
}

static void inline __attribute__((always_inline)) psx_spi_watchdog_reset(pio_spi_t *spi)
{
    // A DualShock that stops hearing the host falls back to Digital Mode.
    spi->protocol.configMode = false;
    spi->protocol.config_responses[0x05][2] = 0;
    memcpy(spi->protocol.config_responses[0x01], init_resp_41_digital, 6);
    spi->protocol.report_mask[0] = 0x03;
    spi->protocol.report_mask[1] = 0;
    spi->protocol.report_mask[2] = 0;
    spi->protocol.rumble_small = 0;
    spi->protocol.rumble_large = 0;
    spi->protocol.locked = false;
    spi->watchdog_active = false;
}

void pio_spi_watchdog_tick(pio_spi_t *spi)
{
    if (!spi || !spi->allocated || !spi->watchdog_active)
        return;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((uint32_t)(now - spi->watchdog_last_activity_ms) >= PSX_SPI_WATCHDOG_TIMEOUT_MS)
        psx_spi_watchdog_reset(spi);

#if PSX_SPI_DEBUG_LOGGING
    if (!spi->timing_dumped && (spi->timing_prepare_idx || spi->timing_send_idx))
    {
        dump_timing_trace(spi);
        dump_config_index_trace(spi);
        spi->timing_dumped = true;
    }
    while (spi->transaction_idx_read != spi->transaction_idx_write)
    {
        uint8_t idx = spi->transaction_idx_read;
        uint8_t cmd = spi->transaction_test[idx][1];

        // 0x42 is polled continuously. Only print the first 0x42 in a
        // consecutive run; the next non-0x42 command will be printed normally.
        static uint8_t last_logged_cmd = 0xFF;
        if (cmd != 0x42 || last_logged_cmd != 0x42)
        {
            printf("SPI transaction: t=%lu us (+%lu us) TX ",
                   (unsigned long)spi->transaction_test_us[idx],
                   (unsigned long)(idx == 0 ? 0 : spi->transaction_test_us[idx] - spi->transaction_test_us[(idx - 1) & 0x07]));
            for (int i = 0; i < 8; i++)
                printf("%02X ", spi->transaction_test[idx][i]);
            printf(" RX ");
            for (int i = 0; i < spi->transaction_test_rx_len[idx]; i++)
                printf("%02X ", spi->transaction_test_rx[idx][i]);
            printf(" | state(after cmd): config=%d analog=%d len=%u mask=%02X %02X %02X\r\n",
                   spi->transaction_test_config[idx],
                   spi->transaction_test_analog[idx],
                   spi->transaction_test_len[idx],
                   spi->transaction_test_mask[idx][0],
                   spi->transaction_test_mask[idx][1],
                   spi->transaction_test_mask[idx][2]);
            if (spi->transaction_test_config_duration_us[idx])
                printf(" | config_duration=%lu us",
                       (unsigned long)spi->transaction_test_config_duration_us[idx]);
            printf("\r\n");
            last_logged_cmd = cmd;
        }

        spi->transaction_idx_read = (spi->transaction_idx_read + 1) & 0x07;
    }
    // Selector captures happen asynchronously from the transaction trace, so
    // dump them after draining the transaction ring rather than at the first
    // getReportFormat() call.
    if (spi->config_index_trace_idx)
    {
        dump_config_index_trace(spi);
        spi->config_index_trace_idx = 0;
    }
#endif
}

static void setup_cs_sm(PIO pio, uint sm, int cipo_pin, int cs_pin, uint *offset)
{
    *offset = pio_add_program(pio, &spi_cs_loop_program);
    pio_sm_config c = spi_cs_loop_program_get_default_config(*offset);
    sm_config_set_in_pins(&c, cs_pin);
    sm_config_set_sideset_pins(&c, cipo_pin);
    pio_sm_set_consecutive_pindirs(pio, sm, cipo_pin, 1, false);

    pio_sm_init(pio, sm, *offset, &c);
}

static void setup_read_initial_sm(PIO pio, uint sm, int copi_pin, int ack_pin, uint offset)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + spi_combined_loop_offset_initial_check, offset + spi_combined_loop_wrap);
    sm_config_set_in_pins(&c, copi_pin);

    sm_config_set_in_shift(
        &c,
        true,  // Shift-to-right = false (i.e. shift to left)
        false, // Autopush enabled
        8      // Autopush threshold = 8
    );

    pio_sm_init(pio, sm, offset, &c);
}

static void setup_combined_sm(PIO pio, uint sm, int cipo_pin, int copi_pin, int sck_pin, int ack_pin, uint *offset)
{
    *offset = fixPio(pio, spi_combined_loop_program, sck_pin);
    pio_sm_config c = spi_combined_loop_program_get_default_config(*offset);
    sm_config_set_in_pins(&c, copi_pin);
    sm_config_set_out_pins(&c, cipo_pin, 1);
    sm_config_set_sideset_pins(&c, ack_pin);
    pio_gpio_init(pio, cipo_pin);
    pio_gpio_init(pio, ack_pin);
    pio_sm_set_consecutive_pindirs(pio, sm, ack_pin, 1, false);

    pio_sm_put(pio, sm, 0xFF);
    pio_sm_exec_wait_blocking(pio, sm, pio_encode_pull(false, true));
    pio_sm_exec_wait_blocking(pio, sm, pio_encode_mov(pio_x, pio_osr));

    sm_config_set_out_shift(
        &c,
        true,  // Shift-to-right = false (i.e. shift to left)
        false, // Autopush enabled
        8      // Autopush threshold = 8
    );

    sm_config_set_in_shift(
        &c,
        true,  // Shift-to-right = false (i.e. shift to left)
        false, // Autopush enabled
        8      // Autopush threshold = 8
    );
    pio_sm_init(pio, sm, *offset, &c);
}

static void configure_read_dma(PIO pio, uint combined_sm, uint *channel)
{
    uint dma_channel = dma_claim_unused_channel(true);

    dma_channel_config channel_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&channel_config, DMA_SIZE_8);
    channel_config_set_dreq(&channel_config, pio_get_dreq(pio, combined_sm, false));
    channel_config_set_read_increment(&channel_config, false);
    channel_config_set_write_increment(&channel_config, true);

    dma_channel_configure(
        dma_channel,
        &channel_config,
        NULL,                                    // dst
        ((uint8_t *)&pio->rxf[combined_sm]) + 3, // src
        1,                                       // transfer count
        false);

    dma_channel_set_irq0_enabled(dma_channel, false);
    dma_channel_set_irq1_enabled(dma_channel, false);

    *channel = dma_channel;
}

static void configure_write_dma(PIO pio, uint combined_sm, uint *channel)
{
    uint dma_channel = dma_claim_unused_channel(true);

    dma_channel_config channel_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&channel_config, DMA_SIZE_8);
    channel_config_set_dreq(&channel_config, pio_get_dreq(pio, combined_sm, true));
    channel_config_set_read_increment(&channel_config, true);
    channel_config_set_write_increment(&channel_config, false);

    dma_channel_configure(
        dma_channel,
        &channel_config,
        &pio->txf[combined_sm], // dst
        NULL,                   // src
        1,                      // transfer count
        false);

    dma_channel_set_irq0_enabled(dma_channel, false);
    dma_channel_set_irq1_enabled(dma_channel, false);

    *channel = dma_channel;
}

inline static int safe_fifo_rx_wait_for_finish(pio_hw_t *pio, uint sm, uint channel)
{
    int wooble = 0;
    while (!pio_sm_is_rx_fifo_empty(pio, sm) && (dma_channel_hw_addr(channel)->transfer_count != 0))
    {
        wooble++;
        if (wooble > 1000)
        {
// This happens if too many bytes are written to buffer
#if PSX_SPI_DEBUG_LOGGING
            printf("DMA Overrun\n");
#endif
            return 1;
        }
    }
    return 0;
}

typedef struct pio_spi_read_info_t
{
    uint8_t num_bytes_read;
    uint8_t num_bytes_written;
    uint num_bits_transacted;
} pio_spi_read_info_t;

static inline __attribute__((always_inline)) void format_next_response(pio_spi_t *spi)
{
    uint8_t response_len = 2;

    if (spi->protocol.configMode)
    {
        response_len += 4;
    }
    else
    {
        // raw_report holds buttons active high; the PSX wants them active low.
        // Inverting here means the wire is always right no matter where raw_report came from.
        if (!is_analog(spi))
        {
            spi->response_buf[response_len++] = ~spi->raw_report[0];
            spi->response_buf[response_len++] = ~spi->raw_report[1];
        }
        else
        {
            uint8_t mask = spi->protocol.report_mask[0];

            if (mask & 0x01)
                spi->response_buf[response_len++] = ~spi->raw_report[0];
            if (mask & 0x02)
                spi->response_buf[response_len++] = ~spi->raw_report[1];
            if (mask & 0x04)
                spi->response_buf[response_len++] = spi->raw_report[2];
            if (mask & 0x08)
                spi->response_buf[response_len++] = spi->raw_report[3];
            if (mask & 0x10)
                spi->response_buf[response_len++] = spi->raw_report[4];
            if (mask & 0x20)
                spi->response_buf[response_len++] = spi->raw_report[5];
            if (mask & 0x40)
                spi->response_buf[response_len++] = spi->raw_report[6];
            if (mask & 0x80)
                spi->response_buf[response_len++] = spi->raw_report[7];

            mask = spi->protocol.report_mask[1];

            if (mask & 0x01)
                spi->response_buf[response_len++] = spi->raw_report[8];
            if (mask & 0x02)
                spi->response_buf[response_len++] = spi->raw_report[9];
            if (mask & 0x04)
                spi->response_buf[response_len++] = spi->raw_report[10];
            if (mask & 0x08)
                spi->response_buf[response_len++] = spi->raw_report[11];
            if (mask & 0x10)
                spi->response_buf[response_len++] = spi->raw_report[12];
            if (mask & 0x20)
                spi->response_buf[response_len++] = spi->raw_report[13];
            if (mask & 0x40)
                spi->response_buf[response_len++] = spi->raw_report[14];
            if (mask & 0x80)
                spi->response_buf[response_len++] = spi->raw_report[15];

            mask = spi->protocol.report_mask[2];

            if (mask & 0x01)
                spi->response_buf[response_len++] = spi->raw_report[16];
            if (mask & 0x02)
                spi->response_buf[response_len++] = spi->raw_report[17];
        }
    }
    spi->response_buf[0] = PSX_SPI_RESPONSE_HEADER(&spi->protocol, response_len - 2);
    spi->response_buf[1] = 0x5A;
    spi->response_len = response_len;
}

static void inline __attribute__((always_inline)) stop_loops(pio_spi_t *spi)
{
    pio_set_sm_mask_enabled(spi->pio, spi->startstop_mask, false); // Stop state machines

    pio_sm_clear_fifos(spi->pio, spi->config.initial_sm);
    pio_restart_sm_mask(spi->pio, spi->startstop_mask); // Restart state machines to known states
    hw_set_bits(&spi->pio->irq, 0xFF);                  // Clear IRQs
    safe_fifo_rx_wait_for_finish(spi->pio, spi->config.combined_sm, spi->channel_read);
    pio_sm_clear_fifos(spi->pio, spi->config.combined_sm);
    pio_sm_clear_fifos(spi->pio, spi->config.initial_sm);
    // push the initial 0xFF and header
    pio_sm_put(spi->pio, spi->config.combined_sm, 0xFF);

    format_next_response(spi);
#if PSX_SPI_DEBUG_LOGGING
    // Capture the exact protocol state used to arm this transaction. Do not
    // printf() here: this path is timing-critical and the trace must not alter it.
    uint8_t tp = spi->timing_prepare_idx;
    uint8_t tn = (uint8_t)((tp + 1) & 0x0F);
    if (tn == spi->timing_send_idx)
    {
        spi->timing_prepare_overflow = true;
    }
    else
    {
        spi->timing_prepare_us[tp] = time_us_32();
        spi->timing_prepare_len[tp] = spi->response_len - 2;
        spi->timing_prepare_config[tp] = spi->protocol.configMode;
        spi->timing_prepare_analog[tp] = is_analog(spi);
        spi->timing_prepare_header[tp] = spi->response_buf[0];
        spi->timing_prepare_idx = tn;
    }
#endif
    // we always send the same data when not in config mode, so theres no need to read the command!
    irq_set_enabled(PIO_IRQ_NUM(spi->pio, 1), spi->protocol.configMode);
    // The response buffer is the canonical descriptor for the transaction
    // we are about to arm. Do not rebuild the header here from protocol state.
    pio_sm_put(spi->pio, spi->config.combined_sm, spi->response_buf[0]);
    pio_sm_put(spi->pio, spi->config.combined_sm, spi->response_buf[1]);

    pio_sm_exec_wait_blocking(spi->pio, spi->config.combined_sm, pio_encode_set(pio_y, 7));
    pio_sm_exec_wait_blocking(spi->pio, spi->config.combined_sm, pio_encode_jmp(spi->offset_combined));
    pio_sm_exec_wait_blocking(spi->pio, spi->config.initial_sm, pio_encode_set(pio_y, (8 * 3) - 1));
    pio_sm_exec_wait_blocking(spi->pio, spi->config.initial_sm, pio_encode_jmp(spi->offset_combined));
    // 3 bytes for the header, -1, and then we don't ack the last byte, leaving only the packet size + 1
    pio_sm_exec_wait_blocking(spi->pio, spi->config.combined_sm,
                              pio_encode_set(pio_x, (spi->response_len - 2) + 1));

    uint irq_wait = pio_encode_wait_irq(1, false, 7);
    pio_sm_exec(spi->pio, spi->config.combined_sm, irq_wait);
    pio_sm_exec(spi->pio, spi->config.initial_sm, irq_wait);
}

static void inline __attribute__((always_inline)) prepare_for_next(pio_spi_t *spi)
{
    // Read FIFO count of write buffer
    stop_loops(spi);

    dma_channel_abort(spi->channel_read);
    dma_channel_abort(spi->channel_write);

    pio_spi_provide_read_buffer(spi, spi->dma_buf, 32);
    // In config mode we need to know the command, so we don't arm the write DMA yet.
    // otherwise we know it and can immediately arm the write DMA.
    if (!spi->protocol.configMode)
    {
        pio_spi_provide_write_buffer(spi, &spi->response_buf[2], spi->response_len - 2);
    }

    pio_enable_sm_mask_in_sync(spi->pio, spi->startstop_mask);
}

static void inline __attribute__((always_inline)) pio_irq(pio_spi_t *spi)
{
    io_rw_32 irqs = spi->pio->irq;
    if (irqs & (1u << 2))
    {
#if PSX_SPI_DEBUG_LOGGING
        if (spi->dma_buf[1] == 0x43 || spi->protocol.configMode)
        {
            spi->protocol.has_new_cmd = true;
            spi->protocol.cmd_id++;
            spi->dma_buf_test[spi->read_idx_write][0] = spi->dma_buf[0];
            spi->dma_buf_test[spi->read_idx_write][1] = spi->dma_buf[1];
            spi->dma_buf_test[spi->read_idx_write][2] = spi->dma_buf[2];
            spi->dma_buf_test[spi->read_idx_write][3] = spi->dma_buf[3];
            spi->dma_buf_test[spi->read_idx_write][4] = spi->dma_buf[4];
            spi->dma_buf_test[spi->read_idx_write][5] = spi->dma_buf[5];
            spi->dma_buf_test[spi->read_idx_write][6] = spi->dma_buf[6];
            spi->dma_buf_test[spi->read_idx_write][7] = spi->dma_buf[7];
            memcpy((void *)spi->response_test[spi->read_idx_write],
                   (const void *)spi->response_buf,
                   sizeof(spi->response_test[spi->read_idx_write]));
            spi->response_test_len[spi->read_idx_write] = spi->response_len;
            spi->read_idx_write = (spi->read_idx_write + 1) & 0x07; // Assuming a buffer size of 8
        }
        // Snapshot the completed command immediately; dma_buf is reused for the next DMA transfer.
        memcpy((void *)spi->transaction_buf, (const void *)spi->dma_buf, sizeof(spi->transaction_buf));

        // Diagnostic trace ring. Keep this entirely out of the normal IRQ path
        // when logging is disabled.
        uint8_t tw = spi->transaction_idx_write;
        uint8_t tn = (uint8_t)((tw + 1) & 0x07);
        bool trace_stored = false;
        if (tn == spi->transaction_idx_read)
        {
            spi->transaction_trace_overflow = true;
        }
        else
        {
            memcpy((void *)spi->transaction_test[tw], (const void *)spi->dma_buf, 8);
            memcpy((void *)spi->transaction_test_rx[tw], (const void *)spi->response_buf, 32);
            spi->transaction_test_rx_len[tw] = spi->response_len;
            spi->transaction_idx_write = tn;
            trace_stored = true;
        }

        // Save the response that was actually queued for this transaction before
        // processing the command, since command processing changes the state for the next one.
        memcpy((void *)spi->last_response_buf, (const void *)spi->response_buf, sizeof(spi->last_response_buf));
        spi->last_response_len = spi->response_len;
        spi->has_new_transaction = true;

#endif
        psx_spi_watchdog_touch(spi);
        PSX_SPI_PROCESS_COMMAND(&spi->protocol, spi->dma_buf);

#if PSX_SPI_DEBUG_LOGGING
        // Snapshot the state produced by this command into the same trace slot.
        // Do not read live protocol state when dumping: several later transactions
        // may have completed by the time getReportFormat() runs.
        if (trace_stored)
        {
            uint32_t trace_us = time_us_32();
            spi->transaction_test_us[tw] = trace_us;

            // Measure the complete config-mode interval from the command
            // which enters config mode through the command which exits it.
            // This is the useful number for determining whether the tester
            // should visibly remain blank.
            if (spi->dma_buf[1] == 0x43 && spi->dma_buf[3] == 0x01)
            {
                spi->config_trace_start_us = trace_us;
                spi->transaction_test_config_duration_us[tw] = 0;
            }
            else if (spi->dma_buf[1] == 0x43 && spi->dma_buf[3] == 0x00 &&
                     spi->config_trace_start_us != 0)
            {
                spi->transaction_test_config_duration_us[tw] =
                    trace_us - spi->config_trace_start_us;
                spi->config_trace_start_us = 0;
            }
            else
            {
                spi->transaction_test_config_duration_us[tw] = 0;
            }

            spi->transaction_test_config[tw] = spi->protocol.configMode;
            spi->transaction_test_analog[tw] = is_analog(spi);
            spi->transaction_test_len[tw] = psx_spi_current_report_len(&spi->protocol);
            spi->transaction_test_mask[tw][0] = spi->protocol.report_mask[0];
            spi->transaction_test_mask[tw][1] = spi->protocol.report_mask[1];
            spi->transaction_test_mask[tw][2] = spi->protocol.report_mask[2];
        }

#endif

        prepare_for_next(spi);
        pio_interrupt_clear(spi->pio, 1);
    }
}

static void __time_critical_func(pio_irq_0)(void)
{
    pio_irq(&pio_spi[0]);
}

static void __time_critical_func(pio_irq_1)(void)
{
    pio_irq(&pio_spi[1]);
}

static void inline __attribute__((always_inline)) pio_data_irq(pio_spi_t *spi)
{
    pio_spi_config_t *cfg = &spi->config;
    uint8_t reg;

    spi->pio->rxf[cfg->initial_sm];
    reg = spi->pio->rxf[cfg->initial_sm] >> 24;
    if (spi->protocol.configMode)
    {
#if PSX_SPI_DEBUG_LOGGING
        spi->has_new_write = true;
        spi->write_id++;
        spi->dma_buf_test2[spi->write_idx_write][0] = reg;
        memcpy(spi->dma_buf_test2[spi->write_idx_write] + 1,
               spi->protocol.config_responses[reg - 0x40], 6);
        spi->write_idx_write = (spi->write_idx_write + 1) & 0x07;
        memcpy((void *)&spi->response_buf[2],
               (const void *)spi->protocol.config_responses[reg - 0x40], 6);
        spi->response_len = 8;
#endif
        pio_spi_provide_write_buffer(
            spi, spi->protocol.config_responses[reg - 0x40],
            dma_encode_transfer_count(6));
    }

    hw_set_bits(&spi->pio->irq, (1u << 0));
}

static void __time_critical_func(pio_data_irq_0)(void)
{
    pio_data_irq(&pio_spi[0]);
}

static void __time_critical_func(pio_data_irq_1)(void)
{
    pio_data_irq(&pio_spi[1]);
}

pio_spi_t *pio_spi_init(const pio_spi_config_t *config)
{
    assert(config);
    assert(config->pio_idx == 0 || config->pio_idx == 1);
    pio_spi_t *spi = &pio_spi[config->pio_idx];
    assert(!spi->allocated);
    memset(spi, 0, sizeof(*spi));
    spi->allocated = true;

    spi->config = *config;
    spi->pio = config->pio_idx == 0 ? pio0 : pio1;
    spi->type = config->type;
    PSX_SPI_PROTOCOL_INIT(&spi->protocol, spi->type);
    spi->protocol.cmd_id = 0;
    spi->watchdog_active = false;
    spi->watchdog_last_activity_ms = 0;
    spi->write_id = 0;
    spi->read_idx_read = 0;
    spi->read_idx_write = 0;
    spi->write_idx_read = 0;
    spi->write_idx_write = 0;
    gpio_init(config->cs_pin);
    gpio_init(config->sck_pin);
    gpio_init(config->copi_pin);
    gpio_init(config->cipo_pin);
    gpio_init(config->ack_pin);

    gpio_set_dir(config->cs_pin, GPIO_IN);
    gpio_set_dir(config->sck_pin, GPIO_IN);
    gpio_set_dir(config->copi_pin, GPIO_IN);
    gpio_set_dir(config->cipo_pin, GPIO_IN);
    gpio_set_dir(config->ack_pin, GPIO_OUT);
    gpio_set_pulls(config->cs_pin, false, false);
    gpio_set_pulls(config->sck_pin, false, false);
    gpio_set_pulls(config->ack_pin, false, false);
    gpio_set_pulls(config->copi_pin, false, false);
    gpio_set_pulls(config->cipo_pin, false, false);
    spi->config.combined_sm = pio_claim_unused_sm(spi->pio, true);
    spi->config.initial_sm = pio_claim_unused_sm(spi->pio, true);
    spi->config.cs_sm = pio_claim_unused_sm(spi->pio, true);

    setup_combined_sm(spi->pio, spi->config.combined_sm, spi->config.cipo_pin, spi->config.copi_pin, spi->config.sck_pin, spi->config.ack_pin, &spi->offset_combined);
    configure_write_dma(spi->pio, spi->config.combined_sm, &spi->channel_write);

    configure_read_dma(spi->pio, spi->config.combined_sm, &spi->channel_read);

    setup_read_initial_sm(spi->pio, spi->config.initial_sm, spi->config.copi_pin, spi->config.ack_pin, spi->offset_combined);

    setup_cs_sm(spi->pio, spi->config.cs_sm, spi->config.cipo_pin, spi->config.cs_pin, &spi->offset_cs);

    pio_set_irq1_source_enabled(spi->pio, pis_interrupt0, true);

    pio_set_irq0_source_enabled(spi->pio, pis_interrupt2, true);

    if (config->pio_idx == 0)
    {
        irq_set_exclusive_handler(PIO0_IRQ_0, pio_irq_0);
        irq_set_enabled(PIO0_IRQ_0, true);
        irq_set_exclusive_handler(PIO0_IRQ_1, pio_data_irq_0);
        irq_set_enabled(PIO0_IRQ_1, true);
    }
    else
    {
        irq_set_exclusive_handler(PIO1_IRQ_0, pio_irq_1);
        irq_set_enabled(PIO1_IRQ_0, true);
        irq_set_exclusive_handler(PIO1_IRQ_1, pio_data_irq_1);
        irq_set_enabled(PIO1_IRQ_1, true);
    }

    spi->startstop_mask = (1u << spi->config.combined_sm) | (1u << spi->config.initial_sm);

    prepare_for_next(spi);

    return spi;
}

void pio_spi_free(pio_spi_t *spi)
{
    assert(spi);
    assert(spi->allocated);
    assert(spi->config.pio_idx == 0 || spi->config.pio_idx == 1);
    assert(spi == &pio_spi[spi->config.pio_idx]);
    pio_spi_stop(spi);
    pio_sm_unclaim(spi->pio, spi->config.cs_sm);
    pio_sm_unclaim(spi->pio, spi->config.initial_sm);
    pio_sm_unclaim(spi->pio, spi->config.combined_sm);
    pio_remove_program(spi->pio, &spi_cs_loop_program, spi->offset_cs);
    pio_remove_program(spi->pio, &spi_combined_loop_program, spi->offset_combined);
    dma_channel_unclaim(spi->channel_read);
    dma_channel_unclaim(spi->channel_write);
    pio_spi[spi->config.pio_idx].allocated = false;
}

void pio_spi_start(const pio_spi_t *spi)
{
    assert(spi);
    assert(spi->allocated);
    assert(spi->config.pio_idx == 0 || spi->config.pio_idx == 1);
    assert(spi == &pio_spi[spi->config.pio_idx]);

    pio_enable_sm_mask_in_sync(spi->pio, (1u << spi->config.cs_sm) | spi->startstop_mask);
}

void pio_spi_stop(pio_spi_t *spi)
{
    assert(spi);
    assert(spi->allocated);
    assert(spi->config.pio_idx == 0 || spi->config.pio_idx == 1);
    assert(spi == &pio_spi[spi->config.pio_idx]);

    pio_sm_set_enabled(spi->pio, spi->config.cs_sm, false);
    pio_sm_exec(spi->pio, spi->config.cs_sm, pio_encode_jmp(spi->offset_cs));
    pio_sm_set_consecutive_pindirs(spi->pio, spi->config.cs_sm, spi->config.cipo_pin, 1, false);

    stop_loops(spi);
    dma_channel_abort(spi->channel_read);
    dma_channel_abort(spi->channel_write);
}