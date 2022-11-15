#include "Arduino.h"
#include "config.h"
#include "controller_reports.h"
#include "fxpt_math.h"
#include "io.h"
#include "pins.h"
#include "pins_define.h"
#include "ps2.h"
#include "util.h"
#include "wii.h"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12

uint8_t debounce[DIGITAL_COUNT];
uint8_t drumVelocity[8];
uint8_t lastTap;
uint8_t lastTapShift;
uint16_t wiiControllerType = WII_NO_EXTENSION;
uint8_t ps2ControllerType = PSX_NO_DEVICE;
uint8_t lastSuccessfulPS2Packet[BUFFER_SIZE];
uint8_t lastSuccessfulWiiPacket[8];
uint8_t lastSuccessfulGH5Packet[2];
uint8_t lastSuccessfulTurntablePacketLeft[3];
uint8_t lastSuccessfulTurntablePacketRight[3];
long lastSuccessfulGHWTPacket;
bool lastGHWTWasSuccessful = false;
bool lastGH5WasSuccessful = false;
bool lastTurntableWasSuccessfulLeft = false;
bool lastTurntableWasSuccessfulRight = false;
bool lastWiiWasSuccessful = false;
bool lastPS2WasSuccessful = false;
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Led_t;
Led_t ledState[LED_COUNT];
static const uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
void init_main(void) {
    initPins();
    twi_init();
    spi_begin();
    memset(ledState, 0, sizeof(ledState));
}
int16_t handle_calibration_xbox_int(int16_t orig_val, int16_t offset, uint16_t multiplier, int16_t deadzone) {
    int32_t val = orig_val;
    val -= offset;
    if (val < deadzone && val > -deadzone) {
        return 0;
    }
    if (val < 0) {
        deadzone = -deadzone;
    }
    val -= deadzone;
    val *= multiplier;
    val /= 1024;
    if (val > INT16_MAX) {
        val = INT16_MAX;
    }
    if (val < INT16_MIN) {
        val = INT16_MIN;
    }
    return val;
}
int16_t handle_calibration_xbox_uint(uint16_t orig_val, int16_t offset, uint16_t multiplier, int16_t deadzone) {
    int32_t val = orig_val;
    val -= offset;
    if (val < deadzone) {
        return 0;
    }
    val -= deadzone;
    val *= multiplier;
    val /= 1024;
    val += UINT16_MAX;
    if (val > INT16_MAX) {
        val = INT16_MAX;
    }
    if (val < INT16_MIN) {
        val = INT16_MIN;
    }
    return val;
}
uint16_t handle_calibration_xbox_trigger_int(int16_t orig_val, int16_t offset, uint16_t multiplier, uint16_t deadzone) {
    int32_t val = orig_val;
    val -= offset;
    if (val < deadzone && val > -deadzone) {
        return 0;
    }
    if (val < 0) {
        deadzone = -deadzone;
    }
    val -= deadzone;
    val *= multiplier;
    val /= 1024;
    if (val > UINT16_MAX) {
        val = UINT16_MAX;
    }
    if (val < 0) {
        val = 0;
    }
    return val;
}
uint16_t handle_calibration_xbox_trigger_uint(uint16_t orig_val, int16_t offset, uint16_t multiplier, uint16_t deadzone) {
    int32_t val = orig_val;
    val -= offset;
    if (val < deadzone) {
        return 0;
    }
    val -= deadzone;
    val *= multiplier;
    val /= 1024;
    val -= UINT16_MAX;
    if (val > UINT16_MAX) {
        val = UINT16_MAX;
    }
    if (val < 0) {
        val = 0;
    }
    return val;
}
uint8_t handle_calibration_ps3_int(int16_t orig_val, int16_t offset, uint16_t multiplier, int16_t deadzone) {
    int8_t ret = handle_calibration_xbox_int(orig_val, offset, multiplier, deadzone) >> 8;
    return (uint8_t)(ret - INT8_MAX - 1);
}
uint8_t handle_calibration_ps3_uint(uint16_t orig_val, int16_t offset, uint16_t multiplier, int16_t deadzone) {
    return handle_calibration_xbox_uint(orig_val, offset, multiplier, deadzone) >> 8;
}
uint8_t handle_calibration_ps3_trigger_int(int16_t orig_val, int16_t offset, uint16_t multiplier, uint16_t deadzone) {
    int8_t ret = handle_calibration_ps3_trigger_int(orig_val, offset, multiplier, deadzone) >> 8;
    return (uint8_t)(ret - INT8_MAX - 1);
}
uint8_t handle_calibration_ps3_trigger_uint(uint16_t orig_val, int16_t offset, uint16_t multiplier, uint16_t deadzone) {
    return handle_calibration_xbox_trigger_uint(orig_val, offset, multiplier, deadzone) >> 8;
}
uint8_t tick(USB_Report_Data_t *combined_report) {
#ifdef INPUT_DJ_TURNTABLE
    uint8_t* dj_left = lastSuccessfulTurntablePacketLeft;
    uint8_t* dj_right = lastSuccessfulTurntablePacketRight;
    bool djLeftValid = twi_readFromPointer(DJ_TWI_PORT, DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketLeft), dj_left);
    bool djRightValid = twi_readFromPointer(DJ_TWI_PORT, DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketRight), dj_right);
    lastTurntableWasSuccessfulLeft = djLeftValid;
    lastTurntableWasSuccessfulRight = djRightValid;
#endif
#ifdef INPUT_GH5_NECK
    uint8_t* fivetar_buttons = lastSuccessfulGH5Packet;
    bool gh5Valid = twi_readFromPointer(GH5_TWI_PORT, GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(lastSuccessfulGH5Packet), lastSuccessfulGH5Packet);
    lastGH5WasSuccessful = gh5Valid;
#endif
#ifdef INPUT_WT_NECK
    long pulse = WT_NECK_READ();
    if (pulse == WT_NECK_READ()) {
        lastTap = pulse;
        lastTapShift = pulse >> 1;
        lastGHWTWasSuccessful = true;
    } else {
        lastGH5WasSuccessful = false;
    }
#endif
#ifdef INPUT_PS2
    uint8_t *ps2Data = tickPS2();
    bool ps2Valid = ps2Data != NULL;
    lastPS2WasSuccessful = ps2Valid;
    if (ps2Valid) {
        memcpy(lastSuccessfulPS2Packet, ps2Data, sizeof(lastSuccessfulPS2Packet));
    }
#endif
#ifdef INPUT_WII
    uint8_t *wiiData = tickWii();
    bool wiiValid = wiiData != NULL;
    lastWiiWasSuccessful = wiiValid;
    uint8_t wiiButtonsLow, wiiButtonsHigh, vel, which = 0;
    uint16_t accX, accY, accZ = 0;
    if (wiiValid) {
        memcpy(lastSuccessfulWiiPacket, wiiData, sizeof(lastSuccessfulWiiPacket));
        wiiButtonsLow = ~wiiData[4];
        wiiButtonsHigh = ~wiiData[5];
        if (hiRes) {
            wiiButtonsLow = ~wiiData[6];
            wiiButtonsHigh = ~wiiData[7];
        }
#ifdef INPUT_WII_DRUM
        vel = (7 - (wiiData[3] >> 5)) << 5;
        which = (wiiData[2] & 0b01111100) >> 1;
        switch (which) {
            case 0x1B:
                drumVelocity[DRUM_KICK] = vel;
                break;
            case 0x12:
                drumVelocity[DRUM_GREEN] = vel;
                break;
            case 0x19:
                drumVelocity[DRUM_RED] = vel;
                break;
            case 0x11:
                drumVelocity[DRUM_YELLOW] = vel;
                break;
            case 0x0F:
                drumVelocity[DRUM_BLUE] = vel;
                break;
            case 0x0E:
                drumVelocity[DRUM_ORANGE] = vel;
                break;
        }
#endif
#ifdef INPUT_WII_NUNCHUK
        accX = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
        accY = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
        accZ = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
#endif
    }
#endif
    TICK_SHARED;
    if (consoleType == XBOX360) {
        USB_XInputReport_Data_t *report = &combined_report->xinput;
        report->buttons = 0;
        tickPins();
        TICK_XINPUT;
        return sizeof(USB_XInputReport_Data_t);
    } else {
        USB_PS3Report_Data_t *report = &combined_report->ps3;
        report->buttons = 0;
        report->hat = 0;
        tickPins();
        TICK_PS3;
        report->hat = (report->hat & 0xf) > 0x0a ? 0x08 : hat_bindings[report->hat];
        return sizeof(USB_PS3Report_Data_t);
    }
}