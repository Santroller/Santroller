#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/ps3.hpp"

// ---------------------------------------------------------------------------
// DS3 / DualShock 3 over BT Classic
// First-party controller only — PS3 instruments used USB receivers, not BT.
// ---------------------------------------------------------------------------
class BtDs3Host : public BluetoothHostInterface
{
public:
    BtDs3Host(uint16_t id) : BluetoothHostInterface(id)
    {
        m_subtype = SubType_Gamepad;
        ps3_output_report_init(&m_output_report);
    }
    ~BtDs3Host() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypePS3; }

    // on_connected() sends the DS3 enable sequence via HID set-report
    void send_init_packets() override;
    void update(bool full_poll, bool send_events) override;
    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    bool tick_axis_digital(proto_Output &type) override;
    uint16_t tick_button_pressure(proto_Output &type) override;

private:
    ps3_output_report m_output_report;
    uint8_t m_player = 0;
    uint8_t m_rumble_left = 0;
    uint8_t m_rumble_right = 0;
    bool m_output_dirty = true;
    bool m_output_sent = false;
    uint32_t m_last_output_ms = 0;
};
