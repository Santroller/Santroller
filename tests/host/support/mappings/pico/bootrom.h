#pragma once
// Fake pico/bootrom.h: records a reboot to the bootloader rather than doing it
#include <stdint.h>

namespace fake_bootrom
{
inline int reset_count = 0;
}

inline void reset_usb_boot(uint32_t gpio_activity_pin_mask, uint32_t disable_interface_mask)
{
    (void)gpio_activity_pin_mask;
    (void)disable_interface_mask;
    fake_bootrom::reset_count++;
}
