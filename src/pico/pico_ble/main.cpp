#include <Arduino.h>
#include <SPI.h>
#include <pico/bootrom.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>

#include "Adafruit_TinyUSB.h"
#include "commands.h"
#include "shared_main.h"

uint8_t const desc_hid_report[] =
    { TUD_HID_REPORT_DESC_GAMEPAD()};
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false);
hid_gamepad_report_t gp;

void setup() {
    init_main();
    Serial.begin(115200);
    usb_hid.setPollInterval(1);
    usb_hid.begin();
    while (!TinyUSBDevice.mounted()) delay(1);

    Serial.println("Adafruit TinyUSB HID Gamepad example");
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