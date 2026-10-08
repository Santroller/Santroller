#pragma once
#include "devices/bt/bt_host.hpp"
#include "emulation/usb/usb_devices.h"
#include "protocols/switch.hpp"
#include "protocols/switch2.hpp"

static inline bool is_switch_name(const char *name)
{
    if (!name || !name[0]) return false;
    return strstr(name, "Pro Controller") != nullptr ||
           strstr(name, "Lic Pro Controller") != nullptr ||
           strstr(name, "Nintendo Wireless Gamepad") != nullptr ||
           strstr(name, "Wireless Gamepad") != nullptr ||
           strstr(name, "Joy-Con") != nullptr ||
           strstr(name, "Switch") != nullptr ||
           strstr(name, "SNES Controller") != nullptr ||
           strstr(name, "NES Controller") != nullptr ||
           strstr(name, "N64 Controller") != nullptr ||
           strstr(name, "SEGA Controller") != nullptr ||
           strstr(name, "MD/Gen Control") != nullptr;
}

static inline uint16_t switch_pid_from_name(const char *name)
{
    if (!name) return SWITCH_PRO_PID;
    if (strstr(name, "Joy-Con (L)")) return SWITCH_JOYCON_L_PID;
    if (strstr(name, "Joy-Con (R)")) return SWITCH_JOYCON_R_PID;
    if (strstr(name, "SNES Controller")) return SWITCH_ONLINE_SNES_PID;
    if (strstr(name, "NES Controller")) return SWITCH_ONLINE_NES_PID;
    if (strstr(name, "N64 Controller")) return SWITCH_ONLINE_N64_PID;
    if (strstr(name, "SEGA Controller") || strstr(name, "MD/Gen Control")) return SWITCH_ONLINE_SEGA_PID;
    return SWITCH_PRO_PID;
}

// ---------------------------------------------------------------------------
// Switch Pro Controller over BT Classic
// Reports come in as 0x30 full input reports (same layout as USB).
// ---------------------------------------------------------------------------
class BtSwitchHost : public BluetoothHostInterface
{
public:
    BtSwitchHost(uint16_t id, bool is_switch2 = false)
        : BluetoothHostInterface(id), m_is_switch2(is_switch2)
    {
        m_subtype = SubType_Gamepad;
    }
    ~BtSwitchHost() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeSwitch; }

    void on_connected() override;
    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    bool m_is_switch2 = false;
    Switch2ControllerState m_switch2_state = {};
};

// ---------------------------------------------------------------------------
// Switch 2 BLE Host
// ---------------------------------------------------------------------------
class BleSwitch2Host : public BluetoothHostInterface
{
public:
    BleSwitch2Host(uint16_t id);
    ~BleSwitch2Host() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeSwitch; }

    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    Switch2ControllerState m_state = {};
};
