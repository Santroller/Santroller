#include "tusb_option.h"
#include "usb/host/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"
#include "hidparser.h"

std::shared_ptr<UsbHostInterface> Ps5Host::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool isThirdParty = info->foundPS5Usage;
    bool isFirstParty = vid == SONY_VID && (pid == PS5_DS_PID || pid == PS5_DS_EDGE_PID);
    uint8_t data[48];
    tusb_control_request_t setup_input_caps = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : 0x0303,
        wIndex : itf_desc->bInterfaceNumber,
        wLength : sizeof(data)
    };
    if (isFirstParty || isThirdParty)
    {
        auto intf = std::make_shared<Ps5Host>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
        intf->m_third_party = isThirdParty;
        if (isThirdParty)
        {
            // request capabilities for 3rd party gamepad
            auto size = intf->send_ctrl_xfer(setup_input_caps, data, nullptr);
            if (size == sizeof(data) && data[2] == 0x27)
            {
                uint8_t capabilities = data[4];
                uint8_t device_type = data[5];
                switch (device_type)
                {
                case 0x00:
                    intf->m_subtype = Gamepad;
                    break;
                case 0x01:
                    intf->m_subtype = RockBandGuitar;
                    break;
                case 0x02:
                    intf->m_subtype = RockBandDrums;
                    break;
                case 0x04:
                    intf->m_subtype = Dancepad;
                    break;
                case 0x06:
                    intf->m_subtype = Wheel;
                    break;
                case 0x07:
                    intf->m_subtype = FightStick;
                    break;
                case 0x08:
                    intf->m_subtype = FlightStick;
                    break;
                default:
                    intf->m_subtype = Gamepad;
                    break;
                }
                if (capabilities & 0x02)
                {
                    intf->m_sensors_supported = true;
                }
                if (capabilities & 0x04)
                {
                    intf->m_lightbar_supported = true;
                }
                if (capabilities & 0x08)
                {
                    intf->m_vibration_supported = true;
                }
                if (capabilities & 0x40)
                {
                    intf->m_touchpad_supported = true;
                }
            }
        }
        else
        {
            // everything supported on first party controller
            intf->m_sensors_supported = true;
            intf->m_lightbar_supported = true;
            intf->m_vibration_supported = true;
            intf->m_touchpad_supported = true;
        }
        p_desc = tu_desc_next(p_desc);
        tusb_hid_descriptor_hid_t *x_desc =
            (tusb_hid_descriptor_hid_t *)p_desc;
        TU_ASSERT(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
        uint8_t endpoints = itf_desc->bNumEndpoints;
        while (endpoints--)
        {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            printf("%02x, %02x\r\n", TUSB_DESC_ENDPOINT, desc_ep->bDescriptorType);
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
            list->host_devices_by_endpoint[intf->m_ep_out] = intf;
        }
        if (intf->m_ep_in)
        {
            list->host_devices_by_endpoint[intf->m_ep_in] = intf;
        }
        assignable_usb_devices.push_back(intf);
        return intf;
    }
    return nullptr;
}

bool Ps5Host::set_config()
{
    return true;
}

bool Ps5Host::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool Ps5Host::tick_digital(UsbButtonType type)
{
    switch (m_subtype)
    {
    case RockBandGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonRed:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonYellow:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonBlue:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonOrange:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonBack:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonStrumUp:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->dpadUp;
        case UsbButtonStrumDown:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    case LiveGuitar:
        switch (type)
        {
        case UsbButtonBlack1:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonBlack2:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonBlack3:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonWhite1:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonWhite2:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonWhite3:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonStrumUp:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->strumBar == 0x00;
        case UsbButtonStrumDown:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->strumBar == 0xFF;
        case UsbButtonDpadUp:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadDown:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        case UsbButtonDpadLeft:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    default:
        switch (type)
        {
        case UsbButtonA:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->a;
        case UsbButtonB:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->b;
        case UsbButtonX:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->x;
        case UsbButtonY:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->y;
        case UsbButtonLeftShoulder:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonRightShoulder:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->start;
        case UsbButtonLeftThumbClick:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->leftThumbClick;
        case UsbButtonRightThumbClick:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->rightThumbClick;
        case UsbButtonGuide:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonDpadUp:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->dpadUp;
        case UsbButtonDpadDown:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    }

    return false;
}
uint16_t Ps5Host::tick_analog(UsbAxisType type)
{
    switch (m_subtype)
    {
    case LiveGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->whammy << 8;
        case UsbAxisTilt:
            return ((PS5GHLGuitar_Data_t *)m_ep_in_buf)->tilt << 2;
        }
    case RockBandGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->whammy << 8;
        case UsbAxisTilt:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->tilt << 8;
        case UsbAxisPickup:
            return ((PS5RockBandGuitar_Data_t *)m_ep_in_buf)->pickup << 8;
        default:
            return 0;
        }
        break;
    default:
        switch (type)
        {
        case UsbAxisLeftTrigger:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->leftTrigger << 8;
        case UsbAxisRightTrigger:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->rightTrigger << 8;
        case UsbAxisLeftStickX:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->leftStickX << 8;
        case UsbAxisLeftStickY:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->leftStickY << 8;
        case UsbAxisRightStickX:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->rightStickX << 8;
        case UsbAxisRightStickY:
            return ((PS5Gamepad_Data_t *)m_ep_in_buf)->rightStickY << 8;
        default:
            return 0;
        }
    }

    return 0;
}