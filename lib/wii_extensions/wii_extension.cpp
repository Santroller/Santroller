#include "wii_extension.hpp"

#include <string.h>

#include "MidiNotes.h"

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
}