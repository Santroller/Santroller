#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "config.hpp"

std::shared_ptr<UsbHostInterface> XInputGamepadHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x01)
    {
        auto intf = std::make_shared<XInputGamepadHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, nullptr);
        uint8_t endpoints = desc_itf->bNumEndpoints;
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
                intf->m_subtype = get_subtype_from_xinput(x_desc->subtype);
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
            list->host_devices_by_endpoint[intf->m_ep_out] = intf;
        }
        if (intf->m_ep_in)
        {
            list->host_devices_by_endpoint[intf->m_ep_in] = intf;
        }
        assignable_devices.push_back(intf);
        return intf;
    }
    return nullptr;
}

bool XInputGamepadHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputGamepadHost::tick_digital(UsbButtonType type)
{
    switch (type)
    {
    case UsbButtonX:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->x;
    case UsbButtonA:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->a;
    case UsbButtonB:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->b;
    case UsbButtonY:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->y;
    case UsbButtonLeftShoulder:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->leftShoulder;
    case UsbButtonRightShoulder:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->rightShoulder;
    case UsbButtonBack:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->back;
    case UsbButtonStart:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->start;
    case UsbButtonLeftThumbClick:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->leftThumbClick;
    case UsbButtonRightThumbClick:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->rightThumbClick;
    case UsbButtonGuide:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->guide;
    case UsbButtonCapture:
        return false;
    case UsbButtonDpadUp:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->dpadUp;
    case UsbButtonDpadDown:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->dpadDown;
    case UsbButtonDpadLeft:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->dpadLeft;
    case UsbButtonDpadRight:
        return ((XInputGamepad_Data_t*)m_ep_in_buf)->dpadRight;
    case UsbButtonKick1:
        return false;
    case UsbButtonKick2:
        return false;
    case UsbButtonLeftBlue:
        return false;
    case UsbButtonLeftRed:
        return false;
    case UsbButtonLeftGreen:
        return false;
    case UsbButtonRightBlue:
        return false;
    case UsbButtonRightRed:
        return false;
    case UsbButtonRightGreen:
        return false;
    case UsbButtonSoloGreen:
        return false;
    case UsbButtonSoloRed:
        return false;
    case UsbButtonSoloYellow:
        return false;
    case UsbButtonSoloBlue:
        return false;
    case UsbButtonSoloOrange:
        return false;
    case UsbButtonGreen:
        return false;
    case UsbButtonRed:
        return false;
    case UsbButtonYellow:
        return false;
    case UsbButtonBlue:
        return false;
    case UsbButtonOrange:
        return false;
    case UsbButtonGenericButton1:
        return false;
    case UsbButtonGenericButton2:
        return false;
    case UsbButtonGenericButton3:
        return false;
    case UsbButtonGenericButton4:
        return false;
    case UsbButtonGenericButton5:
        return false;
    case UsbButtonGenericButton6:
        return false;
    case UsbButtonGenericButton7:
        return false;
    case UsbButtonGenericButton8:
        return false;
    case UsbButtonGenericButton9:
        return false;
    case UsbButtonGenericButton10:
        return false;
    case UsbButtonGenericButton11:
        return false;
    case UsbButtonGenericButton12:
        return false;
    case UsbButtonGenericButton13:
        return false;
    case UsbButtonGenericButton14:
        return false;
    case UsbButtonGenericButton15:
        return false;
    case UsbButtonGenericButton16:
        return false;
    }
    return true;
}
uint16_t XInputGamepadHost::tick_analog(UsbAxisType type)
{
    return 0;
}