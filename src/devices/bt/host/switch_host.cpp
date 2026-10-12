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
    BluetoothHostInterface::on_connected();
}

void BtSwitchHost::send_init_packets()
{
    // Switch to full 0x30 reports, the controller starts out in simple 0x3F mode.
    // Subcommands go out on the interrupt channel, the controller ignores them as SET_REPORTs.
    BtStackLock lock;
    uint8_t res = hid_host_send_report(m_cid, SWITCH_SET_FULL_REPORT_MODE[0], SWITCH_SET_FULL_REPORT_MODE + 1,
                                       sizeof(SWITCH_SET_FULL_REPORT_MODE) - 1);
    printf("Switch set report mode: cid=0x%04x status=0x%02x\r\n", m_cid, res);
    if (res == ERROR_CODE_SUCCESS)
    {
        m_mode_sent_at = millis();
        BluetoothHostInterface::send_init_packets();
    }
}

void BtSwitchHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    // The mode command can get dropped if it arrives too early, so keep asking while it's still in simple mode
    if (!m_is_switch2 && len > 0 && data[0] == SWITCH_PRO_CON_SIMPLE_REPORT_ID && m_init_packets_sent &&
        millis() - m_mode_sent_at > SWITCH_MODE_RETRY_MS)
    {
        m_init_packets_sent = false;
    }
    if (len > 0 && (data[0] == SWITCH_PRO_CON_FULL_REPORT_ID || data[0] == 0x21))
    {
        m_full_reports = true;
    }
    if (m_is_switch2 && len > 0)
    {
        switch2_parse_report(data, len, m_switch2_state);
    }
}

void BtSwitchHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (m_is_switch2 || !m_cid || !m_full_reports)
        return;
    uint32_t now = millis();
    SwitchOutputKind kind;
    uint8_t len = m_output.build(now, m_out_buf, kind);
    if (!len)
        return;
    uint8_t status;
    {
        // update() runs from the main loop, outside BTstack's context
        BtStackLock lock;
        status = hid_host_send_report(m_cid, m_out_buf[0], m_out_buf + 1, len - 1);
    }
    if (status == ERROR_CODE_SUCCESS)
        m_output.sent(kind, now);
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
