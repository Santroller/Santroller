#include "tusb_option.h"
#include "devices/usb/host/xinput_host.h"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/xinput.hpp"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"
#include <algorithm>

static const uint8_t capabilitiesRequest[] = {0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_prescence[] = {0x08, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const char xinput_wireless_gamepad_name[] = "X360 Wireless 0";
static const char xinput_wireless_gamepad_disconnected_name[] = "X360 Wireless Receiver Slot 0";
XInputWirelessGamepadHost::XInputWirelessGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {
    m_delayed_init = true;
}
std::shared_ptr<UsbHostInterface> XInputWirelessGamepadHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass && desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x81, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XInputWirelessGamepadHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    p_desc = tu_desc_next(p_desc);
    XBOX_ID_DESCRIPTOR *x_desc =
        (XBOX_ID_DESCRIPTOR *)p_desc;
    TU_VERIFY(XINPUT_DESC_TYPE_WIRELESS_CAPABILITIES == x_desc->bDescriptorType, nullptr);
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
    usb_host_add_enumerating_interface(intf);

    *out_len = TUD_XINPUT_WIRELESS_CONTROLLER_DESC_LEN;
    printf("found device: %d\r\n", intf->m_subtype);
    return intf;
}

bool XInputWirelessGamepadHost::set_config()
{
    m_has_name = true;
    for (size_t i = 0; i < sizeof(xinput_wireless_gamepad_disconnected_name); i++)
    {
        // skip header
        m_name[(i + 1) * 2] = xinput_wireless_gamepad_disconnected_name[i];
    }
    m_name[(sizeof(xinput_wireless_gamepad_disconnected_name) - 1) * 2] = '1' + (m_ep_out / 2);
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    memcpy(m_ep_out_buf, xbox360w_prescence, sizeof(xbox360w_prescence));
    send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(xbox360w_prescence));
    return true;
}

bool XInputWirelessGamepadHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes >= 2)
        {
            XBOX_WIRELESS_HEADER *header = (XBOX_WIRELESS_HEADER *)m_ep_in_buf;
            if (header->id == 0x08)
            {
                // Disconnected
                if (header->type == 0x00 || !(header->type & 0x80))
                {
                    if (m_found)
                    {
                        printf("Disconnected %02x %02x\r\n", m_dev_addr, m_interface);
                        usb_host_remove_assignable_interface(this);
                        usb_host_add_enumerating_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                        for (size_t i = 0; i < sizeof(xinput_wireless_gamepad_disconnected_name); i++)
                        {
                            // skip header
                            m_name[(i + 1) * 2] = xinput_wireless_gamepad_disconnected_name[i];
                        }
                        m_name[(sizeof(xinput_wireless_gamepad_disconnected_name) - 1) * 2] = '1' + (m_ep_out / 2);
                        m_found = false;
                        m_check_link = millis() + 4000;
                        process_delayed_init();
                    }
                }
            }
            else if (header->id == 0x00)
            {
                // Gamepad inputs
                if (header->type == 0x01 || header->type == 0x03)
                {
                    m_check_caps = 0;
                    memcpy(m_report_buf, m_ep_in_buf + sizeof(header), xferred_bytes - sizeof(header));
                    if (!m_led_set)
                    {
                        set_player_led(m_last_player_led);
                    }
                }
                // Link report
                if (header->type == 0x0f)
                {
                    XBOX_WIRELESS_LINK_REPORT *linkReport = (XBOX_WIRELESS_LINK_REPORT *)m_ep_in_buf;
                    if (linkReport->always_0xCC == 0xCC && !m_found)
                    {
                        m_subtype = get_subtype_from_xinput(linkReport->subtype & ~0x80);
                        printf("Found subtype: %02x %02x %02x\r\n", m_subtype, m_dev_addr, m_interface);
                        for (size_t i = 0; i < sizeof(xinput_wireless_gamepad_name); i++)
                        {
                            // skip header
                            m_name[(i + 1) * 2] = xinput_wireless_gamepad_name[i];
                        }
                        m_name[(sizeof(xinput_wireless_gamepad_name) - 1) * 2] = '0' + m_subtype;
                        memcpy(m_ep_out_buf, capabilitiesRequest, sizeof(capabilitiesRequest));
                        send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(capabilitiesRequest));
                        m_check_caps = millis() + 1000;
                        m_caps_retries = 0;
                        m_found = true;
                        m_led_set = false;
                        usb_host_remove_enumerating_interface(this);
                        usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                        process_delayed_init();
                        set_player_led(m_last_player_led);
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
                            m_wt = true;
                            printf("Found wt\r\n");
                        }
                        m_check_caps = 0;
                        m_caps_retries = 0;
                        if (!m_led_set)
                        {
                            set_player_led(m_last_player_led);
                        }
                    }
                }
            }
        }
        if (!m_found && millis() > m_check_link)
        {
            memcpy(m_ep_out_buf, xbox360w_prescence, sizeof(xbox360w_prescence));
            send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(xbox360w_prescence));
            m_check_link = millis() + 1000;
        }
        if (m_check_caps && millis() > m_check_caps)
        {
            if (m_caps_retries < 2)
            {
                m_caps_retries++;
                memcpy(m_ep_out_buf, capabilitiesRequest, sizeof(capabilitiesRequest));
                send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(capabilitiesRequest));
                m_check_caps = millis() + 1000;
            }
            else
            {
                m_check_caps = 0;
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputWirelessGamepadHost::tick_digital(proto_Output &type)
{
    return xinput_tick_digital_impl(m_report_buf, m_subtype, type, m_wt);
}
uint16_t XInputWirelessGamepadHost::tick_analog(proto_Output &type)
{
    return xinput_tick_analog_impl(m_report_buf, m_subtype, type);
}

void XInputWirelessGamepadHost::set_rumble(uint8_t left, uint8_t right)
{
    if (!m_ep_out || !m_found) return;
    uint8_t buf[12] = {0x00, 0x01, 0x0f, 0xc0, 0x00, left, right, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(m_ep_out_buf, buf, sizeof(buf));
    send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(buf));
}

void XInputWirelessGamepadHost::set_player_led(uint8_t player)
{
    m_last_player_led = player;
    if (!m_ep_out || !m_found) return;
    uint8_t target_player = player;
    if (target_player == 0)
    {
        target_player = (m_ep_out / 2) + 1;
    }
    uint8_t led_code = 0;
    if (target_player == 1) led_code = 6;
    else if (target_player == 2) led_code = 7;
    else if (target_player == 3) led_code = 8;
    else if (target_player == 4) led_code = 9;
    if (led_code == 0) return;
    uint8_t buf[12] = {0x00, 0x00, 0x08, (uint8_t)(0x40 + led_code), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(m_ep_out_buf, buf, sizeof(buf));
    if (send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(buf)))
    {
        m_led_set = true;
    }
}