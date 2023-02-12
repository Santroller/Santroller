#include <Arduino.h>
#include <SPI.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>
#include <pico/bootrom.h>

#include "commands.h"
#include "shared_main.h"
#include "btstack.h"


void setup() {
    init_main();
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