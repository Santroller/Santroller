#pragma once

// #  include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "eeprom.h"
#include "fxpt_math/fxpt_math.h"
#include "i2c.h"
#include "lib_main.h"
#include "pin.h"
#include "timer.h"
#define I2C_ADDR 0x52
void tickWiiExtInput(Controller_t *controller);
// Why dont we just store a xbox -> ps3 map here
const PROGMEM uint8_t xinputToPs3[16] = {
    PS3_DPAD_UP_BT, PS3_DPAD_DOWN_BT, PS3_DPAD_LEFT_BT, PS3_DPAD_RIGHT_BT,
    PS3_START_BT, PS3_SELECT_BT, PS3_LEFT_STICK_BT, PS3_RIGHT_STICK_BT,
    PS3_L1_BT, PS3_R1_BT, PS3_PS_BT, INVALID_PIN,
    PS3_CROSS_BT, PS3_CIRCLE_BT, PS3_SQUARE_BT, PS3_TRIANGLE_BT};
const PROGMEM uint8_t xinputToKBD[16] = {
    THID_KEYBOARD_SC_UP_ARROW, THID_KEYBOARD_SC_DOWN_ARROW, THID_KEYBOARD_SC_LEFT_ARROW, THID_KEYBOARD_SC_RIGHT_ARROW,
    THID_KEYBOARD_SC_ENTER, THID_KEYBOARD_SC_ESCAPE, THID_KEYBOARD_SC_Q, THID_KEYBOARD_SC_E,
    THID_KEYBOARD_SC_R, THID_KEYBOARD_SC_T, THID_KEYBOARD_SC_F, INVALID_PIN,
    THID_KEYBOARD_SC_Z, THID_KEYBOARD_SC_X, THID_KEYBOARD_SC_C, THID_KEYBOARD_SC_V};
const PROGMEM uint8_t wiiButtonBindings[16] = {
    INVALID_PIN, INVALID_PIN, XBOX_START, XBOX_HOME,
    XBOX_BACK, INVALID_PIN, XBOX_DPAD_DOWN, XBOX_DPAD_RIGHT,
    XBOX_DPAD_UP, XBOX_DPAD_LEFT, XBOX_RB, XBOX_Y,
    XBOX_A, XBOX_X, XBOX_B, XBOX_LB};
const PROGMEM uint8_t wiiButtonBindingsUDraw[16] = {
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,
    XBOX_A, XBOX_B, XBOX_X, INVALID_PIN,
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN};
const PROGMEM uint8_t wiiButtonBindingsNunchuk[16] = {
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN,
    XBOX_A, XBOX_B, INVALID_PIN, INVALID_PIN,
    INVALID_PIN, INVALID_PIN, INVALID_PIN, INVALID_PIN};
const PROGMEM uint8_t wiiButtonBindingsDrum[16] = {
    INVALID_PIN, INVALID_PIN, XBOX_START, INVALID_PIN,
    XBOX_BACK, INVALID_PIN, INVALID_PIN, INVALID_PIN,
    INVALID_PIN, INVALID_PIN, XBOX_RB, XBOX_X,
    XBOX_A, XBOX_Y, XBOX_B, XBOX_LB};

// TODO: this button  layout is probably trash
const PROGMEM uint8_t wiiButtonBindingsDJ[16] = {
    INVALID_PIN, XBOX_B, XBOX_START, INVALID_PIN,
    XBOX_BACK, XBOX_LB, INVALID_PIN, INVALID_PIN,
    INVALID_PIN, INVALID_PIN, XBOX_X, XBOX_RB,
    XBOX_Y, XBOX_A, INVALID_PIN, XBOX_LEFT_STICK};
uint16_t wiiExtensionID = WII_NO_EXTENSION;
// uint16_t buttons;
uint8_t bytes = 6;
bool mapNunchukAccelToRightJoy;
uint8_t data[8];
uint16_t *buttons;

bool verifyData(const uint8_t *dataIn, uint8_t dataSize) {
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
    uint8_t data[6];
    twi_readFromPointerSlow(I2C_ADDR, 0xFA, 6, data);
    if (!verifyData(data, sizeof(data))) {
        return WII_NOT_INITIALISED;
    }
    return data[0] << 8 | data[5];
}
int16_t readDrumExt(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return (data[0] - 0x20) << 10;
        case XBOX_L_Y:
            return (data[1] - 0x20) << 10;
    }
    if (bit_check(data[3], 1)) {
        uint16_t vel = ((7 - (data[3] >> 5)) << 13);
        uint8_t which = (data[2] & 0b01111100) >> 1;
        uint8_t p = 0;
        switch (which) {
            case 0x1B:
                p = XBOX_RB;
                break;
            case 0x19:
                p = XBOX_B;
                break;
            case 0x11:
                p = XBOX_X;
                break;
            case 0x0F:
                p = XBOX_Y;
                break;
            case 0x1E:
                p = XBOX_LB;
                break;
            case 0x12:
                p = XBOX_A;
                break;
        }
        if (p == pin->binding) {
            pin->axisInfo->currentValue = vel;
            return vel;
        }
        return pin->axisInfo->currentValue;
    }
    return 0;
    // Mask out unused bits
    // buttons = ~(data[4] | (data[5] << 8)) & 0xfeff;
    // The standard extension bindings are almost correct, but x and y are
    // swapped, so swap them
    // bit_write(!bit_check(data[5], 3), buttons, wiiButtonBindings[XBOX_X]);
    // bit_write(!bit_check(data[5], 5), buttons, wiiButtonBindings[XBOX_Y]);
}
int16_t readGuitarExt(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return ((data[0] & 0x3f) - 32) << 10;
        case XBOX_L_Y:
            return ((data[1] & 0x3f) - 32) << 10;
        case XBOX_R_X:
            int16_t r_x = ((data[3] & 0x1f) - 14);
            if (r_x < 0) {
                r_x = 0;
            }
            r_x = (r_x << 1) + r_x;
            if (r_x > 31) {
                r_x = 31;
            }
            r_x -= 16;
            r_x <<= 11;
            return r_x;
    }
    return 0;
    // buttons = ~(data[4] | data[5] << 8);
}
int16_t readClassicExtHighRes(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return (data[0] - 0x80) << 8;
        case XBOX_L_Y:
            return (data[2] - 0x80) << 8;
        case XBOX_R_X:
            return (data[1] - 0x80) << 8;
        case XBOX_R_Y:
            return (data[3] - 0x80) << 8;
        case XBOX_LT:
            return data[4];
        case XBOX_RT:
            return data[5];
    }
    return 0;
    // buttons = ~(data[6] | (data[7] << 8));
}
int16_t readClassicExt(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return (data[0] & 0x3f) - 32;
        case XBOX_L_Y:
            return (data[1] & 0x3f) - 32;
        case XBOX_R_X:
            return (((data[0] & 0xc0) >> 3) | ((data[1] & 0xc0) >> 5) | (data[2] >> 7)) - 16;
        case XBOX_R_Y:
            return (data[2] & 0x1f) - 16;
        case XBOX_LT:
            return ((data[3] >> 5) | ((data[2] & 0x60) >> 2));
        case XBOX_RT:
            return data[3] & 0x1f;
    }
    return 0;
    // buttons = ~(data[4] | data[5] << 8);
}
int16_t readNunchukExt(Input_t *pin) {
    static int16_t nunR_Y;
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return (data[0] - 0x80) << 8;
        case XBOX_L_Y:
            return (data[1] - 0x80) << 8;
        case XBOX_R_X:
            // We dont want to run this more than once every tick, so we can calculate
            // both and cache
            if (mapNunchukAccelToRightJoy) {
                uint16_t accX = ((data[2] << 2) | ((data[5] & 0xC0) >> 6)) - 511;
                uint16_t accY = ((data[3] << 2) | ((data[5] & 0x30) >> 4)) - 511;
                uint16_t accZ = ((data[4] << 2) | ((data[5] & 0xC) >> 2)) - 511;
                nunR_Y = fxpt_atan2(accY, accZ);
                return fxpt_atan2(accX, accZ);
            }
        case XBOX_R_Y:
            return nunR_Y;
        case XBOX_LT:
            return ((data[3] >> 5) | ((data[2] & 0x60) >> 2));
        case XBOX_RT:
            return data[3] & 0x1f;
    }
    return 0;
    // buttons = 0;
    // bit_write(!bit_check(data[5], 0), buttons, wiiButtonBindings[XBOX_A]);
    // bit_write(!bit_check(data[5], 1), buttons, wiiButtonBindings[XBOX_B]);
}
int16_t readDJExt(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return ((data[0] & 0x3F) - 0x20) << 10;
        case XBOX_L_Y:
            return ((data[1] & 0x3F) - 0x20) << 10;
        case XBOX_R_X:
            return (data[4] & 1) ? 32 + (0x1F - (data[3] & 0x1F)) : 32 - (data[3] & 0x1F);
        case XBOX_R_Y:
            uint8_t rtt =
                (data[2] & 0x80) >> 7 | (data[1] & 0xC0) >> 5 | (data[0] & 0xC0) >> 3;
            return (data[2] & 1) ? 32 + (0x1F - rtt) : 32 - rtt;
        case XBOX_LT:
            return (data[3] & 0xE0) >> 5 | (data[2] & 0x60) >> 2;
        case XBOX_RT:
            return (data[2] & 0x1E) >> 1;
    }
    return 0;
    // buttons = ~(data[4] << 8 | data[5]) & 0x63CD;
}
int16_t readUDrawExt(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return ((data[2] & 0x0f) << 8) | data[0];
        case XBOX_L_Y:
            return ((data[2] & 0xf0) << 4) | data[1];
        case XBOX_RT:
            return data[3];
    }
    return 0;
    // buttons = 0;
    // bit_write(bit_check(data[5], 0), buttons, wiiButtonBindings[XBOX_A]);
    // bit_write(bit_check(data[5], 1), buttons, wiiButtonBindings[XBOX_B]);
    // bit_write(!bit_check(data[5], 2), buttons, wiiButtonBindings[XBOX_X]);
}
int16_t readDrawsomeExt(Input_t *pin) {
    switch (pin->axisInfo->offset) {
        case XBOX_L_X:
            return data[0] | data[1] << 8;
        case XBOX_L_Y:
            return data[2] | data[3] << 8;
        case XBOX_RT:
            return data[4] | (data[5] & 0x0f) << 8;
    }
    return 0;
    // controller->status = data[5]>>4;
}
int16_t readTataconExt(Input_t *pin) {
    // buttons = ~(data[4] << 8 | data[5]);
    return 0;
}
void initWiiExt(void) {
    wiiExtensionID = readExtID();
    if (wiiExtensionID == WII_NOT_INITIALISED) {
        // Send packets needed to initialise a controller
        twi_writeSingleToPointer(I2C_ADDR, 0xF0, 0x55);
        _delay_us(10);
        twi_writeSingleToPointer(I2C_ADDR, 0xFB, 0x00);
        _delay_us(10);
        wiiExtensionID = readExtID();
        _delay_us(10);
    }
    if (wiiExtensionID == WII_UBISOFT_DRAWSOME_TABLET) {
        twi_writeSingleToPointer(I2C_ADDR, 0xFB, 0x01);
        _delay_us(10);
        twi_writeSingleToPointer(I2C_ADDR, 0xF0, 0x55);
        _delay_us(10);
    }
    int16_t (*readFunction)(Input_t *) = NULL;
    uint8_t bindings[16];
    memcpy_P(bindings, wiiButtonBindings, sizeof(wiiButtonBindings));
    // Most extensions use index 4 and 5 for buttons.
    buttons = (uint16_t *)data + 4;
    if (wiiExtensionID == WII_CLASSIC_CONTROLLER ||
        wiiExtensionID == WII_CLASSIC_CONTROLLER_PRO) {
        // Enable high-res mode
        twi_writeSingleToPointer(I2C_ADDR, 0xFE, 0x03);
        _delay_us(10);
        // Some controllers support high res mode, some dont. Some require it, some
        // dont. To mitigate this issue, we can check if the high res specific bytes
        // are zeroed. However this isnt enough. If a byte is corrupted during
        // transit than it may be triggered. Reading twice will allow us to confirm
        // that nothing was corrupted,
        uint8_t check[8];
        uint8_t validate[8];
        while (true) {
            twi_readFromPointerSlow(I2C_ADDR, 0, sizeof(check), check);
            _delay_us(200);
            twi_readFromPointerSlow(I2C_ADDR, 0, sizeof(validate), validate);
            if (memcmp(check, validate, sizeof(validate)) == 0) {
                bool highRes = (check[0x06] || check[0x07]);
                if (highRes) {
                    readFunction = readClassicExtHighRes;
                    bytes = 8;
                    buttons = (uint16_t *)data + 6;
                } else {
                    readFunction = readClassicExt;
                    bytes = 6;
                }
                break;
            }
            _delay_us(200);
        }
    }
    switch (wiiExtensionID) {
        case WII_GUITAR_HERO_GUITAR_CONTROLLER:
            readFunction = readGuitarExt;
            break;
        case WII_CLASSIC_CONTROLLER:
        case WII_CLASSIC_CONTROLLER_PRO:
            break;
        case WII_NUNCHUK:
            readFunction = readNunchukExt;
            memcpy_P(bindings, wiiButtonBindingsNunchuk, sizeof(wiiButtonBindingsNunchuk));
            break;
        case WII_GUITAR_HERO_DRUM_CONTROLLER:
            readFunction = readDrumExt;
            memcpy_P(bindings, wiiButtonBindingsDrum, sizeof(wiiButtonBindingsDrum));
            break;
        case WII_THQ_UDRAW_TABLET:
            readFunction = readUDrawExt;
            memcpy_P(bindings, wiiButtonBindingsUDraw, sizeof(wiiButtonBindingsUDraw));
            break;
        case WII_UBISOFT_DRAWSOME_TABLET:
            readFunction = readDrawsomeExt;
            break;
        case WII_DJ_HERO_TURNTABLE:
            readFunction = readDJExt;
            memcpy_P(bindings, wiiButtonBindingsDJ, sizeof(wiiButtonBindingsDJ));
            break;
        case WII_TAIKO_NO_TATSUJIN_CONTROLLER:
            readFunction = readTataconExt;
            break;
        default:
            wiiExtensionID = WII_NO_EXTENSION;
            readFunction = NULL;
            return;
    }
    uint8_t consoleBindings[16];
    if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH) {
        memcpy_P(consoleBindings, xinputToPs3, sizeof(xinputToPs3));
    } else if (consoleType == KEYBOARD_MOUSE) {
        memcpy_P(consoleBindings, xinputToKBD, sizeof(xinputToKBD));
    }
    for (int i = 0; i < 16; i++) {
        uint8_t binding = bindings[i];
        Input_t *current = pins + pinCount;
        if (binding != INVALID_PIN) {
            current->binding = binding;
            if (consoleType != XBOX360) {
                current->binding = consoleBindings[binding];
            }
            current->mask = _BV(i);
            current->pullup = true;
            // UDraws are dumb and don't invert these pins.
            if (wiiExtensionID == WII_THQ_UDRAW_TABLET) {
                if (binding == XBOX_A || binding == XBOX_B) {
                    current->pullup = false;
                }
            }
            pinCount++;
        }
    }
    AnalogInput_t *an = analogInfo;
    for (int i = 0; i < XBOX_AXIS_COUNT; i++) {
        Input_t *current = pins + pinCount;
        an->analogRead = readFunction;
        an->offset = i;
        current->axisInfo = an;
        current->binding = i;
        an++;
        pinCount++;
    }
}
void tickWiiExtInput() {
    if (wiiExtensionID == WII_NOT_INITIALISED ||
        wiiExtensionID == WII_NO_EXTENSION ||
        (twi_readFromPointerSlow(I2C_ADDR, 0x00, bytes, data) &&
         !verifyData(data, bytes))) {
        initWiiExt();
        return;
    }
}
bool readWiiButton(Input_t *pin) {
    return !!(*buttons & pin->mask) == pin->pullup;
}
void initWiiExtensions(Configuration_t *config) {
    // mapNunchukAccelToRightJoy = config->main.mapNunchukAccelToRightJoy;
}
// TODO: no idea if this actually works but it seems rightish