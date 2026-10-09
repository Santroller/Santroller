#pragma once
// Fake pio_usb.h: core1 sends SOFs on the USB host port while core0 writes flash
#include <stdint.h>

namespace fake_pio_usb
{
inline volatile bool sof_only = false;
inline volatile uint32_t sof_only_changes = 0;
}

inline uint32_t pio_usb_host_last_frame_us() { return 0; }
inline void pio_usb_host_sof_only_frame() {}
inline void pio_usb_host_set_sof_only(bool sof_only)
{
    fake_pio_usb::sof_only = sof_only;
    fake_pio_usb::sof_only_changes = fake_pio_usb::sof_only_changes + 1;
}
