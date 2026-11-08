// NOR flash: erase sets whole sectors to 0xFF, programming can only clear bits
#include <cassert>
#include <cstring>
#include <hardware/flash.h>

void flash_range_erase(uint32_t flash_offs, size_t count)
{
    assert(flash_offs % FLASH_SECTOR_SIZE == 0);
    assert(count % FLASH_SECTOR_SIZE == 0);
    assert(flash_offs + count <= PICO_FLASH_SIZE_BYTES);
    memset(fake_flash::memory + flash_offs, 0xFF, count);
}

void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count)
{
    assert(flash_offs % FLASH_PAGE_SIZE == 0);
    assert(count % FLASH_PAGE_SIZE == 0);
    assert(flash_offs + count <= PICO_FLASH_SIZE_BYTES);
    for (size_t i = 0; i < count; i++)
    {
        uint8_t &cell = fake_flash::memory[flash_offs + i];
        if ((cell & data[i]) != data[i])
        {
            fake_flash::program_without_erase = true;
        }
        cell &= data[i];
    }
}
