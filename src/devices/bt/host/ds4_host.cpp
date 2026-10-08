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
uint16_t ps4_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
bool ps4_parse_capabilities(const uint8_t *data, uint16_t len, uint16_t vid, uint16_t pid,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad);

// ============================================================================
// BtDs4Host
// ============================================================================

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
        // Already in USB format (or unknown; just copy verbatim)
        BluetoothHostInterface::handle_report(data, len);
    }
}

void BtDs4Host::on_connected()
{
    BluetoothHostInterface::on_connected();
}

void BtDs4Host::send_init_packets()
{
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
        if (ps4_parse_capabilities(data, len, m_vid, m_pid, sub, sens, light, vib, touch))
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

bool BtDs4Host::tick_digital(proto_Output &type)
{
    return ps4_tick_digital(m_report_buf, m_subtype, m_third_party, type, nullptr);
}

uint16_t BtDs4Host::tick_analog(proto_Output &type)
{
    return ps4_tick_analog(m_report_buf, m_subtype, m_third_party, type);
}
