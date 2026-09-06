#include "config/FlashPROM.h"
#include <stdio.h>
#include "tusb.h"
#include "utils.h"

uint8_t FlashPROM::writeCache[EEPROM_SIZE_BYTES];

int64_t writeToFlash(alarm_id_t id, void *flashCache)
{
	const uint8_t *flash_base = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START);
	const uint8_t *cache_base = reinterpret_cast<const uint8_t *>(flashCache);

	// Quick check: if the entire flash already matches the cache, nothing to do.
	if (memcmp(flash_base, cache_base, EEPROM_SIZE_BYTES) == 0)
	{
		return 0;
	}

	multicore_lockout_start_blocking();

	for (uint32_t sector_offset = 0; sector_offset < EEPROM_SIZE_BYTES; sector_offset += FLASH_SECTOR_SIZE)
	{
		const uint8_t *flash_sec = flash_base + sector_offset;
		const uint8_t *cache_sec = cache_base + sector_offset;

		// Skip sectors that are already identical to the cache
		if (memcmp(flash_sec, cache_sec, FLASH_SECTOR_SIZE) == 0)
		{
			continue;
		}

		tud_task();
		tuh_task();

		// Erase this modified sector
		uint32_t flash_offset = (intptr_t)EEPROM_ADDRESS_START - (intptr_t)XIP_BASE + sector_offset;
		auto status = save_and_disable_interrupts();
		flash_range_erase(flash_offset, FLASH_SECTOR_SIZE);
		restore_interrupts(status);

		// Program only the pages within this sector
		for (uint32_t page_offset = 0; page_offset < FLASH_SECTOR_SIZE; page_offset += FLASH_PAGE_SIZE)
		{
			tud_task();
			tuh_task();

			const uint8_t *page_data = cache_sec + page_offset;

			// After erasing, all bytes in flash are 0xFF.
			// If this page in the cache is also all 0xFF, skip programming it.
			bool all_ff = true;
			const uint32_t *words = reinterpret_cast<const uint32_t *>(page_data);
			for (size_t w = 0; w < FLASH_PAGE_SIZE / sizeof(uint32_t); ++w)
			{
				if (words[w] != 0xFFFFFFFF)
				{
					all_ff = false;
					break;
				}
			}
			if (all_ff)
			{
				continue;
			}

			status = save_and_disable_interrupts();
			flash_range_program(flash_offset + page_offset, page_data, FLASH_PAGE_SIZE);
			restore_interrupts(status);
		}
	}

	multicore_lockout_end_blocking();

	tud_task();
	tuh_task();

	return 0;
}

void FlashPROM::start()
{

	memcpy(writeCache, reinterpret_cast<uint8_t *>(EEPROM_ADDRESS_START), EEPROM_SIZE_BYTES);
}

void FlashPROM::commit()
{
	should_commit_at = to_ms_since_boot(get_absolute_time()) + EEPROM_WRITE_WAIT;
}

void FlashPROM::commit_now()
{
	writeToFlash(0, writeCache);
}
void FlashPROM::tick()
{
	if (should_commit_at != 0 && to_ms_since_boot(get_absolute_time()) >= should_commit_at)
	{
		commit_now();
		should_commit_at = 0;
	}
}
void FlashPROM::reset()
{
	memset(writeCache, 0, EEPROM_SIZE_BYTES);
	commit();
}