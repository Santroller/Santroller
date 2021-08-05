#include "lib_main.h"

#include <stddef.h>
#include <string.h>

#include "defines.h"
#include "usb/controller_reports.h"
#define PIN_COUNT PORTS* PINS_PER_PORT
ConsoleType_t consoleType;
DeviceType_t deviceType;
InputType_t inputType;
bool guitar;
bool drum;
uint8_t pinCount;
Input_t pins[PIN_COUNT];
AnalogInput_t analogInfo[NUM_ANALOG_INPUTS];
uint32_t lastMillisStrum;
bool mergedStrum = false;
uint8_t inputCount;
void (*tickInput)(void) = NULL;

void init() {
    inputType = DIRECT;
    consoleType = KEYBOARD_MOUSE;
    deviceType = GUITAR_HERO_GUITAR;
    guitar = deviceType == GUITAR_HERO_GUITAR || deviceType == ROCK_BAND_GUITAR || deviceType == GUITAR_HERO_LIVE_GUITAR;
    drum = deviceType == GUITAR_HERO_DRUMS || deviceType == ROCK_BAND_DRUMS;
    inputCount = PIN_COUNT;
    if (inputType == DIRECT) {
        // initPins();
    } else if (inputType == WII) {
        // Init wii, load in the wii inputs as the "pins" instead
        // One interesting thing is we should be able to just load in tilt as a standard pin here too.
    } else if (inputType == PS2) {
        // Init ps2, load in the ps2 inputs as the "pins" instead
    }
}
uint32_t last = 0;
bool val = true;
uint8_t tick(uint8_t* data) {
    if (tickInput) {
        tickInput();
    }
    if (consoleType == KEYBOARD_MOUSE) {
        // if mouse has changed use mouse report, else use keyboard report.
        TUSB_KeyboardReport_Data_t* report = (TUSB_KeyboardReport_Data_t*)data;
        report->rid = REPORT_ID_KBD;
        bit_write(val, report->KeyCodeFlags[THID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN / 8], THID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN % 8);
        // bit_write(val, report->Modifier, (THID_KEYBOARD_MODIFIER_LEFTSHIFT));
        if (millis() - last > 1000) {
            val = !val;
            last = millis();
        }
        return sizeof(TUSB_KeyboardReport_Data_t);
    }
    if (consoleType == XBOX360) {
        USB_XInputReport_Arr_Data_t* report = (USB_XInputReport_Arr_Data_t*)data;
        report->rid = 0;
        report->rsize = sizeof(USB_XInputReport_Arr_Data_t);
        for (int i = 0; i < inputCount; i++) {
            Input_t* pin = pins + i;
            if (pin->binding) {
                if (pin->axisInfo && !pin->axisInfo->isADC) {
                    int16_t val = pin->axisInfo->analogRead(pin);
                    if (pin->binding <= XBOX_RT) {
                        uint8_t val2 = (val >> 8) + 127;
                        report->joys[pin->binding] = val2;
                    } else {
                        report->joys[pin->binding - XBOX_RT] = val;
                    }
                } else {
                    bool val = pin->digitalRead(pin);
                    uint32_t current = millis();
                    uint32_t last = pin->lastMillis;
                    bool usingMerged = mergedStrum && (pin->binding == XBOX_DPAD_DOWN || pin->binding == XBOX_DPAD_UP);
                    if (usingMerged) {
                        last = lastMillisStrum;
                    }
                    if (current - last > pin->milliDeBounce) {
                        bit_write(val, report->buttons, pin->binding);
                        pin->lastMillis = current;
                        if (usingMerged) {
                            lastMillisStrum = current;
                        }
                    }
                }
            }
        }
        return sizeof(USB_XInputReport_Data_t);
    }
    return 0;
}
void packetReceived(uint8_t* data, uint8_t len) {
    if (consoleType == XBOX360) {
        if (data[0] == 0x01) {
            XInputLEDReport_t* leds = (XInputLEDReport_t*)data;
            (void)leds;
            // We can receive led reports here and do things with them
        } else if (data[0] == 0x00) {
            XInputRumbleReport_t* rumble = (XInputRumbleReport_t*)data;
            (void)rumble;
            // We can receive rumble reports here and do things with them
        }
    } else if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH) {
        ps3_output_report* report = (ps3_output_report*)data;
        (void)report;
    }
}