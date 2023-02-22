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

void setup() {
    init_main();
    Serial.begin(57600);
    sei();
    Serial.println("Santroller");
    Serial.print(VERSION_MAJOR);
    Serial.print(".");
    Serial.print(VERSION_MINOR);
    Serial.print(".");
    Serial.println(VERSION_REVISION);
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
bool usb_connected() {
    return false;
}