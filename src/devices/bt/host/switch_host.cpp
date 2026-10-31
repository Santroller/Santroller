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
uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type, const SwitchImuState *imu);

// ============================================================================
// BtSwitchHost
// ============================================================================

// Sends one of the fixed subcommand reports with the packet counter shared with SwitchOutput, so the counter
// keeps counting up across every report like a console does, instead of restarting after the init subcommands
static uint8_t switch_send_subcommand(uint16_t cid, const uint8_t *data, uint8_t len, uint8_t &counter)
{
    uint8_t buf[32];
    memcpy(buf, data, len);
    buf[1] = counter & 0x0F;
    uint8_t status = hid_host_send_report(cid, buf[0], buf + 1, len - 1);
    if (status == ERROR_CODE_SUCCESS)
        counter++;
    return status;
}

void BtSwitchHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    m_imu = {};
    m_imu_init_step = 0;
    m_player_led_unacked = false;
    m_player_led_retries = 0;
}

void BtSwitchHost::send_init_packets()
{
    // Switch to full 0x30 reports, the controller starts out in simple 0x3F mode.
    // Subcommands go out on the interrupt channel, the controller ignores them as SET_REPORTs.
    BtStackLock lock;
    uint8_t res = switch_send_subcommand(m_cid, SWITCH_SET_FULL_REPORT_MODE, sizeof(SWITCH_SET_FULL_REPORT_MODE),
                                         m_output.counter);
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
    // Subcommand replies carry the ack in byte 13 (high bit set) and the subcommand id in byte 14
    if (!m_is_switch2 && len >= 15 && data[0] == 0x21)
    {
        printf("Switch subcommand 0x%02x reply, ack=0x%02x\r\n", data[14], data[13]);
        if (data[14] == SWITCH_SUBCMD_SET_PLAYER_LIGHTS && (data[13] & 0x80))
        {
            m_player_led_unacked = false;
            m_player_led_retries = 0;
        }
    }
    if (!m_is_switch2 && len > 0)
    {
        m_imu.parse(data, len);
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
    // Once in full report mode, turn the IMU on and read its calibration, one subcommand at a time
    static const struct
    {
        const uint8_t *data;
        uint8_t len;
    } imu_subcommands[] = {
        {SWITCH_ENABLE_IMU, sizeof(SWITCH_ENABLE_IMU)},
        {SWITCH_READ_IMU_FACTORY_CALIBRATION, sizeof(SWITCH_READ_IMU_FACTORY_CALIBRATION)},
        {SWITCH_READ_IMU_USER_CALIBRATION, sizeof(SWITCH_READ_IMU_USER_CALIBRATION)},
    };
    if (m_imu_init_step < sizeof(imu_subcommands) / sizeof(imu_subcommands[0]))
    {
        if (now - m_imu_init_at < SWITCH_SUBCOMMAND_INTERVAL_MS)
            return;
        const auto &subcommand = imu_subcommands[m_imu_init_step];
        uint8_t status;
        {
            BtStackLock lock;
            status = switch_send_subcommand(m_cid, subcommand.data, subcommand.len, m_output.counter);
        }
        m_imu_init_at = now;
        if (status == ERROR_CODE_SUCCESS)
            m_imu_init_step++;
        return;
    }
    // Give the last calibration read its reply too, a subcommand sent before that gets dropped
    if (now - m_imu_init_at < SWITCH_SUBCOMMAND_INTERVAL_MS)
        return;
    if (m_player_led_unacked && now - m_player_led_sent_at > SWITCH_PLAYER_LED_RETRY_MS)
    {
        m_player_led_unacked = false;
        if (m_player_led_retries < SWITCH_PLAYER_LED_MAX_RETRIES)
        {
            m_player_led_retries++;
            printf("Switch player lights not acked, resending\r\n");
            m_output.player_dirty = true;
        }
    }
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
    {
        m_output.sent(kind, now);
        if (kind == SwitchOutputKind::PlayerLed)
        {
            m_player_led_unacked = true;
            m_player_led_sent_at = now;
        }
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
            return switch_tick_analog(m_report_buf, type, nullptr);
        }
        return switch2_tick_analog(m_switch2_state, type);
    }
    return switch_tick_analog(m_report_buf, type, &m_imu);
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
        return switch_tick_analog(m_report_buf, type, nullptr);
    }
    return switch2_tick_analog(m_state, type);
}
