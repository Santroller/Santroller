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

#define DS5_FEATURE_REPORT_CALIBRATION 0x05
#define DS5_BT_OUTPUT_REPORT_ID 0x31
#define DS5_OUTPUT_MIN_INTERVAL_MS 10
// Offsets into the effects block, which follows the 2 byte bluetooth header (same layout as USB report 0x02
// without its report id, see SDL's DS5EffectsState_t)
#define DS5_EFFECTS_OFFSET 2
#define DS5_EFFECT_FLAGS0 0
#define DS5_EFFECT_FLAGS1 1
#define DS5_EFFECT_RUMBLE_RIGHT 2
#define DS5_EFFECT_RUMBLE_LEFT 3
#define DS5_EFFECT_FLAGS2 38
#define DS5_EFFECT_LIGHTBAR_SETUP 41
#define DS5_EFFECT_PLAYER_LEDS 43
#define DS5_EFFECT_LIGHTBAR 44
// flags0: compatible rumble, and turn off the audio driven haptics
#define DS5_FLAGS0_RUMBLE (0x01 | 0x02)
// flags1: lightbar colour, player LEDs
#define DS5_FLAGS1_LIGHTBAR 0x04
#define DS5_FLAGS1_PLAYER_LEDS 0x10
// flags2: lightbar setup, used to fade out the bluetooth connection animation
#define DS5_FLAGS2_LIGHTBAR_SETUP 0x02
#define DS5_LIGHTBAR_SETUP_LIGHT_OUT 0x02
#define DS5_MODE_SWITCH_ATTEMPTS 3
#define DS5_MODE_SWITCH_RETRY_MS 500
// Same lengths SDL uses to tell the basic report apart from a USB style 0x01 report
#define DS5_SIMPLE_REPORT_LEN 10
#define DS5_SIMPLE_REPORT_LEN_ALT 78

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
    else if (data[0] == 0x01 && (len == DS5_SIMPLE_REPORT_LEN || len == DS5_SIMPLE_REPORT_LEN_ALT))
    {
        // Basic report the DualSense sends until it is switched to 0x31: sticks, then the buttons / hat,
        // then the triggers. USB has the triggers and a counter before the buttons, so move them around.
        memset(m_report_buf, 0, DS5_SIMPLE_REPORT_LEN + 1);
        m_report_buf[0] = 0x01;
        memcpy(m_report_buf + 1, data + 1, 4);
        m_report_buf[5] = data[8];
        m_report_buf[6] = data[9];
        m_report_buf[8] = data[5];
        m_report_buf[9] = data[6];
        // PS and touchpad, the rest of the byte is a counter
        m_report_buf[10] = data[7] & 0x03;
        // Ask again if the controller is still sending basic reports, up to a few times as clones might never switch
        if (m_init_packets_sent && m_mode_switch_attempts < DS5_MODE_SWITCH_ATTEMPTS &&
            millis() - m_mode_switch_at > DS5_MODE_SWITCH_RETRY_MS)
        {
            m_init_packets_sent = false;
        }
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
    // Over bluetooth the DualSense sends basic 0x01 reports until the host reads the calibration feature
    // report, after which it sends full 0x31 reports. Only one control request can be outstanding, so if
    // this fails the next input report asks again.
    if (hid_host_send_get_report(m_cid, HID_REPORT_TYPE_FEATURE, DS5_FEATURE_REPORT_CALIBRATION) ==
        ERROR_CODE_SUCCESS)
    {
        m_mode_switch_pending = true;
        m_mode_switch_attempts++;
        m_mode_switch_at = millis();
        BluetoothHostInterface::send_init_packets();
    }
}

bool BtDs5Host::consume_mode_switch_response()
{
    if (!m_mode_switch_pending)
        return false;
    m_mode_switch_pending = false;
    return true;
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

void BtDs5Host::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    uint32_t now = millis();
    if (!m_output_dirty || !m_cid || now - m_last_output_ms < DS5_OUTPUT_MIN_INTERVAL_MS)
        return;
    // Bluetooth output report: 0x02 like SDL, the effects block, then a CRC32 at the end
    memset(m_out_buf, 0, sizeof(m_out_buf));
    m_out_buf[0] = DS5_BT_OUTPUT_REPORT_ID;
    m_out_buf[1] = 0x02;
    uint8_t *effects = m_out_buf + DS5_EFFECTS_OFFSET;
    effects[DS5_EFFECT_FLAGS0] = DS5_FLAGS0_RUMBLE;
    effects[DS5_EFFECT_FLAGS1] = DS5_FLAGS1_LIGHTBAR | DS5_FLAGS1_PLAYER_LEDS;
    effects[DS5_EFFECT_RUMBLE_RIGHT] = m_rumble_right;
    effects[DS5_EFFECT_RUMBLE_LEFT] = m_rumble_left;
    if (!m_lightbar_setup_sent)
    {
        effects[DS5_EFFECT_FLAGS2] = DS5_FLAGS2_LIGHTBAR_SETUP;
        effects[DS5_EFFECT_LIGHTBAR_SETUP] = DS5_LIGHTBAR_SETUP_LIGHT_OUT;
    }
    effects[DS5_EFFECT_PLAYER_LEDS] = m_player_indicator;
    memcpy(effects + DS5_EFFECT_LIGHTBAR, m_lightbar, sizeof(m_lightbar));
    bt_host_set_output_crc32(m_out_buf, sizeof(m_out_buf));
    uint8_t status;
    {
        // update() runs from the main loop, outside BTstack's context
        BtStackLock lock;
        status = hid_host_send_report(m_cid, m_out_buf[0], m_out_buf + 1, sizeof(m_out_buf) - 1);
    }
    if (status == ERROR_CODE_SUCCESS)
    {
        m_output_dirty = false;
        m_lightbar_setup_sent = true;
        m_last_output_ms = now;
    }
}

void BtDs5Host::set_rumble(uint8_t left, uint8_t right)
{
    if (m_rumble_left == left && m_rumble_right == right)
        return;
    m_rumble_left = left;
    m_rumble_right = right;
    m_output_dirty = true;
}

void BtDs5Host::set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    if (m_lightbar[0] == r && m_lightbar[1] == g && m_lightbar[2] == b)
        return;
    m_lightbar[0] = r;
    m_lightbar[1] = g;
    m_lightbar[2] = b;
    m_output_dirty = true;
}

void BtDs5Host::set_player_led(uint8_t player)
{
    // Same patterns as USB
    static const uint8_t ps5_leds[] = {0x00, 0x04, 0x0A, 0x15, 0x1B};
    uint8_t indicator = player <= 4 ? ps5_leds[player] : 0;
    if (m_player_indicator == indicator)
        return;
    m_player_indicator = indicator;
    m_output_dirty = true;
}

bool BtDs5Host::tick_digital(proto_Output &type)
{
    return ps5_tick_digital(m_report_buf, m_subtype, m_third_party, type);
}

uint16_t BtDs5Host::tick_analog(proto_Output &type)
{
    return ps5_tick_analog(m_report_buf, m_subtype, m_third_party, type);
}
