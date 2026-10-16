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
        // Santroller 1: SubType is encoded in the version's high byte. Prefer it over the
        // subtype remembered from pairing, as the device may have been reconfigured since.
        if (version != 0 || known_subtype == SubType_Unknown)
        {
            m_subtype = santroller_v1_subtype(version);
        }
        else
        {
            m_subtype = known_subtype;
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
        // The capabilities are queried from update / update_pending, as the HIDS client is
        // usually still reading the HID information here and would reject the query
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
            const SubType previous = m_subtype;
            m_subtype = (SubType)data[1];
            if (len >= 3)
            {
                m_capabilities = data[2];
            }
            m_caps_received = true;
            m_output.subtype = m_subtype;
            m_output.capabilities = m_capabilities;
            m_output.mark_all_dirty();
            if (!m_ready)
            {
                set_ready(true);
            }
            else if (m_subtype != previous)
            {
                // Connected with the subtype remembered from pairing, which is out of date
                bt_host_subtype_changed(this);
            }
        }
        return;
    }

    if (data[0] != 1 || !m_ready) return;

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
    // Still asked for after the readiness timeout registers it as Unknown, so the reply
    // can fix the subtype (and the saved pairing) later on
    query_capabilities();
    if (!m_ready || !m_cid || m_write_pending)
        return;
    sync_output();
    // one pending rumble / LED / stage kit command per pass. A HOGP report write carries
    // the report id in the characteristic, so the payload starts at the command byte.
    uint8_t len = m_output.peek(m_out_buf, sizeof(m_out_buf));
    if (!len)
        return;
    uint8_t status;
    {
        BtStackLock lock;
        status = hids_host_send_write_report(m_cid, ReportIdGamepad, HID_REPORT_TYPE_OUTPUT, m_out_buf + 1, len - 1);
        if (status == ERROR_CODE_SUCCESS)
            m_write_pending = true;
    }
    if (status == ERROR_CODE_SUCCESS)
        m_output.commit(m_out_buf[1]);
}

void BleSantrollerHost::query_capabilities()
{
    // Until the capabilities report (0x10) arrives. Even with the subtype already known from
    // pairing, the capability bits decide which outputs to send. This runs on a timer rather
    // than per input report, as an idle device (drums) sends none.
    if (!m_is_v2 || m_caps_received || !m_cid)
        return;
    uint32_t now = millis();
    if (m_query_attempts && now - m_last_query_ms < 250)
        return;
    uint8_t status;
    {
        BtStackLock lock;
        // A read is answered directly. Some hosts' stacks can't read input reports, so every
        // other attempt writes the query instead, which the device answers with a notification.
        if (m_query_attempts % 2 == 0)
        {
            status = hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
        }
        else
        {
            static const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
            status = hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
        }
    }
    // busy with another query: try again on the next pass
    if (status != ERROR_CODE_SUCCESS)
        return;
    m_query_attempts++;
    m_last_query_ms = now;
}

bool BleSantrollerHost::tick_digital(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_digital_impl(m_report_buf, m_subtype, type, false, true);
    }
    return santroller_v1_tick_digital(m_report_buf, m_subtype, type);
}

uint16_t BleSantrollerHost::tick_analog(proto_Output &type)
{
    if (m_is_v2)
    {
        return santroller_v2_fix_analog(type, xinput_tick_analog_impl(m_report_buf, m_subtype, type, true), m_capabilities);
    }
    return santroller_v1_tick_analog(m_report_buf, m_subtype, type);
}

uint16_t BleSantrollerHost::tick_button_pressure(proto_Output &type)
{
    if (m_is_v2)
        return xinput_tick_button_pressure_impl(m_report_buf, m_subtype, type, false, true);
    return santroller_v1_tick_button_pressure(m_report_buf, m_subtype, type);
}
