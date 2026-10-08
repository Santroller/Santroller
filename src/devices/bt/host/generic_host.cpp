#include "devices/bt/host/generic_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "devices/usb/host/generic_hid_tick_helpers.h"

// ============================================================================
// BtGenericHost
// ============================================================================

BtGenericHost::~BtGenericHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
}

void BtGenericHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    fill_generic_report(m_info, m_report_buf, &m_data);
}

bool BtGenericHost::tick_digital(proto_Output &type)
{
    return generic_hid_tick_digital_impl(m_data, type);
}

uint16_t BtGenericHost::tick_analog(proto_Output &type)
{
    return generic_hid_tick_analog_impl(m_data, type);
}

// ============================================================================
// BleGenericHost (HID-over-GATT / HIDS fallback)
// ============================================================================

BleGenericHost::~BleGenericHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
}

void BleGenericHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    fill_generic_report(m_info, m_report_buf, &m_data);
}

bool BleGenericHost::tick_digital(proto_Output &type)
{
    return generic_hid_tick_digital_impl(m_data, type);
}

uint16_t BleGenericHost::tick_analog(proto_Output &type)
{
    return generic_hid_tick_analog_impl(m_data, type);
}
