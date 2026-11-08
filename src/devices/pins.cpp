#include "devices/pins.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "pico/time.h"
#if PICO_RP2350
#include "hardware/structs/sysinfo.h"
#endif

// The ADC's reference, as on a Pico
static constexpr uint32_t ADC_VREF_MV = 3300;

bool rp2350b()
{
#if PICO_RP2350
    // 0 = QFN80 (RP2350B), 1 = QFN60 (RP2350A)
    return !(sysinfo_hw->package_sel & SYSINFO_PACKAGE_SEL_BITS);
#else
    return false;
#endif
}

uint32_t gpio_count()
{
    return rp2350b() ? 48 : 30;
}

uint32_t adc_base_pin()
{
    return rp2350b() ? 40 : 26;
}

uint32_t adc_pin_count()
{
    return rp2350b() ? 8 : 4;
}

bool is_adc_pin(uint32_t pin)
{
    return pin >= adc_base_pin() && pin < adc_base_pin() + adc_pin_count();
}

uint32_t adc_input_for_pin(uint32_t pin)
{
    return pin - adc_base_pin();
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
    adc_select_input(adc_input_for_pin(pin));
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

// A 10k pot at mid travel is ~2.5k against the ~50k pulls, so it moves under 10% between them.
// A floating pin goes from one rail to the other.
static constexpr uint32_t FLOATING_SPREAD = 4095 / 2;
static constexpr uint32_t PULL_SETTLE_US = 200;
static constexpr uint32_t FLOATING_SAMPLES = 4;

// Average ADC reading with only the pull up, or only the pull down, on
static uint32_t adc_read_with_pull(uint32_t pin, bool up)
{
    gpio_set_pulls(pin, up, !up);
    busy_wait_us(PULL_SETTLE_US);
    // the first reading after the pulls change can lag
    adc_read();
    uint32_t total = 0;
    for (uint32_t i = 0; i < FLOATING_SAMPLES; i++)
    {
        total += adc_read();
    }
    return total / FLOATING_SAMPLES;
}

bool adc_pin_floating(uint32_t pin)
{
    if (!is_adc_pin(pin))
    {
        return false;
    }
    BtStackLock lock;
    const uint32_t pad = pads_bank0_hw->io[pin];
    const uint32_t ctrl = io_bank0_hw->io[pin].ctrl;
    // adc_gpio_init turns the digital input off, which also keeps the RP2350 E9 pull-down latch
    // from holding the pin up
    adc_gpio_init(pin);
    adc_select_input(adc_input_for_pin(pin));
    const uint32_t up = adc_read_with_pull(pin, true);
    const uint32_t down = adc_read_with_pull(pin, false);
    io_bank0_hw->io[pin].ctrl = ctrl;
    pads_bank0_hw->io[pin] = pad;
    return up > down && up - down > FLOATING_SPREAD;
}
