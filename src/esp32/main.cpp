#include <SPI.h>

#include "Arduino.h"
#include "BleGamepad.h"
#include "reports/controller_reports.h"
#include "shared_main.h"

BleGamepad bleGamepad;

void setup(void) {
    Serial.begin(9600);
    init_main();
    bleGamepad.begin();
}

void loop(void) {
    tick();
}

void send_report_to_pc(const void *report, uint8_t len) {
    if (bleGamepad.isConnected()) {
        bleGamepad.sendReport((void*) report, len);
    }
}

bool ready_for_next_packet() {
    return bleGamepad.isConnected();
}