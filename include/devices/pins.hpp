#pragma once
#include <stdint.h>
#include "enums.pb.h"

// Whether the pin can be read with the ADC (GP26 to GP29, or GP40 to GP47 on an RP2350B)
bool is_adc_pin(int32_t pin);
// Sets the pin's pulls for a PinMode
void gpio_set_pin_mode(uint32_t pin, PinMode mode);
// The average voltage on an ADC pin over samples readings, in mV. Safe on GP29 of a Pico W, and leaves
// the pin set up as it was, so anything else reading it digitally still can.
uint32_t adc_read_pin_mv(uint32_t pin, uint32_t samples);
// Whether nothing drives an ADC pin, found by reading it with each pull. A pot or hall sensor holds
// the pin about where it was, while a pin with nothing on it follows the pull. Leaves the pin set
// up as it was.
bool adc_pin_floating(uint32_t pin);
