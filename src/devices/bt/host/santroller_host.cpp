#include "devices/bt/host/santroller_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"
#include "ble/gatt-service/hids_host.h"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/hid.hpp"
#include "protocols/xinput.hpp"
#include "protocols/santroller_v1.hpp"
#include "protocols/santroller_v2.hpp"

// ============================================================================
// BleSantrollerHost (Santroller 1 & 2 BLE Host)
// ============================================================================

BleSantrollerHost::BleSantrollerHost(uint16_t id, bool is_v2, uint16_t version, SubType known_subtype)
    : BluetoothHostInterface(id), m_is_v2(is_v2)
{
    m_output.is_v2 = is_v2;
    if (m_is_v2)
    {
        if (known_subtype != SubType_Unknown)
        {
            m_subtype = known_subtype;
            m_ready = true;
        }
        else
        {
            m_subtype = SubType_Unknown;
            m_ready = false;
        }
    }
    else
    {
        // Santroller 1: SubType is encoded in version high byte: (version >> 8) & 0xFF
        uint8_t st = (version >> 8) & 0xFF;
        if (known_subtype != SubType_Unknown)
        {
            m_subtype = known_subtype;
        }
        else if (st != 0)
        {
            m_subtype = (SubType)st;
        }
        else
        {
            m_subtype = SubType_Gamepad;
        }
        m_ready = true;
    }
}

void BleSantrollerHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    m_output.mark_all_dirty();
    if (m_is_v2)
    {
        if (m_subtype != SubType_Unknown)
        {
            set_ready(true);
        }
        // Query Santroller capabilities (Report 0x10) over GATT. Even with the subtype
        // already known from pairing, the capability bits decide which outputs to send.
        hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
        static const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
        hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
    }
    else
    {
        set_ready(true);
    }
}

void BleSantrollerHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (m_is_v2)
    {
        handle_report_v2(data, len);
    }
    else
    {
        handle_report_v1(data, len);
    }
}

void BleSantrollerHost::handle_report_v2(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;

    if (data[0] == ReportIdSantrollerCapabilities)
    {
        if (len >= 2)
        {
            m_subtype = (SubType)data[1];
            if (len >= 3)
            {
                m_capabilities = data[2];
            }
            m_output.subtype = m_subtype;
            m_output.capabilities = m_capabilities;
            m_output.mark_all_dirty();
            if (!m_ready)
            {
                set_ready(true);
            }
        }
        return;
    }

    if (data[0] != 1) return;

    if (!m_ready)
    {
        if (++m_query_attempts % 30 == 1)
        {
            hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
            static const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
            hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
        }
        return;
    }

    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);
    santroller_v2_normalize_report(m_report_buf, copy_len, m_subtype);
}

void BleSantrollerHost::handle_report_v1(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);
}

void BleSantrollerHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (!m_ready || !m_cid)
        return;
    sync_output();
    // one pending rumble / LED / stage kit command per pass. A HOGP report write carries
    // the report id in the characteristic, so the payload starts at the command byte.
    uint8_t buf[8];
    uint8_t len = m_output.peek(buf, sizeof(buf));
    if (!len)
        return;
    uint8_t status;
    {
        BtStackLock lock;
        status = hids_host_send_write_report(m_cid, ReportIdGamepad, HID_REPORT_TYPE_OUTPUT, buf + 1, len - 1);
    }
    if (status == ERROR_CODE_SUCCESS)
        m_output.commit(buf[1]);
}

bool BleSantrollerHost::tick_digital(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_digital_impl(m_report_buf, m_subtype, type);
    }
    return santroller_v1_tick_digital(m_report_buf, m_subtype, type);
}

uint16_t BleSantrollerHost::tick_analog(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_analog_impl(m_report_buf, m_subtype, type);
    }
    return santroller_v1_tick_analog(m_report_buf, m_subtype, type);
}

uint16_t BleSantrollerHost::tick_button_pressure(proto_Output &type)
{
    if (m_is_v2)
        return xinput_tick_button_pressure_impl(m_report_buf, m_subtype, type);
    return tick_digital(type) ? UINT16_MAX : 0;
}
