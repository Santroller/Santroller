#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "wm_crypto.h"


#include "spi.hpp"

class WiiExtensionEmulation
{
public:
    WiiExtensionEmulation(WiiExtType type);
    void update();
    void setInputs(uint8_t *inputs, uint8_t len);
    uint8_t wii_data_format();
    void recv_data(uint8_t addr, uint8_t data);
    void recv_end(uint8_t addr, uint8_t len);
    uint8_t req_data(uint8_t addr);

private:
    WiiExtType mType = WiiExtType::WiiNoExtension;
    // virtual register
    uint8_t twi_reg[256];
    ext_crypto_state state;
    bool djhEuphoriaLedState = false;
};