#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "wm_crypto.h"


#include "spi.hpp"

#define WII_ADDR 0x52
typedef struct
{
    uint8_t registers[256];
    uint8_t mem_address;
    uint8_t transfer_length;
    bool encrypted;
    bool mem_address_written;
    bool djh_euphoria_led_state;
    ext_crypto_state state;
    SubType type;
    uint32_t last_activity_ms;
} wii_extension_context_t;
class WiiExtensionEmulation
{
public:
    WiiExtensionEmulation(uint8_t block, uint8_t sda, uint8_t scl);
    ~WiiExtensionEmulation();
    void begin(SubType type);
    void end();
    void update();
    void set_inputs(uint8_t *inputs, uint8_t len);
    bool get_djh_euphoria_led_state() { return m_context ? m_context->djh_euphoria_led_state : false; }
    uint8_t wii_data_format();
    bool is_communicating() const;

private:
    WiiExtType m_type = WiiExtType::WiiNoExtension;
    uint8_t m_block;
    uint8_t m_sda;
    uint8_t m_scl;
    wii_extension_context_t* m_context = nullptr;
};