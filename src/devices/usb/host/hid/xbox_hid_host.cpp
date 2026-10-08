#include "tusb_option.h"
#include "devices/usb/host/hid/xbox_hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "utils.h"

// Xbox One / Series controllers that enumerate as a HID device instead of using GIP
static bool is_xbox_hid(uint16_t vid, uint16_t pid)
{
    return is_xbox_hid_controller(vid, pid) || (vid == XBOX_VID && pid >= 0x02e0 && pid <= 0x02ef);
}

std::shared_ptr<UsbHostInterface> XboxHidHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE || !is_xbox_hid(vid, pid))
    {
        return nullptr;
    }
    auto intf = std::make_shared<XboxHidHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
    intf->m_pid = pid;
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc =
        (tusb_hid_descriptor_hid_t *)p_desc;
    TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
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
    intf->m_desc.init(info, pid);
    usb_host_add_assignable_interface(intf);
    return intf;
}

bool XboxHidHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr == m_ep_out)
    {
        m_out_result = result;
        m_out_done.store(true, std::memory_order_release);
    }
    if (ep_addr & 0x80)
    {
        xbox_hid_parse_report(m_ep_in_buf, xferred_bytes, m_desc, m_state);
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XboxHidHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void XboxHidHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (m_out_done.exchange(false, std::memory_order_acquire))
    {
        m_out_pending = false;
        if (m_out_result != XFER_RESULT_SUCCESS)
        {
            m_rumble.dirty = true;
            printf("Xbox HID output failed: dev=%u ep=%u result=%u\r\n", m_dev_addr, m_ep_out, unsigned(m_out_result));
        }
    }

    uint32_t now = millis();
    if (m_out_pending || (m_ep_out && usbh_edpt_busy(m_dev_addr, m_ep_out)) || !m_rumble.should_send(now))
        return;
    if (!send_rumble())
    {
        if (!m_out_submit_failed)
            printf("Xbox HID output submission failed: dev=%u ep=%u\r\n", m_dev_addr, m_ep_out);
        m_out_submit_failed = true;
        return;
    }
    m_out_submit_failed = false;
    m_rumble.sent(now);
    m_out_pending = m_ep_out != 0;
}

bool XboxHidHost::send_rumble()
{
    m_ep_out_buf[0] = XBOX_HID_RUMBLE_REPORT_ID;
    xbox_hid_build_rumble(m_rumble.left, m_rumble.right, m_ep_out_buf + 1);
    if (m_ep_out)
    {
        return send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(m_ep_out_buf));
    }
    return set_report(XBOX_HID_RUMBLE_REPORT_ID, HID_REPORT_TYPE_OUTPUT,
                      m_ep_out_buf, sizeof(m_ep_out_buf)) == sizeof(m_ep_out_buf);
}

bool XboxHidHost::tick_digital(proto_Output &type)
{
    return xbox_hid_tick_digital(m_state, type);
}

uint16_t XboxHidHost::tick_analog(proto_Output &type)
{
    return xbox_hid_tick_analog(m_state, type);
}
