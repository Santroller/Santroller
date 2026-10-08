#include "devices/bt/host/steam_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "ble/gatt-service/hids_host.h"

// ============================================================================
// BleSteamHost (Valve Steam Controller BLE)
// ============================================================================

BleSteamHost::BleSteamHost(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
}

void BleSteamHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    if (m_cid)
    {
        hids_host_send_write_report(m_cid, 0, HID_REPORT_TYPE_FEATURE,
                                    (uint8_t *)STEAM_CMD_CLEAR_MAPPINGS_BUF, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF));
        hids_host_send_write_report(m_cid, 0, HID_REPORT_TYPE_FEATURE,
                                    (uint8_t *)STEAM_CMD_DISABLE_LIZARD_BUF, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF));
    }
    if (m_con_handle && m_char_handle)
    {
        gatt_client_write_value_of_characteristic_without_response(
            m_con_handle, m_char_handle, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF),
            (uint8_t *)STEAM_CMD_CLEAR_MAPPINGS_BUF);
        gatt_client_write_value_of_characteristic_without_response(
            m_con_handle, m_char_handle, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF),
            (uint8_t *)STEAM_CMD_DISABLE_LIZARD_BUF);
    }
    set_ready(true);
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
