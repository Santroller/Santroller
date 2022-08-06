#include <stdint.h>

#include "config.h"
#include "pins_define.h"
#include "Arduino.h"
#include "util.h"
uint16_t adcReading[NUM_ANALOG_INPUTS];
bool first = true;
uint8_t debounce[DIGITAL_COUNT];

int16_t adc(uint8_t pin, int16_t offset, int16_t multiplier, int16_t deadzone) {
    adc_select_input(pin);
    int32_t val = (adc_read() - 2048) * 16;
    val -= offset;
    val *= multiplier;
    val /= 1024;
    val += INT16_MIN;
    if (val > INT16_MAX) val = INT16_MAX;
    if (val < INT16_MIN) val = INT16_MIN;
    if (val < deadzone && val > -deadzone) val = 0;
    return (int16_t)val;
}

uint16_t adc_trigger(uint8_t pin, int16_t offset, int16_t multiplier, int16_t deadzone) {
    adc_select_input(pin);
    uint32_t val = adc_read() * 16;
    val -= offset;
    val *= multiplier;
    val /= 1024;
    val += INT16_MAX;
    if (val > INT16_MAX) val = UINT16_MAX;
    if (val < INT16_MIN) val = 0;
    if (val < deadzone) val = 0;
    return (uint16_t)val;
}

int lastAnalogValue[NUM_ANALOG_INPUTS];
uint32_t lastDigitalValue;

void initDetectionDigital(void) {
    for (int i = 0; i < NUM_TOTAL_PINS; i++) {
        if (_BV(i) & SKIP_MASK_PICO) continue;
        gpio_init(i);
        gpio_set_dir(i, false);
        gpio_set_pulls(i, true, false);
    }
    lastDigitalValue = gpio_get_all() & SKIP_MASK_PICO;
}

void initDetectionAnalog(void) {
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
        gpio_init(i + PIN_A0);
        gpio_set_dir(i + PIN_A0, false);
        gpio_set_pulls(i + PIN_A0, true, false);
        gpio_set_input_enabled(i + PIN_A0, false);
        adc_select_input(i);
        lastAnalogValue[i] = adc_read();
    }
}

int detectDigital() {
    uint32_t changed = (lastDigitalValue & gpio_get_all()) & (~SKIP_MASK_PICO);
    for (int i = 0; i < NUM_TOTAL_PINS; i++) {
        if (changed & _BV(i)) {
            return i;
        }
    }
    return -1;
}

int detectAnalog() {
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
        adc_select_input(i);
        if (abs(adc_read() - lastAnalogValue[i]) > 30) {
            return i;
        }
    }
    return -1;
}



void initPins(void) {
    PIN_INIT;
}