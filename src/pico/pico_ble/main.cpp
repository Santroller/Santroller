#include <Arduino.h>
#include <SPI.h>
#include <pico/bootrom.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>

#include "Adafruit_TinyUSB.h"
#include "commands.h"
#include "hid.h"
#include "shared_main.h"

Adafruit_USBD_HID usb_hid(pc_descriptor, sizeof(pc_descriptor), HID_ITF_PROTOCOL_NONE, 1, false);
hid_gamepad_report_t gp;
uint16_t get_report(uint8_t report_id,
                    hid_report_type_t report_type,
                    uint8_t *buffer, uint16_t reqlen) {
    return hid_get_report(buffer, reqlen, report_type, report_id);
}
void set_report(uint8_t report_id,
                hid_report_type_t report_type,
                uint8_t const *buffer,
                uint16_t bufsize) {
    hid_set_report(buffer, bufsize, report_type, report_id);
}
void setup() {
    init_main();
    Serial.begin(115200);
    usb_hid.enableOutEndpoint(true);
    usb_hid.setReportCallback(get_report, set_report);
    usb_hid.begin();
    // Here, we can actually use mounted to switch between USB and bluetooth mode!
    while (!TinyUSBDevice.mounted()) delay(1);
}
USB_Report_Data_t report;
unsigned int last = 0;

void loop() {
    tick();
}
void reboot(void) {
    watchdog_enable(1, false);
    for (;;) {
    }
}
void bootloader(void) {
    reset_usb_boot(0, 0);
}
void reset_usb(void) {
}
void send_report_to_controller(uint8_t *report, uint8_t len) {
}