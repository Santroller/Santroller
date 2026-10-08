#pragma once
#include "devices/bt/bt_host.hpp"
#include "hidparser.h"

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
