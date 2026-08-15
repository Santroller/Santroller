#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "wm_crypto.h"


#include "spi.hpp"

#define WII_ADDR 0x52
typedef struct
{
    uint8_t twi_reg[256];
    uint8_t mem_address;
    uint8_t transfer_len;
    bool mem_address_written;
    bool djhEuphoriaLedState;
    ext_crypto_state state;
} wii_extension_context_t;
class WiiExtensionEmulation
{
public:
    WiiExtensionEmulation(uint8_t block, uint8_t sda, uint8_t sdl);
    void begin(SubType type);
    void end();
    void update();
    void setInputs(uint8_t *inputs, uint8_t len);
    uint8_t wii_data_format();

private:
    WiiExtType mType = WiiExtType::WiiNoExtension;
    uint8_t mBlock;
    uint8_t sda;
    uint8_t scl;
    wii_extension_context_t* context;
    bool djhEuphoriaLedState = false;
};