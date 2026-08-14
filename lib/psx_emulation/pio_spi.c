#include "pio_spi.h"
#include "pio_spi.pio.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include <string.h>
#include <stdio.h>

uint fixPio(PIO pio, pio_program_t program, int sck_pin)
{
    uint16_t data[10];
    memcpy(data, program.instructions, program.length * sizeof(uint16_t));
    uint16_t prevClk0 = pio_encode_wait_gpio(false, PIN_SCK);
    uint16_t prevClk1 = pio_encode_wait_gpio(true, PIN_SCK);
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

static void setup_combined_sm(PIO pio, uint sm, int cipo_pin, int copi_pin, int sck_pin, int ack_pin, uint8_t default_write_value, uint *offset)
{
    *offset = fixPio(pio, spi_combined_loop_program, sck_pin);
    pio_sm_config c = spi_combined_loop_program_get_default_config(*offset);
    sm_config_set_in_pins(&c, copi_pin);
    sm_config_set_out_pins(&c, cipo_pin, 1);
    sm_config_set_sideset_pins(&c, ack_pin);
    pio_gpio_init(pio, cipo_pin);
    pio_gpio_init(pio, ack_pin);
    sm_config_set_clkdiv_int_frac(&c, 70, 0x00);
    pio_sm_set_consecutive_pindirs(pio, sm, ack_pin, 1, false);

    pio_sm_put(pio, sm, default_write_value);
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
            printf("DMA Overrun\n");
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

inline static uint read_register(PIO pio, uint sm, enum pio_src_dest reg)
{
    uint move_isr = pio_encode_mov(pio_isr, reg);
    pio_sm_exec_wait_blocking(pio, sm, move_isr);
    uint push = pio_encode_push(false, false);
    pio_sm_exec_wait_blocking(pio, sm, push);
    return pio_sm_get(pio, sm);
}

static pio_spi_read_info_t __time_critical_func(stop_loops)(pio_spi_t *spi)
{
    pio_set_sm_mask_enabled(spi->pio, spi->startstop_mask, false); // Stop state machines
    // Read FIFO count of write buffer
    uint test = read_register(spi->pio, spi->config.combined_sm, pio_x);
    
    uint left_in_write_queue = pio_sm_get_tx_fifo_level(spi->pio, spi->config.combined_sm);
    pio_sm_clear_fifos(spi->pio, spi->config.initial_sm);
    uint bits_transacted = read_register(spi->pio, spi->config.initial_sm, pio_y);
    pio_restart_sm_mask(spi->pio, spi->startstop_mask); // Restart state machines to known states
    hw_set_bits(&spi->pio->irq, 0xFF);                  // Clear IRQs
    safe_fifo_rx_wait_for_finish(spi->pio, spi->config.combined_sm, spi->channel_read);
    pio_sm_clear_fifos(spi->pio, spi->config.combined_sm);
    pio_sm_clear_fifos(spi->pio, spi->config.initial_sm);
    // push the initial 0xFF and header
    pio_sm_put(spi->pio, spi->config.combined_sm, 0xFF);
    pio_sm_put(spi->pio, spi->config.combined_sm, spi->header);
    pio_sm_put(spi->pio, spi->config.combined_sm, 0x5A);
    pio_sm_exec_wait_blocking(spi->pio, spi->config.combined_sm, pio_encode_set(pio_y, 7));
    pio_sm_exec_wait_blocking(spi->pio, spi->config.combined_sm, pio_encode_jmp(spi->offset_combined));
    // 3 bytes for header
    pio_sm_exec_wait_blocking(spi->pio, spi->config.initial_sm, pio_encode_set(pio_y, (8 * 3) - 1));
    pio_sm_exec_wait_blocking(spi->pio, spi->config.initial_sm, pio_encode_jmp(spi->offset_combined));
    // 3 bytes for the header, -1, and then we don't ack the last byte, leaving only the packet size + 1
    pio_sm_exec_wait_blocking(spi->pio, spi->config.combined_sm, pio_encode_set(pio_x, ((spi->header & 0xf) * 2) + 1));

    uint irq_wait = pio_encode_wait_irq(1, false, 7);
    pio_sm_exec(spi->pio, spi->config.combined_sm, irq_wait);
    pio_sm_exec(spi->pio, spi->config.initial_sm, irq_wait);

    pio_spi_read_info_t ret = {
        .num_bytes_written = test,
        .num_bytes_read = 0,
        .num_bits_transacted = test,
    };

    return ret;
}

static pio_spi_read_info_t __time_critical_func(prepare_for_next)(pio_spi_t *spi)
{
    // Read FIFO count of write buffer
    pio_spi_read_info_t byte_info = stop_loops(spi);

    uint8_t reads_remaining = dma_channel_hw_addr(spi->channel_read)->transfer_count;
    uint8_t writes_remaining = dma_channel_hw_addr(spi->channel_write)->transfer_count;

    dma_channel_abort(spi->channel_read);
    dma_channel_abort(spi->channel_write);

    pio_enable_sm_mask_in_sync(spi->pio, spi->startstop_mask);

    byte_info.num_bytes_written = spi->write_buf_len ? spi->write_buf_len - writes_remaining - byte_info.num_bytes_written : 0;
    byte_info.num_bytes_read = spi->read_buf_len ? spi->read_buf_len - reads_remaining : 0;

    return byte_info;
}

static void __time_critical_func(pio_irq)(pio_spi_t *spi)
{
    io_rw_32 irqs = spi->pio->irq;
    gpio_put(spi->config.dbg_pin, true);
    if (irqs & (1u << 1))
    {
        // SEGGER_RTT_printf(0, "PIO IRQ CS Falling\n");
        if (spi->config.transaction_started)
        {
            spi->config.transaction_started(spi->config.callback_ctx);
        }
        pio_interrupt_clear(spi->pio, 1);
    }
    if (irqs & (1u << 2))
    {
        // SEGGER_RTT_printf(0, "PIO IRQ CS Rising\n");

        if (spi->config.transaction_ended)
        {
            spi->config.transaction_ended(spi->config.callback_ctx);
        }
        pio_spi_read_info_t read_info = prepare_for_next(spi);

        spi->write_buf_len = 0;
        spi->read_buf_len = 0;
        pio_interrupt_clear(spi->pio, 1);
    }
    gpio_put(spi->config.dbg_pin, false);
}

static void __time_critical_func(pio_irq_0)(void)
{
    pio_irq(&pio_spi[0]);
}

static void __time_critical_func(pio_irq_1)(void)
{
    pio_irq(&pio_spi[1]);
}

static void __time_critical_func(pio_data_irq_0)(void)
{
#ifdef RUN_DBG_LEDS_DURING_DATA_REQUEST
    gpio_put(pio_spi[0].config.dbg_pin, true);
#endif
    pio_spi_config_t *cfg = &pio_spi[0].config;
    pio0->rxf[cfg->initial_sm];
    uint8_t reg = pio0->rxf[cfg->initial_sm] >> 24;
    cfg->data_request(cfg->callback_ctx, reg);
    hw_set_bits(&pio0->irq, (1u << 0));
#ifdef RUN_DBG_LEDS_DURING_DATA_REQUEST
    gpio_put(pio_spi[0].config.dbg_pin, false);
#endif
}

static void __time_critical_func(pio_data_irq_1)(void)
{
#ifdef RUN_DBG_LEDS_DURING_DATA_REQUEST
    gpio_put(pio_spi[1].config.dbg_pin, true);
#endif
    pio_spi_config_t *cfg = &pio_spi[1].config;
    pio1->rxf[cfg->initial_sm];
    uint8_t reg = pio1->rxf[cfg->initial_sm] >> 24;
    cfg->data_request(cfg->callback_ctx, reg);
    hw_set_bits(&pio1->irq, (1u << 0));
#ifdef RUN_DBG_LEDS_DURING_DATA_REQUEST
    gpio_put(pio_spi[1].config.dbg_pin, true);
#endif
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
    spi->header = 0x41;

    gpio_init(config->dbg_pin);
    gpio_set_dir(config->dbg_pin, GPIO_OUT);
    gpio_set_pulls(config->dbg_pin, false, false);

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

    setup_combined_sm(spi->pio, spi->config.combined_sm, spi->config.cipo_pin, spi->config.copi_pin, spi->config.sck_pin, spi->config.ack_pin, spi->config.default_write_value, &spi->offset_combined);
    configure_write_dma(spi->pio, spi->config.combined_sm, &spi->channel_write);

    configure_read_dma(spi->pio, spi->config.combined_sm, &spi->channel_read);

    setup_read_initial_sm(spi->pio, spi->config.initial_sm, spi->config.copi_pin, spi->config.ack_pin, spi->offset_combined);

    setup_cs_sm(spi->pio, spi->config.cs_sm, spi->config.cipo_pin, spi->config.cs_pin, &spi->offset_cs);

    if (spi->config.cs_active_high)
    {
        gpio_set_inover(spi->config.cs_pin, GPIO_OVERRIDE_INVERT);
    }

    if (spi->config.trigger_on_falling)
    {
        gpio_set_inover(spi->config.sck_pin, GPIO_OVERRIDE_INVERT);
    }

    if (config->data_request)
    {
        pio_set_irq1_source_enabled(spi->pio, pis_interrupt0, true);
    }

    pio_set_irq0_source_enabled(spi->pio, pis_interrupt1, true);
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
    spi->dgb_mask = (1u << config->dbg_pin);

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