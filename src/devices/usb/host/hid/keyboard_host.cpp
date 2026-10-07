#include "tusb_option.h"
#include "devices/usb/host/hid/keyboard_mouse_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"


std::shared_ptr<UsbHostInterface> KeyboardHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD)
    {
        auto intf = std::make_shared<KeyboardHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
        uint8_t endpoints = itf_desc->bNumEndpoints;
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
        USB_FreeReportInfo(info);
        return intf;
    }
    return nullptr;
}

bool KeyboardHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes >= sizeof(m_keys))
        {
            memcpy(m_keys, m_ep_in_buf, sizeof(m_keys));
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool KeyboardHost::key_pressed(uint8_t keycode)
{
    // modifiers (left ctrl 0xE0 to right gui 0xE7) are bits in the first byte
    if (keycode >= 0xE0 && keycode <= 0xE7)
    {
        return m_keys[0] & (1 << (keycode - 0xE0));
    }
    for (uint8_t i = 2; i < sizeof(m_keys); i++)
    {
        if (m_keys[i] == keycode)
        {
            return true;
        }
    }
    return false;
}

bool KeyboardHost::set_config()
{
    UsbHostInterface::set_config();
    // Boot protocol keeps the report a fixed 8 bytes, whatever the keyboard's own layout is
    tusb_control_request_t set_protocol = {
        .bmRequestType = 0x21,
        .bRequest = HID_REQ_CONTROL_SET_PROTOCOL,
        .wValue = HID_PROTOCOL_BOOT,
        .wIndex = m_interface,
        .wLength = 0};
    send_ctrl_xfer(set_protocol, nullptr, nullptr);
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}
// Inputs picked in the config tool as a USB button carry the key as the type
bool KeyboardHost::tick_digital(proto_Output& type)
{
    return type.which_mapping == proto_Output_keycode_tag && key_pressed(type.mapping.keycode);
}
uint16_t KeyboardHost::tick_analog(proto_Output& type)
{
    return tick_digital(type) ? UINT16_MAX : 0;
}