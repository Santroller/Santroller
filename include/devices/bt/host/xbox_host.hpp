#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/xbox_hid.hpp"

// ---------------------------------------------------------------------------
// Xbox One S / Elite 2 / Series controllers over BT Classic
// These are plain HID over Bluetooth, see protocols/xbox_hid.hpp
// ---------------------------------------------------------------------------
class BtXboxHost : public BluetoothHostInterface
{
public:
    BtXboxHost(uint16_t id, uint16_t pid, HID_ReportInfo_t *info);
    ~BtXboxHost() {}

    void set_report_info(HID_ReportInfo_t *info) { m_desc.init(info, m_pid); }

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeXboxOne; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void update(bool full_poll, bool send_events) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    void set_rumble(uint8_t left, uint8_t right) override { m_rumble.set(left, right); }
    bool has_rumble() const override { return true; }

private:
    XboxHidState m_state = {};
    XboxHidDescriptor m_desc;
    XboxHidRumble m_rumble;
    uint8_t m_out_buf[XBOX_HID_RUMBLE_LEN] = {};
};

// ---------------------------------------------------------------------------
// Xbox One S / Elite 2 / Series controllers over BLE, see protocols/xbox_hid.hpp
// ---------------------------------------------------------------------------
class BleXboxHost : public BluetoothHostInterface
{
public:
    BleXboxHost(uint16_t id, uint16_t pid, HID_ReportInfo_t *info) : BluetoothHostInterface(id)
    {
        m_subtype = SubType_Gamepad;
        m_pid = pid;
        m_desc.init(info, pid);
    }
    ~BleXboxHost() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeXboxOne; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void update(bool full_poll, bool send_events) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    void set_rumble(uint8_t left, uint8_t right) override { m_rumble.set(left, right); }
    bool has_rumble() const override { return true; }

private:
    XboxHidState m_state = {};
    XboxHidDescriptor m_desc;
    XboxHidRumble m_rumble;
    uint8_t m_out_buf[XBOX_HID_RUMBLE_LEN] = {};
};
