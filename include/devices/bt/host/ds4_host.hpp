#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/ps4.hpp"

// ---------------------------------------------------------------------------
// DS4 / DualShock 4 over BT Classic
// BT Classic wire format uses report-id 0x11 with 2-byte header + CRC32 tail;
// we strip those in handle_report() so m_report_buf always holds the canonical
// USB-compatible 0x01 report.
// ---------------------------------------------------------------------------
class BtDs4Host : public BluetoothHostInterface
{
public:
    BtDs4Host(uint16_t id, SubType subtype, bool third_party,
              bool sensors, bool lightbar, bool vibration, bool touchpad,
              uint16_t vid = 0, uint16_t pid = 0)
        : BluetoothHostInterface(id),
          m_third_party(third_party),
          m_sensors_supported(sensors),
          m_lightbar_supported(lightbar),
          m_vibration_supported(vibration),
          m_touchpad_supported(touchpad)
    {
        m_vid = vid;
        m_pid = pid;
        m_subtype = subtype;
        m_ready = true;
    }
    ~BtDs4Host() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypePS4; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void handle_feature_report(const uint8_t *data, uint16_t len) override;
    void handle_feature_report_failed() override;
    void send_init_packets() override;
    void on_connected() override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    bool m_third_party;
    bool m_capabilities_requested = false;
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
};
