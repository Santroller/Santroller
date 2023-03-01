#include <SPI.h>

#include "Arduino.h"
#include "BleGamepad.h"
#include "reports/controller_reports.h"
#include "shared_main.h"

BleGamepad bleGamepad;

void setup(void) {
    Serial.begin(115200);
    init_main();
    bleGamepad.begin();
}

void loop(void) {
    tick();
}

void send_report(uint8_t size, uint8_t* report) {
    if (bleGamepad.isConnected()) {
        bleGamepad.sendReport((void*) report, size);
    }
}

bool ready_for_next_packet() {
    return bleGamepad.isConnected();
}