#include "tusb_option.h"
#include "devices/usb/host/hid/steam_triton_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "hidparser.h"
#include "devices/usb.hpp"
#include "utils.h"

// The wired controller puts its keyboard / mouse (lizard mode) and gamepad reports behind report IDs,
// make sure the interface declares the vendor defined controller state reports
static bool has_state_report(HID_ReportInfo_t *info)
{
    return info && info->foundSteamTritonReport;
}

std::shared_ptr<UsbHostInterface> SteamTritonHost::open(std::shared_ptr<UsbHostDevice> list,
                                                         tusb_desc_interface_t const *itf_desc,
                                                         uint16_t max_len,
                                                         uint16_t vid,
                                                         uint16_t pid,
                                                         uint16_t revision,
                                                         HID_ReportInfo_t *info)
{
    if (vid != VALVE_USB_VID)
        return nullptr;
    if (is_dongle(pid))
    {
        // Interfaces 2..5 are the wireless controller slots
        if (itf_desc->bInterfaceNumber < 2 || itf_desc->bInterfaceNumber > 5)
            return nullptr;
    }
    else if (pid != VALVE_STEAM_TRITON_WIRED_PID || !has_state_report(info))
    {
        return nullptr;
    }

    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<SteamTritonHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, vid, pid);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc = (tusb_hid_descriptor_hid_t *)p_desc;
    if (HID_DESC_TYPE_HID != x_desc->bDescriptorType)
        return nullptr;
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        if (TUSB_DESC_ENDPOINT != desc_ep->bDescriptorType)
            return nullptr;
        // Only the IN endpoint is used, settings go over feature reports
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = tu_min16(desc_ep->wMaxPacketSize, sizeof(intf->m_ep_in_buf));
            if (!tuh_edpt_open(dev_addr, desc_ep))
                return nullptr;
        }
    }
    if (!intf->m_ep_in)
        return nullptr;
    printf("steam_triton_open: opened itf %d successfully (ep_in: %02x)\r\n", itf_desc->bInterfaceNumber, intf->m_ep_in);
    list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    if (intf->m_delayed_init)
        usb_host_add_enumerating_interface(intf);
    else
        usb_host_add_assignable_interface(intf);
    USB_FreeReportInfo(info);
    return intf;
}

bool SteamTritonHost::set_config()
{
    UsbHostInterface::set_config();
    usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    if (!is_dongle(m_pid))
    {
        // Wired controller is always connected
        m_connected = true;
        disable_lizard_mode();
    }
    return true;
}

void SteamTritonHost::disable_lizard_mode()
{
    memset(m_feature_buf, 0, sizeof(m_feature_buf));
    m_feature_buf[0] = TRITON_FEATURE_REPORT_ID;
    memcpy(m_feature_buf + 1, TRITON_DISABLE_LIZARD_MSG, sizeof(TRITON_DISABLE_LIZARD_MSG));
    set_report(TRITON_FEATURE_REPORT_ID, HID_REPORT_TYPE_FEATURE, m_feature_buf, sizeof(m_feature_buf));
    m_last_lizard_update = millis();
}

void SteamTritonHost::set_connected(bool connected)
{
    if (m_connected == connected || !is_dongle(m_pid))
        return;
    m_connected = connected;
    auto itf = host_devices[m_dev_addr]->host_devices_by_itf[m_interface];
    if (connected)
    {
        printf("Steam Triton controller connected on itf %d\r\n", m_interface);
        usb_host_remove_enumerating_interface(this);
        usb_host_add_assignable_interface(itf);
        m_lizard_dirty = true;
    }
    else
    {
        printf("Steam Triton controller disconnected on itf %d\r\n", m_interface);
        m_state = {};
        usb_host_remove_assignable_interface(this);
        usb_host_add_enumerating_interface(itf);
    }
    process_delayed_init();
}

bool SteamTritonHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes > 0)
        {
            uint8_t id = m_ep_in_buf[0];
            if ((id == TRITON_REPORT_WIRELESS || id == TRITON_REPORT_WIRELESS_X) && xferred_bytes >= 2)
            {
                if (m_ep_in_buf[1] == TRITON_WIRELESS_CONNECT)
                    set_connected(true);
                else if (m_ep_in_buf[1] == TRITON_WIRELESS_DISCONNECT)
                    set_connected(false);
            }
            else if (steam_triton_is_state_report(id))
            {
                // A puck may already have a controller linked before we started listening
                set_connected(true);
                steam_triton_parse_report(m_ep_in_buf, xferred_bytes, m_state);
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

void SteamTritonHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (!m_connected)
        return;
    if (m_lizard_dirty || (millis() - m_last_lizard_update) >= TRITON_LIZARD_REFRESH_MS)
    {
        m_lizard_dirty = false;
        disable_lizard_mode();
    }
}

bool SteamTritonHost::tick_digital(proto_Output &type)
{
    return steam_triton_tick_digital(m_state, type);
}

uint16_t SteamTritonHost::tick_analog(proto_Output &type)
{
    return steam_triton_tick_analog(m_state, type);
}
