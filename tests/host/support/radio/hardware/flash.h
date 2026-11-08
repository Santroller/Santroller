#pragma once
// Fake hardware/flash.h for the radio firmware tests: "flash" is a RAM array, and programming only clears
// bits like real NOR flash. The area (radio_firmware_layout.h) has a sector on either side that must never
// be touched.
#include <stddef.h>
#include <stdint.h>

#define FLASH_PAGE_SIZE (1u << 8)
#define FLASH_SECTOR_SIZE (1u << 12)
#define PICO_FLASH_SIZE_BYTES (0x12000u)

namespace fake_flash
{
alignas(FLASH_SECTOR_SIZE) inline uint8_t memory[PICO_FLASH_SIZE_BYTES];
inline bool program_without_erase = false;
}

#define XIP_BASE (reinterpret_cast<uintptr_t>(fake_flash::memory))

void flash_range_erase(uint32_t flash_offs, size_t count);
void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);
