#include "wii_extension.hpp"

#include <string.h>

#include "devices/midi.hpp"
#include "main.hpp"
#include <cmath>
#include "utils.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

static WiiExtension *wiiinstances[2];
void process_data_0(bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (wiiinstances[0])
    {
        wiiinstances[0]->processData(running, timeout, abort_detected, stop_detected);
    }
}
void process_data_1(bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (wiiinstances[1])
    {
        wiiinstances[1]->processData(running, timeout, abort_detected, stop_detected);
    }
}

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    WiiExtension *inst = (WiiExtension *)user_data;
    inst->processData(false, false, false, false);
    return 0;
}

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
// state machine to handle polling wii extensions
void WiiExtension::processData(bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (timeout || abort_detected)
    {
        if (status != WII_INIT_FINISH_ENC)
        {
            failCount++;
        }
        // during high load, there might be the occassional drop, so allow a few failures
        if (failCount > 10 || status == WII_INIT_FINISH_ENC)
        {
            status = WII_INIT_FINISH_ENC;
            mType = WiiExtType::WiiNoExtension;
            restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
            return;
        }
    }
    if (stop_detected)
    {
        if (!abort_detected) {
            failCount = 0;
        }
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
                for (int i = 0; i < WII_ID_LEN; i++)
                {
                    printf("%02x, ", bufferRx[i]);
                }
                printf("\r\n");
                wiiPointer = 0;
                wiiBytes = 6;
                hiRes = false;
                hasTapBar = false;
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
        {
            status = WII_INPUTS_WRITE_PTR;
            if (verifyData(bufferRx, wiiBytes))
            {
                if (s_box)
                {
                    for (int i = 0; i < 8; i++)
                    {
                        bufferRx[i] = (uint8_t)(((bufferRx[i] ^ s_box) + s_box) & 0xFF);
                    }
                }
                // Update the led if it changes
                if (mType == WiiExtType::WiiDjHeroTurntable && ledUpdated)
                {
                    status = WII_INPUTS_UPDATE_LED;
                    ledUpdated = false;
                }
                if (mType == WiiExtType::WiiGuitarHeroDrums)
                {
                    // https://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_World_Tour_(Wii)_Drums
                    uint8_t velocity = ((bufferRx[4] & 0b00000001) |
                                        ((bufferRx[4] & 0b10000000) >> 6) |
                                        ((bufferRx[3] & 0b00000001) << 2) |
                                        ((bufferRx[2] & 0b00000001) << 3) |
                                        ((bufferRx[3] & (0b11100000)) >> 1));
                    uint8_t note = (bufferRx[2] >> 1) & 0x7f;
                    uint8_t channel = ((~bufferRx[3]) >> 1) & 0xF;
                    velocity = 0x7F - velocity;
                    note = 0x7F - note;
                    if (velocity || note)
                    {
                        // Sadly the wii drums don't include the status byte, so we have to make one up.
                        uint8_t packet[] = {0, (uint8_t)(MIDI_CIN_NOTE_ON << 4 | channel), note, velocity};
                        m_device->processMidiData(packet, sizeof(packet));
                    }
                }
                if (mType == WiiExtType::WiiGuitarHeroGuitar)
                {
                    auto lastTapWii = (bufferRx[2] & 0x1f);

                    // GH3 guitars set this bit, while WT and GH5 guitars do not
                    if (!hasTapBar)
                    {
                        if (lastTapWii == 0x0F)
                        {
                            hasTapBar = true;
                        }
                        lastTapWii = 0;
                    }
                }
                memcpy(mBuffer, bufferRx, wiiBytes);
            }
            break;
        }
        case WII_INPUTS_UPDATE_LED:
            status = WII_INPUTS_WRITE_PTR;
            break;
        }
        // add 200us delay between commands otherwise the extension is overwhelmed
        restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case WII_INIT_FINISH_ENC:
        bufferTx[0] = WII_ENCRYPTION_STATE_ID;
        bufferTx[1] = WII_ENCRYPTION_FINISH_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_FB_0:
        bufferTx[0] = 0xFB;
        bufferTx[1] = 0x00;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_ID_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INIT_DRAWSOME:
        bufferTx[0] = 0xFB;
        bufferTx[1] = 0x01;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_0:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_1:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_2:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_READ_ID_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INIT_READ_DATA_WRITE_PTR:
        bufferTx[0] = wiiPointer;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_DATA_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, wiiBytes);
        break;
    case WII_INIT_ENABLE_ENC_0:
        bufferTx[0] = WII_ENCRYPTION_STATE_ID;
        bufferTx[1] = WII_ENCRYPTION_ENABLE_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_1:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID;
        memset(bufferTx + 1, 0, 6);
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 7, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_2:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID_2;
        memset(bufferTx + 1, 0, 6);
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 7, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_3:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID_3;
        memset(bufferTx + 1, 0, 4);
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 5, nullptr, 0);
        break;
    case WII_INIT_ENC_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_ENC_READ_ID_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INPUTS_WRITE_PTR:
        bufferTx[0] = wiiPointer;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INPUTS_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, wiiBytes);
        break;
    case WII_INPUTS_UPDATE_LED:
    {
        // encrypt if encryption is enabled
        uint8_t state = nextEuphoriaLedState ? 1 : 0;
        if (s_box)
        {
            state = (state - s_box) ^ s_box;
        }
        bufferTx[0] = WII_DJ_EUPHORIA;
        bufferTx[1] = state;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    }
    }
}

WiiExtension::WiiExtension(MidiDevice *midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock) : mInterface(block, sda, scl, clock, block == 0 ? process_data_0 : process_data_1), mFound(false), m_block(block), m_device(midiDevice)
{
    wiiinstances[m_block] = this;
    processData(false, false, false, false);
}
WiiExtension::~WiiExtension()
{
    wiiinstances[m_block] = nullptr;
}

void WiiExtension::setEuphoriaLed(bool state)
{
    nextEuphoriaLedState = state;
    ledUpdated = true;
}
void WiiExtension::tick()
{
    // fully driven by DMA and interrupts
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