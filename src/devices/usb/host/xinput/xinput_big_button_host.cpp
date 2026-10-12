#include "tusb_option.h"
#include "devices/usb/host/xinput_host.h"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/xinput.hpp"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"


std::shared_ptr<UsbHostInterface> XInputBigButtonHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass && desc_itf->bInterfaceSubClass == 0x5D &&
                  desc_itf->bInterfaceProtocol == 0x04,
              nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    auto intf = std::make_shared<XInputBigButtonHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
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
            intf->m_ep_in_size = TU_MIN(desc_ep->wMaxPacketSize, sizeof(intf->m_ep_in_buf));
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
    *out_len = (uint16_t)((uintptr_t)p_desc - (uintptr_t)desc_itf);
    return intf;
}

bool XInputBigButtonHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes >= sizeof(XInputBigButton_Data_t))
        {
            auto report = (const XInputBigButton_Data_t *)m_ep_in_buf;
            // The buttons are laid out like a normal XInput report, just after the controller number
            if (report->controllerNumber < 4)
            {
                m_buttons[report->controllerNumber] = m_ep_in_buf[3] | (m_ep_in_buf[4] << 8);
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

// All four Big Button pads are combined, A / B / Y / X / LB double as the frets like the old firmware
bool XInputBigButtonHost::tick_digital(proto_Output& type)
{
    XInputGamepad_Data_t report = {};
    uint16_t buttons = m_buttons[0] | m_buttons[1] | m_buttons[2] | m_buttons[3];
    memcpy(((uint8_t *)&report) + 2, &buttons, sizeof(buttons));
    return xinput_tick_digital_impl((const uint8_t *)&report, Gamepad, type);
}
uint16_t XInputBigButtonHost::tick_analog(proto_Output& type)
{
    return 0;
}
bool XInputBigButtonHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}