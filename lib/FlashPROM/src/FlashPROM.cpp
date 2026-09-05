/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#include "FlashPROM.h"
#include <stdio.h>

uint8_t FlashPROM::writeCache[EEPROM_SIZE_BYTES];
volatile static spin_lock_t *flashLock = nullptr;

int64_t writeToFlash(alarm_id_t id, void *flashCache)
{
	while (is_spin_locked(flashLock))
		;

	multicore_lockout_start_blocking();
	uint32_t interrupts = spin_lock_blocking(flashLock);
	flash_range_erase((intptr_t)EEPROM_ADDRESS_START - (intptr_t)XIP_BASE, EEPROM_SIZE_BYTES);
	flash_range_program((intptr_t)EEPROM_ADDRESS_START - (intptr_t)XIP_BASE, reinterpret_cast<uint8_t *>(flashCache), EEPROM_SIZE_BYTES);

	multicore_lockout_end_blocking();
	spin_unlock(flashLock, interrupts);

	return 0;
}

void FlashPROM::start()
{
	if (flashLock == nullptr)
		flashLock = spin_lock_instance(spin_lock_claim_unused(true));

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
void FlashPROM::tick() {
	if (should_commit_at != 0 && to_ms_since_boot(get_absolute_time()) >= should_commit_at) {
		printf("Committing flash at %lu ms\n", to_ms_since_boot(get_absolute_time()));
		commit_now();
		should_commit_at = 0;
	}
}
void FlashPROM::reset()
{
	memset(writeCache, 0, EEPROM_SIZE_BYTES);
	commit();
}