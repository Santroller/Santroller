
#include "wii.h"

#include <string.h>

#include "Arduino.h"
#include "commands.h"
#include "config.h"
#include "controllers.h"
#include "io.h"
#include "shared_main.h"
uint8_t wiiBytes;
#ifdef INPUT_WII
uint8_t wiiPointer = 0;
bool hiRes = false;
uint8_t s_box = 0;
bool verifyData(const uint8_t* dataIn, uint8_t dataSize) {
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
uint16_t readExtID(void) {
    uint8_t data[WII_ID_LEN];
    memset(data, 0, sizeof(data));
    twi_readFromPointerSlow(WII_TWI_PORT, WII_ADDR, WII_READ_ID, WII_ID_LEN, data);
    delayMicroseconds(200);
    if (!verifyData(data, sizeof(data))) {
        return WII_NOT_INITIALISED;
    }
    return data[0] << 8 | data[5];
}
void initWiiExt(void) {
    // Send packets needed to initialise a controller
    if (!twi_writeSingleToPointer(WII_TWI_PORT, WII_ADDR, WII_ENCRYPTION_STATE_ID, WII_ENCRYPTION_FINISH_ID)) {
        return;
    }
    delayMicroseconds(10);
    twi_writeSingleToPointer(WII_TWI_PORT, WII_ADDR, 0xFB, 0x00);
    delayMicroseconds(10);
    wiiControllerType = readExtID();
    delayMicroseconds(10);
    if (wiiControllerType == WII_UBISOFT_DRAWSOME_TABLET) {
        // Drawsome tablet needs some additional init
        twi_writeSingleToPointer(WII_TWI_PORT, WII_ADDR, 0xFB, 0x01);
        delayMicroseconds(10);
    }
    wiiPointer = 0;
    wiiBytes = 6;
    hiRes = false;
    s_box = 0;
    if (wiiControllerType == WII_CLASSIC_CONTROLLER ||
        wiiControllerType == WII_CLASSIC_CONTROLLER_PRO) {
        // Enable high-res mode (try a few times, sometimes the controller doesnt
        // pick it up)
        for (int i = 0; i < 3; i++) {
            twi_writeSingleToPointer(WII_TWI_PORT, WII_ADDR, WII_SET_RES_MODE, WII_HIGHRES_MODE);
            delayMicroseconds(200);
        }

        // Some controllers support high res mode, some dont. Some require it, some
        // dont. When a controller goes into high res mode, its ID will change,
        // so check.

        uint8_t id[WII_ID_LEN];
        twi_readFromPointerSlow(WII_TWI_PORT, WII_ADDR, WII_READ_ID, WII_ID_LEN, id);
        delayMicroseconds(200);
        if (id[4] == WII_HIGHRES_MODE) {
            hiRes = true;
            wiiBytes = 8;
        } else {
            hiRes = false;
        }
    } else if (wiiControllerType == WII_TAIKO_NO_TATSUJIN_CONTROLLER) {
        // We can cheat a little with these controllers, as most of the bytes that
        // get read back are constant. Hence we start at 0x5 instead of 0x0.
        wiiPointer = 5;
        wiiBytes = 1;
    }
    delayMicroseconds(200);
    uint8_t data[8] = {0};
    twi_readFromPointerSlow(WII_TWI_PORT, WII_ADDR, wiiPointer, wiiBytes, data);
    uint8_t orCheck = 0x00;
    for (int i = 0; i < wiiBytes; i++) {
        orCheck |= data[i];
    }
    // It appears when you disable encryption on some third party controllers, they stop replying with inputs
    if (orCheck == 0) {
        delayMicroseconds(200);

        // Enable encryption
        twi_writeSingleToPointer(WII_TWI_PORT, WII_ADDR, WII_ENCRYPTION_STATE_ID, WII_ENCRYPTION_ENABLE_ID);
        delayMicroseconds(200);
        // Write zeroed key in blocks
        uint8_t key[6] = {0};
        twi_writeToPointer(WII_TWI_PORT, WII_ADDR, WII_ENCRYPTION_KEY_ID, 6, key);
        delayMicroseconds(200);
        twi_writeToPointer(WII_TWI_PORT, WII_ADDR, WII_ENCRYPTION_KEY_ID_2, 6, key);
        delayMicroseconds(200);
        twi_writeToPointer(WII_TWI_PORT, WII_ADDR, WII_ENCRYPTION_KEY_ID_3, 4, key);
        delayMicroseconds(200);
        uint8_t id[WII_ID_LEN];
        twi_readFromPointerSlow(WII_TWI_PORT, WII_ADDR, WII_READ_ID, WII_ID_LEN, id);
        // first party controllers return all FFs for the ID, third party ones don't
        s_box = FIRST_PARTY_SBOX;
        if (id[3] != 0xFF) {
            s_box = THIRD_PARTY_SBOX;
        }
    }
}
bool initialised = false;
bool lastWiiEuphoriaLed = false;
bool hadDrum = false;
static uint8_t packetIssueCount = 0;
bool wiiDataValid() {
    return initialised;
}
uint8_t* tickWii() {
    static uint8_t data[8];
    memset(data, 0, sizeof(data));
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
    // Update the led if it changes
    if (wiiControllerType == WII_DJ_HERO_TURNTABLE) {
        if (lastWiiEuphoriaLed != lastEuphoriaLed) {
            lastWiiEuphoriaLed = lastEuphoriaLed;
            // encrypt if encryption is enabled
            uint8_t state = lastEuphoriaLed ? 1 : 0;
            if (s_box) {
                state = (state - s_box) ^ s_box;
            }
            twi_writeSingleToPointer(WII_TWI_PORT, WII_ADDR, WII_DJ_EUPHORIA, state);
        }
    }
#endif
    if (wiiControllerType == WII_NOT_INITIALISED ||
        wiiControllerType == WII_NO_EXTENSION ||
        !twi_readFromPointerSlow(WII_TWI_PORT, WII_ADDR, wiiPointer, wiiBytes, data) ||
        !verifyData(data, wiiBytes)) {
        if (initialised) {
            packetIssueCount++;
            if (packetIssueCount < 10) {
                return lastSuccessfulWiiPacket;
            }
        }
        packetIssueCount = 0;
        initialised = false;
        initWiiExt();
        return NULL;
    }
    packetIssueCount = 0;
    // decrypt if encryption is enabled
    if (s_box) {
        for (int i = 0; i < 8; i++) {
            data[i] = (uint8_t)(((data[i] ^ s_box) + s_box) & 0xFF);
        }
    }
    if (wiiControllerType == WII_GUITAR_HERO_DRUM_CONTROLLER) {
        // https://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_World_Tour_(Wii)_Drums
        uint8_t velocity = ((data[4] & 0b00000001) |
                            ((data[4] & 0b10000000) >> 6) |
                            ((data[3] & 0b00000001) << 2) |
                            ((data[2] & 0b00000001) << 3) |
                            ((data[3] & (0b11100000)) >> 1));
        uint8_t note = (data[2] >> 1) & 0x7f;
        uint8_t channel = ((~data[3]) >> 1) & 0xF;
        velocity = 0x7F - velocity;
        note = 0x7F - note;
        if (velocity || note) {
            // map gh notes to their rb counterparts in rb mode
#if DEVICE_TYPE == ROCK_BAND_DRUMS
            switch (note) {
                case GH_MIDI_NOTE_GREEN:
                    onNote(channel, RB_MIDI_NOTE_GREEN, velocity);
                    break;
                case GH_MIDI_NOTE_RED:
                    onNote(channel, RB_MIDI_NOTE_RED, velocity);
                    break;
                case GH_MIDI_NOTE_YELLOW:
                    onNote(channel, RB_MIDI_NOTE_YELLOW, velocity);
                    break;
                case GH_MIDI_NOTE_BLUE:
                    onNote(channel, RB_MIDI_NOTE_BLUE, velocity);
                    break;
                case GH_MIDI_NOTE_ORANGE:
                    onNote(channel, RB_MIDI_NOTE_GREEN, velocity);
                    break;
                default:
                    onNote(channel, note, velocity);
                    break;
            }
#else
            onNote(channel, note, velocity);
#endif
        }
    }
    initialised = true;
    return data;
}
#endif