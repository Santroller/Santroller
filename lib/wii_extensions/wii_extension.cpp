#include "wii_extension.hpp"

#include <string.h>

#include "MidiNotes.h"
#include "main.hpp"
#include <cmath>
#include "utils.h"

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
WiiExtType WiiExtension::readExtID()
{
    uint8_t data[WII_ID_LEN];
    memset(data, 0, sizeof(data));
    mInterface.readRegisterSlow(WII_ADDR, WII_READ_ID, WII_ID_LEN, data);
    sleep_us(200);
    if (!verifyData(data, sizeof(data)))
    {
        return WiiExtType::WiiNoExtension;
    }
    return static_cast<WiiExtType>(data[0] << 8 | data[5]);
}
void WiiExtension::initWiiExt()
{
    // Send packets needed to initialise a controller
    if (!mInterface.writeRegister(WII_ADDR, WII_ENCRYPTION_STATE_ID, WII_ENCRYPTION_FINISH_ID))
    {
        mType = WiiExtType::WiiNoExtension;
        return;
    }
    sleep_us(10);
    mInterface.writeRegister(WII_ADDR, 0xFB, 0x00);
    sleep_us(10);
    mType = readExtID();
    sleep_us(10);
    if (mType == WiiExtType::WiiUbisoftDrawsomeTablet)
    {
        // Drawsome tablet needs some additional init
        mInterface.writeRegister(WII_ADDR, 0xFB, 0x01);
        sleep_us(10);
    }
    wiiPointer = 0;
    wiiBytes = 6;
    hiRes = false;
    s_box = 0;
    if (mType == WiiExtType::WiiClassicController ||
        mType == WiiExtType::WiiClassicControllerPro)
    {
        // Enable high-res mode (try a few times, sometimes the controller doesnt
        // pick it up)
        for (int i = 0; i < 3; i++)
        {
            mInterface.writeRegister(WII_ADDR, WII_SET_RES_MODE, WII_HIGHRES_MODE);
            sleep_us(200);
        }

        // Some controllers support high res mode, some dont. Some require it, some
        // dont. When a controller goes into high res mode, its ID will change,
        // so check.

        uint8_t id[WII_ID_LEN];
        mInterface.readRegisterSlow(WII_ADDR, WII_READ_ID, WII_ID_LEN, id);
        sleep_us(200);
        if (id[4] == WII_HIGHRES_MODE)
        {
            hiRes = true;
            wiiBytes = 8;
        }
        else
        {
            hiRes = false;
        }
    }
    else if (mType == WiiExtType::WiiTaikoNoTatsujinController)
    {
        // We can cheat a little with these controllers, as most of the bytes that
        // get read back are constant. Hence we start at 0x5 instead of 0x0.
        wiiPointer = 5;
        wiiBytes = 1;
    }
    sleep_us(200);
    uint8_t data[8] = {0};
    mInterface.readRegisterSlow(WII_ADDR, wiiPointer, wiiBytes, data);
    uint8_t orCheck = 0x00;
    for (int i = 0; i < wiiBytes; i++)
    {
        orCheck |= data[i];
    }
    // It appears when you disable encryption on some third party controllers, they stop replying with inputs
    if (orCheck == 0)
    {
        sleep_us(200);

        // Enable encryption
        mInterface.writeRegister(WII_ADDR, WII_ENCRYPTION_STATE_ID, WII_ENCRYPTION_ENABLE_ID);
        sleep_us(200);
        // Write zeroed key in blocks
        uint8_t key[6] = {0};
        mInterface.writeRegister(WII_ADDR, WII_ENCRYPTION_KEY_ID, 6, key);
        sleep_us(200);
        mInterface.writeRegister(WII_ADDR, WII_ENCRYPTION_KEY_ID_2, 6, key);
        sleep_us(200);
        mInterface.writeRegister(WII_ADDR, WII_ENCRYPTION_KEY_ID_3, 4, key);
        sleep_us(200);
        uint8_t id[WII_ID_LEN];
        mInterface.readRegisterSlow(WII_ADDR, WII_READ_ID, WII_ID_LEN, id);
        // first party controllers return all FFs for the ID, third party ones don't
        s_box = FIRST_PARTY_SBOX;
        if (id[3] != 0xFF)
        {
            s_box = THIRD_PARTY_SBOX;
        }
    }
}
void WiiExtension::setEuphoriaLed(bool state)
{
    nextEuphoriaLedState = state;
    ledUpdated = true;
}
void WiiExtension::tick()
{
    if (micros() - lastTick > 750) {
        lastTick = micros();
    } else {
        return;
    }
    static uint8_t wiiData[8];
    memset(wiiData, 0, sizeof(wiiData));
    if (mType == WiiExtType::WiiNotInitialised ||
        mType == WiiExtType::WiiNoExtension ||
        !mInterface.readRegisterSlow(WII_ADDR, wiiPointer, wiiBytes, wiiData) ||
        !verifyData(wiiData, wiiBytes))
    {
        if (mFound)
        {
            packetIssueCount++;
            if (packetIssueCount < 10)
            {
                return;
            }
        }
        packetIssueCount = 0;
        mFound = false;
        initWiiExt();
        return;
    }
    packetIssueCount = 0;
    // decrypt if encryption is enabled
    if (s_box)
    {
        for (int i = 0; i < 8; i++)
        {
            wiiData[i] = (uint8_t)(((wiiData[i] ^ s_box) + s_box) & 0xFF);
        }
    }
    mFound = true;
    // Update the led if it changes
    if (mType == WiiExtType::WiiDjHeroTurntable && ledUpdated)
    {
        ledUpdated = false;
        // encrypt if encryption is enabled
        uint8_t state = nextEuphoriaLedState ? 1 : 0;
        if (s_box)
        {
            state = (state - s_box) ^ s_box;
        }
        mInterface.writeRegister(WII_ADDR, WII_DJ_EUPHORIA, state);
    }
    if (mType == WiiExtType::WiiGuitarHeroDrums)
    {
        // https://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_World_Tour_(Wii)_Drums
        uint8_t velocity = ((wiiData[4] & 0b00000001) |
                            ((wiiData[4] & 0b10000000) >> 6) |
                            ((wiiData[3] & 0b00000001) << 2) |
                            ((wiiData[2] & 0b00000001) << 3) |
                            ((wiiData[3] & (0b11100000)) >> 1));
        uint8_t note = (wiiData[2] >> 1) & 0x7f;
        uint8_t channel = ((~wiiData[3]) >> 1) & 0xF;
        velocity = 0x7F - velocity;
        note = 0x7F - note;
        if (velocity || note)
        {
            // Sadly the wii drums don't include the status byte, so we have to make one up.
            uint8_t packet[] = {MIDI_NAMESPACE::NoteOn, channel, note, velocity};
            midiInterface.parsePacket(packet, sizeof(packet));
        }
    }
    if (mType == WiiExtType::WiiGuitarHeroGuitar)
    {
        auto lastTapWii = (wiiData[2] & 0x1f);

        // GH3 guitars set this bit, while WT and GH5 guitars do not
        if (!hasTapBar)
        {
            if (lastTapWii == 0x0F)
            {
                hasTapBar = true;
            }
            lastTapWii = 0;
            lastTap = 0x80;
        }
        else if (lastTapWii == 0x0f)
        {
            lastTap = 0x80;
        }
        else if (lastTapWii < 0x05)
        {
            lastTap = 0x15;
        }
        else if (lastTapWii < 0x0A)
        {
            lastTap = 0x30;
        }
        else if (lastTapWii < 0x0C)
        {
            lastTap = 0x4D;
        }
        else if (lastTapWii < 0x12)
        {
            lastTap = 0x66;
        }
        else if (lastTapWii < 0x14)
        {
            lastTap = 0x9A;
        }
        else if (lastTapWii < 0x17)
        {
            lastTap = 0xAF;
        }
        else if (lastTapWii < 0x1A)
        {
            lastTap = 0xC9;
        }
        else if (lastTapWii < 0x1F)
        {
            lastTap = 0xE6;
        }
        else
        {
            lastTap = 0xFF;
        }
    }
    memcpy(mBuffer, wiiData, sizeof(wiiData));
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
                return (mBuffer[2]) << 8;
            case WiiAxisType::WiiAxisClassicRightStickX:
                return (mBuffer[1]) << 8;
            case WiiAxisType::WiiAxisClassicRightStickY:
                return (mBuffer[3]) << 8;
            case WiiAxisType::WiiAxisClassicLeftTrigger:
                return mBuffer[4] << 8;
            case WiiAxisType::WiiAxisClassicRightTrigger:
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
                return ((mBuffer[1] & 0x3f)) << 10;
            case WiiAxisType::WiiAxisClassicRightStickX:
                return ((((mBuffer[0] & 0xc0) >> 3) | ((mBuffer[1] & 0xc0) >> 5) | (mBuffer[2] >> 7))) << 11;
            case WiiAxisType::WiiAxisClassicRightStickY:
                return ((mBuffer[2] & 0x1f)) << 11;
            case WiiAxisType::WiiAxisClassicLeftTrigger:
                return (((mBuffer[3] & 0xE0) >> 5 | (mBuffer[2] & 0x60) >> 2)) << 11;
            case WiiAxisType::WiiAxisClassicRightTrigger:
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
        case WiiAxisType::WiiAxisDjTurntableRight: {
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
        case WiiAxisType::WiiAxisGuitarTapBar:
            return lastTap;
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
            return ((mBuffer[2] << 2) | ((mBuffer[5] & 0xC0) >> 6));
        case WiiAxisType::WiiAxisNunchukAccelerationY:
            return ((mBuffer[3] << 2) | ((mBuffer[5] & 0x30) >> 4));
        case WiiAxisType::WiiAxisNunchukAccelerationZ:
            return ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2));
        case WiiAxisType::WiiAxisNunchukRotationPitch:
            return std::atan2(((mBuffer[3] << 2) | ((mBuffer[5] & 0x30) >> 4)), ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)));
        case WiiAxisType::WiiAxisNunchukRotationRoll:
            return std::atan2(((mBuffer[2] << 2) | ((mBuffer[5] & 0xC0) >> 6)), ((mBuffer[4] << 2) | ((mBuffer[5] & 0xC) >> 2)));
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
        case WiiButtonGuitarYellow:
            return ((wiiButtonsHigh) & (1 << 3));
        case WiiButtonGuitarGreen:
            return ((wiiButtonsHigh) & (1 << 4));
        case WiiButtonGuitarBlue:
            return ((wiiButtonsHigh) & (1 << 5));
        case WiiButtonGuitarRed:
            return ((wiiButtonsHigh) & (1 << 6));
        case WiiButtonGuitarOrange:
            return ((wiiButtonsHigh) & (1 << 7));
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