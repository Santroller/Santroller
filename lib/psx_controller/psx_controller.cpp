#include "psx_controller.hpp"
#include <hardware/gpio.h>
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
static const uint8_t commandEnableRumble[] = {0x01, 0x4d, 0x00, 0x01, 0xff,
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
PSXController *controller;
void attentionInterrupt(uint gpio, uint32_t events)
{
    controller->processData(true, false);
}

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    PSXController *inst = (PSXController *)user_data;
    inst->processData(false, true);
    return 0;
}
PSXController::PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin) : interface(block, SPI_CPHA_1, SPI_CPOL_1, sck, mosi, miso, false, clock), m_attPin(attPin), m_ackPin(ackPin)
{
    printf("psx controller init!\r\n");
    gpio_init(attPin);
    gpio_set_dir(attPin, true);
    gpio_init(ackPin);
    gpio_set_dir(ackPin, false);
    gpio_set_irq_enabled_with_callback(ackPin, GPIO_IRQ_EDGE_RISE, true, &attentionInterrupt);
    controller = this;
    autoShiftData(commandPollInput, sizeof(commandPollInput));
}
void PSXController::noAttention(void)
{
    done = true;
    gpio_put(m_attPin, true);
    timeout_alarm_id = add_alarm_in_us(packet_delay, restart_handler, this, true);
}
void PSXController::signalAttention(void)
{
    done = false;
    gpio_put(m_attPin, false);
    timeout_alarm_id = add_alarm_in_us(ATTN_DELAY, restart_handler, this, true);
}
bool PSXController::autoShiftData(const uint8_t *out, const uint8_t len)
{
    uint8_t *ret = nullptr;
    ps2Idx = 0;
    ps2Len = len;
    ps2DataLen = len;
    ps2DataOut = out;
    memset(ps2Data, 0, sizeof(ps2Data[0]));
    signalAttention();
    return ret;
}
void PSXController::processData(bool ack, bool timeout)
{
    // if the controller sends an ack after we are done, ignore it!
    if (done && ack)
    {
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
                timeout_alarm_id = add_alarm_in_ms(100, restart_handler, this, true);
                return;
            }
            break;
        case CONNECTION_DELAY:
            status = FIRST_INPUTS;
            autoShiftData(commandPollInput, sizeof(commandPollInput));
            return;
        case FIRST_INPUTS:
            if (isConfigReply(ps2Data))
            {
                status = ENABLE_ANALOG_MODE;
                autoShiftData(commandSetMode, sizeof(commandSetMode));
            }
            else
            {
                status = ENTER_CONFIG;
                autoShiftData(commandEnterConfig, sizeof(commandEnterConfig));
            }
            return;
        case ENTER_CONFIG:
            if (valid)
            {
                // poll the controller so we know it has entered config mode before we send config commands
                status = FIRST_INPUTS;
            }
            else
            {
                // config mode not supported
                status = SECOND_INPUTS;
            }
            autoShiftData(commandPollInput, sizeof(commandPollInput));
            return;
        case ENABLE_ANALOG_MODE:
            status = ENABLE_PRESSURES;
            autoShiftData(commandSetPressures, sizeof(commandSetPressures));
            return;
        case ENABLE_PRESSURES:
            status = ENABLE_PRESSURES_2;
            autoShiftData(commandSetPressures, sizeof(commandSetPressures));
            return;
        case ENABLE_PRESSURES_2:
            status = EXIT_CONFIG;
            autoShiftData(commandExitConfig, sizeof(commandExitConfig));
            return;
        case EXIT_CONFIG:
            if (!isConfigReply(ps2Data))
            {
                status = SECOND_INPUTS;
            }
            if (!isValidReply(ps2Data))
            {
                status = DISCONNECTED;
                break;
            }
            autoShiftData(commandPollInput, sizeof(commandPollInput));
            return;
        case SECOND_INPUTS:
            status = ENUMERATED;
            packet_delay = 5000;
            if (isDualShock2Reply(ps2Data))
            {
                if ((~ps2Data[3]) & (1 << 7) && (~ps2Data[3]) & (1 << 5) && (~ps2Data[3]) & (1 << 6))
                {
                    // Check if dpad left right down is held
                    type = PS2ControllerTypePopNMusic;
                }
                else if ((~ps2Data[3]) & (1 << 7))
                {
                    // Check if dpad left is held
                    type = PS2ControllerTypeGuitar;
                }
                else
                {
                    // pretty much the only controller that works at 1ms
                    packet_delay = 1000;
                    type = PS2ControllerTypeDualshock2;
                }
            }
            else if (isDualShockReply(ps2Data))
            {
                if ((~ps2Data[3]) & (1 << 7))
                {
                    type = PS2ControllerTypeGuitar;
                }
                else
                {
                    type = PS2ControllerTypeDualshock;
                }
            }
            else if (isFlightStickReply(ps2Data))
            {
                type = PS2ControllerTypeFlightStick;
            }
            else if (isNegconReply(ps2Data))
            {
                type = PS2ControllerTypeNegCon;
            }
            else if (isJogconReply(ps2Data))
            {
                type = PS2ControllerTypeJogCon;
            }
            else if (isGunconReply(ps2Data))
            {
                type = PS2ControllerTypeGunCon;
            }
            else if (isMouseReply(ps2Data))
            {
                type = PS2ControllerTypeMouse;
            }
            else if (isDigitalReply(ps2Data))
            {
                type = PS2ControllerTypeDigital;
            }
            // printf("found ps2 controller! %d\r\n", type);
            break;
        case ENUMERATED:
            if (valid)
            {
                memcpy(lastInputs, ps2Data, BUFFER_SIZE);
                missing = 0;
            }
            else
            {
                // allow a few failures before reinit
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
        autoShiftData(commandPollInput, sizeof(commandPollInput));
        return;
    }

    uint8_t resp = interface.transfer(ps2DataOut != nullptr ? ps2DataOut[ps2Idx] : 0x5A);
    ps2Data[ps2Idx++] = resp;
    // no more data in command
    if (ps2Idx > ps2DataLen)
    {
        ps2DataOut = nullptr;
    }
    if (ps2Idx == 4)
    {
        if (isValidReply(ps2Data))
        {
            ps2Len = 3 + (ps2Data[1] & 0x0F) * 2;
        }
        else
        {
            valid = false;
            noAttention();
            return;
        }
    }
    if (ps2Idx < ps2Len)
    {
        timeout_alarm_id = add_alarm_in_us(INTER_CMD_BYTE_DELAY, restart_handler, this, true);
        return;
    }
    valid = true;
    noAttention();
}

uint16_t PSXController::readAxis(PS2AxisType axisType)
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
bool PSXController::readButton(PS2ButtonType buttonType)
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