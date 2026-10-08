#include "config/FlashPROM.h"
#include <stdio.h>
#include "tusb.h"
#include "utils.h"
#include "pio_usb.h"

// A flash erase stalls core0 with interrupts off for tens of ms. USB devices suspend after 3ms
// without SOF, so core1 sends SOF in the meantime instead of being locked out.
static volatile bool core1_ready = false;
static volatile bool sof_requested = false;
static volatile bool sof_running = false;

void __not_in_flash_func(flash_core1_loop)()
{
	core1_ready = true;
	while (true)
	{
		if (!sof_requested)
		{
			continue;
		}
		sof_running = true;
		uint32_t next = pio_usb_host_last_frame_us() + 1000;
		while (sof_requested)
		{
			uint32_t now = timer_hw->timerawl;
			if (static_cast<int32_t>(now - next) < 0)
			{
				continue;
			}
			pio_usb_host_sof_only_frame();
			next += 1000;
			// Don't burst SOFs to catch up if we started late
			if (static_cast<int32_t>(now - next) >= 0)
			{
				next = now + 1000;
			}
		}
		sof_running = false;
	}
}

static void flash_write_begin()
{
	while (!core1_ready)
	{
		tight_loop_contents();
	}
	// Stop core0's frames first so only one core drives the bus
	pio_usb_host_set_sof_only(true);
	sof_requested = true;
	while (!sof_running)
	{
		tight_loop_contents();
	}
}

static void flash_write_end()
{
	sof_requested = false;
	while (sof_running)
	{
		tight_loop_contents();
	}
	pio_usb_host_set_sof_only(false);
}

alignas(uint32_t) uint8_t FlashPROM::writeCache[EEPROM_SIZE_BYTES];

int64_t writeToFlash(alarm_id_t id, void *flashCache)
{
	const uint8_t *flash_base = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START);
	const uint8_t *cache_base = reinterpret_cast<const uint8_t *>(flashCache);

	// Quick check: if the entire flash already matches the cache, nothing to do.
	if (memcmp(flash_base, cache_base, EEPROM_SIZE_BYTES) == 0)
	{
		return 0;
	}

	flash_write_begin();

	for (uint32_t sector_offset = 0; sector_offset < EEPROM_SIZE_BYTES; sector_offset += FLASH_SECTOR_SIZE)
	{
		const uint8_t *flash_sec = flash_base + sector_offset;
		const uint8_t *cache_sec = cache_base + sector_offset;

		// Skip sectors that are already identical to the cache
		if (memcmp(flash_sec, cache_sec, FLASH_SECTOR_SIZE) == 0)
		{
			continue;
		}

		// Erase this modified sector
		uint32_t flash_offset = (intptr_t)EEPROM_ADDRESS_START - (intptr_t)XIP_BASE + sector_offset;
		auto status = save_and_disable_interrupts();
		flash_range_erase(flash_offset, FLASH_SECTOR_SIZE);
		restore_interrupts(status);

		// Program only the pages within this sector
		for (uint32_t page_offset = 0; page_offset < FLASH_SECTOR_SIZE; page_offset += FLASH_PAGE_SIZE)
		{

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

	flash_write_end();

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