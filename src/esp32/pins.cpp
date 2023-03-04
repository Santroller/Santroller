#include <stdint.h>

#include "Arduino.h"
#include "config.h"
#include "io_define.h"
#include "util.h"
uint16_t adcReading[NUM_ANALOG_INPUTS];
bool first = true;
uint16_t adc(uint8_t pin) {
    return analogRead(pin) << 4;
}

void initPins(void) {
    PIN_INIT;
}

uint8_t digital_read(uint8_t port, uint8_t mask) {
    port = port * 8;
    uint8_t ret = 0;
    for (uint i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            pinMode(port+i, PULLUP);
            if (digitalRead(port+i)) {
                ret |= 1 << i; 
            }
        }
    }
    PIN_INIT;
    return ret;
}

uint16_t adc_read(uint8_t pin, uint8_t mask) {
    bool detecting = pin & (1 << 7);
    if (detecting) {
        pin = pin & ~(1 << 7);
        adcAttachPin(pin);
    }
    uint16_t data = analogRead(pin) << 4;
    if (detecting) {
        PIN_INIT;
    }
    return data;
}