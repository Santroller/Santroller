#include "tusb_option.h"
#include "devices/usb/host/hid/generic_hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "devices/usb/host/generic_hid_tick_helpers.h"

std::shared_ptr<UsbHostInterface> GenericHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE || !info)
    {
        return nullptr;
    }
    auto intf = std::make_shared<GenericHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, info);
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
    usb_host_add_assignable_interface(intf);
    return intf;
}

GenericHost::~GenericHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
    }
}

void GenericHost::disconnect()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
    HidHost::disconnect();
}

bool GenericHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            fill_generic_report(m_info, m_ep_in_buf, &m_data);
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool GenericHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}
bool GenericHost::tick_digital(proto_Output &type)
{
    return generic_hid_tick_digital_impl(m_data, type);
}

uint16_t GenericHost::tick_analog(proto_Output &type)
{
    return generic_hid_tick_analog_impl(m_data, type);
}
