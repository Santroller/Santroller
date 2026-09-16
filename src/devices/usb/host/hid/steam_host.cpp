#include "tusb_option.h"
#include "devices/usb/host/hid/steam_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

std::shared_ptr<UsbHostInterface> SteamHost::open(std::shared_ptr<UsbHostDevice> list,
                                                   tusb_desc_interface_t const *itf_desc,
                                                   uint16_t max_len,
                                                   uint16_t vid,
                                                   uint16_t pid,
                                                   uint16_t revision,
                                                   HID_ReportInfo_t *info)
{
    bool is_steam = (vid == VALVE_USB_VID && (pid == VALVE_STEAM_CONTROLLER_WIRED_PID ||
                                              pid == VALVE_STEAM_CONTROLLER_DONGLE_PID ||
                                              pid == VALVE_STEAM_DECK_PID)) ||
                    (vid == HORI_VID && pid == HORI_STEAM_CONTROLLER_PID);
    if (!is_steam)
    {
        return nullptr;
    }

    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<SteamHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, vid, pid);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc = (tusb_hid_descriptor_hid_t *)p_desc;
    TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
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
    USB_FreeReportInfo(info);
    return intf;
}

void SteamHost::send_init_step()
{
    if (m_vid != VALVE_USB_VID) return;

    switch (m_init_step)
    {
    case 0:
    {
        // Clear digital mappings via Feature Report
        memcpy(m_ep_out_buf, STEAM_CMD_CLEAR_MAPPINGS_BUF, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF));
        bool status = false;
        set_report(0, HID_REPORT_TYPE_FEATURE, m_ep_out_buf, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF), &status);
        m_init_step++;
        break;
    }
    case 1:
    {
        // Disable lizard mode via Feature Report
        memcpy(m_ep_out_buf, STEAM_CMD_DISABLE_LIZARD_BUF, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF));
        bool status = false;
        set_report(0, HID_REPORT_TYPE_FEATURE, m_ep_out_buf, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF), &status);
        m_init_step++;
        break;
    }
    default:
        break;
    }
}

bool SteamHost::set_config()
{
    HidHost::set_config();
    m_init_step = 0;
    send_init_step();
    send_init_step();

    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool SteamHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes > 0)
        {
            steam_parse_usb_report(m_ep_in_buf, xferred_bytes, m_state);
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool SteamHost::tick_digital(proto_Output &type)
{
    return steam_tick_digital(m_state, type);
}

uint16_t SteamHost::tick_analog(proto_Output &type)
{
    return steam_tick_analog(m_state, type);
}

