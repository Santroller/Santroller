#pragma once
// Fake hardware/flash.h for the config tests: "flash" is a RAM array with an XIP address of its own, and
// programming only clears bits like real NOR flash, so writing a page without erasing it shows up
#include <stddef.h>
#include <stdint.h>

#define FLASH_PAGE_SIZE (1u << 8)
#define FLASH_SECTOR_SIZE (1u << 12)
// Just big enough for the config area plus a sector in front of it that must never be touched
#define PICO_FLASH_SIZE_BYTES (0x8000u + FLASH_SECTOR_SIZE)

namespace fake_flash
{
alignas(FLASH_SECTOR_SIZE) inline uint8_t memory[PICO_FLASH_SIZE_BYTES];
inline uint32_t erase_count = 0;
inline uint32_t program_count = 0;
inline bool program_without_erase = false;
}

#define XIP_BASE (reinterpret_cast<uintptr_t>(fake_flash::memory))

void flash_range_erase(uint32_t flash_offs, size_t count);
void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);
