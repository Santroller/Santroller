#include "devices/bt/host/steam_triton_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "ble/gatt-service/hids_host.h"

// ============================================================================
// BleSteamTritonHost (2026 Steam Controller BLE)
// ============================================================================

BleSteamTritonHost::BleSteamTritonHost(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
}

// Must be called with the btstack lock held
void BleSteamTritonHost::disable_lizard_mode()
{
    m_last_lizard_update = millis();
    if (m_input_handle)
    {
        if (!m_con_handle || !m_report_char_handle)
            return;
        // SDL sends the whole 63 byte message, fall back to just the setting if the MTU is too small
        uint8_t msg[TRITON_FEATURE_REPORT_LEN - 1] = {};
        memcpy(msg, TRITON_DISABLE_LIZARD_MSG, sizeof(TRITON_DISABLE_LIZARD_MSG));
        if (gatt_client_write_value_of_characteristic_without_response(
                m_con_handle, m_report_char_handle, sizeof(msg), msg) != ERROR_CODE_SUCCESS)
        {
            gatt_client_write_value_of_characteristic_without_response(
                m_con_handle, m_report_char_handle, sizeof(TRITON_DISABLE_LIZARD_MSG),
                (uint8_t *)TRITON_DISABLE_LIZARD_MSG);
        }
    }
    else if (m_cid)
    {
        hids_host_send_write_report(m_cid, TRITON_FEATURE_REPORT_ID, HID_REPORT_TYPE_FEATURE,
                                    TRITON_DISABLE_LIZARD_MSG, sizeof(TRITON_DISABLE_LIZARD_MSG));
    }
}

void BleSteamTritonHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    // on_connected runs from btstack callbacks, so the lock is already held
    disable_lizard_mode();
    set_ready(true);
}

void BleSteamTritonHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (!m_ready || (millis() - m_last_lizard_update) < TRITON_LIZARD_REFRESH_MS)
        return;
    BtStackLock lock;
    disable_lizard_mode();
}

void BleSteamTritonHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (len == 0)
        return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);

    if (m_input_handle)
        steam_triton_parse_state(data, len, m_state);
    else
        steam_triton_parse_report(data, len, m_state);
}

bool BleSteamTritonHost::tick_digital(proto_Output &type)
{
    return steam_triton_tick_digital(m_state, type);
}

uint16_t BleSteamTritonHost::tick_analog(proto_Output &type)
{
    return steam_triton_tick_analog(m_state, type);
}
