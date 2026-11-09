#pragma once

#include <stdint.h>

// The flash chip actually fitted, which can be smaller than the layout the firmware is built for:
// boards sold as RP2350 boards come with 2MB chips (eg W25Q16JV) as well as the Pico 2's 4MB one.
// Flash addresses past the end of the chip wrap around to its start, so writing a layout's download
// slot onto a smaller chip lands on the static firmware and the running app.

// Read the chip's JEDEC ID. Call once at boot, before core1 is launched, as it takes flash out of XIP
void detect_flash_chip_size();
// The chip's size in bytes, or 0 if its JEDEC ID didn't give one
uint32_t flash_chip_size();
// Whether the chip is known to be smaller than the flash the firmware is laid out for
bool flash_chip_smaller_than_layout();
