#include "tusb_option.h"
#include "devices/usb/host/hid/keyboard_mouse_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

std::shared_ptr<KeyboardHost> KeyboardHost::open_common(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc)
{
    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<KeyboardHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
    intf->m_boot_subclass = itf_desc->bInterfaceSubClass == HID_SUBCLASS_BOOT;
    bool keyboard = intf->m_decoder.parse_report_descriptor(HidHost::s_report_desc, HidHost::s_report_desc_len);
    if (itf_desc->bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD)
    {
        if (!intf->m_decoder.field_count())
        {
            // couldn't make sense of the descriptor, so fall back to boot protocol
            intf->m_boot_protocol = true;
            intf->m_decoder.use_boot_layout();
        }
    }
    else if (!keyboard)
    {
        return nullptr;
    }

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
    // Keyboards with NKRO often send keys on a second interface instead of the boot one, and media keys
    // usually get their own interface, so only the first keyboard interface is assignable and it
    // reports the keys from all of them
    auto primary = list->keyboard.lock();
    if (primary)
    {
        primary->m_others.push_back(intf);
    }
    else
    {
        list->keyboard = intf;
        usb_host_add_assignable_interface(intf);
    }
    return intf;
}

std::shared_ptr<UsbHostInterface> KeyboardHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_KEYBOARD)
    {
        return nullptr;
    }
    auto intf = open_common(list, itf_desc);
    if (intf)
    {
        USB_FreeReportInfo(info);
    }
    return intf;
}

std::shared_ptr<UsbHostInterface> KeyboardHost::open_report(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE)
    {
        return nullptr;
    }
    auto intf = open_common(list, itf_desc);
    if (intf)
    {
        USB_FreeReportInfo(info);
    }
    return intf;
}

bool KeyboardHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            m_decoder.handle_report(m_ep_in_buf, xferred_bytes);
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool KeyboardHost::key_pressed(uint8_t keycode)
{
    if (m_decoder.key_pressed(keycode))
    {
        return true;
    }
    for (auto &weak : m_others)
    {
        auto other = weak.lock();
        if (other && other->m_decoder.key_pressed(keycode))
        {
            return true;
        }
    }
    return false;
}

bool KeyboardHost::consumer_pressed(uint16_t usage)
{
    if (m_decoder.consumer_pressed(usage))
    {
        return true;
    }
    for (auto &weak : m_others)
    {
        auto other = weak.lock();
        if (other && other->m_decoder.consumer_pressed(usage))
        {
            return true;
        }
    }
    return false;
}

bool KeyboardHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_boot_protocol || m_boot_subclass)
    {
        // Only boot interfaces support picking the protocol. Report protocol is the default, but a
        // previous host may have left it in boot protocol
        tusb_control_request_t set_protocol = {
            .bmRequestType = 0x21,
            .bRequest = HID_REQ_CONTROL_SET_PROTOCOL,
            .wValue = (uint16_t)(m_boot_protocol ? HID_PROTOCOL_BOOT : HID_PROTOCOL_REPORT),
            .wIndex = m_interface,
            .wLength = 0};
        send_ctrl_xfer(set_protocol, nullptr, nullptr);
    }
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}
// Inputs picked in the config tool as a USB button carry the key as the type
bool KeyboardHost::tick_digital(proto_Output& type)
{
    if (type.which_mapping == proto_Output_consumerKey_tag)
    {
        return consumer_pressed(type.mapping.consumerKey);
    }
    return type.which_mapping == proto_Output_keycode_tag && key_pressed(type.mapping.keycode);
}
uint16_t KeyboardHost::tick_analog(proto_Output& type)
{
    return tick_digital(type) ? UINT16_MAX : 0;
}
