#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/xbox_bt.hpp"

// ---------------------------------------------------------------------------
// Xbox One S / Elite 2 / Series controllers over BT Classic
// These are plain HID over Bluetooth, see protocols/xbox_bt.hpp
// ---------------------------------------------------------------------------
class BtXboxHost : public BluetoothHostInterface
{
public:
    BtXboxHost(uint16_t id, uint16_t pid);
    ~BtXboxHost() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeXboxOne; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void update(bool full_poll, bool send_events) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    void set_rumble(uint8_t left, uint8_t right) override { m_rumble.set(left, right); }
    bool has_rumble() const override { return true; }

private:
    XboxBtState m_state = {};
    XboxBtRumble m_rumble;
    uint8_t m_out_buf[XBOX_BT_RUMBLE_LEN] = {};
};

// ---------------------------------------------------------------------------
// Xbox One S / Elite 2 / Series controllers over BLE, see protocols/xbox_bt.hpp
// ---------------------------------------------------------------------------
class BleXboxHost : public BluetoothHostInterface
{
public:
    BleXboxHost(uint16_t id, uint16_t pid) : BluetoothHostInterface(id)
    {
        m_subtype = SubType_Gamepad;
        m_pid = pid;
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
    XboxBtState m_state = {};
    XboxBtRumble m_rumble;
    uint8_t m_out_buf[XBOX_BT_RUMBLE_LEN] = {};
};
