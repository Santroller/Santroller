#include <Arduino.h>
#include <SPI.h>
#include <avr/io.h>
#include <util/delay.h>

#include "Usb.h"
#include "config.h"
#include "defines.h"
#include "descriptors.h"
#include "hid.h"
#include "packets.h"
#include "rf.h"
#include "shared_main.h"
#include <avr/wdt.h>

void setup() {
    Serial.begin(57600);
    sei();
    init_main();
    Serial.println("Santroller");
}
uint8_t buffer[200];
void loop() {
    if (Serial.available()) {
        if (Serial.read() == 0x1f) {
            while (Serial.available() < 3) {
            }
            bool success = true;
            uint8_t command = Serial.read();
            uint8_t wValue = Serial.read() | (Serial.read() << 8);
            uint8_t size = handle_serial_command(command, wValue, buffer, &success);
            if (success) {
                Serial.write(size);
                Serial.write(buffer, size);
            }
        }
    }
    tick();
}
void reset_usb() {
    cli();
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
}