#pragma once
#include <cstring>
#include "emulation/usb/usb_devices.h"
#include "devices/bt/bt_host.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/ps5.hpp"
#include "protocols/switch.hpp"
#include "protocols/switch2.hpp"
#include "protocols/xbox_one.hpp"
#include "wii_extension_decoder.hpp"
#include "hidparser.h"

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

extern "C" {
#include "gip_device.h"
#include "gip_report_queue.h"
#include "gip_button_mapping.h"
}

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
    }
    ~BtDs3Host() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypePS3; }

    // on_connected() sends the DS3 enable sequence via HID set-report
    void on_connected() override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
};

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
        if (m_third_party)
        {
            m_ready = false;
        }
    }
    ~BtDs4Host() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypePS4; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void handle_feature_report(const uint8_t *data, uint16_t len) override;
    void handle_feature_report_failed() override;
    void request_capabilities() override;
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
    uint16_t m_vid;
    uint16_t m_pid;
};

// ---------------------------------------------------------------------------
// DS5 / DualSense over BT Classic
// BT Classic wire format uses report-id 0x31; we strip the 2-byte header so
// m_report_buf aligns with USB 0x01 report (PS5Gamepad_Data_t).
// ---------------------------------------------------------------------------
class BtDs5Host : public BluetoothHostInterface
{
public:
    BtDs5Host(uint16_t id, SubType subtype, bool third_party,
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
        if (m_third_party)
        {
            m_ready = false;
        }
    }
    ~BtDs5Host() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypePS5; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void handle_feature_report(const uint8_t *data, uint16_t len) override;
    void handle_feature_report_failed() override;
    void request_capabilities() override;
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
// Xbox One / GIP over BT Classic
// Uses the same gip_device_t state machine as XboxOneHost (USB).
// ---------------------------------------------------------------------------
class BtXboxOneHost : public BluetoothHostInterface
{
public:
    BtXboxOneHost(uint16_t id);
    ~BtXboxOneHost();

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeXboxOne; }

    void handle_report(const uint8_t *data, uint16_t len) override;
    void on_connected() override;
    void on_disconnected() override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    void update(bool full_poll, bool send_events) override;

    // Public so C callbacks can access
    gip_device_t m_gip_device;
    gip_report_queue_t *m_report_queue;

private:
    void send_hid_output(const uint8_t *data, uint16_t len);
    uint8_t m_out_buf[64] = {};
};

// ---------------------------------------------------------------------------
// Generic HID fallback for BT Classic devices not matched by VID/PID
// ---------------------------------------------------------------------------
class BtGenericHost : public BluetoothHostInterface
{
public:
    BtGenericHost(uint16_t id, HID_ReportInfo_t *info)
        : BluetoothHostInterface(id), m_info(info)
    {
        m_subtype = SubType_Gamepad;
        if (!m_info)
        {
            m_ready = false;
        }
    }
    ~BtGenericHost();

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGeneric; }

    void set_report_info(HID_ReportInfo_t *info)
    {
        if (m_info && m_info != info)
        {
            USB_FreeReportInfo(m_info);
        }
        m_info = info;
        if (m_info)
        {
            m_ready = true;
        }
    }

    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    HID_ReportInfo_t *m_info;
    USB_Host_Data_t m_data = {};
};

// ---------------------------------------------------------------------------
// Nintendo Wii Remote & Wii U Pro Controller over BT Classic
// Supports Wiimote core buttons + extensions (Nunchuk, Classic, Guitar, Drums,
// Turntable, Taiko, uDraw, Drawsome) shared with I2C via WiiExtensionDecoder.
// ---------------------------------------------------------------------------
class BtWiiHost : public BluetoothHostInterface
{
public:
    BtWiiHost(uint16_t id, bool is_pro_controller = false);
    ~BtWiiHost() {}

    BtControllerType controller_type() const override
    {
        return m_is_pro ? BtControllerType_BtControllerTypeWiiUPro : BtControllerType_BtControllerTypeWii;
    }

    void on_connected() override;
    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

    bool is_wii_extension(WiiExtType type) override
    {
        return m_decoder.mType == type;
    }

    void set_rumble(uint8_t left, uint8_t right) override;
    void set_player_led(uint8_t player) override;
    bool has_rumble() const override { return true; }
    bool has_player_led() const override { return true; }

    WiiExtensionDecoder m_decoder;

private:
    void send_status_request();
    void send_init_extension();
    void send_disable_encryption();
    void send_read_extension_id();
    void send_report_mode(uint8_t mode);
    void send_player_led(uint8_t led);
    void send_feedback();

    bool m_is_pro;
    uint8_t m_player = 0;
    bool m_rumble = false;
    uint8_t m_wii_buttons[2] = {};
    uint8_t m_fsm_state = 0;
    bool m_has_ext = false;
    bool m_led_sent = false;
    uint8_t m_cmd_buf[24] = {};
};

// ---------------------------------------------------------------------------
// Factory — given VID/PID/subtype info, create the right host object.
// Called by bt_classic_rx after SDP query completes.
// ---------------------------------------------------------------------------
std::shared_ptr<BluetoothHostInterface> bt_classic_create_host(uint16_t vid, uint16_t pid,
                                                                uint16_t version,
                                                                uint16_t device_id,
                                                                HID_ReportInfo_t *info,
                                                                SubType known_subtype = SubType_Unknown,
                                                                bool known_ready = false,
                                                                const char *dev_name = nullptr,
                                                                BtControllerType known_controller_type = BtControllerType_BtControllerTypeGeneric);

