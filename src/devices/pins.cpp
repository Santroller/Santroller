#include "devices/pins.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"

// The last ADC channel is the temperature sensor rather than a pin
static constexpr int32_t ADC_PINS = NUM_ADC_CHANNELS - 1;
// The ADC's reference, as on a Pico
static constexpr uint32_t ADC_VREF_MV = 3300;

bool is_adc_pin(int32_t pin)
{
    return pin >= ADC_BASE_PIN && pin < ADC_BASE_PIN + ADC_PINS;
}

void gpio_set_pin_mode(uint32_t pin, PinMode mode)
{
    switch (mode)
    {
    case PinMode_BusKeep:
        gpio_set_pulls(pin, true, true);
        break;
    case PinMode_Floating:
        gpio_set_pulls(pin, false, false);
        break;
    case PinMode_PullDown:
        gpio_set_pulls(pin, false, true);
        break;
    case PinMode_PullUp:
        gpio_set_pulls(pin, true, false);
        break;
    }
}

uint32_t adc_read_pin_mv(uint32_t pin, uint32_t samples)
{
    uint32_t total = 0;
    // On a Pico W, GP29 is also the wireless chip's SPI clock. Holding the stack lock keeps
    // the chip off the bus while it is used as an ADC, and the driver takes the pin back at
    // the start of its next transfer. Without bluetooth this lock does nothing.
    BtStackLock lock;
    // Put the pad back afterwards. Left in ADC mode its digital input stays off, so anything else
    // reading the pin (eg a button input) sees it low, and is set up again, high, on every reload
    const uint32_t pad = pads_bank0_hw->io[pin];
    const uint32_t ctrl = io_bank0_hw->io[pin].ctrl;
    adc_gpio_init(pin);
    adc_select_input(pin - ADC_BASE_PIN);
    // the first readings after switching inputs can be low
    adc_read();
    for (uint32_t i = 0; i < samples; i++)
    {
        total += adc_read();
    }
    io_bank0_hw->io[pin].ctrl = ctrl;
    pads_bank0_hw->io[pin] = pad;
    // 12 bit readings of 0 to 3.3V
    return total * ADC_VREF_MV / (samples * 4095);
}
