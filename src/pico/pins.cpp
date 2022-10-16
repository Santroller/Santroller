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

uint32_t digital_read(uint8_t port, uint8_t mask) {
    uint32_t mask32 = mask << port;
    for(uint i=0;i<NUM_BANK0_GPIOS;i++) {
        if (mask32 & 1) {
            gpio_init(i);
            gpio_set_pulls(i, true,false);
        }
        mask32 >>= 1;
    }
    uint32_t ret = sio_hw->gpio_in;
    PIN_INIT;
    return ret;
}

uint16_t adc_read(uint8_t pin, uint8_t mask) {
    gpio_function f = gpio_get_function(pin);
    bool down = gpio_is_pulled_down(pin);
    bool up = gpio_is_pulled_up(pin);
    gpio_init(pin);
    gpio_set_pulls(pin, true, false);
    gpio_set_input_enabled(pin, false);
    adc_select_input(pin);
    uint16_t data = adc_read();
    gpio_set_function(pin, f);
    gpio_set_pulls(pin, up, down);
    return data;
}