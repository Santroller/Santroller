
#include "devices/wii.hpp"

#include <string.h>

#include "interfaces/core.hpp"

bool WiiDevice::verifyData(const uint8_t* dataIn, uint8_t dataSize) {
    uint8_t orCheck = 0x00;   // Check if data is zeroed (bad connection)
    uint8_t andCheck = 0xFF;  // Check if data is maxed (bad init)

    for (int i = 0; i < dataSize; i++) {
        orCheck |= dataIn[i];
        andCheck &= dataIn[i];
    }

    if (orCheck == 0x00 || andCheck == 0xFF) {
        return false;  // No data or bad data
    }

    return true;
}
WiiExtType_t WiiDevice::readExtID() {
    uint8_t data[WII_ID_LEN];
    memset(data, 0, sizeof(data));
    mInterface->readFromPointerSlow(WII_ADDR, WII_READ_ID, WII_ID_LEN, data);
    Core::delayMicroseconds(200);
    if (!verifyData(data, sizeof(data))) {
        return WII_NOT_INITIALISED;
    }
    return static_cast<WiiExtType_t>(data[0] << 8 | data[5]);
}
void WiiDevice::initWiiExt() {
    // Send packets needed to initialise a controller
    if (!mInterface->writeSingleToPointer(WII_ADDR, WII_ENCRYPTION_STATE_ID, WII_ENCRYPTION_FINISH_ID)) {
        return;
    }
    Core::delayMicroseconds(10);
    mInterface->writeSingleToPointer(WII_ADDR, 0xFB, 0x00);
    Core::delayMicroseconds(10);
    parser.mType = readExtID();
    Core::delayMicroseconds(10);
    if (parser.mType == WII_UBISOFT_DRAWSOME_TABLET) {
        // Drawsome tablet needs some additional init
        mInterface->writeSingleToPointer(WII_ADDR, 0xFB, 0x01);
        Core::delayMicroseconds(10);
    }
    wiiPointer = 0;
    wiiBytes = 6;
    parser.hiRes = false;
    s_box = 0;
    if (parser.mType == WII_CLASSIC_CONTROLLER ||
        parser.mType == WII_CLASSIC_CONTROLLER_PRO) {
        // Enable high-res mode (try a few times, sometimes the controller doesnt
        // pick it up)
        for (int i = 0; i < 3; i++) {
            mInterface->writeSingleToPointer(WII_ADDR, WII_SET_RES_MODE, WII_HIGHRES_MODE);
            Core::delayMicroseconds(200);
        }

        // Some controllers support high res mode, some dont. Some require it, some
        // dont. When a controller goes into high res mode, its ID will change,
        // so check.

        uint8_t id[WII_ID_LEN];
        mInterface->readFromPointerSlow(WII_ADDR, WII_READ_ID, WII_ID_LEN, id);
        Core::delayMicroseconds(200);
        if (id[4] == WII_HIGHRES_MODE) {
            parser.hiRes = true;
            wiiBytes = 8;
        } else {
            parser.hiRes = false;
        }
    } else if (parser.mType == WII_TAIKO_NO_TATSUJIN_CONTROLLER) {
        // We can cheat a little with these controllers, as most of the bytes that
        // get read back are constant. Hence we start at 0x5 instead of 0x0.
        wiiPointer = 5;
        wiiBytes = 1;
    }
    Core::delayMicroseconds(200);
    uint8_t data[8] = {0};
    mInterface->readFromPointerSlow(WII_ADDR, wiiPointer, wiiBytes, data);
    uint8_t orCheck = 0x00;
    for (int i = 0; i < wiiBytes; i++) {
        orCheck |= data[i];
    }
    // It appears when you disable encryption on some third party controllers, they stop replying with inputs
    if (orCheck == 0) {
        Core::delayMicroseconds(200);

        // Enable encryption
        mInterface->writeSingleToPointer(WII_ADDR, WII_ENCRYPTION_STATE_ID, WII_ENCRYPTION_ENABLE_ID);
        Core::delayMicroseconds(200);
        // Write zeroed key in blocks
        uint8_t key[6] = {0};
        mInterface->writeToPointer(WII_ADDR, WII_ENCRYPTION_KEY_ID, 6, key);
        Core::delayMicroseconds(200);
        mInterface->writeToPointer(WII_ADDR, WII_ENCRYPTION_KEY_ID_2, 6, key);
        Core::delayMicroseconds(200);
        mInterface->writeToPointer(WII_ADDR, WII_ENCRYPTION_KEY_ID_3, 4, key);
        Core::delayMicroseconds(200);
        uint8_t id[WII_ID_LEN];
        mInterface->readFromPointerSlow(WII_ADDR, WII_READ_ID, WII_ID_LEN, id);
        // first party controllers return all FFs for the ID, third party ones don't
        s_box = FIRST_PARTY_SBOX;
        if (id[3] != 0xFF) {
            s_box = THIRD_PARTY_SBOX;
        }
    }
}
void WiiDevice::setEuphoriaLed(bool state) {
    nextEuphoriaLedState = state;
    ledUpdated = true;
}
void WiiDevice::tick(san_base_t* data) {
    if (Core::micros() - lastTick < 750) {
        return;
    }
    lastTick = micros();
    static uint8_t wiiData[8];
    memset(wiiData, 0, sizeof(wiiData));
    if (parser.mType == WII_NOT_INITIALISED ||
        parser.mType == WII_NO_EXTENSION ||
        !mInterface->readFromPointerSlow(WII_ADDR, wiiPointer, wiiBytes, wiiData) ||
        !verifyData(wiiData, wiiBytes)) {
        if (mFound) {
            packetIssueCount++;
            if (packetIssueCount < 10) {
                return;
            }
        }
        packetIssueCount = 0;
        mFound = false;
        initWiiExt();
        return;
    }
    // Update the led if it changes
    if (parser.mType == WII_DJ_HERO_TURNTABLE && ledUpdated) {
        ledUpdated = false;
        // encrypt if encryption is enabled
        uint8_t state = nextEuphoriaLedState ? 1 : 0;
        if (s_box) {
            state = (state - s_box) ^ s_box;
        }
        mInterface->writeSingleToPointer(WII_ADDR, WII_DJ_EUPHORIA, state);
    }
    packetIssueCount = 0;
    // decrypt if encryption is enabled
    if (s_box) {
        for (int i = 0; i < 8; i++) {
            wiiData[i] = (uint8_t)(((wiiData[i] ^ s_box) + s_box) & 0xFF);
        }
    }
    parser.parse(wiiData, data);
    mFound = true;
}

unsigned WiiDevice::available() {
    if (mBufferIndex > 0) {
        mBufferIndex--;
    }
    return mBufferIndex;
};