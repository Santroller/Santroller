#include "tusb_option.h"
#include "devices/usb/host/ogxbox_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"


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
    usb_host_add_assignable_interface(intf);
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

bool OGXboxHost::tick_digital(proto_Output& type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (OGXboxGamepad_Data_t *)m_ep_in_buf;
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:
            return data->a;
        case Gamepad_B:
            return data->b;
        case Gamepad_X:
            return data->x;
        case Gamepad_Y:
            return data->y;
        case Gamepad_LeftShoulder:
            return data->leftShoulder;
        case Gamepad_RightShoulder:
            return data->rightShoulder;
        case Gamepad_Back:
            return data->back;
        case Gamepad_Start:
            return data->start;
        case Gamepad_LeftThumbClick:
            return data->leftThumbClick;
        case Gamepad_RightThumbClick:
            return data->rightThumbClick;
        case Gamepad_DpadUp:
            return data->dpadUp;
        case Gamepad_DpadDown:
            return data->dpadDown;
        case Gamepad_DpadLeft:
            return data->dpadLeft;
        case Gamepad_DpadRight:
            return data->dpadRight;
        default:
            return false;
        }
    }

    return false;
}
uint16_t OGXboxHost::tick_analog(proto_Output& type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (OGXboxGamepad_Data_t *)m_ep_in_buf;
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftTrigger:
            return data->leftTrigger << 8;
        case Gamepad_RightTrigger:
            return data->rightTrigger << 8;
        case Gamepad_LeftStickX:
            return data->leftStickX + INT16_MAX;
        case Gamepad_LeftStickY:
            return data->leftStickY + INT16_MAX;
        case Gamepad_RightStickX:
            return data->rightStickX + INT16_MAX;
        case Gamepad_RightStickY:
            return data->rightStickY + INT16_MAX;
        default:
            return 0;
        }
    }

    return 0;
}