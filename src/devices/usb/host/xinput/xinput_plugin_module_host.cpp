#include "tusb_option.h"
#include "devices/usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "devices/usb.hpp"
#include <cstring>

std::shared_ptr<UsbHostInterface> XInputModuleHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass && desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x02, nullptr);

    uint8_t dev_addr = list->dev_addr();
    // the controller is always the first interface
    auto intf = std::make_shared<XInputModuleHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id, list->host_devices_by_itf[0]);
    uint16_t len = desc_itf->bLength;
    uint8_t const *p_desc = tu_desc_next(desc_itf);
    // The endpoint comes after a vendor descriptor
    while (len < max_len && tu_desc_type(p_desc) != TUSB_DESC_INTERFACE)
    {
        if (tu_desc_type(p_desc) == TUSB_DESC_ENDPOINT)
        {
            tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
            if ((desc_ep->bEndpointAddress & 0x80) && !intf->m_ep_in)
            {
                intf->m_ep_in = desc_ep->bEndpointAddress;
                intf->m_ep_in_size = std::min<uint16_t>(tu_edpt_packet_size(desc_ep), sizeof(intf->m_ep_in_buf));
                TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
            }
        }
        len += tu_desc_len(p_desc);
        p_desc = tu_desc_next(p_desc);
    }
    TU_VERIFY(intf->m_ep_in, nullptr);
    list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    // gets updates to keep the chatpad going, and becomes assignable once it's used
    usb_host_add_enumerating_interface(intf);
    *out_len = len;
    return intf;
}

bool XInputModuleHost::set_config()
{
    tusb_desc_device_t desc;
    if (tuh_descriptor_get_device_local(m_dev_addr, &desc))
    {
        m_bcd_device = desc.bcdDevice;
    }
    XInputChatpadHost::set_config();
    m_configured = true;
    return true;
}

bool XInputModuleHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr == m_ep_in)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            handle_keys(m_ep_in_buf, xferred_bytes);
        }
        m_reading = result != XFER_RESULT_FAILED && usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void XInputModuleHost::send_command(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint8_t *data, uint16_t len)
{
    tusb_control_request_t setup = {};
    setup.bmRequestType = request_type;
    setup.bRequest = request;
    setup.wValue = value;
    setup.wIndex = index;
    setup.wLength = len;
    send_ctrl_xfer(setup, data, nullptr);
}

void XInputModuleHost::update(bool full_poll, bool send_events)
{
    XInputChatpadHost::update(full_poll, send_events);
    // Like xboxdrv, only firmware known to take the init sequence gets it
    if (!m_configured || !controller_takes_chatpad() || (m_bcd_device != 0x0110 && m_bcd_device != 0x0114))
    {
        return;
    }
    if (!m_initialised)
    {
        m_initialised = true;
        const uint8_t code[2] = {static_cast<uint8_t>(m_bcd_device == 0x0114 ? 0x09 : 0x01),
                                 static_cast<uint8_t>(m_bcd_device == 0x0114 ? 0x00 : 0x02)};
        // xboxdrv's init sequence, which often fails partway without it mattering
        send_command(0x40, 0xA9, 0xA30C, 0x4423, nullptr, 0);
        send_command(0x40, 0xA9, 0x2344, 0x7F03, nullptr, 0);
        send_command(0x40, 0xA9, 0x5839, 0x6832, nullptr, 0);
        memcpy(m_ctrl_buf, code, sizeof(code));
        send_command(0xC0, 0xA1, 0x0000, 0xE416, m_ctrl_buf, sizeof(m_ctrl_buf));
        memcpy(m_ctrl_buf, code, sizeof(code));
        send_command(0x40, 0xA1, 0x0000, 0xE416, m_ctrl_buf, sizeof(m_ctrl_buf));
        memcpy(m_ctrl_buf, code, sizeof(code));
        send_command(0xC0, 0xA1, 0x0000, 0xE416, m_ctrl_buf, sizeof(m_ctrl_buf));
        restart(1000, 2);
    }
    auto command = next_command();
    if (!command)
    {
        return;
    }
    send_command(0x41, 0x00, *command, m_interface, nullptr, 0);
    // It only starts reporting once it's enabled
    if (*command == chatpad_enable && !m_reading)
    {
        m_reading = usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
}
