#include "tusb_option.h"
#include "devices/usb/host/hid/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"


// Mice send reports only while something changes, so movement older than this has stopped
#define MOUSE_IDLE_US 20000
// Movement per report is small, scale it up into an axis (128 counts / 4 scroll notches = full)
#define MOUSE_MOVE_SCALE 256
#define MOUSE_SCROLL_SCALE 8192

MouseHost::MouseHost(uint8_t dev_addr, uint8_t interface, uint16_t id, HID_ReportInfo_t *info) : HidHost(dev_addr, interface, id), m_info(info)
{
    m_subtype = KeyboardMouse;
    find_items();
}

MouseHost::~MouseHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
    }
}

void MouseHost::disconnect()
{
    memset(m_axis_items, 0, sizeof(m_axis_items));
    memset(m_button_items, 0, sizeof(m_button_items));
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
    HidHost::disconnect();
}

void MouseHost::find_items()
{
    if (!m_info)
    {
        return;
    }
    for (HID_ReportItem_t *item = m_info->FirstReportItem; item; item = item->Next)
    {
        const auto &usage = item->Attributes.Usage;
        if (usage.Page == HID_USAGE_PAGE_DESKTOP)
        {
            // in MouseAxisType order: x, y, horizontal scroll, vertical scroll
            switch (usage.Usage)
            {
            case HID_USAGE_DESKTOP_X:
                m_axis_items[0] = item;
                break;
            case HID_USAGE_DESKTOP_Y:
                m_axis_items[1] = item;
                break;
            case HID_USAGE_DESKTOP_WHEEL:
                m_axis_items[3] = item;
                break;
            }
        }
        else if (usage.Page == HID_USAGE_PAGE_CONSUMER && usage.Usage == HID_USAGE_CONSUMER_AC_PAN)
        {
            m_axis_items[2] = item;
        }
        else if (usage.Page == HID_USAGE_PAGE_BUTTON && usage.Usage >= 1 && usage.Usage <= 3)
        {
            // HID buttons 1 / 2 / 3 are left / right / middle
            m_button_items[usage.Usage - 1] = item;
        }
    }
}

// Read an item out of a report, which starts with its report id when the device uses them
static bool read_item(const uint8_t *report, HID_ReportItem_t *item)
{
    if (item->ReportID)
    {
        if (item->ReportID != report[0])
        {
            return false;
        }
        report++;
    }
    return USB_GetHIDReportItemInfo(item->ReportID, report, item);
}

static int32_t signed_value(const HID_ReportItem_t *item)
{
    uint8_t bits = item->Attributes.BitSize;
    int32_t value = item->Value;
    // relative axes have a negative logical minimum, so sign extend them
    if ((int32_t)item->Attributes.Logical.Minimum < 0 && bits < 32 && (value & (1 << (bits - 1))))
    {
        value -= 1 << bits;
    }
    return value;
}

std::shared_ptr<UsbHostInterface> MouseHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceProtocol == HID_ITF_PROTOCOL_MOUSE)
    {
        auto intf = std::make_shared<MouseHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, info);
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
        return intf;
    }
    return nullptr;
}

bool MouseHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool MouseHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes)
        {
            bool updated = false;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (m_axis_items[i] && read_item(m_ep_in_buf, m_axis_items[i]))
                {
                    m_movement[i] = signed_value(m_axis_items[i]);
                    updated = true;
                }
            }
            for (uint8_t i = 0; i < 3; i++)
            {
                if (m_button_items[i] && read_item(m_ep_in_buf, m_button_items[i]))
                {
                    m_buttons = m_button_items[i]->Value ? m_buttons | (1 << i) : m_buttons & ~(1 << i);
                    updated = true;
                }
            }
            if (updated)
            {
                m_last_report_us = time_us_32();
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool MouseHost::mouse_button(MouseButtonType button)
{
    switch (button)
    {
    case Mouse_Left:
        return m_buttons & 0x01;
    case Mouse_Right:
        return m_buttons & 0x02;
    case Mouse_Middle:
        return m_buttons & 0x04;
    }
    return false;
}

uint16_t MouseHost::mouse_axis(MouseAxisType axis)
{
    const int32_t center = UINT16_MAX / 2;
    uint8_t index = axis - Mouse_MoveX;
    if (index >= 4 || time_us_32() - m_last_report_us > MOUSE_IDLE_US)
    {
        return center;
    }
    int32_t value = center + m_movement[index] * (index < 2 ? MOUSE_MOVE_SCALE : MOUSE_SCROLL_SCALE);
    return value < 0 ? 0 : value > UINT16_MAX ? UINT16_MAX : value;
}

// Inputs picked in the config tool as a USB button / axis carry a mouse output as the type
bool MouseHost::tick_digital(proto_Output& type)
{
    if (type.which_mapping == proto_Output_mouseButton_tag)
    {
        return mouse_button(type.mapping.mouseButton);
    }
    if (type.which_mapping == proto_Output_mouseAxis_tag)
    {
        return mouse_axis(type.mapping.mouseAxis) != UINT16_MAX / 2;
    }
    return false;
}
uint16_t MouseHost::tick_analog(proto_Output& type)
{
    if (type.which_mapping == proto_Output_mouseAxis_tag)
    {
        return mouse_axis(type.mapping.mouseAxis);
    }
    return tick_digital(type) ? UINT16_MAX : 0;
}