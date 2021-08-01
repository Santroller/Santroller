#include "lib_main.h"

#include "defines.h"
#include "usb/controller_reports.h"
#define PIN_COUNT PORTS* PINS_PER_PORT
ConsoleType_t consoleType;
DeviceType_t deviceType;
bool guitar;
bool drum;
uint8_t pinCount;
Pin_t pins[PIN_COUNT] = {{0xFF}};
AnalogPin_t analogInfo[NUM_ANALOG_INPUTS] = {{0xFF}};
uint32_t lastMillisStrum;
bool mergedStrum = false;

void init() {
    consoleType = XBOX360;
    deviceType = GUITAR_HERO_GUITAR;
    guitar = deviceType == GUITAR_HERO_GUITAR || deviceType == ROCK_BAND_GUITAR || deviceType == GUITAR_HERO_LIVE_GUITAR;
    drum = deviceType == GUITAR_HERO_DRUMS || deviceType == ROCK_BAND_DRUMS;
    initPins();
}
uint8_t tick(uint8_t* data) {
    if (consoleType == XBOX360) {
        USB_XInputReport_Arr_Data_t* report = (USB_XInputReport_Arr_Data_t*)data;
        report->rid = 0;
        report->rsize = sizeof(USB_XInputReport_Arr_Data_t);
        for (int i = 0; i < PIN_COUNT; i++) {
            Pin_t* pin = pins + i;
            if (pin->binding != 0xFF) {
                if (pin->axisInfo && !pin->axisInfo->isADC) {
                    int16_t val = pin->axisInfo->currentValue;
                    if (pin->binding <= XBOX_RT) {
                        uint8_t val2 = (val >> 8) + 127;
                        report->joys[pin->binding] = val2;
                    } else {
                        report->joys[pin->binding - XBOX_RT] = val;
                    }
                } else {
                    bool val = readDigital(pin);
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