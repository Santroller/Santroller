
#include "devices/wii.hpp"

#include <string.h>

#include "interfaces/core.hpp"

uint8_t wiiBytes;
uint8_t wiiPointer = 0;
bool hiRes = false;
uint8_t s_box = 0;
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
    mType = readExtID();
    Core::delayMicroseconds(10);
    if (mType == WII_UBISOFT_DRAWSOME_TABLET) {
        // Drawsome tablet needs some additional init
        mInterface->writeSingleToPointer(WII_ADDR, 0xFB, 0x01);
        Core::delayMicroseconds(10);
    }
    wiiPointer = 0;
    wiiBytes = 6;
    hiRes = false;
    s_box = 0;
    if (mType == WII_CLASSIC_CONTROLLER ||
        mType == WII_CLASSIC_CONTROLLER_PRO) {
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
            hiRes = true;
            wiiBytes = 8;
        } else {
            hiRes = false;
        }
    } else if (mType == WII_TAIKO_NO_TATSUJIN_CONTROLLER) {
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
    static uint8_t wiiData[8];
    memset(wiiData, 0, sizeof(wiiData));
    if (mType == WII_NOT_INITIALISED ||
        mType == WII_NO_EXTENSION ||
        !mInterface->readFromPointerSlow(WII_ADDR, wiiPointer, wiiBytes, wiiData) ||
        !verifyData(wiiData, wiiBytes)) {
        if (initialised) {
            packetIssueCount++;
            if (packetIssueCount < 10) {
                return;
            }
        }
        packetIssueCount = 0;
        initialised = false;
        initWiiExt();
        return;
    }
    // Update the led if it changes
    if (mType == WII_DJ_HERO_TURNTABLE && ledUpdated) {
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
    uint8_t wiiButtonsLow = ~wiiData[4];
    uint8_t wiiButtonsHigh = ~wiiData[5];
    if (hiRes) {
        wiiButtonsLow = ~wiiData[6];
        wiiButtonsHigh = ~wiiData[7];
    }
    switch (mType) {
        case WII_NUNCHUK:
            data->gamepad.accel[0] = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
            data->gamepad.accel[1] = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
            data->gamepad.accel[2] = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
            data->gamepad.a = (wiiButtonsHigh) & (1 << 1);
            data->gamepad.b = (wiiButtonsHigh) & (1 << 0);
            data->gamepad.leftStickX = (wiiData[0] - 0x80) << 8;
            data->gamepad.leftStickY = (wiiData[1] - 0x80) << 8;
            break;
        case WII_CLASSIC_CONTROLLER:
        case WII_CLASSIC_CONTROLLER_PRO:
            if (hiRes) {
                data->gamepad.leftStickX = (wiiData[0] - 0x80) << 8;
                data->gamepad.leftStickY = (wiiData[2] - 0x80) << 8;
                data->gamepad.rightStickX = (wiiData[1] - 0x80) << 8;
                data->gamepad.rightStickY = (wiiData[3] - 0x80) << 8;
                data->gamepad.leftTrigger = wiiData[4] << 8;
                data->gamepad.rightTrigger = wiiData[5] << 8;
            } else {
                data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
                data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
                data->gamepad.rightStickX = ((((wiiData[0] & 0xc0) >> 3) | ((wiiData[1] & 0xc0) >> 5) | (wiiData[2] >> 7)) - 16) << 11;
                data->gamepad.rightStickY = ((wiiData[2] & 0x1f) - 16) << 11;
                data->gamepad.leftTrigger = (((wiiData[3] & 0xE0) >> 5 | (wiiData[2] & 0x60) >> 2)) << 11;
                data->gamepad.rightTrigger = (wiiData[3] & 0x1f) << 11;
            }
            data->gamepad.l2 = (wiiButtonsLow) & (1 << 5);
            data->gamepad.r2 = (wiiButtonsLow) & (1 << 1);
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            data->gamepad.guide = (wiiButtonsLow) & (1 << 3);
            data->gamepad.dpadUp = (wiiButtonsHigh) & (1 << 0);
            data->gamepad.dpadDown = (wiiButtonsLow) & (1 << 6);
            data->gamepad.dpadLeft = (wiiButtonsHigh) & (1 << 1);
            data->gamepad.dpadRight = (wiiButtonsLow) & (1 << 7);
            data->gamepad.leftShoulder = (wiiButtonsHigh) & (1 << 7);
            data->gamepad.rightShoulder = (wiiButtonsHigh) & (1 << 2);

            // TODO: face button mapping swapping
            data->gamepad.a = (wiiButtonsHigh) & (1 << 4);
            data->gamepad.b = (wiiButtonsHigh) & (1 << 6);
            data->gamepad.x = (wiiButtonsHigh) & (1 << 3);
            data->gamepad.y = (wiiButtonsHigh) & (1 << 5);

            break;
        case WII_THQ_UDRAW_TABLET:
            data->mouse.x = ((wiiData[2] & 0x0f) << 8) | wiiData[0];
            data->mouse.y = ((wiiData[2] & 0xf0) << 4) | wiiData[1];
            data->mouse.penPressure = (wiiData[3]);
            data->mouse.left = (~wiiButtonsHigh) & (1 << 2);
            data->mouse.middle = (wiiButtonsHigh) & (1 << 0);
            data->mouse.right = (wiiButtonsHigh) & (1 << 1);
            break;
        case WII_UBISOFT_DRAWSOME_TABLET:
            data->mouse.x = (wiiData[0] | wiiData[1] << 8);
            data->mouse.y = (wiiData[2] | wiiData[3] << 8);
            data->mouse.penPressure = (wiiData[4] | (wiiData[5] & 0x0f) << 8);
            break;
        case WII_GUITAR_HERO_GUITAR_CONTROLLER: {
            data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
            data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            data->guitar.whammy = (wiiData[3] & 0x1f) << 11;
            data->guitar.green = ((wiiButtonsHigh) & (1 << 4));
            data->guitar.red = ((wiiButtonsHigh) & (1 << 6));
            data->guitar.yellow = ((wiiButtonsHigh) & (1 << 3));
            data->guitar.blue = ((wiiButtonsHigh) & (1 << 5));
            data->guitar.orange = ((wiiButtonsHigh) & (1 << 7));
            data->guitar.pedal = ((wiiButtonsHigh) & (1 << 2));
            data->guitar.strumUp = ((wiiButtonsHigh) & (1 << 0));
            data->guitar.strumDown = ((wiiButtonsLow) & (1 << 6));
            uint8_t lastTapWii = (wiiData[2] & 0x1f);
            // convert GH tap bar to solo frets
            if (!hasTapBar) {
                // only guitars with a tap bar will ever set this to 0x0F
                if (lastTapWii == 0x0F) {
                    hasTapBar = true;
                }
            } else if (lastTapWii == 0x0f) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x05) {
                data->guitar.soloGreen = true;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x0A) {
                data->guitar.soloGreen = true;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x0C) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = true;
            } else if (lastTapWii < 0x12) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x14) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x17) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x1A) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = false;
            } else if (lastTapWii < 0x1F) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = true;
            } else {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = true;
            }
            break;
        }
        case WII_GUITAR_HERO_DRUM_CONTROLLER: {
            data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
            data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            // GH drums send us raw midi, so we just ignore all the buttons and deal with the MIDI data as-is
            uint8_t velocity = ((wiiData[4] & 0b00000001) |
                                ((wiiData[4] & 0b10000000) >> 6) |
                                ((wiiData[3] & 0b00000001) << 2) |
                                ((wiiData[2] & 0b00000001) << 3) |
                                ((wiiData[3] & (0b11100000)) >> 1));
            uint8_t note = (wiiData[2] >> 1) & 0x7f;
            uint8_t channel = ((~wiiData[3]) >> 1) & 0xF;
            velocity = 0x7F - velocity;
            note = 0x7F - note;
            if (velocity || note) {
                if (velocity) {
                    mBuffer[mBufferIndex++] = NoteOn | note;
                } else {
                    mBuffer[mBufferIndex++] = NoteOff | note;
                }
                mBuffer[mBufferIndex++] = note;
                mBuffer[mBufferIndex++] = velocity;
            }
            break;
        }
        case WII_DJ_HERO_TURNTABLE: {
            struct {
                union {
                    signed int ltt : 6;
                    struct {
                        unsigned int ltt40 : 5;
                        unsigned int ltt5 : 1;
                    };
                };
            } ltt_t;
            struct {
                union {
                    signed int rtt : 6;
                    struct {
                        unsigned int rtt0 : 1;
                        unsigned int rtt21 : 2;
                        unsigned int rtt43 : 2;
                        unsigned int rtt5 : 1;
                    };
                };
            } rtt_t;
            ltt_t.ltt5 = (wiiData[4] & 1);
            ltt_t.ltt40 = (wiiData[3] & 0x1F);
            rtt_t.rtt0 = (wiiData[2] & 0x80) >> 7;
            rtt_t.rtt21 = (wiiData[1] & 0xC0) >> 6;
            rtt_t.rtt43 = (wiiData[0] & 0xC0) >> 6;
            rtt_t.rtt5 = (wiiData[2] & 1);
            data->gamepad.leftStickX = ((wiiData[0] & 0x3f) - 32) << 10;
            data->gamepad.leftStickY = ((wiiData[1] & 0x3f) - 32) << 10;
            data->gamepad.start = (wiiButtonsLow) & (1 << 2);
            data->gamepad.back = (wiiButtonsLow) & (1 << 4);
            data->turntable.leftTableVelocity = (ltt_t.ltt << 10);
            data->turntable.rightTableVelocity = (rtt_t.rtt << 10);
            data->turntable.crossfader = ((wiiData[2] & 0x1E) >> 1) << 12;
            data->turntable.effectsKnob = (((wiiData[3] & 0xE0) >> 5 | (wiiData[2] & 0x60) >> 2)) << 11;
            data->turntable.leftGreen = (wiiButtonsHigh) & (1 << 3);
            data->turntable.leftRed = (wiiButtonsLow) & (1 << 5);
            data->turntable.leftBlue = (wiiButtonsHigh) & (1 << 7);
            data->turntable.rightGreen = (wiiButtonsHigh) & (1 << 5);
            data->turntable.rightRed = (wiiButtonsLow) & (1 << 1);
            data->turntable.rightBlue = (wiiButtonsHigh) & (1 << 2);
            data->turntable.euphoria = (wiiButtonsHigh) & (1 << 4);
            break;
        }
        case WII_TAIKO_NO_TATSUJIN_CONTROLLER:
            data->taiko.leftDrumRim = (~wiiData[0]) & (1 << 5);
            data->taiko.leftDrumCenter = (~wiiData[0]) & (1 << 6);
            data->taiko.rightDrumRim = (~wiiData[0]) & (1 << 3);
            data->taiko.rightDrumCenter = (~wiiData[0]) & (1 << 4);
            break;
    }
    initialised = true;
}

unsigned WiiDevice::available() {
    if (mBufferIndex > 0) {
        mBufferIndex--;
    }
    return mBufferIndex;
};