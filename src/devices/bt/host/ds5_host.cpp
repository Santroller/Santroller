#include "devices/bt/host/ds5_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

// The USB host headers pull in TinyUSB, whose HID enums clash with btstack's, so the
// shared tick functions from the USB hosts are declared here instead.
bool ps5_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
uint16_t ps5_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
bool ps5_parse_capabilities(const uint8_t *data, uint16_t len,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad);

// ============================================================================
// BtDs5Host (DualSense over BT Classic)
// ============================================================================

void BtDs5Host::handle_report(const uint8_t *data, uint16_t len)
{
    if (len < 2) return;

    if (data[0] == 0x31 && len >= 11)
    {
        uint16_t raw_payload_len = (len >= 2) ? (len - 2) : 0;
        uint16_t payload_len = raw_payload_len < (sizeof(m_report_buf) - 1)
                                   ? raw_payload_len
                                   : (sizeof(m_report_buf) - 1);
        m_report_buf[0] = 0x01;
        memcpy(m_report_buf + 1, data + 2, payload_len);
    }
    else
    {
        BluetoothHostInterface::handle_report(data, len);
    }
}

void BtDs5Host::on_connected()
{
    BluetoothHostInterface::on_connected();
}

void BtDs5Host::send_init_packets()
{
}

void BtDs5Host::handle_feature_report(const uint8_t *data, uint16_t len)
{
    if (m_third_party)
    {
        SubType sub = m_subtype;
        bool sens = m_sensors_supported;
        bool light = m_lightbar_supported;
        bool vib = m_vibration_supported;
        bool touch = m_touchpad_supported;
        if (ps5_parse_capabilities(data, len, sub, sens, light, vib, touch))
        {
            m_subtype = sub;
            m_sensors_supported = sens;
            m_lightbar_supported = light;
            m_vibration_supported = vib;
            m_touchpad_supported = touch;
            printf("PS5 3rd-party capabilities: subtype=%d, sensors=%d, light=%d, vib=%d, touch=%d\r\n",
                   (int)m_subtype, (int)m_sensors_supported, (int)m_lightbar_supported,
                   (int)m_vibration_supported, (int)m_touchpad_supported);
        }
        else
        {
            printf("PS5 3rd-party capabilities: parse failed, defaulting to subtype=%d\r\n", (int)m_subtype);
        }
        m_ready = true;
    }
}

void BtDs5Host::handle_feature_report_failed()
{
    m_ready = true;
}

bool BtDs5Host::tick_digital(proto_Output &type)
{
    return ps5_tick_digital(m_report_buf, m_subtype, m_third_party, type);
}

uint16_t BtDs5Host::tick_analog(proto_Output &type)
{
    return ps5_tick_analog(m_report_buf, m_subtype, m_third_party, type);
}
