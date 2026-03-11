#include "psx_controller.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdio.h>

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
static const uint8_t commandEnterConfig[] = {0x43, 0x00, 0x01, 0x5A,
                                             0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandExitConfig[] = {0x43, 0x00, 0x00, 0x5A,
                                            0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandEnableRumble[] = {0x4d, 0x00, 0x01, 0xff,
                                              0xff, 0xff, 0xff, 0xff};
static const uint8_t commandGetStatus[] = {0x45, 0x00, 0x00, 0x5A,
                                           0x5A, 0x5A, 0x5A, 0x5A};

static const uint8_t commandGetExtra[] = {0x46, 0x00, 0x00, 0x5A,
                                          0x5A, 0xA, 0x5A, 0x5A};
static const uint8_t commandGetExtra2[] = {0x46, 0x00, 0x00, 0x5A,
                                           0x5A, 0xA, 0x5A, 0x5A};

static const uint8_t commandSetMode[] = {0x44, 0x00, /* enabled */ 0x01,
                                         /* locked */ 0x00, 0x02};

// Enable all analog values
static const uint8_t commandSetPressures[] = {0x4F, 0x00, 0xFF, 0xFF,
                                              0x03, 0x00, 0x00, 0x00};

// The pressures data format is 0x4F, 0x00, followed by 18 bits, for each
// of the 18 possible bytes that a controller can return

// For some controllers, we want the buttons (2 bytes, and then 4 bytes of
// sticks (rx, ry, lx, ly)).
static const uint8_t commandSetPressuresSticksOnly[] = {
    0x4F, 0x00, 0b111111, 0x00, 0b00, 0x00, 0x00, 0x00};

// For guitars, we want the buttons (2 bytes, and then ly).
static const uint8_t commandSetPressuresGuitar[] = {0x4F, 0x00, 0b110001, 0x00,
                                                    0b00, 0x00, 0x00, 0x00};

// For the mouse, we want the buttons (2 bytes, and then 2 bytes of axis (x,
// y)). We also want triggers, which luckily happen to be the last two
// bits and hence in their own byte
static const uint8_t commandSetPressuresMouse[] = {0x4F, 0x00, 0b1111, 0x00,
                                                   0b11, 0x00, 0x00, 0x00};

static const uint8_t commandPollInput[] = {0x42, 0x00, 0xFF, 0xFF};
static volatile bool spiAcknowledged;
void attentionInterrupt(uint gpio, uint32_t events)
{
    spiAcknowledged = true;
}
PSXController::PSXController(uint8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t attPin, uint8_t ackPin, MultitapPort port) : interface(block, SPI_CPHA_1, SPI_CPOL_1, sck, mosi, miso, false, clock), m_port(port), m_attPin(attPin), m_ackPin(ackPin), connected(false)
{
    gpio_init(attPin);
    gpio_set_dir(attPin, true);
    gpio_init(ackPin);
    gpio_set_dir(ackPin, false);
    gpio_set_irq_enabled_with_callback(ackPin, GPIO_IRQ_EDGE_RISE, true, &attentionInterrupt);
    spiAcknowledged = false;
}
void PSXController::noAttention(void)
{
    gpio_put(m_attPin, true);
    sleep_us(ATTN_DELAY);
}
void PSXController::signalAttention(void)
{
    gpio_put(m_attPin, false);
    sleep_us(ATTN_DELAY);
}
void PSXController::shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len)
{
    unsigned long m;
    for (uint8_t i = 0; i < len; ++i)
    {
        uint8_t resp = interface.transfer(out != nullptr ? out[i] : 0x5A);
        if (in != nullptr)
        {
            in[i] = resp;
        }

        spiAcknowledged = false;
        m = to_us_since_boot(get_absolute_time());
        while (!spiAcknowledged)
        {
            // If for some reason the controller doesn't respond to us, we need to
            // make sure we finish the transfer anyways
            if (to_us_since_boot(get_absolute_time()) - m > INTER_CMD_BYTE_DELAY)
            {
                break;
            }
        }
    }
}
bool PSXController::autoShiftData(uint8_t *in, const uint8_t *out, const uint8_t len)
{
    uint8_t *ret = nullptr;
    uint8_t port = m_port;

    if (len >= 2 && len <= BUFFER_SIZE)
    {
        signalAttention();
        // All commands have at least 3 bytes, so shift out those first
        shiftDataInOut(&port, in, 1);
        shiftDataInOut(out, in + 1, 2);
        if (isValidReply(in))
        {
            // Reply is good, get full length
            uint8_t replyLen = (in[1] & 0x0F) * 2;

            uint8_t left = replyLen - (len - 2);
            // Shift out rest of command
            shiftDataInOut(out + 2, in + 3, len - 2);

            if (left == 0)
            {
                // The whole reply was gathered
                ret = in;
            }
            else if (len + left <= BUFFER_SIZE)
            {
                // Part of reply is still missing and we have space for it
                shiftDataInOut(nullptr, in + len + 1, left);
                ret = in;
            }
            else
            {
                // Reply incomplete but not enough space provided
            }
        }
        noAttention();
    }
    return ret;
}
bool PSXController::sendCommand(uint8_t *in, const uint8_t *buf, uint8_t len)
{
    bool ret = false;
    unsigned long start = to_ms_since_boot(get_absolute_time());
    do
    {
        /* We can't know if we have successfully enabled analog mode until
         * we get out of config mode, so let's just be happy if we get a few
         * consecutive valid replies
         */
        if (autoShiftData(in, buf, len))
        {
            if (buf == commandEnterConfig)
            {
                ret = isConfigReply(in);
            }
            else if (buf == commandExitConfig)
            {
                ret = !isConfigReply(in);
            }
        }

        if (!ret)
        {
            sleep_ms(COMMAND_RETRY_INTERVAL);
        }
    } while (!ret && to_ms_since_boot(get_absolute_time()) - start <= COMMAND_TIMEOUT);
    return ret;
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
            return (ps2Data[7] - 128) << 8;
        case PS2AxisLeftStickY:
            return -(ps2Data[8] - 127) << 8;
        case PS2AxisRightStickX:
            return (ps2Data[5] - 128) << 8;
        case PS2AxisRightStickY:
            return -(ps2Data[6] - 127) << 8;
        case PS2AxisDualshock2RightButton:
            return ps2Data[9] << 8;
        case PS2AxisDualshock2LeftButton:
            return ps2Data[10] << 8;
        case PS2AxisDualshock2UpButton:
            return ps2Data[11] << 8;
        case PS2AxisDualshock2DownButton:
            return ps2Data[12] << 8;
        case PS2AxisDualshock2Triangle:
            return ps2Data[13] << 8;
        case PS2AxisDualshock2Circle:
            return ps2Data[14] << 8;
        case PS2AxisDualshock2Cross:
            return ps2Data[15] << 8;
        case PS2AxisDualshock2Square:
            return ps2Data[16] << 8;
        case PS2AxisDualshock2L1:
            return ps2Data[17] << 8;
        case PS2AxisDualshock2R1:
            return ps2Data[18] << 8;
        case PS2AxisDualshock2L2:
            return ps2Data[19] << 8;
        case PS2AxisDualshock2R2:
            return ps2Data[20] << 8;
        default:
            return 0;
        }
    case PS2ControllerTypeDualshock:
    case PS2ControllerTypeFlightStick:
        switch (axisType)
        {
        case PS2AxisLeftStickX:
            return (ps2Data[7] - 128) << 8;
        case PS2AxisLeftStickY:
            return -(ps2Data[8] - 127) << 8;
        case PS2AxisRightStickX:
            return (ps2Data[5] - 128) << 8;
        case PS2AxisRightStickY:
            return -(ps2Data[6] - 127) << 8;
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeGuitar:
        switch (axisType)
        {
        case PS2AxisGuitarWhammy:
            return -(ps2Data[8] - 0x80) << 9;
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeNegCon:
        switch (axisType)
        {
        case PS2AxisNegConTwist:
            return (ps2Data[5] - 128) << 8;
        case PS2AxisNegConI:
            return ps2Data[6];
        case PS2AxisNegConIi:
            return ps2Data[7];
        case PS2AxisNegConL:
            return ps2Data[8];
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeJogCon:
        switch (axisType)
        {
        case PS2AxisJogConWheel:
            return (ps2Data[6] << 8) | ps2Data[5];
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeGunCon:
        switch (axisType)
        {
        case PS2AxisGunConHSync:
            return (ps2Data[6] << 8) | ps2Data[5];
        case PS2AxisGunConVSync:
            return (ps2Data[8] << 8) | ps2Data[7];
        default:
            return 0;
        }
        break;
    case PS2ControllerTypeMouse:
        switch (axisType)
        {
        case PS2AxisMouseX:
            return (ps2Data[5] - 128) << 8;
        case PS2AxisMouseY:
            return -(ps2Data[6] - 127) << 8;

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
            return (~ps2Data[3]) & (1 << 0);
        case PS2ButtonL3:
            return (~ps2Data[3]) & (1 << 1);
        case PS2ButtonR3:
            return (~ps2Data[3]) & (1 << 2);
        case PS2ButtonStart:
            return (~ps2Data[3]) & (1 << 3);
        case PS2ButtonDpadUp:
            return (~ps2Data[3]) & (1 << 4);
        case PS2ButtonDpadRight:
            return (~ps2Data[3]) & (1 << 5);
        case PS2ButtonDpadDown:
            return (~ps2Data[3]) & (1 << 6);
        case PS2ButtonDpadLeft:
            return (~ps2Data[3]) & (1 << 7);
        case PS2ButtonL2:
            return (~ps2Data[4]) & (1 << 0);
        case PS2ButtonR2:
            return (~ps2Data[4]) & (1 << 1);
        case PS2ButtonL1:
            return (~ps2Data[4]) & (1 << 2);
        case PS2ButtonR1:
            return (~ps2Data[4]) & (1 << 3);
        case PS2ButtonTriangle:
            return (~ps2Data[4]) & (1 << 4);
        case PS2ButtonCircle:
            return (~ps2Data[4]) & (1 << 5);
        case PS2ButtonCross:
            return (~ps2Data[4]) & (1 << 6);
        case PS2ButtonSquare:
            return (~ps2Data[4]) & (1 << 7);
        default:
            return 0;
        }
    case PS2ControllerTypeGuitar:
        switch (buttonType)
        {
        case PS2ButtonGuitarGreen:
            return (~ps2Data[4]) & (1 << 1);
        case PS2ButtonGuitarRed:
            return (~ps2Data[4]) & (1 << 5);
        case PS2ButtonGuitarYellow:
            return (~ps2Data[4]) & (1 << 4);
        case PS2ButtonGuitarBlue:
            return (~ps2Data[4]) & (1 << 6);
        case PS2ButtonGuitarOrange:
            return (~ps2Data[4]) & (1 << 7);
        case PS2ButtonGuitarStrumUp:
            return (~ps2Data[3]) & (1 << 4);
        case PS2ButtonGuitarStrumDown:
            return (~ps2Data[3]) & (1 << 6);
        case PS2ButtonGuitarDpadUp:
            return ((ps2Data[5] >> 6) == 0) && ps2Data[5] != 0;
        case PS2ButtonGuitarDpadDown:
            return ((ps2Data[5] >> 6) == 2) && ps2Data[5] != 128;
        case PS2ButtonGuitarDpadLeft:
            return ((ps2Data[5] >> 6) == 1) && ps2Data[5] != 127 && ps2Data[5] != 123;
        case PS2ButtonGuitarDpadRight:
            return ((ps2Data[5] >> 6) == 3) && ps2Data[5] != 255;
        case PS2ButtonGuitarSelect:
            return (~ps2Data[3]) & (1 << 0);
        case PS2ButtonGuitarStart:
            return (~ps2Data[3]) & (1 << 3);
        case PS2ButtonGuitarTilt:
            return (~ps2Data[4]) & (1 << 0);
        case PS2ButtonGuitarTapGreen:
            return ps2Data[7] <= 0x3F;
        case PS2ButtonGuitarTapRed:
            return ps2Data[7] <= 0x5F && ps2Data[7] > 0x2F;
        case PS2ButtonGuitarTapYellow:
            return (ps2Data[7] <= 0x6F && ps2Data[7] > 0x5F) || (ps2Data[7] <= 0x9F && ps2Data[7] > 0x8F);
        case PS2ButtonGuitarTapBlue:
            return ps2Data[7] <= 0xEF && ps2Data[7] > 0xBF;
        case PS2ButtonGuitarTapOrange:
            return ps2Data[7] > 0xEF;
        default:
            return 0;
        }
    case PS2ControllerTypeNegCon:
        switch (buttonType)
        {
        case PS2ButtonNegConA:
            return (~ps2Data[4]) & (1 << 5);
        case PS2ButtonNegConB:
            return (~ps2Data[4]) & (1 << 4);
        case PS2ButtonNegConStart:
            return (~ps2Data[3]) & (1 << 3);
        case PS2ButtonNegConR:
            return (~ps2Data[4]) & (1 << 3);
        default:
            return 0;
        }
    case PS2ControllerTypeMouse:
        switch (buttonType)
        {
        case PS2ButtonMouseLeft:
            return (~ps2Data[3]) & (1 << 3);
        case PS2ButtonMouseRight:
            return (~ps2Data[3]) & (1 << 2);
        default:
            return 0;
        }
    case PS2ControllerTypeTaiko:
        switch (buttonType)
        {
        case PS2ButtonTaikoRimLeft:
            return (~ps2Data[4]) & (1 << 2);
        case PS2ButtonTaikoRimRight:
            return (~ps2Data[4]) & (1 << 3);
        case PS2ButtonTaikoCenterLeft:
            return (~ps2Data[3]) & (1 << 7);
        case PS2ButtonTaikoCenterRight:
            return (~ps2Data[4]) & (1 << 5);
        default:
            return 0;
        }
    }
    return false;
}
extern unsigned long millis_at_boot;
bool PSXController::controller_valid(MultitapPort m_port)
{
    return autoShiftData(ps2Data, commandPollInput, sizeof(commandPollInput)) != 0;
}
void PSXController::tick()
{
    // PS2 guitars die if you poll them too fast
    if (type == PS2ControllerTypeGuitar && to_us_since_boot(get_absolute_time()) - last < 3000)
    {
        return;
    }
    if (!connected)
    {
        if (!autoShiftData(ps2Data, commandPollInput, sizeof(commandPollInput)))
        {
            return;
        }
        if (sendCommand(ps2Data, commandEnterConfig, sizeof(commandEnterConfig)))
        {
            // Enable analog sticks
            sendCommand(ps2Data, commandSetMode, sizeof(commandSetMode));
            // Enable pressure sensitive buttons
            sendCommand(ps2Data, commandSetPressures, sizeof(commandSetPressures));
            sendCommand(ps2Data, commandExitConfig, sizeof(commandExitConfig));
        }
        autoShiftData(ps2Data, commandPollInput, sizeof(commandPollInput));
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
        connected = true;
        invalidCount = 0;
    }
    if (connected)
    {
        if (autoShiftData(ps2Data, commandPollInput, sizeof(commandPollInput)))
        {
            invalidCount = 0;
            if (isConfigReply(ps2Data))
            {
                // We're stuck in config mode, try to get out
                sendCommand(ps2Data, commandExitConfig, sizeof(commandExitConfig));
                autoShiftData(ps2Data, commandPollInput, sizeof(commandPollInput));
            }
            last = to_us_since_boot(get_absolute_time());
        }
        else
        {
            // Ocassionally, the controller returns a bad packet because it isn't ready. We should ignore that instead of reinitialisng, and
            // We only want to reinit if we recevied several bad packets in a row.
            invalidCount++;
            if (invalidCount > 10)
            {
                connected = false;
            }
        }
    }
}