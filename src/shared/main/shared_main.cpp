#include "Arduino.h"
#include "config.h"
#include "controller_reports.h"
#include "pins.h"
uint8_t debounce[DIGITAL_COUNT];
static const uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
void init_main(void) {
    initPins();
}
void tick(REPORT_TYPE* report) {
    tickPins();
    report->buttons = 0;
#if CONSOLE_TYPE != PC_XINPUT
    report->hat = 0;
#endif
    TICK;
#if CONSOLE_TYPE != PC_XINPUT
    report->hat = (report->hat & 0xf) > 0x0a ? 0x08 : hat_bindings[report->hat];
#endif
}