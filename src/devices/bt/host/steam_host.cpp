#include "devices/bt/host/steam_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

// ============================================================================
// BleSteamHost (Valve Steam Controller BLE)
// ============================================================================

BleSteamHost::BleSteamHost(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
}

static const struct
{
    const uint8_t *buf;
    uint16_t len;
} steam_setup_steps[] = {
    {STEAM_CMD_CLEAR_MAPPINGS_BUF, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF)},
    {STEAM_CMD_DISABLE_LIZARD_BUF, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF)},
};
#define STEAM_SETUP_STEP_COUNT (sizeof(steam_setup_steps) / sizeof(steam_setup_steps[0]))

// Must be called with the btstack lock held
void BleSteamHost::send_setup()
{
    if (!m_con_handle || !m_char_handle)
        return;
    // Writes without response fail while there is no room to send, so each one is retried until it goes out
    while (m_setup_step < STEAM_SETUP_STEP_COUNT)
    {
        auto &step = steam_setup_steps[m_setup_step];
        if (gatt_client_write_value_of_characteristic_without_response(m_con_handle, m_char_handle, step.len,
                                                                       (uint8_t *)step.buf) != ERROR_CODE_SUCCESS)
            return;
        m_setup_step++;
    }
}

void BleSteamHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    // on_connected runs from btstack callbacks, so the lock is already held
    send_setup();
    set_ready(true);
}

void BleSteamHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (m_setup_step >= STEAM_SETUP_STEP_COUNT)
        return;
    BtStackLock lock;
    send_setup();
}

void BleSteamHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);

    steam_parse_ble_report(data, len, m_state);
}

bool BleSteamHost::tick_digital(proto_Output &type)
{
    return steam_tick_digital(m_state, type);
}

uint16_t BleSteamHost::tick_analog(proto_Output &type)
{
    return steam_tick_analog(m_state, type);
}
