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
// Reports start out as simple 0x3F reports, then switch to 0x30 full input reports (same layout as USB)
// once the report mode subcommand has been sent.
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
    void send_init_packets() override;
    void handle_report(const uint8_t *data, uint16_t len) override;
    void update(bool full_poll, bool send_events) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    bool nintendo_face_layout() const override { return true; }

    // Switch 2 controllers use a different output protocol
    void set_rumble(uint8_t left, uint8_t right) override { m_output.set_rumble(left, right); }
    void set_player_led(uint8_t player) override { m_output.set_player(player); }
    bool has_rumble() const override { return !m_is_switch2; }
    bool has_motion() const override { return m_is_switch2 ? m_switch2_state.has_imu : m_imu.has_sample; }
    bool has_player_led() const override { return !m_is_switch2; }

private:
    static constexpr uint32_t SWITCH_MODE_RETRY_MS = 500;
    // between the subcommands sent after the report mode, so each gets its reply before the next
    static constexpr uint32_t SWITCH_SUBCOMMAND_INTERVAL_MS = 50;
    bool m_is_switch2 = false;
    uint32_t m_mode_sent_at = 0;
    // Set once full reports arrive, so no output goes out ahead of the report mode command
    bool m_full_reports = false;
    SwitchOutput m_output;
    uint8_t m_out_buf[SWITCH_OUTPUT_MAX_LEN] = {};
    Switch2ControllerState m_switch2_state = {};
    SwitchImuState m_imu;
    uint8_t m_imu_init_step = 0;
    uint32_t m_imu_init_at = 0;
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
    bool nintendo_face_layout() const override { return true; }
    bool has_motion() const override { return m_state.has_imu; }

private:
    Switch2ControllerState m_state = {};
};
