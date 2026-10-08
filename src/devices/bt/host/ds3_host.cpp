#include "devices/bt/host/ds3_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "protocols/ps3.hpp"

// The USB host headers pull in TinyUSB, whose HID enums clash with btstack's, so the
// shared tick functions from the USB hosts are declared here instead.
bool ps3_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, bool wt = false);
uint16_t ps3_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);
uint16_t ps3_tick_button_pressure(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type);

// ============================================================================
// BtDs3Host
// ============================================================================

void BtDs3Host::send_init_packets()
{
    // Enable DS3 HID reports. Over bluetooth this is 42 03 00 00 (Linux
    // sixaxis_set_operational_bt), not the 42 0c 00 00 used over USB.
    static const uint8_t enable[] = {0x42, 0x03, 0x00, 0x00};
    hid_host_send_set_report(m_cid, HID_REPORT_TYPE_FEATURE, 0xF4, enable, sizeof(enable));
    // marks init done, otherwise every input report queues the enable again
    BluetoothHostInterface::send_init_packets();
}

#define DS3_BT_OUTPUT_MIN_INTERVAL_MS 150

void BtDs3Host::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (!m_output_dirty || !init_packets_sent())
        return;
    // Some DS3/SIXAXIS revisions lock up over bluetooth (ignoring rumble/LEDs for
    // seconds, or needing a power cycle) if output reports come less than 150ms apart
    // (see DsHidMini's output rate control). Changes in between just leave the output
    // dirty, so the latest state goes out once the window passes.
    if (m_output_sent && millis() - m_last_output_ms < DS3_BT_OUTPUT_MIN_INTERVAL_MS)
        return;
    m_output_report.rumble.right_motor_on = m_rumble_right ? 1 : 0;
    m_output_report.rumble.left_motor_force = m_rumble_left;
    m_output_report.leds_bitmap = ps3_leds_bitmap_for_player(m_player);
    // Over bluetooth the DS3 does want the report id in front of the data (unlike USB,
    // Linux only skips it there); BTstack sends 52 01 followed by the same 35 bytes.
    // Only one control request can be outstanding, so if the enable is still in
    // flight this fails and we retry next pass.
    // update() runs from the main loop, outside BTstack's context
    uint8_t status;
    {
        BtStackLock lock;
        status = hid_host_send_set_report(m_cid, HID_REPORT_TYPE_OUTPUT, PS3_RUMBLE_ID,
                                          reinterpret_cast<const uint8_t *>(&m_output_report),
                                          sizeof(m_output_report));
    }
    if (status == ERROR_CODE_SUCCESS)
    {
        m_output_dirty = false;
        m_output_sent = true;
        m_last_output_ms = millis();
    }
}

void BtDs3Host::set_rumble(uint8_t left, uint8_t right)
{
    if (m_rumble_left != left || m_rumble_right != right)
        m_output_dirty = true;
    m_rumble_left = left;
    m_rumble_right = right;
}

void BtDs3Host::set_player_led(uint8_t player)
{
    if (m_player != player)
        m_output_dirty = true;
    m_player = player;
}

bool BtDs3Host::tick_digital(proto_Output &type)
{
    return ps3_tick_digital(m_report_buf, m_subtype, false, type);
}

uint16_t BtDs3Host::tick_analog(proto_Output &type)
{
    return ps3_tick_analog(m_report_buf, m_subtype, false, type);
}

bool BtDs3Host::tick_axis_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag &&
        (type.mapping.gamepadAxis == Gamepad_LeftTrigger || type.mapping.gamepadAxis == Gamepad_RightTrigger))
    {
        return tick_digital(type);
    }
    return BluetoothHostInterface::tick_axis_digital(type);
}

uint16_t BtDs3Host::tick_button_pressure(proto_Output &type)
{
    return ps3_tick_button_pressure(m_report_buf, m_subtype, false, type);
}
