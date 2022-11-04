#include <stdint.h>

#include "Arduino.h"
#include "config.h"
#include "pins_define.h"
#include "util.h"
uint16_t adcReading[NUM_ANALOG_INPUTS];
bool first = true;
uint16_t adc(uint8_t pin) {
    adc_select_input(pin);
    return adc_read();
}

void initPins(void) {
    adc_init();
    PIN_INIT;
}
void tickPins(void) {
}

uint8_t digital_read(uint8_t port, uint8_t mask) {
    port = port * 8;
    uint32_t mask32 = mask << port;
    for(uint i=0;i<NUM_BANK0_GPIOS;i++) {
        if (mask32 & (1 << i)) {
            gpio_init(i);
            gpio_set_pulls(i, true,false);
        }
    }
    uint32_t ret = sio_hw->gpio_in;
    PIN_INIT;
    return ((ret >> port) & 0xff);
}

uint16_t adc_read(uint8_t pin, uint8_t mask) {
    gpio_init(pin);
    gpio_set_pulls(pin, true, false);
    gpio_set_input_enabled(pin, false);
    adc_select_input(pin - PIN_A0);
    uint16_t data = adc_read() << 4;
    PIN_INIT;
    return data;
}