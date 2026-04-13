#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"

std::shared_ptr<UsbHostInterface> XInputWirelessAudioHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass && desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x82, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XInputWirelessAudioHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    p_desc = tu_desc_next(p_desc);
    XBOX_ID_DESCRIPTOR *x_desc =
        (XBOX_ID_DESCRIPTOR *)p_desc;
    TU_VERIFY(XINPUT_DESC_TYPE_WIRELESS_CAPABILITIES == x_desc->bDescriptorType, nullptr);
    intf->m_subtype = get_subtype_from_xinput(x_desc->subtype);
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
    *out_len = TUD_XINPUT_WIRELESS_AUDIO_DESC_LEN;
    // TODO: audio
    // assignable_usb_devices.push_back(intf);
    return intf;
}

bool XInputWirelessAudioHost::set_config()
{
    UsbHostInterface::set_config();
    XInputInputCapabilities_t caps;
    XInputVibrationCapabilities_t caps_vibr;
    uint32_t serial;

    tusb_control_request_t setup_input_caps = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : INPUT_CAPABILITIES_WVALUE,
        wIndex : m_interface,
        wLength : sizeof(XInputInputCapabilities_t)
    };
    tusb_control_request_t setup_vibration_caps = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : VIBRATION_CAPABILITIES_WVALUE,
        wIndex : m_interface,
        wLength : sizeof(XInputVibrationCapabilities_t)
    };
    tusb_control_request_t setup_serial = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_DEVICE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : SERIAL_NUMBER_WVALUE,
        wIndex : m_interface,
        wLength : sizeof(serial)
    };
    // request serial, some controllers might expect this
    send_ctrl_xfer(setup_serial, &serial, nullptr);
    // request input capabilities, lets us differenciate controllers
    if (send_ctrl_xfer(setup_input_caps, &caps, nullptr) != 0)
    {
        // GHL guitars set no navigation
        if (m_subtype == GuitarHeroGuitar && caps.flags & XINPUT_FLAGS_NO_NAV)
        {
            m_subtype = LiveGuitar;
        }
        // GH drums don't set force feedback
        if (m_subtype == RockBandDrums && (caps.flags & XINPUT_FLAGS_FORCE_FEEDBACK) == 0)
        {
            m_subtype = GuitarHeroDrums;
        }
        // Pro guitars we have to identify by vid+pid
        if (caps.leftThumbX == HARMONIX_VID)
        {
            switch (caps.leftThumbY)
            {
            case XBOX_360_MUSTANG_PID:
            case XBOX_360_MPA_MUSTANG_PID:
                m_subtype = ProGuitarMustang;
                break;
            case XBOX_360_MPA_SQUIRE_PID:
            case XBOX_360_SQUIRE_PID:
                m_subtype = ProGuitarSquire;
                break;
            }
        }
    }
    // request vibration caps since some devices expect it
    send_ctrl_xfer(setup_vibration_caps, &caps_vibr, nullptr);
    return true;
}

bool XInputWirelessAudioHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputWirelessAudioHost::tick_digital(UsbButtonType type)
{
    return false;
}
uint16_t XInputWirelessAudioHost::tick_analog(UsbAxisType type)
{
    return 0;
}