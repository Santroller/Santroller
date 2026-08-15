#include "tusb_option.h"
#include "usb/host/ogxbox_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"

std::shared_ptr<UsbHostInterface> OGXboxHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(desc_itf->bInterfaceClass == 0x58, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<OGXboxHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    intf->m_subtype = SubType_Gamepad;
    uint8_t endpoints = desc_itf->bNumEndpoints;
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
            usbh_edpt_xfer(dev_addr, intf->m_ep_in, intf->m_ep_in_buf, intf->m_ep_in_size);
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
    assignable_usb_devices.push_back(intf);
    *out_len = TUD_OGXBOX_GAMEPAD_DESC_LEN;
    return intf;
}

bool OGXboxHost::set_config()
{
    UsbHostInterface::set_config();
    return true;
}

bool OGXboxHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80 && result != XFER_RESULT_FAILED)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool OGXboxHost::tick_digital(UsbButtonType type)
{
    switch (type)
    {
    case UsbButtonA:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->a;
    case UsbButtonB:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->b;
    case UsbButtonX:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->x;
    case UsbButtonY:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->y;
    case UsbButtonLeftShoulder:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->leftShoulder;
    case UsbButtonRightShoulder:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->rightShoulder;
    case UsbButtonBack:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->back;
    case UsbButtonStart:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->start;
    case UsbButtonLeftThumbClick:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->leftThumbClick;
    case UsbButtonRightThumbClick:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->rightThumbClick;
    case UsbButtonDpadUp:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->dpadUp;
    case UsbButtonDpadDown:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->dpadDown;
    case UsbButtonDpadLeft:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->dpadLeft;
    case UsbButtonDpadRight:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->dpadRight;
    default:
        return false;
    }

    return false;
}
uint16_t OGXboxHost::tick_analog(UsbAxisType type)
{
    switch (type)
    {
    case UsbAxisLeftTrigger:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->leftTrigger << 8;
    case UsbAxisRightTrigger:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->rightTrigger << 8;
    case UsbAxisLeftStickX:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->leftStickX + INT16_MAX;
    case UsbAxisLeftStickY:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->leftStickY + INT16_MAX;
    case UsbAxisRightStickX:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->rightStickX + INT16_MAX;
    case UsbAxisRightStickY:
        return ((OGXboxGamepad_Data_t *)m_ep_in_buf)->rightStickY + INT16_MAX;
    default:
        return 0;
    }

    return 0;
}