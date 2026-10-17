/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef FLASHPROM_H_
#define FLASHPROM_H_

#include <stdint.h>
#include <string.h>
#include <pico/lock_core.h>
#include <pico/multicore.h>
#include <hardware/flash.h>
#include <hardware/timer.h>

#define EEPROM_SIZE_BYTES 0x8000 // Reserve 32k of flash memory (ensure this value is divisible by 256)
#define EEPROM_ADDRESS_START (XIP_BASE + PICO_FLASH_SIZE_BYTES - EEPROM_SIZE_BYTES)
#ifdef PFB_RESERVED_FILESYSTEM_SIZE_KB
// Firmware updates write everything below the reserved end of flash, the config must stay out of the way
static_assert(PFB_RESERVED_FILESYSTEM_SIZE_KB * 1024 >= EEPROM_SIZE_BYTES, "The config isn't inside the flash the bootloader reserves");
#endif

// Warning: If the write wait is too long it can stall other processes
#define EEPROM_WRITE_WAIT 50 // Amount of time in ms to wait before blocking core1 and committing to flash

class FlashPROM
{
public:
	// Writes something other than the whole cache, eg just the aux block onto the saved config
	using CommitWriter = void (*)(void *context);
	void start();
	// Commit after EEPROM_WRITE_WAIT. With a writer, that runs instead of writing the cache, unless a
	// commit of the whole cache is already waiting (which has everything the writer would)
	void commit(CommitWriter writer = nullptr, void *context = nullptr);
	// Make flash at offset (from the start of this area) match data, rewriting only the sectors that
	// differ. offset must be sector aligned
	static void write_range(uint32_t offset, const uint8_t *data, uint32_t size);
	void reset();
	void tick();
	void commit_now();

	alignas(uint32_t) static uint8_t writeCache[EEPROM_SIZE_BYTES];
	volatile uint32_t should_commit_at = 0;

private:
	CommitWriter m_writer = nullptr;
	void *m_writer_context = nullptr;

};

inline FlashPROM EEPROM;

// Core1's main loop. Runs from RAM so flash writes don't need to lock core1 out, and keeps the
// USB host's SOF going while core0 is stalled by a flash write
void flash_core1_loop();
// Wrap flash writes in these, so core1 sends the USB host's SOF while core0 can't
void flash_write_begin();
void flash_write_end();

#endif