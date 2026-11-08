#pragma once
#include <stdint.h>
#include "enums.pb.h"

// Whether this is an RP2350B (QFN80), with GPIO 30 - 47 and the ADC on GP40 - GP47. The RP2350 build runs
// on both packages, so this is read from the chip rather than known at build time.
bool rp2350b();
// How many GPIOs the chip has: 48 on an RP2350B, 30 otherwise
uint32_t gpio_count();
// The first ADC pin (GP26, or GP40 on an RP2350B) and how many there are (4, or 8 on an RP2350B). The SDK's
// ADC_BASE_PIN / NUM_ADC_CHANNELS are the RP2350B's in the RP2350 build, so use these instead.
uint32_t adc_base_pin();
uint32_t adc_pin_count();
// Whether the pin can be read with the ADC (GP26 to GP29, or GP40 to GP47 on an RP2350B)
bool is_adc_pin(uint32_t pin);
// The ADC input an ADC pin is on, for adc_select_input
uint32_t adc_input_for_pin(uint32_t pin);
// Sets the pin's pulls for a PinMode
void gpio_set_pin_mode(uint32_t pin, PinMode mode);
// The average voltage on an ADC pin over samples readings, in mV. Safe on GP29 of a Pico W, and leaves
// the pin set up as it was, so anything else reading it digitally still can.
uint32_t adc_read_pin_mv(uint32_t pin, uint32_t samples);
// Whether nothing drives an ADC pin, found by reading it with each pull. A pot or hall sensor holds
// the pin about where it was, while a pin with nothing on it follows the pull. Leaves the pin set
// up as it was.
bool adc_pin_floating(uint32_t pin);
