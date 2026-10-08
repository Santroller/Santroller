#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/steam_controller.hpp"

// ---------------------------------------------------------------------------
// Valve Steam Controller BLE Host
// ---------------------------------------------------------------------------
class BleSteamHost : public BluetoothHostInterface
{
public:
    BleSteamHost(uint16_t id);
    ~BleSteamHost() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGeneric; }

    void on_connected() override;
    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    uint16_t m_char_handle = 0;
    uint16_t m_con_handle = 0;

private:
    SteamControllerState m_state = {};
};
