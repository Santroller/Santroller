#include "wii_extension_emulation.hpp"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
// calibration data
const unsigned char cal_data[32] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
const uint8_t id_guitar[6] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x03};
const uint8_t id_drum[6] = {0x01, 0x00, 0xA4, 0x20, 0x01, 0x03};
const uint8_t id_turntable[6] = {0x03, 0x00, 0xA4, 0x20, 0x01, 0x03};
const uint8_t id_classic[6] = {0x01, 0x00, 0xA4, 0x20, 0x01, 0x01};

WiiExtensionEmulation::WiiExtensionEmulation(WiiExtType type) : mType(type)
{
    memset(twi_reg, 0, sizeof(twi_reg));
    twi_reg[0xF0] = 0; // disable encryption
    // set id
    for (unsigned int i = 0, j = 0xFA; i < 6; i++, j++)
    {
        switch (type)
        {
        case WiiGuitarHeroGuitar:
            twi_reg[j] = id_guitar[i];
            break;
        case WiiGuitarHeroDrums:
            twi_reg[j] = id_drum[i];
            break;
        case WiiDjHeroTurntable:
            twi_reg[j] = id_turntable[i];
            break;
        default:
            twi_reg[j] = id_classic[i];
            break;
        }
    }

    // set calibration data
    for (unsigned int i = 0, j = 0x20; i < 6; i++, j++)
    {
        twi_reg[j] = cal_data[i];
    }
}
// TODO: hook all this stuff up
void WiiExtensionEmulation::setInputs(uint8_t *inputs, uint8_t len)
{
    memcpy(twi_reg, inputs, len);
}

uint8_t WiiExtensionEmulation::wii_data_format()
{
    return twi_reg[0xFE];
}
// TODO: figure out how we want to handle this
void WiiExtensionEmulation::recv_data(uint8_t addr, uint8_t data)
{
    if (twi_reg[0xF0] == 0xAA && addr != 0xF0) // if encryption is on
    {
        // decrypt
        ext_decrypt_bytes(&state, &data, 0, 1);
    }
    twi_reg[addr] = data;
    // Euphoria LED
    if (addr == 0xFB)
    {
        djhEuphoriaLedState = twi_reg[addr];
    }
}
void WiiExtensionEmulation::recv_end(uint8_t addr, uint8_t len)
{
    if (addr >= 0x4C && addr < 0x50)
    {
        if (addr + len == 0x50)
        {
            // generate tables once all data is loaded
            ext_generate_tables(&state, &twi_reg[0x40]);
        }
    }
}

// Called when the I2C slave is read from
uint8_t WiiExtensionEmulation::req_data(uint8_t addr)
{
    uint8_t d = twi_reg[addr];
    if (twi_reg[0xF0] == 0xAA) // encryption is on
    {
        // encrypt
        ext_encrypt_bytes(&state, &d, 0, 1);
    }
    return d;
}