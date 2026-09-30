#include "psx_controller.hpp"
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>
#include "psx_controller.pio.h"
#include <pico/time.h>
#include <stdio.h>
#include "utils.h"

static inline bool isValidReply(const uint8_t *status)
{
    return status[1] != 0xFF && (status[2] == 0x5A || status[2] == 0x00);
}

static inline bool isFlightStickReply(const uint8_t *status)
{
    return (status[1] & 0xF0) == 0x50;
}

static inline bool isNegconReply(const uint8_t *status)
{
    return status[1] == 0x23;
}
static inline bool isJogconReply(const uint8_t *status)
{
    return (status[1] & 0xF0) == 0xE0;
}

static inline bool isGunconReply(const uint8_t *status)
{
    return status[1] == 0x63;
}
static inline bool isMouseReply(const uint8_t *status)
{
    return status[1] == 0x12;
}

static inline bool isDualShockReply(const uint8_t *status)
{
    return (status[1] & 0xF0) == 0x70;
}

static inline bool isDualShock2Reply(const uint8_t *status)
{
    return status[1] == 0x79;
}

static inline bool isDigitalReply(const uint8_t *status)
{
    return (status[1] & 0xF0) == 0x40;
}

static inline bool isConfigReply(const uint8_t *status)
{
    return (status[1] & 0xF0) == 0xF0;
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

void attentionInterrupt(uint gpio, uint32_t events)
{
    if (controller)
        controller->process_data(true, false);
}

static void dma_complete_handler()
{
    if (controller)
        controller->pio_dma_complete();
    dma_hw->ints1 = 1u << 0;
}

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    PSXController *inst = (PSXController *)user_data;
    inst->process_data(false, true);
    return 0;
}
PSXController::PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin) : interface(block, SPI_CPHA_1, SPI_CPOL_1, sck, mosi, miso, false, clock), m_attPin(attPin), m_ackPin(ackPin)
{
    printf("psx controller init!\r\n");
    gpio_init(attPin);
    gpio_set_dir(attPin, true);
    gpio_init(ackPin);
    gpio_set_dir(ackPin, false);
    controller = this;

    // The PS2 poll path is deterministic once the controller has been
    // enumerated, so let PIO own the byte-level exchange and DMA own the
    // packet buffer. The existing GPIO/SPI path is retained for enumeration.
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);

    uint16_t patched[32];
    memcpy(patched, psx_controller_spi_program.instructions,
           psx_controller_spi_program.length * sizeof(uint16_t));

    const uint16_t ack_low =
        pio_encode_wait_gpio(false, PIN_ACK_PLACEHOLDER);
    const uint16_t ack_high =
        pio_encode_wait_gpio(true, PIN_ACK_PLACEHOLDER);
    const uint16_t new_ack_low = pio_encode_wait_gpio(false, ackPin);
    const uint16_t new_ack_high = pio_encode_wait_gpio(true, ackPin);

    for (uint i = 0; i < psx_controller_spi_program.length; ++i)
    {
        if (patched[i] == ack_low)
            patched[i] = new_ack_low;
        else if (patched[i] == ack_high)
            patched[i] = new_ack_high;
    }

    pio_program_t program = psx_controller_spi_program;
    program.instructions = patched;
    offset = pio_add_program(pio, &program);

    pio_sm_config cfg = psx_controller_spi_program_get_default_config(offset);
    sm_config_set_out_pins(&cfg, mosi, 1);
    sm_config_set_in_pins(&cfg, miso);
    sm_config_set_sideset_pins(&cfg, sck);
    sm_config_set_out_shift(&cfg, true, false, 8);
    sm_config_set_in_shift(&cfg, true, true, 8);

    // Each bit consumes two PIO instructions, so this produces the requested
    // PS2 clock rate rather than twice the requested rate.
    sm_config_set_clkdiv(&cfg,
                         (float)clock_get_hz(clk_sys) / (2.0f * clock));

    pio_gpio_init(pio, sck);
    pio_gpio_init(pio, mosi);
    pio_gpio_init(pio, miso);
    pio_gpio_init(pio, ackPin);

    pio_sm_set_consecutive_pindirs(pio, sm, sck, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, mosi, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, miso, 1, false);
    pio_sm_set_consecutive_pindirs(pio, sm, ackPin, 1, false);

    pio_sm_init(pio, sm, offset, &cfg);
    pio_sm_set_pins(pio, sm, 1u << sck);

    dma_rx = dma_claim_unused_channel(true);
    dma_tx = dma_claim_unused_channel(true);

    dma_channel_config rx_cfg = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
    channel_config_set_dreq(&rx_cfg, pio_get_dreq(pio, sm, false));
    channel_config_set_read_increment(&rx_cfg, false);
    channel_config_set_write_increment(&rx_cfg, true);
    dma_channel_configure(
        dma_rx, &rx_cfg, ps2Data,
        ((uint8_t *)&pio->rxf[sm]) + 3, BUFFER_SIZE, false);

    dma_channel_config tx_cfg = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_8);
    channel_config_set_dreq(&tx_cfg, pio_get_dreq(pio, sm, true));
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    dma_channel_configure(
        dma_tx, &tx_cfg, &pio->txf[sm], ps2DataOutBuffer, BUFFER_SIZE, false);

    dma_channel_set_irq1_enabled(dma_rx, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_complete_handler);
    irq_set_enabled(DMA_IRQ_1, true);
}
void PSXController::begin() {
    gpio_set_irq_enabled_with_callback(m_ackPin, GPIO_IRQ_EDGE_RISE, true, &attentionInterrupt);
    auto_shift_data(commandPollInput, sizeof(commandPollInput));
}
void PSXController::end() {
    gpio_set_irq_enabled(m_ackPin, GPIO_IRQ_EDGE_RISE, false);
    cancel_alarm(timeout_alarm_id);

    if (pio_active)
    {
        dma_channel_abort(dma_rx);
        dma_channel_abort(dma_tx);
        pio_sm_set_enabled(pio, sm, false);
        pio_sm_clear_fifos(pio, sm);
        pio_active = false;
    }
}
void PSXController::load_state(const DeviceReloadState *state) {
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
void PSXController::save_state(DeviceReloadState& state) const  {
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

PSXController::~PSXController() {
    printf("~PSXController\r\n");
}
void PSXController::no_attention(void)
{
    done = true;
    if (!pio_active)
        gpio_set_irq_enabled(m_ackPin, GPIO_IRQ_EDGE_RISE, true);
    gpio_put(m_attPin, true);
    timeout_alarm_id = add_alarm_in_us(packet_delay, restart_handler, this, true);
}
void PSXController::signal_attention(void)
{
    done = false;
    gpio_put(m_attPin, false);
    timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
}
bool PSXController::auto_shift_data(const uint8_t *out, const uint8_t len)
{
    ps2Idx = 0;
    ps2DataLen = len;
    ps2DataOut = out;

    // Before enumeration the response length is not known, so retain the
    // existing ACK-driven path. Once enumerated, 0x42's response length is
    // already known from the previous header and the whole transaction can
    // be handed to PIO + DMA.
    if (status != ENUMERATED)
    {
        ps2Len = len;
        memset(ps2Data, 0, sizeof(ps2Data[0]));
        signal_attention();
        return true;
    }

    if (ps2Len < 5 || ps2Len > BUFFER_SIZE)
        ps2Len = 5;

    memset(ps2Data, 0, sizeof(ps2Data));
    memset(ps2DataOutBuffer, 0x5A, sizeof(ps2DataOutBuffer));
    memcpy(ps2DataOutBuffer, out, len < BUFFER_SIZE ? len : BUFFER_SIZE);

    cancel_alarm(timeout_alarm_id);
    gpio_set_irq_enabled(m_ackPin, GPIO_IRQ_EDGE_RISE, false);
    gpio_put(m_attPin, false);
    done = false;
    pio_active = true;

    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);

    // X counts the number of bytes after the current byte. The PIO therefore
    // knows exactly when to stop and, importantly, does not ACK the last byte.
    pio_sm_exec(pio, sm, pio_encode_set(pio_x, ps2Len - 1));
    pio_sm_exec(pio, sm, pio_encode_jmp(offset));

    dma_channel_abort(dma_rx);
    dma_channel_abort(dma_tx);
    dma_channel_set_irq1_enabled(dma_rx, true);
    dma_channel_set_write_addr(dma_rx, ps2Data, false);
    dma_channel_set_trans_count(dma_rx, ps2Len, false);
    dma_channel_set_read_addr(dma_tx, ps2DataOutBuffer, false);
    dma_channel_set_trans_count(dma_tx, ps2Len, false);
    pio_started = false;

    // Give ATT the same setup time as the old implementation, then let the
    // state machine and DMA run without CPU intervention.
    timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
    return true;
}
void PSXController::pio_dma_complete()
{
    if (!pio_active)
        return;

    cancel_alarm(timeout_alarm_id);
    dma_channel_set_irq1_enabled(dma_rx, false);
    pio_sm_set_enabled(pio, sm, false);
    pio_active = false;
    pio_started = false;
    done = true;
    valid = isValidReply(ps2Data);

    process_data(false, false);
}

void PSXController::process_data(bool ack, bool timeout)
{
    if (pio_active)
    {
        if (ack)
            return;

        if (timeout && !pio_started)
        {
            // ATT has been low for the normal setup interval. Start both DMA
            // directions and then release the transaction entirely to PIO.
            dma_channel_start_channel_mask((1u << dma_rx) | (1u << dma_tx));
            pio_started = true;
            timeout_alarm_id = add_alarm_in_us(packet_delay, restart_handler, this, true);
            pio_sm_set_enabled(pio, sm, true);
            return;
        }

        if (timeout)
        {
            dma_channel_abort(dma_rx);
            dma_channel_abort(dma_tx);
            pio_sm_set_enabled(pio, sm, false);
            pio_sm_clear_fifos(pio, sm);
            pio_active = false;
            pio_started = false;
            valid = false;
            no_attention();
        }
        return;
    }

    // Existing byte-at-a-time path used during enumeration.
    if (done && ack)
        return;

    cancel_alarm(timeout_alarm_id);
    if (done)
    {
        switch (status)
        {
        case DISCONNECTED:
            if (valid)
            {
                status = CONNECTION_DELAY;
                timeout_alarm_id = add_alarm_in_ms(100, restart_handler, this, true);
                return;
            }
            break;
        case CONNECTION_DELAY:
            status = FIRST_INPUTS;
            auto_shift_data(commandPollInput, sizeof(commandPollInput));
            return;
        case FIRST_INPUTS:
            if (isConfigReply(ps2Data))
            {
                status = ENABLE_ANALOG_MODE;
                auto_shift_data(commandSetMode, sizeof(commandSetMode));
            }
            else
            {
                status = ENTER_CONFIG;
                auto_shift_data(commandEnterConfig, sizeof(commandEnterConfig));
            }
            return;
        case ENTER_CONFIG:
            if (valid)
                status = FIRST_INPUTS;
            else
                status = SECOND_INPUTS;
            auto_shift_data(commandPollInput, sizeof(commandPollInput));
            return;
        case ENABLE_ANALOG_MODE:
            status = ENABLE_RUMBLE;
            auto_shift_data(commandEnableRumble, sizeof(commandEnableRumble));
            return;
        case ENABLE_RUMBLE:
            status = ENABLE_PRESSURES;
            auto_shift_data(commandSetPressures, sizeof(commandSetPressures));
            return;
        case ENABLE_PRESSURES:
            status = ENABLE_PRESSURES_2;
            auto_shift_data(commandSetPressures, sizeof(commandSetPressures));
            return;
        case ENABLE_PRESSURES_2:
            status = EXIT_CONFIG;
            auto_shift_data(commandExitConfig, sizeof(commandExitConfig));
            return;
        case EXIT_CONFIG:
            if (!isConfigReply(ps2Data))
                status = SECOND_INPUTS;
            if (!isValidReply(ps2Data))
            {
                status = DISCONNECTED;
                break;
            }
            auto_shift_data(commandPollInput, sizeof(commandPollInput));
            return;
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

        // For a poll, the controller ID tells us the response length. This is
        // the value the PIO path uses on the next transaction.
        if (valid && status == ENUMERATED &&
            (ps2Data[2] == 0x5A || ps2Data[2] == 0x00))
        {
            uint8_t response_len = 3 + (ps2Data[1] & 0x0F) * 2;
            if (response_len <= BUFFER_SIZE)
                ps2Len = response_len;
        }

        auto_shift_data(m_poll_cmd, sizeof(m_poll_cmd));
        return;
    }

    uint8_t resp = interface.transfer(ps2DataOut != nullptr ? ps2DataOut[ps2Idx] : 0x5A);
    ps2Data[ps2Idx++] = resp;

    if (ps2Idx > ps2DataLen)
        ps2DataOut = nullptr;

    if (ps2Idx == 4)
    {
        if (isValidReply(ps2Data))
        {
            ps2Len = 3 + (ps2Data[1] & 0x0F) * 2;
        }
        else
        {
            valid = false;
            no_attention();
            return;
        }
    }

    if (ps2Idx < ps2Len)
    {
        timeout_alarm_id = add_alarm_in_us(INTER_CMD_BYTE_DELAY, restart_handler, this, true);
        return;
    }

    valid = true;
    no_attention();
}
uint16_t PSXController::read_axis(PS2AxisType axisType)
{
    switch (type)
    {
    case PS2ControllerTypeDigital:
    case PS2ControllerTypeUnknown:
        break;
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