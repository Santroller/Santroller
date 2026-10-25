#include "devices/bt/host/ds4_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "protocols/ps4.hpp"

// The USB host headers pull in TinyUSB, whose HID enums clash with btstack's, so the
// shared tick functions from the USB hosts are declared here instead.
bool ps4_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, uint32_t *last_ghl_poke);
uint16_t ps4_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, const PSImuCalibration *imu);
bool ps4_parse_capabilities(const uint8_t *data, uint16_t len, uint16_t vid, uint16_t pid,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad, PSImuCalibration &imu);

// ============================================================================
// BtDs4Host
// ============================================================================

#define DS4_FEATURE_REPORT_CALIBRATION_BT 0x05
#define DS4_BT_OUTPUT_REPORT_ID 0x11
#define DS4_USB_OUTPUT_REPORT_ID 0x05
#define DS4_OUTPUT_MIN_INTERVAL_MS 10
#define DS4_MODE_SWITCH_ATTEMPTS 3
#define DS4_MODE_SWITCH_RETRY_MS 500

// BT Classic DS4 uses report 0x11:
//   byte 0: 0xa1 (HID input report prefix)
//   byte 1: 0x11 (report id)
//   bytes 2–3: unknown
//   bytes 4+: same as USB 0x01 report (leftStickX, leftStickY, …)
//   last 4 bytes: CRC32
// We normalise it so m_report_buf looks like a USB 0x01 report
// (report_id=0x01, leftStickX, …) so we can reuse PS4 tick logic unchanged.
void BtDs4Host::handle_report(const uint8_t *data, uint16_t len)
{
    if (len < 2) return;

    // Detect BT-specific framing: first byte is 0x11 when BTstack strips the
    // leading 0xa1, otherwise it arrives as plain report already.
    if (data[0] == 0x11 && len >= 12)
    {
        // Strip 3-byte BT header, copy as report_id=0x01 + payload, drop 4-byte CRC tail
        uint16_t raw_payload_len = (len >= 7) ? (len - 7) : 0;
        uint16_t payload_len = raw_payload_len < (sizeof(m_report_buf) - 1)
                                   ? raw_payload_len
                                   : (sizeof(m_report_buf) - 1);
        m_report_buf[0] = 0x01;
        memcpy(m_report_buf + 1, data + 3, payload_len);
    }
    else
    {
        // The basic 0x01 report the DS4 sends until it is switched to 0x11 already matches USB
        // for the sticks, buttons and triggers, so it can be used as is
        BluetoothHostInterface::handle_report(data, len);
        // Ask again if the controller is still sending basic reports, up to a few times as clones might never switch
        if (data[0] == 0x01 && m_init_packets_sent &&
            m_mode_switch_attempts < DS4_MODE_SWITCH_ATTEMPTS && millis() - m_mode_switch_at > DS4_MODE_SWITCH_RETRY_MS)
        {
            m_init_packets_sent = false;
        }
    }
}

void BtDs4Host::on_connected()
{
    BluetoothHostInterface::on_connected();
}

void BtDs4Host::send_init_packets()
{
    // Over bluetooth the DS4 sends basic 0x01 reports (no touchpad or motion) until the host reads the
    // calibration feature report, after which it sends full 0x11 reports. Only one control request can be
    // outstanding, so if this fails the next input report asks again.
    if (hid_host_send_get_report(m_cid, HID_REPORT_TYPE_FEATURE, DS4_FEATURE_REPORT_CALIBRATION_BT) ==
        ERROR_CODE_SUCCESS)
    {
        m_mode_switch_pending = true;
        m_mode_switch_attempts++;
        m_mode_switch_at = millis();
        BluetoothHostInterface::send_init_packets();
    }
}

bool BtDs4Host::consume_mode_switch_response()
{
    if (!m_mode_switch_pending)
        return false;
    m_mode_switch_pending = false;
    return true;
}

void BtDs4Host::handle_feature_report(const uint8_t *data, uint16_t len)
{
    if (m_third_party)
    {
        printf("PS4 Feature Report received (len=%d): ", len);
        for (int i = 0; i < len; i++)
            printf("%02x ", data[i]);
        printf("\r\n");

        SubType sub = m_subtype;
        bool sens = m_sensors_supported;
        bool light = m_lightbar_supported;
        bool vib = m_vibration_supported;
        bool touch = m_touchpad_supported;
        if (ps4_parse_capabilities(data, len, m_vid, m_pid, sub, sens, light, vib, touch, m_imu_calibration))
        {
            m_subtype = sub;
            m_sensors_supported = sens;
            m_lightbar_supported = light;
            m_vibration_supported = vib;
            m_touchpad_supported = touch;
            printf("PS4 3rd-party capabilities: subtype=%d, sensors=%d, light=%d, vib=%d, touch=%d\r\n",
                   (int)m_subtype, (int)m_sensors_supported, (int)m_lightbar_supported,
                   (int)m_vibration_supported, (int)m_touchpad_supported);
        }
        else
        {
            printf("PS4 3rd-party capabilities: parse failed, keeping subtype=%d\r\n", (int)m_subtype);
        }
        m_ready = true;
    }
}

void BtDs4Host::handle_feature_report_failed()
{
    m_ready = true;
}

void BtDs4Host::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    uint32_t now = millis();
    if (!m_output_dirty || !m_cid || now - m_last_output_ms < DS4_OUTPUT_MIN_INTERVAL_MS)
        return;
    uint16_t len;
    if (m_third_party)
    {
        // Like SDL, third party controllers get the USB output report over bluetooth too
        ps4_output_report report = {};
        report.report_id = DS4_USB_OUTPUT_REPORT_ID;
        report.valid_flag0 = 0x07;
        report.motor_left = m_rumble_left;
        report.motor_right = m_rumble_right;
        report.lightbar_red = m_lightbar[0];
        report.lightbar_green = m_lightbar[1];
        report.lightbar_blue = m_lightbar[2];
        memcpy(m_out_buf, &report, sizeof(report));
        len = sizeof(report);
    }
    else
    {
        // Bluetooth output report: 0xC0 is HID + CRC (0x04 asks for 4ms input reports), flags 0x01 rumble and
        // 0x02 lightbar, then the same rumble / lightbar layout as USB and a CRC32 at the end
        memset(m_out_buf, 0, sizeof(m_out_buf));
        m_out_buf[0] = DS4_BT_OUTPUT_REPORT_ID;
        m_out_buf[1] = 0xC0 | 0x04;
        m_out_buf[3] = 0x03;
        m_out_buf[6] = m_rumble_right;
        m_out_buf[7] = m_rumble_left;
        memcpy(m_out_buf + 8, m_lightbar, sizeof(m_lightbar));
        bt_host_set_output_crc32(m_out_buf, sizeof(m_out_buf));
        len = sizeof(m_out_buf);
    }
    uint8_t status;
    {
        // update() runs from the main loop, outside BTstack's context
        BtStackLock lock;
        status = hid_host_send_report(m_cid, m_out_buf[0], m_out_buf + 1, len - 1);
    }
    if (status == ERROR_CODE_SUCCESS)
    {
        m_output_dirty = false;
        m_last_output_ms = now;
    }
}

void BtDs4Host::set_rumble(uint8_t left, uint8_t right)
{
    if (m_rumble_left == left && m_rumble_right == right)
        return;
    m_rumble_left = left;
    m_rumble_right = right;
    m_output_dirty = true;
}

void BtDs4Host::set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    if (m_lightbar[0] == r && m_lightbar[1] == g && m_lightbar[2] == b)
        return;
    m_lightbar[0] = r;
    m_lightbar[1] = g;
    m_lightbar[2] = b;
    m_output_dirty = true;
}

void BtDs4Host::set_player_led(uint8_t player)
{
    // Same as USB, the DS4 shows the player with the lightbar colour
    if (player == 0)
        set_lightbar(0, 0, 0);
    else if (player <= 4)
        set_lightbar(ps4_colors[player - 1][0], ps4_colors[player - 1][1], ps4_colors[player - 1][2]);
}

bool BtDs4Host::tick_digital(proto_Output &type)
{
    return ps4_tick_digital(m_report_buf, m_subtype, m_third_party, type, nullptr);
}

uint16_t BtDs4Host::tick_analog(proto_Output &type)
{
    // First party controllers over bluetooth use the nominal scale, as their calibration is only read
    // to switch them to full reports
    return ps4_tick_analog(m_report_buf, m_subtype, m_third_party, type, m_sensors_supported ? &m_imu_calibration : nullptr);
}
