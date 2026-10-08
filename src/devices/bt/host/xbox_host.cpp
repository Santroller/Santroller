#include "devices/bt/host/xbox_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "ble/gatt-service/hids_host.h"

// ============================================================================
// BtXboxHost (Xbox One S / Elite 2 / Series over BT Classic)
// ============================================================================

BtXboxHost::BtXboxHost(uint16_t id, uint16_t pid) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
    m_pid = pid;
}

void BtXboxHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    xbox_bt_parse_report(data, len, m_pid == XBOX_BT_SERIES_PID, m_state);
}

void BtXboxHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    uint32_t now = millis();
    if (!m_cid || !m_rumble.should_send(now))
        return;
    xbox_bt_build_rumble(m_rumble.left, m_rumble.right, m_out_buf);
    uint8_t status;
    {
        BtStackLock lock;
        status = hid_host_send_report(m_cid, XBOX_BT_RUMBLE_REPORT_ID, m_out_buf, sizeof(m_out_buf));
    }
    if (status == ERROR_CODE_SUCCESS)
        m_rumble.sent(now);
}

bool BtXboxHost::tick_digital(proto_Output &type)
{
    return xbox_bt_tick_digital(m_state, type);
}

uint16_t BtXboxHost::tick_analog(proto_Output &type)
{
    return xbox_bt_tick_analog(m_state, type);
}

// ============================================================================
// BleXboxHost (Xbox One S / Elite 2 / Series BLE)
// ============================================================================

void BleXboxHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    xbox_bt_parse_report(data, len, m_pid == XBOX_BT_SERIES_PID, m_state);
}

void BleXboxHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    uint32_t now = millis();
    if (!m_cid || !m_rumble.should_send(now))
        return;
    xbox_bt_build_rumble(m_rumble.left, m_rumble.right, m_out_buf);
    uint8_t status;
    {
        BtStackLock lock;
        status = hids_host_send_write_report(m_cid, XBOX_BT_RUMBLE_REPORT_ID, HID_REPORT_TYPE_OUTPUT,
                                             m_out_buf, sizeof(m_out_buf));
    }
    if (status == ERROR_CODE_SUCCESS)
        m_rumble.sent(now);
}

bool BleXboxHost::tick_digital(proto_Output &type)
{
    return xbox_bt_tick_digital(m_state, type);
}

uint16_t BleXboxHost::tick_analog(proto_Output &type)
{
    return xbox_bt_tick_analog(m_state, type);
}
