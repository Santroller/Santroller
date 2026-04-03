#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "protocols/xinput.hpp"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "devices/usb.hpp"
#include "config.hpp"
#include "utils.h"
#include <algorithm>
static const uint8_t capabilitiesRequest[] = {0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_prescence[] = {0x08, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
std::shared_ptr<UsbHostInterface> XInputWirelessGamepadHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x81)
    {
        auto intf = std::make_shared<XInputWirelessGamepadHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_DESC_TYPE_WIRELESS_CAPABILITIES == x_desc->bDescriptorType, nullptr);
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

        *out_len = TUD_XINPUT_WIRELESS_CONTROLLER_DESC_LEN;
        printf("found device: %d\r\n", intf->m_subtype);
        return intf;
    }
    return nullptr;
}

bool XInputWirelessGamepadHost::set_config()
{
    send_intr_xfer(m_ep_out, xbox360w_prescence, sizeof(xbox360w_prescence));
    send_intr_xfer(m_ep_out, xbox360w_prescence, sizeof(xbox360w_prescence));
    return true;
}

bool XInputWirelessGamepadHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        XBOX_WIRELESS_HEADER *header = (XBOX_WIRELESS_HEADER *)m_ep_in_buf;
        if (header->id == 0x08)
        {
            // Disconnected
            if (header->type == 0x00)
            {
                if (m_found)
                {
                    printf("Disconnected %02x %02x\r\n", m_dev_addr, m_interface);
                    assignable_usb_devices.erase(std::remove_if(assignable_usb_devices.begin(), assignable_usb_devices.end(), [this](std::shared_ptr<UsbHostInterface> intf)
                                                                { return intf.get() == this; }));
                    m_found = false;
                    reload();
                }
            }
        }
        else if (header->id == 0x00)
        {
            // Gamepad inputs
            if (header->type == 0x01 || header->type == 0x03)
            {
                memcpy(m_report_buf, m_ep_in_buf + sizeof(header), xferred_bytes - sizeof(header));
            }
            // Link report
            if (header->type == 0x0f)
            {
                XBOX_WIRELESS_LINK_REPORT *linkReport = (XBOX_WIRELESS_LINK_REPORT *)m_ep_in_buf;
                if (linkReport->always_0xCC == 0xCC && !m_found)
                {
                    m_subtype = get_subtype_from_xinput(linkReport->subtype & ~0x80);
                    printf("Found subtype: %02x %02x %02x\r\n", m_subtype, m_dev_addr, m_interface);
                    send_intr_xfer(m_ep_out, capabilitiesRequest, sizeof(capabilitiesRequest));
                    m_check_caps = millis() + 1000;
                    m_found = true;
                    assignable_usb_devices.push_back(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                    reload();
                }
            }
            // Capabilities report
            if (header->type == 0x05)
            {
                XBOX_WIRELESS_CAPABILITIES *caps = (XBOX_WIRELESS_CAPABILITIES *)m_ep_in_buf;
                if (caps->always_0x12 == 0x12)
                {
                    printf("Found capabilities: %02x %02x\r\n", m_dev_addr, m_interface);
                    if (caps->leftStickX == 0xFFC0 && caps->rightStickX == 0xFFC0)
                    {
                        // usb_host_devices[i].type.sub_type = XINPUT_GUITAR_WT;
                        printf("Found wt\r\n");
                    }
                    m_check_caps = 0;
                }
            }
        }
        if (!m_found && millis() > m_check_link)
        {
            send_intr_xfer(m_ep_out, xbox360w_prescence, sizeof(xbox360w_prescence));
            m_check_link = millis() + 1000;
        }
        if (m_check_caps && millis() > m_check_caps)
        {
            send_intr_xfer(m_ep_out, capabilitiesRequest, sizeof(capabilitiesRequest));
            m_check_caps = millis() + 1000;
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputWirelessGamepadHost::tick_digital(UsbButtonType type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->a;
        case UsbButtonRed:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->b;
        case UsbButtonYellow:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->y;
        case UsbButtonBlue:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->x;
        case UsbButtonOrange:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->leftShoulder;
        case UsbButtonBack:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->back;
        case UsbButtonStart:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->start;
        case UsbButtonGuide:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->guide;
        case UsbButtonStrumUp:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->dpadUp;
        case UsbButtonStrumDown:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    case RockBandGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->a;
        case UsbButtonRed:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->b;
        case UsbButtonYellow:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->y;
        case UsbButtonBlue:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->x;
        case UsbButtonOrange:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->leftShoulder;
        case UsbButtonBack:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->back;
        case UsbButtonStart:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->start;
        case UsbButtonGuide:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->guide;
        case UsbButtonStrumUp:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->dpadUp;
        case UsbButtonStrumDown:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    case LiveGuitar:
        switch (type)
        {
        case UsbButtonBlack1:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->a;
        case UsbButtonBlack2:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->b;
        case UsbButtonBlack3:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->y;
        case UsbButtonWhite1:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->x;
        case UsbButtonWhite2:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->leftShoulder;
        case UsbButtonWhite3:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->rightShoulder;
        case UsbButtonBack:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->back;
        case UsbButtonStart:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->start;
        case UsbButtonGuide:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->guide;
        case UsbButtonStrumUp:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->strumBar == 0x00;
        case UsbButtonStrumDown:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->strumBar == 0xFF;
        case UsbButtonDpadUp:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->dpadLeft;
        case UsbButtonDpadDown:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->dpadRight;
        case UsbButtonDpadLeft:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    default:
        switch (type)
        {
        case UsbButtonA:
            return ((XInputGamepad_Data_t *)m_report_buf)->a;
        case UsbButtonB:
            return ((XInputGamepad_Data_t *)m_report_buf)->b;
        case UsbButtonX:
            return ((XInputGamepad_Data_t *)m_report_buf)->x;
        case UsbButtonY:
            return ((XInputGamepad_Data_t *)m_report_buf)->y;
        case UsbButtonLeftShoulder:
            return ((XInputGamepad_Data_t *)m_report_buf)->leftShoulder;
        case UsbButtonRightShoulder:
            return ((XInputGamepad_Data_t *)m_report_buf)->rightShoulder;
        case UsbButtonBack:
            return ((XInputGamepad_Data_t *)m_report_buf)->back;
        case UsbButtonStart:
            return ((XInputGamepad_Data_t *)m_report_buf)->start;
        case UsbButtonLeftThumbClick:
            return ((XInputGamepad_Data_t *)m_report_buf)->leftThumbClick;
        case UsbButtonRightThumbClick:
            return ((XInputGamepad_Data_t *)m_report_buf)->rightThumbClick;
        case UsbButtonGuide:
            return ((XInputGamepad_Data_t *)m_report_buf)->guide;
        case UsbButtonDpadUp:
            return ((XInputGamepad_Data_t *)m_report_buf)->dpadUp;
        case UsbButtonDpadDown:
            return ((XInputGamepad_Data_t *)m_report_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XInputGamepad_Data_t *)m_report_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputGamepad_Data_t *)m_report_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    }

    return false;
}
uint16_t XInputWirelessGamepadHost::tick_analog(UsbAxisType type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->whammy + INT16_MAX;
        case UsbAxisTilt:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->tilt + INT16_MAX;
        case UsbAxisSlider:
            return ((XInputGuitarHeroGuitar_Data_t *)m_report_buf)->slider + INT16_MAX;
        default:
            return 0;
        }
        break;
    case LiveGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->whammy + INT16_MAX;
        case UsbAxisTilt:
            return ((XInputGHLGuitar_Data_t *)m_report_buf)->tilt + INT16_MAX;
        default:
            return 0;
        }
        break;
    case RockBandGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->whammy + INT16_MAX;
        case UsbAxisTilt:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->tilt + INT16_MAX;
        case UsbAxisPickup:
            return ((XInputRockBandGuitar_Data_t *)m_report_buf)->pickup + INT16_MAX;
        default:
            return 0;
        }
    default:
        switch (type)
        {
        case UsbAxisLeftTrigger:
            return ((XInputGamepad_Data_t *)m_report_buf)->leftTrigger << 8;
        case UsbAxisRightTrigger:
            return ((XInputGamepad_Data_t *)m_report_buf)->rightTrigger << 8;
        case UsbAxisLeftStickX:
            return ((XInputGamepad_Data_t *)m_report_buf)->leftStickX + INT16_MAX;
        case UsbAxisLeftStickY:
            return ((XInputGamepad_Data_t *)m_report_buf)->leftStickY + INT16_MAX;
        case UsbAxisRightStickX:
            return ((XInputGamepad_Data_t *)m_report_buf)->rightStickX + INT16_MAX;
        case UsbAxisRightStickY:
            return ((XInputGamepad_Data_t *)m_report_buf)->rightStickY + INT16_MAX;
        default:
            return 0;
        }
    }

    return 0;
}