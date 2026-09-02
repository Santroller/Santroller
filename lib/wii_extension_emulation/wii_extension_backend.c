#include "wii_extension_backend.h"
#include <string.h>

static const uint8_t classic_calibration[16] = {
    0xE1, 0x19, 0x7C, 0xEF, 0x22, 0x7C, 0xE6, 0x1E,
    0x85, 0xDE, 0x15, 0x8B, 0x0E, 0x22, 0x8F, 0xE4
};
static const uint8_t nunchuk_calibration[16] = {
    0x81, 0x80, 0x7F, 0x22, 0xB5, 0xB3, 0xB3, 0x03,
    0x00, 0x00, 0x7C, 0x00, 0x00, 0x83, 0x14, 0x69
};
static const uint8_t balance_board_id[6] = {0x00, 0x00, 0xA4, 0x20, 0x04, 0x02};

void wii_extension_backend_init(uint8_t *registers, bool *encrypted, uint8_t extension_type)
{
    memset(registers, 0, 256);
    *encrypted = false;
    registers[0xF0] = 0;

    const uint8_t *calibration = extension_type == WII_EXTENSION_NUNCHUK
                                      ? nunchuk_calibration : classic_calibration;
    if (extension_type == WII_EXTENSION_BALANCE_BOARD)
    {
        registers[0xFA] = balance_board_id[0];
        registers[0xFB] = balance_board_id[1];
        registers[0xFC] = balance_board_id[2];
        registers[0xFD] = balance_board_id[3];
        registers[0xFE] = balance_board_id[4];
        registers[0xFF] = balance_board_id[5];
        return;
    }
    for (uint8_t i = 0; i < sizeof(classic_calibration); i++)
    {
        registers[0x20 + i] = calibration[i];
        registers[0x30 + i] = calibration[i];
    }

    registers[0xFA] = extension_type == WII_EXTENSION_DRUMS ? 0x01 :
                      extension_type == WII_EXTENSION_TURNTABLE ? 0x03 : 0x00;
    registers[0xFB] = 0x00;
    registers[0xFC] = 0xA4;
    registers[0xFD] = 0x20;
    registers[0xFE] = extension_type == WII_EXTENSION_NUNCHUK ? 0x00 : 0x01;
    registers[0xFF] = extension_type == WII_EXTENSION_NUNCHUK ? 0x00 :
                      extension_type == WII_EXTENSION_CLASSIC ? 0x01 : 0x03;
}

uint8_t wii_extension_backend_read(const uint8_t *registers, bool encrypted,
                                   const struct ext_crypto_state *crypto, uint8_t offset)
{
    uint8_t value = registers[offset];
    if (encrypted)
    {
        value = (value - crypto->ft[offset % 8]) ^ crypto->sb[offset % 8];
    }
    return value;
}

uint8_t wii_extension_backend_write(uint8_t *registers, bool *encrypted,
                                    struct ext_crypto_state *crypto, uint8_t offset,
                                    uint8_t value)
{
    if (*encrypted && offset != 0xF0)
    {
        value = (value ^ crypto->sb[offset % 8]) + crypto->ft[offset % 8];
    }

    registers[offset] = value;
    if (offset == 0xF0)
    {
        if (value == 0xAA)
        {
            *encrypted = true;
        }
        else if (value == 0x55)
        {
            *encrypted = false;
        }
    }
    return value;
}

void wii_extension_backend_generate_tables(const uint8_t *registers,
                                            struct ext_crypto_state *crypto,
                                            bool *encrypted)
{
    ext_generate_tables(crypto, &registers[0x40]);
    *encrypted = true;
}
