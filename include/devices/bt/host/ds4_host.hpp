#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/ps4.hpp"

#define DS4_BT_OUTPUT_REPORT_LEN 78

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
    bool consume_mode_switch_response() override;
    void send_init_packets() override;
    void on_connected() override;
    void update(bool full_poll, bool send_events) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    void set_rumble(uint8_t left, uint8_t right) override;
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b) override;
    void set_player_led(uint8_t player) override;
    bool has_rumble() const override { return m_vibration_supported; }
    bool has_motion() const override { return m_sensors_supported; }
    bool has_lightbar() const override { return m_lightbar_supported; }
    bool has_player_led() const override { return m_lightbar_supported; }

private:
    bool m_third_party;
    uint8_t m_rumble_left = 0;
    uint8_t m_rumble_right = 0;
    uint8_t m_lightbar[3] = {};
    bool m_output_dirty = false;
    uint32_t m_last_output_ms = 0;
    uint8_t m_out_buf[DS4_BT_OUTPUT_REPORT_LEN] = {};
    bool m_mode_switch_pending = false;
    uint8_t m_mode_switch_attempts = 0;
    uint32_t m_mode_switch_at = 0;
    bool m_capabilities_requested = false;
    bool m_sensors_supported;
    bool m_lightbar_supported;
    bool m_vibration_supported;
    bool m_touchpad_supported;
    PSImuCalibration m_imu_calibration;
};
