#include "lib_main.h"

#include "defines.h"
#include "switch/response.h"
#include "usb/controller_reports.h"
ConsoleType_t consoleType;
DeviceType_t deviceType;
bool guitar;
bool drum;

void init() {
    consoleType = XBOX360;
    deviceType = GUITAR_HERO_GUITAR;
    guitar = deviceType == GUITAR_HERO_GUITAR || deviceType == ROCK_BAND_GUITAR || deviceType == GUITAR_HERO_LIVE_GUITAR;
    drum = deviceType == GUITAR_HERO_DRUMS || deviceType == ROCK_BAND_DRUMS;
}
uint8_t tick(uint8_t* data) {
    if (consoleType == XBOX360) {
        USB_XInputReport_Data_t* report = (USB_XInputReport_Data_t*)data;
        report->rid = 0;
        report->rsize = sizeof(USB_XInputReport_Data_t);
        report->l_x += 100;
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
    } else if (consoleType == PS3 || consoleType == WII_RB) {
        ps3_output_report* report = (ps3_output_report*)data;
        (void)report;
    } else if (consoleType == SWITCH) {
        process_OUT_report(data, len);
    }
}