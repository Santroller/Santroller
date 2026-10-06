#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "wm_crypto.h"


#include "spi.hpp"

#define WII_ADDR 0x52
// How long to hold detect low when the subtype changes, so the Wiimote sees the
// extension unplugged and re-reads its ID for the new subtype.
#define WII_SWAP_DISCONNECT_MS 500
// How long the extension keeps its subtype after its last user releases it. Covers a
// config reload tearing down and rebuilding the Wii instance.
#define WII_RELEASE_GRACE_MS 1000
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
    // written from the I2C IRQ
    volatile uint32_t last_activity_ms;
} wii_extension_context_t;
class WiiExtensionEmulation
{
public:
    // detect is the extension detect pin, or -1 if it is hardwired high
    WiiExtensionEmulation(uint8_t block, uint8_t sda, uint8_t scl, int8_t detect);
    ~WiiExtensionEmulation();
    void begin(SubType type);
    // Profile instances hold the extension while they exist. Once the last one lets go
    // and nothing re-acquires it within WII_RELEASE_GRACE_MS, go back to idle_type.
    // Unlike PSX we can't go fully silent while idle: with detect low the Wiimote stops
    // polling, so we'd never see it to activate a profile.
    void acquire();
    void release(SubType idle_type);
    void end();
    void update();
    void set_inputs(uint8_t *inputs, uint8_t len);
    bool get_djh_euphoria_led_state() { return m_context ? m_context->djh_euphoria_led_state : false; }
    uint8_t wii_data_format();
    bool is_communicating() const;

private:
    void start(SubType type);
    WiiExtType m_type = WiiExtType::WiiNoExtension;
    uint8_t m_block;
    uint8_t m_sda;
    uint8_t m_scl;
    int8_t m_detect;
    bool m_reconnecting = false;
    SubType m_reconnect_type;
    uint32_t m_reconnect_at_ms = 0;
    uint8_t m_users = 0;
    bool m_release_pending = false;
    SubType m_idle_type;
    uint32_t m_release_at_ms = 0;
    wii_extension_context_t* m_context = nullptr;
};