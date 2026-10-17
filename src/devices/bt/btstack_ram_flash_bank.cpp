#include "hal_flash_bank.h"
#include <stdint.h>
#include <string.h>

// btstack_cyw43_init() always sets BTstack up with the SDK's flash bank TLV, which lives in the last two
// sectors of flash, inside the config. Setting it up writes a header there when neither bank has one,
// which wipes that part of the config. Pairings are kept in the config's aux block instead
// (BtTlvStorage, installed straight after), so the bank it gets only has to exist: keep it in RAM.
// btstack_cyw43_init() picks it up through pico_flash_bank_instance(), which the link wraps to this.

#define RAM_BANK_SIZE 512

static uint8_t s_banks[2][RAM_BANK_SIZE];

static uint32_t ram_bank_get_size(void *)
{
    return RAM_BANK_SIZE;
}

static uint32_t ram_bank_get_alignment(void *)
{
    return 1;
}

static void ram_bank_erase(void *, int bank)
{
    memset(s_banks[bank], 0xFF, RAM_BANK_SIZE);
}

static void ram_bank_read(void *, int bank, uint32_t offset, uint8_t *buffer, uint32_t size)
{
    if (offset >= RAM_BANK_SIZE)
    {
        return;
    }
    if (size > RAM_BANK_SIZE - offset)
    {
        size = RAM_BANK_SIZE - offset;
    }
    memcpy(buffer, s_banks[bank] + offset, size);
}

static void ram_bank_write(void *, int bank, uint32_t offset, const uint8_t *data, uint32_t size)
{
    if (offset >= RAM_BANK_SIZE)
    {
        return;
    }
    if (size > RAM_BANK_SIZE - offset)
    {
        size = RAM_BANK_SIZE - offset;
    }
    // Like flash, a write can only clear bits
    for (uint32_t i = 0; i < size; i++)
    {
        s_banks[bank][offset + i] &= data[i];
    }
}

static const hal_flash_bank_t s_ram_bank = {
    &ram_bank_get_size,
    &ram_bank_get_alignment,
    &ram_bank_erase,
    &ram_bank_read,
    &ram_bank_write,
};

extern "C" const hal_flash_bank_t *__wrap_pico_flash_bank_instance(void)
{
    static bool s_erased = false;
    if (!s_erased)
    {
        s_erased = true;
        memset(s_banks, 0xFF, sizeof(s_banks));
    }
    return &s_ram_bank;
}
