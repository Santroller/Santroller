#pragma once
#include "protocols/santroller_output.hpp"
#include "devices/bt/bt_host.hpp"
#include "hidparser.h"
#include "protocols/steam_controller.hpp"
#include "protocols/switch2.hpp"

// ---------------------------------------------------------------------------
// Generic HID-over-GATT fallback for BLE devices not matched by PnP VID/PID
// ---------------------------------------------------------------------------
class BleGenericHost : public BluetoothHostInterface
{
public:
    BleGenericHost(uint16_t id, HID_ReportInfo_t *info)
        : BluetoothHostInterface(id), m_info(info)
    {
        m_subtype = SubType_Gamepad;
    }
    ~BleGenericHost();

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGeneric; }

    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;

private:
    HID_ReportInfo_t *m_info;
    USB_Host_Data_t m_data = {};
};

// ---------------------------------------------------------------------------
// Guitar Hero Live iOS BLE Guitar
// Direct GATT characteristic notifications (UUID: 533e1524-3abe-f33f-cd00-594e8b0a8ea3)
// ---------------------------------------------------------------------------
class BleGhlIosHost : public BluetoothHostInterface
{
public:
    BleGhlIosHost(uint16_t id) : BluetoothHostInterface(id)
    {
        m_subtype = LiveGuitar;
    }
    ~BleGhlIosHost() {}

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeGhlIos; }

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
};

// ---------------------------------------------------------------------------
// Santroller 1 & 2 BLE Host
// ---------------------------------------------------------------------------
class BleSantrollerHost : public BluetoothHostInterface
{
public:
    BleSantrollerHost(uint16_t id, bool is_v2, uint16_t version, SubType known_subtype = SubType_Unknown);
    ~BleSantrollerHost() override = default;

    BtControllerType controller_type() const override { return BtControllerType_BtControllerTypeSantroller; }

    void on_connected() override;
    void handle_report(const uint8_t *data, uint16_t len) override;

    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    uint16_t tick_button_pressure(proto_Output &type) override;

    void update(bool full_poll, bool send_events) override;
    void set_rumble(uint8_t left, uint8_t right) override { sync_output(); m_output.set_rumble(left, right); }
    void set_player_led(uint8_t player) override { m_output.set_player(player); }
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b) override { m_output.set_rgb(r, g, b); }
    void set_euphoria_led(bool state) override { sync_output(); m_output.set_euphoria(state); }
    void set_stagekit_led(uint8_t param, uint8_t command) override { sync_output(); m_output.set_stagekit(param, command); }
    bool has_rumble() const override { return m_output.has_rumble(); }
    bool has_player_led() const override { return m_output.supports(CapabilityHasStandardPlayerLeds); }
    bool has_lightbar() const override { return m_output.supports(CapabilityHasRGBIndicatorLed); }
    bool has_euphoria_led() const override { return m_output.has_euphoria(); }
    bool has_stagekit_led() const override { return m_output.has_stagekit(); }

private:
    void sync_output() { m_output.subtype = m_subtype; }
    SantrollerOutputState m_output;
    void handle_report_v1(const uint8_t *data, uint16_t len);
    void handle_report_v2(const uint8_t *data, uint16_t len);

    bool m_is_v2 = false;
    uint8_t m_capabilities = 0;
    uint8_t m_query_attempts = 0;
};

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

// ---------------------------------------------------------------------------
// Factory — given VID/PID from DIS PnP characteristic, create the right host.
// Called by ble_rx after GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID.
// ---------------------------------------------------------------------------
std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         SubType known_subtype = SubType_Unknown);
