#include "Arduino.h"
#include "config.h"
#include "controller_reports.h"
#include "io.h"
#include "pins.h"
#include "pins_define.h"
#include "util.h"
#include "ps2.h"
#include "wii.h"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12

uint8_t debounce[DIGITAL_COUNT];
uint8_t drumVelocity[8];
uint8_t lastTap;
uint16_t wiiControllerType = WII_NO_EXTENSION;
uint8_t ps2ControllerType = PSX_NO_DEVICE;
static const uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
void init_main(void) {
    initPins();
}

uint8_t tick(USB_Report_Data_t *combined_report) {
#ifdef INPUT_DJ_TURNTABLE
    uint8_t dj_left[3] = {0, 0, 0};
    uint8_t dj_right[3] = {0, 0, 0};
    twi_readFromPointer(DJ_TWI_PORT, DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(dj_left), dj_left);
    twi_readFromPointer(DJ_TWI_PORT, DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(dj_right), dj_right);
#endif
#ifdef INPUT_GH5_NECK
    uint8_t fivetar_buttons[2] = {0, 0};
    twi_readFromPointer(GH5_TWI_PORT, GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(fivetar_buttons), fivetar_buttons);
#endif
#ifdef INPUT_WT_NECK
    long pulse = WT_NECK_READ();
    if (pulse == WT_NECK_READ()) {
        lastTap = pulse;
    }
#endif
#ifdef INPUT_PS2
    uint8_t* ps2Data = tickPS2();
    bool ps2Valid = ps2Data == NULL;
#endif
#ifdef INPUT_WII
    uint8_t* wiiData = tickWii();
    bool wiiValid = wiiData == NULL;
#ifdef INPUT_WII_DRUM
    uint8_t vel = (7 - (wiiData[3] >> 5)) << 5;
    uint8_t which = (wiiData[2] & 0b01111100) >> 1;
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
        default:
            break;
    }
#endif
#ifdef INPUT_WII_NUNCHUK
    uint16_t accX = ((wiiData[2] << 2) | ((wiiData[5] & 0xC0) >> 6)) - 511;
    uint16_t accY = ((wiiData[3] << 2) | ((wiiData[5] & 0x30) >> 4)) - 511;
    uint16_t accZ = ((wiiData[4] << 2) | ((wiiData[5] & 0xC) >> 2)) - 511;
#endif
#endif
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