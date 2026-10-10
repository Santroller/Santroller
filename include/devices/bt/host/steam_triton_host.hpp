#pragma once
#include "devices/bt/bt_host.hpp"
#include "protocols/steam_triton.hpp"

// ---------------------------------------------------------------------------
// 2026 Steam Controller (Triton) BLE Host
// Works over HID over GATT (reports carry their ID) or directly on the Valve
// GATT service (m_input_handle set, notifications carry no report ID).
// ---------------------------------------------------------------------------
class BleSteamTritonHost : public BluetoothHostInterface
{
public:
    BleSteamTritonHost(uint16_t id);
    ~BleSteamTritonHost() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGeneric; }

    void on_connected() override;
    void handle_report(const uint8_t *data, uint16_t len) override;
    void update(bool full_poll, bool send_events) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    // Direct GATT only
    uint16_t m_con_handle = 0;
    uint16_t m_input_handle = 0;
    uint16_t m_report_char_handle = 0;

private:
    void disable_lizard_mode();

    SteamTritonState m_state = {};
    uint32_t m_last_lizard_update = 0;
};
