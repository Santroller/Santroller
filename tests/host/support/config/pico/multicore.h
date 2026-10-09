#pragma once
// Fake pico/multicore.h and the pico/platform bits FlashPROM uses
#include <stdint.h>

#define __not_in_flash_func(x) x
inline void tight_loop_contents() {}
inline uint32_t save_and_disable_interrupts() { return 0; }
inline void restore_interrupts(uint32_t) {}
