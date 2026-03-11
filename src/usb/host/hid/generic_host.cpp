#include "tusb_option.h"
#include "usb/host/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"
#include "hidparser.h"
std::shared_ptr<UsbHostInterface> GenericHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE)
    {
        return nullptr;
    }
    auto intf = std::make_shared<GenericHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, info);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc =
        (tusb_hid_descriptor_hid_t *)p_desc;
    TU_ASSERT(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep), nullptr);
            usbh_edpt_xfer(dev_addr, intf->m_ep_in, intf->m_ep_in_buf, intf->m_ep_in_size);
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep), nullptr);
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
    assignable_usb_devices.push_back(intf);
    return intf;
}

GenericHost::~GenericHost()
{
    USB_FreeReportInfo(m_info);
}

bool GenericHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        fill_generic_report(m_info, m_ep_in_buf, &m_data);
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool GenericHost::set_config()
{
    return true;
}
bool GenericHost::tick_digital(UsbButtonType type)
{
    switch (type)
    {
    case UsbButtonX:
        return m_data.x;
    case UsbButtonA:
        return m_data.a;
    case UsbButtonB:
        return m_data.b;
    case UsbButtonY:
        return m_data.y;
    case UsbButtonLeftShoulder:
        return m_data.leftShoulder;
    case UsbButtonRightShoulder:
        return m_data.rightShoulder;
    case UsbButtonBack:
        return m_data.back;
    case UsbButtonStart:
        return m_data.start;
    case UsbButtonLeftThumbClick:
        return m_data.leftThumbClick;
    case UsbButtonRightThumbClick:
        return m_data.rightThumbClick;
    case UsbButtonGuide:
        return m_data.guide;
    case UsbButtonCapture:
        return m_data.capture;
    case UsbButtonDpadUp:
        return m_data.dpadUp;
    case UsbButtonDpadDown:
        return m_data.dpadDown;
    case UsbButtonDpadLeft:
        return m_data.dpadLeft;
    case UsbButtonDpadRight:
        return m_data.dpadRight;
    case UsbButtonGenericButton1:
        return m_data.genericButton1;
    case UsbButtonGenericButton2:
        return m_data.genericButton2;
    case UsbButtonGenericButton3:
        return m_data.genericButton3;
    case UsbButtonGenericButton4:
        return m_data.genericButton4;
    case UsbButtonGenericButton5:
        return m_data.genericButton5;
    case UsbButtonGenericButton6:
        return m_data.genericButton6;
    case UsbButtonGenericButton7:
        return m_data.genericButton7;
    case UsbButtonGenericButton8:
        return m_data.genericButton8;
    case UsbButtonGenericButton9:
        return m_data.genericButton9;
    case UsbButtonGenericButton10:
        return m_data.genericButton10;
    case UsbButtonGenericButton11:
        return m_data.genericButton11;
    case UsbButtonGenericButton12:
        return m_data.genericButton12;
    case UsbButtonGenericButton13:
        return m_data.genericButton13;
    case UsbButtonGenericButton14:
        return m_data.genericButton14;
    case UsbButtonGenericButton15:
        return m_data.genericButton15;
    case UsbButtonGenericButton16:
        return m_data.genericButton16;
    default:
        return false;
    }
    return false;
}
uint16_t GenericHost::tick_analog(UsbAxisType type)
{
    switch (type)
    {
    case UsbAxisLeftTrigger:
        return m_data.leftTrigger;
    case UsbAxisRightTrigger:
        return m_data.rightTrigger;
    case UsbAxisLeftStickX:
        return m_data.leftStickX;
    case UsbAxisLeftStickY:
        return m_data.leftStickY;
    case UsbAxisRightStickX:
        return m_data.rightStickX;
    case UsbAxisRightStickY:
        return m_data.rightStickY;
    case UsbAxisGenericAxisX:
        return m_data.genericAxisX;
    case UsbAxisGenericAxisY:
        return m_data.genericAxisY;
    case UsbAxisGenericAxisZ:
        return m_data.genericAxisZ;
    case UsbAxisGenericAxisRx:
        return m_data.genericAxisRx;
    case UsbAxisGenericAxisRy:
        return m_data.genericAxisRy;
    case UsbAxisGenericAxisRz:
        return m_data.genericAxisRz;
    case UsbAxisGenericAxisSlider:
        return m_data.genericAxisSlider;
    default:
        return 0;
    }
    return 0;
}
