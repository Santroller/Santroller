#include "Arduino.h"
#include "config.h"
#include "controller_reports.h"
#include "pins.h"
#include "util.h"
uint8_t debounce[DIGITAL_COUNT];
static const uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
void init_main(void) {
    initPins();
}
uint8_t tick(USB_Report_Data_t* combined_report) {
    if (consoleType == XBOX360) {
        USB_XInputReport_Data_t* report = &combined_report->xinput;
        report->buttons = 0;
        tickPins();
        TICK_XINPUT;
        return sizeof(USB_XInputReport_Data_t);
    } else {
        USB_PS3Report_Data_t* report = &combined_report->ps3;
        report->buttons = 0;
        report->hat = 0;
        tickPins();
        TICK_PS3;
        report->hat = (report->hat & 0xf) > 0x0a ? 0x08 : hat_bindings[report->hat];
        return sizeof(USB_PS3Report_Data_t);
    }
}