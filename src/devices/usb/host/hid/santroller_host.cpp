#include "tusb_option.h"
#include "devices/usb/host/hid/santroller_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "devices/usb.hpp"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/santroller_v1.hpp"
#include "protocols/santroller_v2.hpp"
#include "hidparser.h"
#include <string.h>

static bool has_generic_desktop_input(const HID_ReportInfo_t *info)
{
    for (const HID_ReportItem_t *item = info->FirstReportItem; item; item = item->Next)
    {
        if (item->ItemType == HID_REPORT_ITEM_In && item->Attributes.Usage.Page == 0x01)
            return true;
    }
    return false;
}

std::shared_ptr<UsbHostInterface> SantrollerHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    if (vid != SANTROLLER_VID || pid != SANTROLLER_PID || !info)
    {
        return nullptr;
    }
    // A Santroller 2's gamepad interface carries the capabilities output usage. Without
    // it this is either a Santroller 1, or a Santroller 2's configurator interface (same
    // VID/PID); only the former has Generic Desktop inputs.
    bool is_v2 = info->foundSantrollerV2OutputUsage;
    if (!is_v2 && !has_generic_desktop_input(info))
    {
        return nullptr;
    }
    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<SantrollerHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, is_v2);
    intf->m_output.is_v2 = is_v2;
    if (!is_v2)
    {
        // Santroller 1 encodes its subtype in bcdDevice instead
        uint8_t subtype = (revision >> 8) & 0xFF;
        intf->m_subtype = subtype ? (SubType)subtype : SubType_Gamepad;
        intf->m_output.subtype = intf->m_subtype;
    }
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc = (tusb_hid_descriptor_hid_t *)p_desc;
    TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize > sizeof(intf->m_ep_in_buf) ? sizeof(intf->m_ep_in_buf) : desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
    }
    if (intf->m_ep_out)
    {
        list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
    }
    if (intf->m_ep_in)
    {
        list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    }
    if (is_v2)
    {
        // not assignable until the capabilities report tells us what it is
        usb_host_add_enumerating_interface(intf);
    }
    else
    {
        usb_host_add_assignable_interface(intf);
    }
    USB_FreeReportInfo(info);
    return intf;
}

void SantrollerHost::request_capabilities()
{
    // Any output report with id 0x10 makes the Santroller send its capabilities report
    uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
    set_report(ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
}

bool SantrollerHost::set_config()
{
    UsbHostInterface::set_config();
    m_configured = true;
    m_output.mark_all_dirty();
    if (m_is_v2)
        request_capabilities();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void SantrollerHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (len == 0)
        return;

    if (!m_is_v2)
    {
        if (data[0] != ReportIdGamepad)
            return;
        uint16_t copy_len = len < sizeof(m_report) ? len : sizeof(m_report);
        memcpy(m_report, data, copy_len);
        return;
    }

    if (data[0] == ReportIdSantrollerCapabilities)
    {
        if (len < 2)
            return;
        SubType subtype = (SubType)data[1];
        m_capabilities = len >= 3 ? data[2] : 0;
        bool changed = m_ready && subtype != m_subtype;
        m_subtype = subtype;
        m_output.subtype = subtype;
        m_output.capabilities = m_capabilities;
        m_output.mark_all_dirty();
        if (!m_ready)
        {
            m_ready = true;
            printf("Santroller ready: dev=%u itf=%u subtype=%d\r\n", m_dev_addr, m_interface, (int)m_subtype);
            usb_host_remove_enumerating_interface(this);
            usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
            process_delayed_init();
        }
        else if (changed)
        {
            process_delayed_init();
        }
        return;
    }

    if (data[0] != ReportIdGamepad)
        return;

    if (!m_ready)
    {
        // capabilities request got lost, ask again now and then
        if (++m_query_attempts % 30 == 1)
            request_capabilities();
        return;
    }

    uint16_t copy_len = len < sizeof(m_report) ? len : sizeof(m_report);
    memcpy(m_report, data, copy_len);
    santroller_v2_normalize_report(m_report, copy_len, m_subtype);
}

bool SantrollerHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS)
            handle_report(m_ep_in_buf, xferred_bytes);
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void SantrollerHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (!m_ready || !m_configured)
        return;
    // one pending rumble / LED / stage kit command per pass
    uint8_t buf[8];
    uint8_t len = m_output.peek(buf, sizeof(buf));
    if (len && set_report(ReportIdGamepad, HID_REPORT_TYPE_OUTPUT, buf, len) == len)
        m_output.commit(buf[1]);
}

bool SantrollerHost::tick_digital(proto_Output &type)
{
    if (!m_is_v2)
        return santroller_v1_tick_digital(m_report, m_subtype, type);
    return xinput_tick_digital_impl(m_report, m_subtype, type);
}

uint16_t SantrollerHost::tick_analog(proto_Output &type)
{
    if (!m_is_v2)
        return santroller_v1_tick_analog(m_report, m_subtype, type);
    return xinput_tick_analog_impl(m_report, m_subtype, type);
}

uint16_t SantrollerHost::tick_button_pressure(proto_Output &type)
{
    if (!m_is_v2)
        return santroller_v1_tick_digital(m_report, m_subtype, type) ? UINT16_MAX : 0;
    return xinput_tick_button_pressure_impl(m_report, m_subtype, type);
}
