#pragma once
#include "devices/bt/bt_host.hpp"
#include "hidparser.h"

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

private:
    void handle_report_v1(const uint8_t *data, uint16_t len);
    void handle_report_v2(const uint8_t *data, uint16_t len);
    bool tick_digital_v1(proto_Output &type);
    uint16_t tick_analog_v1(proto_Output &type);

    bool m_is_v2 = false;
    uint8_t m_capabilities = 0;
    uint8_t m_query_attempts = 0;
};

// ---------------------------------------------------------------------------
// Factory — given VID/PID from DIS PnP characteristic, create the right host.
// Called by ble_rx after GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID.
// ---------------------------------------------------------------------------
std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         const uint8_t *desc = nullptr,
                                                         uint16_t desc_len = 0,
                                                         SubType known_subtype = SubType_Unknown);

