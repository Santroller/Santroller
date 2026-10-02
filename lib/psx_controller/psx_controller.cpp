#include "psx_controller.hpp"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

static inline bool isValidReply(const uint8_t *status)
{
    return status[1] != 0xFF && (status[2] == 0x5A || status[2] == 0x00);
}

static inline bool isFlightStickReply(const uint8_t *status)
{
    return isValidReply(status) && (status[1] & 0xF0) == 0x50;
}

static inline bool isNegconReply(const uint8_t *status)
{
    return isValidReply(status) && status[1] == 0x23;
}
static inline bool isJogconReply(const uint8_t *status)
{
    return isValidReply(status) && (status[1] & 0xF0) == 0xE0;
}

static inline bool isGunconReply(const uint8_t *status)
{
    return isValidReply(status) && status[1] == 0x63;
}
static inline bool isMouseReply(const uint8_t *status)
{
    return isValidReply(status) && status[1] == 0x12;
}

static inline bool isDualShockReply(const uint8_t *status)
{
    return isValidReply(status) && (status[1] & 0xF0) == 0x70;
}

static inline bool isDualShock2Reply(const uint8_t *status)
{
    return isValidReply(status) && status[1] == 0x79;
}

static inline bool isDigitalReply(const uint8_t *status)
{
    return isValidReply(status) && (status[1] & 0xF0) == 0x40;
}

static inline bool isConfigReply(const uint8_t *status)
{
    return isValidReply(status) && (status[1] & 0xF0) == 0xF0;
}
static const uint8_t commandEnterConfig[] = {0x01, 0x43, 0x00, 0x01, 0x5A,
                                             0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandExitConfig[] = {0x01, 0x43, 0x00, 0x00, 0x5A,
                                            0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandEnableRumble[] = {0x01, 0x4d, 0x00, 0x00, 0x01,
                                              0xff, 0xff, 0xff, 0xff};
static const uint8_t commandGetStatus[] = {0x01, 0x45, 0x00, 0x00, 0x5A,
                                           0x5A, 0x5A, 0x5A, 0x5A};

static const uint8_t commandGetExtra[] = {0x01, 0x46, 0x00, 0x00, 0x5A,
                                          0x5A, 0xA, 0x5A, 0x5A};
static const uint8_t commandGetExtra2[] = {0x01, 0x46, 0x00, 0x00, 0x5A,
                                           0x5A, 0xA, 0x5A, 0x5A};

static const uint8_t commandSetMode[] = {0x01, 0x44, 0x00, /* enabled */ 0x01,
                                         /* locked */ 0x00, 0x02};

// Enable all analog values
static const uint8_t commandSetPressures[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF,
                                              0x03, 0x00, 0x00, 0x00};

// The pressures data format is 0x4F, 0x00, followed by 18 bits, for each
// of the 18 possible bytes that a controller can return

// For some controllers, we want the buttons (2 bytes, and then 4 bytes of
// sticks (rx, ry, lx, ly)).
static const uint8_t commandSetPressuresSticksOnly[] = {
    0x01, 0x4F, 0x00, 0b111111, 0x00, 0b00, 0x00, 0x00, 0x00};

// For guitars, we want the buttons (2 bytes, and then ly).
static const uint8_t commandSetPressuresGuitar[] = {0x01, 0x4F, 0x00, 0b110001, 0x00,
                                                    0b00, 0x00, 0x00, 0x00};

// For the mouse, we want the buttons (2 bytes, and then 2 bytes of axis (x,
// y)). We also want triggers, which luckily happen to be the last two
// bits and hence in their own byte
static const uint8_t commandSetPressuresMouse[] = {0x01, 0x4F, 0x00, 0b1111, 0x00,
                                                   0b11, 0x00, 0x00, 0x00};

static const uint8_t commandPollInput[] = {0x01, 0x42, 0x00, 0xFF, 0xFF};
static PSXController *controller;

static void trace_ps2_packet(const char *tag, const uint8_t *data, uint8_t len)
{
#if PS2_DEBUG
    printf("[PS2] %s len=%u:", tag, len);
    uint8_t shown = len > BUFFER_SIZE ? BUFFER_SIZE : len;
    for (uint8_t i = 0; i < shown; ++i)
        printf(" %02X", data[i]);
    printf("\r\n");
#else
    (void)tag;
    (void)data;
    (void)len;
#endif
}

void attentionInterrupt(uint gpio, uint32_t events)
{
    if (controller)
        controller->process_data(true, false);
}

static void dma_complete_handler()
{
    if (controller)
        controller->spi_dma_complete();
}

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    PSXController *inst = (PSXController *)user_data;
    // A stale alarm can outlive the controller across config reloads
    if (inst != controller)
        return 0;
    inst->process_data(false, true);
    return 0;
}
PSXController::PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin) 
    : m_block(block), m_clock(clock), m_attPin(attPin), m_ackPin(ackPin), m_sckPin(sck), m_mosiPin(mosi), m_misoPin(miso)
{
    controller = this;
}

void PSXController::begin()
{
    PS2_PRINT("[PS2] begin sck=%d mosi=%d miso=%d att=%u ack=%u clock=%u block=%u\r\n",
              m_sckPin, m_mosiPin, m_misoPin, m_attPin, m_ackPin, m_clock, m_block);

    gpio_init(m_attPin);
    gpio_set_dir(m_attPin, true);
    gpio_put(m_attPin, true);
    gpio_init(m_ackPin);
    gpio_set_dir(m_ackPin, false);
    gpio_set_pulls(m_ackPin, true, false);
    controller = this;

    spi = (m_block == 0) ? spi0 : spi1;
    spi_init(spi, m_clock);
    spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    if (m_sckPin != -1)
        gpio_set_function(m_sckPin, GPIO_FUNC_SPI);
    if (m_mosiPin != -1)
        gpio_set_function(m_mosiPin, GPIO_FUNC_SPI);
    if (m_misoPin != -1)
    {
        gpio_set_function(m_misoPin, GPIO_FUNC_SPI);
        gpio_set_pulls(m_misoPin, true, false);
    }

    // Initialize PIO ACK Pacer (try pio0 first, fallback to pio1)
    pio = pio0;
    if (pio_can_add_program(pio, &psx_ack_pacer_program) && (sm = pio_claim_unused_sm(pio, false)) != (uint)-1)
    {
        pio_offset = pio_add_program(pio, &psx_ack_pacer_program);
    }
    else
    {
        pio = pio1;
        if (pio_can_add_program(pio, &psx_ack_pacer_program) && (sm = pio_claim_unused_sm(pio, false)) != (uint)-1)
        {
            pio_offset = pio_add_program(pio, &psx_ack_pacer_program);
        }
        else
        {
            pio = nullptr;
        }
    }

    if (pio != nullptr)
    {
        pio_sm_config c = psx_ack_pacer_program_get_default_config(pio_offset);
        sm_config_set_jmp_pin(&c, m_ackPin);
        sm_config_set_in_pins(&c, m_ackPin);

        float clkdiv = (float)clock_get_hz(clk_sys) / 1771428.0f;
        sm_config_set_clkdiv(&c, clkdiv);

        pio_sm_init(pio, sm, pio_offset, &c);
        pio_initialized = true;
        PS2_PRINT("[PS2] PIO ACK pacer initialized pio=%u sm=%u offset=%u\r\n", pio_get_index(pio), sm, pio_offset);
    }

    dma_rx = dma_claim_unused_channel(false);
    dma_tx = dma_claim_unused_channel(false);

    if (dma_rx >= 0 && dma_tx >= 0)
    {
        dma_channel_config rx_cfg = dma_channel_get_default_config(dma_rx);
        channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
        channel_config_set_dreq(&rx_cfg, spi_get_dreq(spi, false));
        channel_config_set_read_increment(&rx_cfg, false);
        channel_config_set_write_increment(&rx_cfg, true);
        dma_channel_configure(
            dma_rx, &rx_cfg, ps2Data,
            &spi_get_hw(spi)->dr, BUFFER_SIZE, false);

        if (pio_initialized)
        {
            dma_tx_pacer = dma_claim_unused_channel(false);
            if (dma_tx_pacer >= 0)
            {
                dma_channel_config pacer_cfg = dma_channel_get_default_config(dma_tx_pacer);
                channel_config_set_transfer_data_size(&pacer_cfg, DMA_SIZE_32);
                channel_config_set_dreq(&pacer_cfg, pio_get_dreq(pio, sm, false));
                channel_config_set_read_increment(&pacer_cfg, false);
                channel_config_set_write_increment(&pacer_cfg, false);
                dma_channel_configure(
                    dma_tx_pacer, &pacer_cfg,
                    &dma_hw->ch[dma_tx].al1_transfer_count_trig,
                    &pio->rxf[sm], BUFFER_SIZE, false);

                dma_channel_config tx_cfg = dma_channel_get_default_config(dma_tx);
                channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
                channel_config_set_dreq(&tx_cfg, DREQ_FORCE);
                channel_config_set_read_increment(&tx_cfg, true);
                channel_config_set_write_increment(&tx_cfg, false);
                dma_channel_configure(
                    dma_tx, &tx_cfg, &spi_get_hw(spi)->dr, ps2DataOutBuffer, 1, false);
            }
            else
            {
                pio_initialized = false;
            }
        }

        if (!pio_initialized)
        {
            dma_timer = dma_claim_unused_timer(false);
            if (dma_timer >= 0)
            {
                uint16_t denom = (uint16_t)(clock_get_hz(clk_sys) / 28571);
                dma_timer_set_fraction(dma_timer, 1, denom);
                uint timer_dreq = dma_get_timer_dreq(dma_timer);

                dma_channel_config tx_cfg = dma_channel_get_default_config(dma_tx);
                channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
                channel_config_set_dreq(&tx_cfg, timer_dreq);
                channel_config_set_read_increment(&tx_cfg, true);
                channel_config_set_write_increment(&tx_cfg, false);
                dma_channel_configure(
                    dma_tx, &tx_cfg, &spi_get_hw(spi)->dr, ps2DataOutBuffer, BUFFER_SIZE, false);
            }
        }

        dma_channel_set_irq1_enabled(dma_rx, true);
        irq_set_exclusive_handler(DMA_IRQ_1, dma_complete_handler);
        irq_set_enabled(DMA_IRQ_1, true);
    }

    gpio_set_irq_enabled_with_callback(m_ackPin, GPIO_IRQ_EDGE_RISE, true, &attentionInterrupt);
    auto_shift_data(commandPollInput, sizeof(commandPollInput));
}
static inline void abort_dma_if_active(int channel)
{
    if (channel < 0)
        return;
    if (dma_channel_is_busy(channel))
    {
        dma_channel_abort(channel);
        while (dma_hw->abort & (1u << channel))
            tight_loop_contents();
    }
}

void PSXController::end()
{
    gpio_set_irq_enabled(m_ackPin, GPIO_IRQ_EDGE_RISE, false);
    cancel_alarm(timeout_alarm_id);

    if (spi_active)
    {
        abort_dma_if_active(dma_rx);
        abort_dma_if_active(dma_tx);
        abort_dma_if_active(dma_tx_pacer);
        if (pio_initialized)
            pio_sm_set_enabled(pio, sm, false);
        dma_hw->ints1 = 1u << dma_rx;
        spi_active = false;
    }
}
void PSXController::load_state(const DeviceReloadState *state)
{
    type = state->ps2_type;
    valid = state->ps2_valid;
    hasTapBar = state->ps2_hasTapBar;
    missing = state->ps2_missing;
    last = state->ps2_last;
    lastInit = state->ps2_lastInit;
    invalidCount = state->ps2_invalidCount;
    memcpy(ps2Data, state->ps2_data, sizeof(ps2Data));
    memcpy(lastInputs, state->ps2_lastInputs, sizeof(lastInputs));
    ps2Idx = state->ps2_idx;
    ps2Len = state->ps2_len;
    ps2DataLen = state->ps2_dataLen;
    done = state->ps2_done;
    packet_delay = state->ps2_packetDelay;
}
void PSXController::save_state(DeviceReloadState &state) const
{
    state.ps2_type = type;
    state.ps2_valid = valid;
    state.ps2_hasTapBar = hasTapBar;
    state.ps2_missing = missing;
    state.ps2_last = last;
    state.ps2_lastInit = lastInit;
    state.ps2_invalidCount = invalidCount;
    memcpy(state.ps2_data, ps2Data, sizeof(ps2Data));
    memcpy(state.ps2_lastInputs, lastInputs, sizeof(lastInputs));
    state.ps2_idx = ps2Idx;
    state.ps2_len = ps2Len;
    state.ps2_dataLen = ps2DataLen;
    state.ps2_done = done;
    state.ps2_packetDelay = packet_delay;
}

PSXController::~PSXController()
{
    PS2_PRINT("~PSXController\r\n");
    end();

    if (dma_rx >= 0)
    {
        dma_channel_set_irq1_enabled(dma_rx, false);
        dma_channel_unclaim(dma_rx);
        dma_rx = -1;
    }
    if (dma_tx >= 0)
    {
        dma_channel_unclaim(dma_tx);
        dma_tx = -1;
    }
    if (dma_tx_pacer >= 0)
    {
        dma_channel_unclaim(dma_tx_pacer);
        dma_tx_pacer = -1;
    }
    if (dma_timer >= 0)
    {
        dma_timer_unclaim(dma_timer);
        dma_timer = -1;
    }
    if (pio_initialized && pio != nullptr)
    {
        pio_sm_set_enabled(pio, sm, false);
        pio_sm_unclaim(pio, sm);
        pio_remove_program(pio, &psx_ack_pacer_program, pio_offset);
        pio_initialized = false;
        pio = nullptr;
    }
    if (controller == this)
        controller = nullptr;
}
void PSXController::no_attention(void)
{
    PS2_PRINT("[PS2] transaction end state=%d valid=%d spi=%d\r\n", status, valid, spi_active);
    done = true;
    if (!spi_active)
        gpio_set_irq_enabled(m_ackPin, GPIO_IRQ_EDGE_RISE, true);
    gpio_put(m_attPin, true);
    cancel_alarm(timeout_alarm_id);
    timeout_alarm_id = add_alarm_in_us(packet_delay, restart_handler, this, true);
}
void PSXController::signal_attention(void)
{
    done = false;
    gpio_put(m_attPin, false);
    cancel_alarm(timeout_alarm_id);
    timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
}
bool PSXController::auto_shift_data(const uint8_t *out, const uint8_t len)
{
    PS2_PRINT("[PS2] start state=%d cmd=%02X len=%u ps2Len=%u\r\n",
              status, len > 1 ? out[1] : 0, len, ps2Len);
    ps2Idx = 0;
    ps2DataLen = len;
    ps2DataOut = out;

    if (status == ENUMERATED && (ps2Len < 5 || ps2Len > BUFFER_SIZE))
        ps2Len = 5;

    uint8_t target_len = (status == ENUMERATED) ? ps2Len : 4;
    if (target_len < 4 || target_len > BUFFER_SIZE)
        target_len = 4;

    memset(ps2Data, 0, sizeof(ps2Data));
    memset(ps2DataOutBuffer, 0x5A, sizeof(ps2DataOutBuffer));
    for (uint8_t i = 0; i < (len < BUFFER_SIZE ? len : BUFFER_SIZE); ++i)
        ps2DataOutBuffer[i] = revbits(out[i]);
    for (uint8_t i = len; i < BUFFER_SIZE; ++i)
        ps2DataOutBuffer[i] = revbits(0x5A);

#if PS2_DEBUG
    printf("[PS2] TX DMA:");
    for (int i = 0; i < 8; i++)
        printf(" %02X", out[i < len ? i : 0]);
    printf("\r\n");
#endif

    cancel_alarm(timeout_alarm_id);
    gpio_set_irq_enabled(m_ackPin, GPIO_IRQ_EDGE_RISE, false);
    gpio_put(m_attPin, false);
    done = false;
    spi_active = true;
    spi_header = (status != ENUMERATED);
    PS2_PRINT("[PS2] SPI armed header=%d target=%u tx=%02X\r\n",
              spi_header, target_len, out[1]);

    // Drain any stale hardware SPI RX FIFO contents
    while (spi_is_readable(spi))
        (void)spi_get_hw(spi)->dr;

    abort_dma_if_active(dma_rx);
    abort_dma_if_active(dma_tx);
    abort_dma_if_active(dma_tx_pacer);
    if (pio_initialized)
        pio_sm_set_enabled(pio, sm, false);
    dma_hw->ints1 = 1u << dma_rx;
    dma_channel_set_irq1_enabled(dma_rx, true);
    dma_channel_set_write_addr(dma_rx, ps2Data, false);
    dma_channel_set_trans_count(dma_rx, target_len, false);
    dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer, false);
    if (pio_initialized)
        dma_channel_set_trans_count(dma_tx_pacer, target_len, false);
    else
        dma_channel_set_trans_count(dma_tx, target_len, false);
    spi_dma_offset = 0;
    spi_dma_len = target_len;
    spi_started = false;

    timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
    return true;
}
void PSXController::spi_header_complete()
{
    if (!spi_active || !spi_header)
        return;

    trace_ps2_packet("header", ps2Data, 4);
    PS2_PRINT("[PS2] header complete state=%d response_id=%02X computed_len=%u\r\n",
              status, ps2Data[1], (uint)(3 + (ps2Data[1] & 0x0F) * 2));

    dma_channel_set_irq1_enabled(dma_rx, false);

    uint8_t response_len = 3 + (ps2Data[1] & 0x0F) * 2;
    if (response_len < 4 || response_len > BUFFER_SIZE)
    {
        spi_active = false;
        spi_header = false;
        valid = false;
        no_attention();
        return;
    }

    ps2Len = response_len;

    dma_channel_set_write_addr(dma_rx, ps2Data + 4, false);
    dma_channel_set_trans_count(dma_rx, response_len - 4, false);
    dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer + 4, false);
    dma_channel_set_trans_count(dma_tx, response_len - 4, false);
    dma_channel_set_irq1_enabled(dma_rx, true);

    spi_dma_offset = 4;
    spi_dma_len = response_len - 4;
    spi_header = false;

    cancel_alarm(timeout_alarm_id);
    timeout_alarm_id = add_alarm_in_us(20000, restart_handler, this, true);

    if (pio_initialized)
    {
        dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer + 4, false);
        dma_channel_set_trans_count(dma_tx_pacer, response_len - 4, false);

        pio_sm_set_enabled(pio, sm, false);
        pio_sm_clear_fifos(pio, sm);
        pio_sm_exec(pio, sm, pio_encode_jmp(pio_offset));
        pio_sm_set_enabled(pio, sm, true);

        dma_start_channel_mask((1u << dma_rx) | (1u << dma_tx_pacer));
    }
    else
    {
        dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer + 4, false);
        dma_channel_set_trans_count(dma_tx, response_len - 4, false);
        dma_start_channel_mask((1u << dma_rx) | (1u << dma_tx));
    }
}

void PSXController::spi_dma_complete()
{
    if (dma_rx >= 0)
        dma_hw->ints1 = 1u << dma_rx;

    if (!spi_active)
        return;

    for (uint8_t i = spi_dma_offset; i < spi_dma_offset + spi_dma_len && i < BUFFER_SIZE; ++i)
        ps2Data[i] = revbits(ps2Data[i]);

    if (spi_header)
    {
        spi_header_complete();
        return;
    }

    trace_ps2_packet("complete", ps2Data, ps2Len);

    cancel_alarm(timeout_alarm_id);
    dma_channel_set_irq1_enabled(dma_rx, false);
    spi_active = false;
    spi_started = false;
    done = true;
    valid = isValidReply(ps2Data);

    if (pio_initialized)
        pio_sm_set_enabled(pio, sm, false);

    process_data(false, false);
}

void PSXController::process_data(bool ack, bool timeout)
{
    if (spi_active)
    {
        if (ack)
            return;

        if (timeout && !spi_started)
        {
            PS2_PRINT("[PS2] SPI start state=%d header=%d DATA=%d ACK=%d\r\n", status, spi_header, gpio_get(m_misoPin), gpio_get(m_ackPin));
            if (status == ENUMERATED && (ps2Len < 5 || ps2Len > BUFFER_SIZE))
                ps2Len = 5;

            uint8_t target_len = (status == ENUMERATED) ? ps2Len : 4;
            if (target_len < 4 || target_len > BUFFER_SIZE)
                target_len = 4;

            spi_dma_offset = 0;
            spi_dma_len = target_len;

            if (pio_initialized)
            {
                dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer, false);
                dma_channel_set_trans_count(dma_tx_pacer, target_len, false);

                pio_sm_set_enabled(pio, sm, false);
                pio_sm_clear_fifos(pio, sm);
                pio_sm_exec(pio, sm, pio_encode_jmp(pio_offset));
                pio_sm_set_enabled(pio, sm, true);

                dma_start_channel_mask((1u << dma_rx) | (1u << dma_tx_pacer));
            }
            else
            {
                dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer, false);
                dma_channel_set_trans_count(dma_tx, target_len, false);
                dma_start_channel_mask((1u << dma_rx) | (1u << dma_tx));
            }

            spi_started = true;
            timeout_alarm_id = add_alarm_in_us(20000, restart_handler, this, true);
            return;
        }

        if (timeout)
        {
            PS2_PRINT("[PS2] SPI TIMEOUT state=%d header=%d started=%d SCK=%d MOSI=%d DATA=%d ACK=%d rxleft=%u txleft=%u\r\n",
                      status, spi_header, spi_started,
                      gpio_get(m_sckPin), gpio_get(m_mosiPin),
                      gpio_get(m_misoPin), gpio_get(m_ackPin),
                      dma_hw->ch[dma_rx].transfer_count,
                      pio_initialized ? dma_hw->ch[dma_tx_pacer].transfer_count : dma_hw->ch[dma_tx].transfer_count);
            abort_dma_if_active(dma_rx);
            abort_dma_if_active(dma_tx);
            abort_dma_if_active(dma_tx_pacer);
            if (pio_initialized)
                pio_sm_set_enabled(pio, sm, false);
            dma_hw->ints1 = 1u << dma_rx;
            spi_active = false;
            spi_started = false;
            valid = false;
            done = true;
            no_attention();
        }
        return;
    }

    cancel_alarm(timeout_alarm_id);
    if (done)
    {
        switch (status)
        {
        case DISCONNECTED:
            if (valid)
            {
                status = CONNECTION_DELAY;
                m_config_retries = 0;
                packet_delay = 100000;
                no_attention();
                return;
            }
            break;
        case CONNECTION_DELAY:
            status = FIRST_INPUTS;
            packet_delay = 100;
            break;
        case FIRST_INPUTS:
            if (isConfigReply(ps2Data))
                status = ENABLE_ANALOG_MODE;
            else
                status = ENTER_CONFIG;
            packet_delay = 100;
            break;
        case ENTER_CONFIG:
            if (valid)
            {
                m_config_retries = 0;
                status = FIRST_INPUTS;
            }
            else
            {
                if (m_config_retries < 5)
                {
                    m_config_retries++;
                    status = FIRST_INPUTS;
                }
                else
                {
                    status = SECOND_INPUTS;
                }
            }
            packet_delay = 100;
            break;
        case ENABLE_ANALOG_MODE:
            if (!valid && m_config_retries < 5)
            {
                m_config_retries++;
                status = FIRST_INPUTS;
                break;
            }
            status = ENABLE_RUMBLE;
            packet_delay = 100;
            break;
        case ENABLE_RUMBLE:
            if (!valid && m_config_retries < 5)
            {
                m_config_retries++;
                status = FIRST_INPUTS;
                break;
            }
            status = ENABLE_PRESSURES;
            packet_delay = 100;
            break;
        case ENABLE_PRESSURES:
            if (!valid && m_config_retries < 5)
            {
                m_config_retries++;
                status = FIRST_INPUTS;
                break;
            }
            status = ENABLE_PRESSURES_2;
            packet_delay = 100;
            break;
        case ENABLE_PRESSURES_2:
            if (!valid && m_config_retries < 5)
            {
                m_config_retries++;
                status = FIRST_INPUTS;
                break;
            }
            status = EXIT_CONFIG;
            packet_delay = 100;
            break;
        case EXIT_CONFIG:
            if (!isValidReply(ps2Data))
            {
                if (m_config_retries < 5)
                {
                    m_config_retries++;
                    status = FIRST_INPUTS;
                    break;
                }
                status = DISCONNECTED;
                break;
            }
            if (!isConfigReply(ps2Data))
                status = SECOND_INPUTS;
            packet_delay = 100;
            break;
        case SECOND_INPUTS:
            status = ENUMERATED;
            packet_delay = 5000;
            if (isDualShock2Reply(ps2Data))
            {
                if ((~ps2Data[3]) & (1 << 7) && (~ps2Data[3]) & (1 << 5) && (~ps2Data[3]) & (1 << 6))
                    type = PS2ControllerTypePopNMusic;
                else if ((~ps2Data[3]) & (1 << 7))
                    type = PS2ControllerTypeGuitar;
                else
                {
                    packet_delay = 1000;
                    type = PS2ControllerTypeDualshock2;
                }
            }
            else if (isDualShockReply(ps2Data))
            {
                if ((~ps2Data[3]) & (1 << 7))
                    type = PS2ControllerTypeGuitar;
                else
                    type = PS2ControllerTypeDualshock;
            }
            else if (isFlightStickReply(ps2Data))
                type = PS2ControllerTypeFlightStick;
            else if (isNegconReply(ps2Data))
                type = PS2ControllerTypeNegCon;
            else if (isJogconReply(ps2Data))
                type = PS2ControllerTypeJogCon;
            else if (isGunconReply(ps2Data))
                type = PS2ControllerTypeGunCon;
            else if (isMouseReply(ps2Data))
                type = PS2ControllerTypeMouse;
            else if (isDigitalReply(ps2Data))
                type = PS2ControllerTypeDigital;
            break;
        case ENUMERATED:
            PS2_PRINT("[PS2] state ENUMERATED valid=%d\r\n", valid);
            if (valid)
            {
                memcpy(lastInputs, ps2Data, BUFFER_SIZE);
                missing = 0;
            }
            else
            {
                missing++;
                if (missing > 10)
                {
                    status = DISCONNECTED;
                    type = PS2ControllerTypeUnknown;
                    packet_delay = 10000;
                }
            }
            break;
        }

        m_poll_cmd[0] = 0x01;
        m_poll_cmd[1] = 0x42;
        m_poll_cmd[2] = 0x00;
        m_poll_cmd[3] = m_rumble_small ? 0x01 : 0x00;
        m_poll_cmd[4] = m_rumble_large;

        if (valid && status == ENUMERATED &&
            (ps2Data[2] == 0x5A || ps2Data[2] == 0x00))
        {
            uint8_t response_len = 3 + (ps2Data[1] & 0x0F) * 2;
            if (response_len <= BUFFER_SIZE)
                ps2Len = response_len;
        }

        no_attention();
        done = false;
        return;
    }

    switch (status)
    {
    case ENTER_CONFIG:
        auto_shift_data(commandEnterConfig, sizeof(commandEnterConfig));
        break;
    case ENABLE_ANALOG_MODE:
        auto_shift_data(commandSetMode, sizeof(commandSetMode));
        break;
    case ENABLE_RUMBLE:
        auto_shift_data(commandEnableRumble, sizeof(commandEnableRumble));
        break;
    case ENABLE_PRESSURES:
    case ENABLE_PRESSURES_2:
        auto_shift_data(commandSetPressures, sizeof(commandSetPressures));
        break;
    case EXIT_CONFIG:
        auto_shift_data(commandExitConfig, sizeof(commandExitConfig));
        break;
    case ENUMERATED:
        auto_shift_data(m_poll_cmd, sizeof(m_poll_cmd));
        break;
    default:
        auto_shift_data(commandPollInput, sizeof(commandPollInput));
        break;
    }
    return;
}

uint16_t PSXController::read_axis(PS2AxisType axisType)
{
    switch (type)
    {
    case PS2ControllerTypeUnknown:
        return 0;
    case PS2ControllerTypeDualshock2:
        switch (axisType)
        {
        case PS2AxisLeftStickX:
            return lastInputs[7] << 8;
        case PS2AxisLeftStickY:
            return (255 - lastInputs[8]) << 8;
        case PS2AxisRightStickX:
            return (lastInputs[5]) << 8;
        case PS2AxisRightStickY:
            return (255 - lastInputs[6]) << 8;
        case PS2AxisDualshock2RightButton:
            return lastInputs[9] << 8;
        case PS2AxisDualshock2LeftButton:
            return lastInputs[10] << 8;
        case PS2AxisDualshock2UpButton:
            return lastInputs[11] << 8;
        case PS2AxisDualshock2DownButton:
            return lastInputs[12] << 8;
        case PS2AxisDualshock2Triangle:
            return lastInputs[13] << 8;
        case PS2AxisDualshock2Circle:
            return lastInputs[14] << 8;
        case PS2AxisDualshock2Cross:
            return lastInputs[15] << 8;
        case PS2AxisDualshock2Square:
            return lastInputs[16] << 8;
        case PS2AxisDualshock2L1:
            return lastInputs[17] << 8;
        case PS2AxisDualshock2R1:
            return lastInputs[18] << 8;
        case PS2AxisDualshock2L2:
            return lastInputs[19] << 8;
        case PS2AxisDualshock2R2:
            return lastInputs[20] << 8;
        default:
            return 0;
        }
    case PS2ControllerTypeDualshock:
    case PS2ControllerTypeFlightStick:
        switch (axisType)
        {
        case PS2AxisLeftStickX:
            return lastInputs[7] << 8;
        case PS2AxisLeftStickY:
            return (255 - lastInputs[8]) << 8;
        case PS2AxisRightStickX:
            return (lastInputs[5]) << 8;
        case PS2AxisRightStickY:
            return (255 - lastInputs[6]) << 8;
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeGuitar:
        switch (axisType)
        {
        case PS2AxisGuitarWhammy:
            return (0x80 - lastInputs[8]) << 9;
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeNegCon:
        switch (axisType)
        {
        case PS2AxisNegConTwist:
            return (lastInputs[5]) << 8;
        case PS2AxisNegConI:
            return lastInputs[6];
        case PS2AxisNegConIi:
            return lastInputs[7];
        case PS2AxisNegConL:
            return lastInputs[8];
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeJogCon:
        switch (axisType)
        {
        case PS2AxisJogConWheel:
            return (lastInputs[6] << 8) | lastInputs[5];
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeGunCon:
        switch (axisType)
        {
        case PS2AxisGunConHSync:
            return (lastInputs[6] << 8) | lastInputs[5];
        case PS2AxisGunConVSync:
            return (lastInputs[8] << 8) | lastInputs[7];
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeMouse:
        switch (axisType)
        {
        case PS2AxisMouseX:
            return (lastInputs[5]) << 8;
        case PS2AxisMouseY:
            return (255 - lastInputs[6]) << 8;
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeTaiko:
        return 0;
    case PS2ControllerTypePopNMusic:
        return 0;
    case PS2ControllerTypeDigital:
        return 0;
    }
    return 0;
}

bool PSXController::read_button(PS2ButtonType buttonType)
{
    switch (type)
    {
    case PS2ControllerTypeUnknown:
        return 0;
    case PS2ControllerTypeGunCon:
    case PS2ControllerTypeJogCon:
    case PS2ControllerTypeDigital:
    case PS2ControllerTypePopNMusic:
    case PS2ControllerTypeFlightStick:
    case PS2ControllerTypeDualshock:
    case PS2ControllerTypeDualshock2:
        switch (buttonType)
        {
        case PS2ButtonSelect:
            return (~lastInputs[3]) & (1 << 0);
        case PS2ButtonL3:
            return (~lastInputs[3]) & (1 << 1);
        case PS2ButtonR3:
            return (~lastInputs[3]) & (1 << 2);
        case PS2ButtonStart:
            return (~lastInputs[3]) & (1 << 3);
        case PS2ButtonDpadUp:
            return (~lastInputs[3]) & (1 << 4);
        case PS2ButtonDpadRight:
            return (~lastInputs[3]) & (1 << 5);
        case PS2ButtonDpadDown:
            return (~lastInputs[3]) & (1 << 6);
        case PS2ButtonDpadLeft:
            return (~lastInputs[3]) & (1 << 7);
        case PS2ButtonL2:
            return (~lastInputs[4]) & (1 << 0);
        case PS2ButtonR2:
            return (~lastInputs[4]) & (1 << 1);
        case PS2ButtonL1:
            return (~lastInputs[4]) & (1 << 2);
        case PS2ButtonR1:
            return (~lastInputs[4]) & (1 << 3);
        case PS2ButtonTriangle:
            return (~lastInputs[4]) & (1 << 4);
        case PS2ButtonCircle:
            return (~lastInputs[4]) & (1 << 5);
        case PS2ButtonCross:
            return (~lastInputs[4]) & (1 << 6);
        case PS2ButtonSquare:
            return (~lastInputs[4]) & (1 << 7);
        default:
            return 0;
        }
    case PS2ControllerTypeGuitar:
        switch (buttonType)
        {
        case PS2ButtonGuitarGreen:
            return (~lastInputs[4]) & (1 << 1);
        case PS2ButtonGuitarRed:
            return (~lastInputs[4]) & (1 << 5);
        case PS2ButtonGuitarYellow:
            return (~lastInputs[4]) & (1 << 4);
        case PS2ButtonGuitarBlue:
            return (~lastInputs[4]) & (1 << 6);
        case PS2ButtonGuitarOrange:
            return (~lastInputs[4]) & (1 << 7);
        case PS2ButtonGuitarStrumUp:
            return (~lastInputs[3]) & (1 << 4);
        case PS2ButtonGuitarStrumDown:
            return (~lastInputs[3]) & (1 << 6);
        case PS2ButtonGuitarDpadUp:
            return ((lastInputs[5] >> 6) == 0) && lastInputs[5] != 0;
        case PS2ButtonGuitarDpadDown:
            return ((lastInputs[5] >> 6) == 2) && lastInputs[5] != 128;
        case PS2ButtonGuitarDpadLeft:
            return ((lastInputs[5] >> 6) == 1) && lastInputs[5] != 127 && lastInputs[5] != 123;
        case PS2ButtonGuitarDpadRight:
            return ((lastInputs[5] >> 6) == 3) && lastInputs[5] != 255;
        case PS2ButtonGuitarSelect:
            return (~lastInputs[3]) & (1 << 0);
        case PS2ButtonGuitarStart:
            return (~lastInputs[3]) & (1 << 3);
        case PS2ButtonGuitarTilt:
            return (~lastInputs[4]) & (1 << 0);
        case PS2ButtonGuitarTapGreen:
            return lastInputs[7] <= 0x3F;
        case PS2ButtonGuitarTapRed:
            return lastInputs[7] <= 0x5F && lastInputs[7] > 0x2F;
        case PS2ButtonGuitarTapYellow:
            return (lastInputs[7] <= 0x6F && lastInputs[7] > 0x5F) || (lastInputs[7] <= 0x9F && lastInputs[7] > 0x8F);
        case PS2ButtonGuitarTapBlue:
            return lastInputs[7] <= 0xEF && lastInputs[7] > 0xBF;
        case PS2ButtonGuitarTapOrange:
            return lastInputs[7] > 0xEF;
        default:
            return 0;
        }
    case PS2ControllerTypeNegCon:
        switch (buttonType)
        {
        case PS2ButtonNegConA:
            return (~lastInputs[4]) & (1 << 5);
        case PS2ButtonNegConB:
            return (~lastInputs[4]) & (1 << 4);
        case PS2ButtonNegConStart:
            return (~lastInputs[3]) & (1 << 3);
        case PS2ButtonNegConR:
            return (~lastInputs[4]) & (1 << 3);
        default:
            return 0;
        }
    case PS2ControllerTypeMouse:
        switch (buttonType)
        {
        case PS2ButtonMouseLeft:
            return (~lastInputs[3]) & (1 << 3);
        case PS2ButtonMouseRight:
            return (~lastInputs[3]) & (1 << 2);
        default:
            return 0;
        }
    case PS2ControllerTypeTaiko:
        switch (buttonType)
        {
        case PS2ButtonTaikoRimLeft:
            return (~lastInputs[4]) & (1 << 2);
        case PS2ButtonTaikoRimRight:
            return (~lastInputs[4]) & (1 << 3);
        case PS2ButtonTaikoCenterLeft:
            return (~lastInputs[3]) & (1 << 7);
        case PS2ButtonTaikoCenterRight:
            return (~lastInputs[4]) & (1 << 5);
        default:
            return 0;
        }
    }
    return false;
}

extern unsigned long millis_at_boot;
bool PSXController::controller_valid()
{
    return type != PS2ControllerTypeUnknown;
}
void PSXController::tick()
{
}
void PSXController::set_rumble(uint8_t left, uint8_t right)
{
    m_rumble_large = left;
    m_rumble_small = right ? 1 : 0;
}