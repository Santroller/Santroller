#include "devices/bt/host/switch_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "protocols/switch.hpp"

// The USB host headers pull in TinyUSB, whose HID enums clash with btstack's, so the
// shared tick functions from the USB hosts are declared here instead.
bool switch_tick_digital(const uint8_t *buf, proto_Output &type);
uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type);

// ============================================================================
// BtSwitchHost
// ============================================================================

void BtSwitchHost::on_connected()
{
    // Switch Pro: send USB mode command so it sends full 0x30 reports
    // CMD 0x03 sets the input report mode; 0x30 = full controller state
    static const uint8_t cmd[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x30};
    hid_host_send_set_report(m_cid, HID_REPORT_TYPE_OUTPUT, cmd[0], cmd + 1, sizeof(cmd) - 1);
}

void BtSwitchHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    if (m_is_switch2 && len > 0)
    {
        switch2_parse_report(data, len, m_switch2_state);
    }
}

bool BtSwitchHost::tick_digital(proto_Output &type)
{
    if (m_is_switch2)
    {
        if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
        {
            return switch_tick_digital(m_report_buf, type);
        }
        return switch2_tick_digital(m_switch2_state, type);
    }
    return switch_tick_digital(m_report_buf, type);
}

uint16_t BtSwitchHost::tick_analog(proto_Output &type)
{
    if (m_is_switch2)
    {
        if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
        {
            return switch_tick_analog(m_report_buf, type);
        }
        return switch2_tick_analog(m_switch2_state, type);
    }
    return switch_tick_analog(m_report_buf, type);
}

// ============================================================================
// BleSwitch2Host (Switch 2 BLE Host)
// ============================================================================

BleSwitch2Host::BleSwitch2Host(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
    set_ready(true);
}

void BleSwitch2Host::handle_report(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);

    switch2_parse_report(data, len, m_state);
}

bool BleSwitch2Host::tick_digital(proto_Output &type)
{
    if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
    {
        return switch_tick_digital(m_report_buf, type);
    }
    return switch2_tick_digital(m_state, type);
}

uint16_t BleSwitch2Host::tick_analog(proto_Output &type)
{
    if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
    {
        return switch_tick_analog(m_report_buf, type);
    }
    return switch2_tick_analog(m_state, type);
}
