#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/stadia.hpp"

// ---------------------------------------------------------------------------
// Google Stadia controller over BLE (after the Bluetooth firmware update), see protocols/stadia.hpp
// ---------------------------------------------------------------------------
class BleStadiaHost : public BluetoothHostInterface
{
public:
    BleStadiaHost(uint16_t id) : BluetoothHostInterface(id)
    {
        m_subtype = SubType_Gamepad;
    }
    ~BleStadiaHost() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGeneric; }

    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    void update(bool full_poll, bool send_events) override;
    void set_rumble(uint8_t left, uint8_t right) override { m_rumble.set(left, right); }
    bool has_rumble() const override { return true; }

private:
    Stadia_Data_t m_data = {};
    StadiaRumble m_rumble;
    uint8_t m_out_buf[STADIA_RUMBLE_LEN] = {};
};
