/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#include "config/FlashPROM.h"
#include <stdio.h>
#include "tusb.h"
#include "utils.h"

uint8_t FlashPROM::writeCache[EEPROM_SIZE_BYTES];

int64_t writeToFlash(alarm_id_t id, void *flashCache)
{
	multicore_lockout_start_blocking();
	for (uint32_t i = 0; i < EEPROM_SIZE_BYTES; i += FLASH_SECTOR_SIZE)
	{
		tud_task();
		tuh_task();
		auto status = save_and_disable_interrupts();
		flash_range_erase((intptr_t)EEPROM_ADDRESS_START - (intptr_t)XIP_BASE + i, FLASH_SECTOR_SIZE);
		restore_interrupts(status);
	}
	for (uint32_t i = 0; i < EEPROM_SIZE_BYTES; i += FLASH_PAGE_SIZE)
	{
		tud_task();
		tuh_task();
		auto status = save_and_disable_interrupts();
		flash_range_program((intptr_t)EEPROM_ADDRESS_START - (intptr_t)XIP_BASE + i, reinterpret_cast<uint8_t *>(flashCache) + i, FLASH_PAGE_SIZE);
		restore_interrupts(status);
	}
	multicore_lockout_end_blocking();

	return 0;
}

void FlashPROM::start()
{

	memcpy(writeCache, reinterpret_cast<uint8_t *>(EEPROM_ADDRESS_START), EEPROM_SIZE_BYTES);
}

/* We don't have an actual EEPROM, so we need to be extra careful about minimizing writes. Instead
	of writing when a commit is requested, we update a time to actually commit. That way, if we receive multiple requests
	to commit in that timeframe, we'll hold off until the user is done sending changes. */
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