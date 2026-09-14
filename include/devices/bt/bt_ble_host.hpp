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
// Factory — given VID/PID from DIS PnP characteristic, create the right host.
// Called by ble_rx after GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID.
// ---------------------------------------------------------------------------
std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         SubType known_subtype = SubType_Unknown);

