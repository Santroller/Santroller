#include "config/flash_chip.h"

#include <hardware/flash.h>
#include <hardware/sync.h>
#include <stdio.h>

static uint32_t s_flash_chip_size = 0;

void detect_flash_chip_size()
{
    // JEDEC ID: manufacturer, memory type, then capacity, which is log2 of the size in bytes on the
    // chips these boards use (Winbond, GigaDevice, Puya, Zbit...): 0x15 is 2MB, 0x16 4MB
    uint8_t tx[4] = {0x9f, 0, 0, 0};
    uint8_t rx[4] = {0, 0, 0, 0};
    uint32_t status = save_and_disable_interrupts();
    flash_do_cmd(tx, rx, sizeof(tx));
    restore_interrupts(status);
    uint8_t capacity = rx[3];
    // Anything else isn't a capacity we can trust, so leave the size unknown rather than guess
    if (capacity >= 0x10 && capacity <= 0x1f)
    {
        s_flash_chip_size = 1u << capacity;
    }
    printf("flash chip %02x %02x %02x, %lu bytes\r\n", rx[1], rx[2], rx[3], (unsigned long)s_flash_chip_size);
}

uint32_t flash_chip_size()
{
    return s_flash_chip_size;
}

bool flash_chip_smaller_than_layout()
{
    return s_flash_chip_size != 0 && s_flash_chip_size < PICO_FLASH_SIZE_BYTES;
}
