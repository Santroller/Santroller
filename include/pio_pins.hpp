#pragma once
#include "hardware/pio.h"
#include <initializer_list>

// Claims a free state machine and loads program on a PIO that can reach every one of pins. On an RP2350B each
// PIO reaches either GP0 - GP31 or GP16 - GP47 (its GPIO base), so this picks a PIO whose base covers them, or
// rebases an unused one. pio_claim_free_sm_and_add_program ignores the base, and would hand out a PIO that
// can't reach the pins. Pins too far apart for any one PIO (eg GP2 and GP40) fail.
static inline bool pio_claim_for_pins(const pio_program_t *program, PIO *pio, uint *sm, uint *offset,
                                      std::initializer_list<uint> pins)
{
    uint lo = ~0u;
    uint hi = 0;
    for (uint pin : pins)
    {
        lo = pin < lo ? pin : lo;
        hi = pin > hi ? pin : hi;
    }
    if (lo > hi || hi >= NUM_BANK0_GPIOS)
    {
        return false;
    }
    return pio_claim_free_sm_and_add_program_for_gpio_range(program, pio, sm, offset, lo, hi - lo + 1, true);
}

// Whether a PIO with its current GPIO base can reach every one of pins, for code that picks a PIO itself
static inline bool pio_reaches_pins(PIO pio, std::initializer_list<uint> pins)
{
    uint base = pio_get_gpio_base(pio);
    for (uint pin : pins)
    {
        if (pin < base || pin >= base + 32)
        {
            return false;
        }
    }
    return true;
}

// Like pio_reaches_pins, but a PIO with nothing loaded yet is rebased to reach them if it has to. The SDK refuses
// to rebase a PIO with any program in it, as whatever runs there would lose its pins.
static inline bool pio_try_reach_pins(PIO pio, std::initializer_list<uint> pins)
{
    if (pio_reaches_pins(pio, pins))
    {
        return true;
    }
#if PICO_PIO_USE_GPIO_BASE
    uint lo = ~0u;
    uint hi = 0;
    for (uint pin : pins)
    {
        lo = pin < lo ? pin : lo;
        hi = pin > hi ? pin : hi;
    }
    uint base = hi >= 32 ? 16 : 0;
    return lo >= base && pio_set_gpio_base(pio, base) == PICO_OK;
#else
    return false;
#endif
}
