#include "tusb_option.h"
#include "devices/usb/host/hid/steam_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "devices/usb.hpp"

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

    if (vid == VALVE_USB_VID)
    {
        if (pid == VALVE_STEAM_CONTROLLER_DONGLE_PID)
        {
            // Interfaces 1..4 are the wireless controller slots (0 is emulated keyboard)
            if (itf_desc->bInterfaceNumber < 1 || itf_desc->bInterfaceNumber > 4)
            {
                return nullptr;
            }
        }
        else if (pid == VALVE_STEAM_CONTROLLER_WIRED_PID || pid == VALVE_STEAM_DECK_PID)
        {
            // Interface 2 is the gamepad controller (0 is mouse, 1 is keyboard)
            if (itf_desc->bInterfaceNumber != 2)
            {
                return nullptr;
            }
        }
    }

    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    auto intf = std::make_shared<SteamHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, vid, pid);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc = (tusb_hid_descriptor_hid_t *)p_desc;
    if (HID_DESC_TYPE_HID != x_desc->bDescriptorType)
    {
        return nullptr;
    }
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        if (TUSB_DESC_ENDPOINT != desc_ep->bDescriptorType)
        {
            return nullptr;
        }
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            if (!tuh_edpt_open(dev_addr, desc_ep))
            {
                return nullptr;
            }
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            if (!tuh_edpt_open(dev_addr, desc_ep))
            {
                return nullptr;
            }
        }
    }
    printf("steam_open: opened itf %d successfully (ep_in: %02x, ep_out: %02x)\r\n", itf_desc->bInterfaceNumber, intf->m_ep_in, intf->m_ep_out);
    if (intf->m_ep_out)
    {
        list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
    }
    if (intf->m_ep_in)
    {
        list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    }
    if (intf->m_delayed_init)
    {
        usb_host_add_enumerating_interface(intf);
    }
    else
    {
        usb_host_add_assignable_interface(intf);
    }
    USB_FreeReportInfo(info);
    return intf;
}

void SteamHost::configure_controller()
{
    printf("steam: configuring controller on itf %d\r\n", m_interface);
    bool status = false;

    // Clear digital mappings via Feature Report (0x81)
    memset(m_ep_out_buf, 0, sizeof(m_ep_out_buf));
    m_ep_out_buf[0] = STEAM_CMD_CLEAR_MAPPINGS;
    set_report(0, HID_REPORT_TYPE_FEATURE, m_ep_out_buf, sizeof(m_ep_out_buf), &status);

    // Disable lizard mode via Feature Report (0x87)
    memset(m_ep_out_buf, 0, sizeof(m_ep_out_buf));
    m_ep_out_buf[0] = STEAM_CMD_WRITE_REGISTER; // 0x87
    m_ep_out_buf[1] = 0x0F;                     // 15 bytes of settings (5 * 3 bytes)
    m_ep_out_buf[2] = STEAM_REG_GYRO_MODE;   m_ep_out_buf[3] = 0x00; m_ep_out_buf[4] = 0x00;
    m_ep_out_buf[5] = STEAM_REG_LPAD_MODE;   m_ep_out_buf[6] = 0x07; m_ep_out_buf[7] = 0x00;
    m_ep_out_buf[8] = STEAM_REG_RPAD_MODE;   m_ep_out_buf[9] = 0x07; m_ep_out_buf[10] = 0x00;
    m_ep_out_buf[11] = STEAM_REG_RPAD_MARGIN; m_ep_out_buf[12] = 0x00; m_ep_out_buf[13] = 0x00;
    m_ep_out_buf[14] = STEAM_REG_LED;         m_ep_out_buf[15] = 0x64; m_ep_out_buf[16] = 0x00;
    set_report(0, HID_REPORT_TYPE_FEATURE, m_ep_out_buf, sizeof(m_ep_out_buf), &status);
}

bool SteamHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    if (m_vid != VALVE_USB_VID) return true;

    if (m_pid == VALVE_STEAM_CONTROLLER_DONGLE_PID)
    {
        // Query wireless status (0xB4) - do not enable pairing (0xAD) or configure empty slots
        memset(m_ep_out_buf, 0, sizeof(m_ep_out_buf));
        m_ep_out_buf[0] = 0xB4;
        set_report(0, HID_REPORT_TYPE_FEATURE, m_ep_out_buf, sizeof(m_ep_out_buf));
    }
    else
    {
        m_connected = true;
        configure_controller();
    }
    return true;
}

bool SteamHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes > 0)
        {
            // Log any non-input packets (connect/disconnect/status)
            if (m_ep_in_buf[2] != 0x01)
            {
                printf("steam packet on itf %d: len %ld [%02x %02x %02x %02x %02x]\r\n",
                       m_interface, (long)xferred_bytes,
                       m_ep_in_buf[0], m_ep_in_buf[1], m_ep_in_buf[2],
                       xferred_bytes > 3 ? m_ep_in_buf[3] : 0,
                       xferred_bytes > 4 ? m_ep_in_buf[4] : 0);
            }

            // Check for wireless connect/disconnect events from dongle
            if (xferred_bytes >= 5 && m_ep_in_buf[0] == 0x01 && m_ep_in_buf[1] == 0x00 && m_ep_in_buf[2] == 0x03)
            {
                uint8_t event_type = m_ep_in_buf[4];
                printf("steam dongle event on itf %d: 0x%02x\r\n", m_interface, event_type);
                if (event_type == 0x01) // Disconnected
                {
                    if (m_connected)
                    {
                        printf("Steam controller disconnected on itf %d\r\n", m_interface);
                        m_connected = false;
                        usb_host_remove_assignable_interface(this);
                        usb_host_add_enumerating_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                        process_delayed_init();
                    }
                }
                else if (event_type == 0x02 || event_type == 0x03) // Connected or newly paired
                {
                    if (!m_connected)
                    {
                        printf("Steam controller connected on itf %d\r\n", m_interface);
                        m_connected = true;
                        usb_host_remove_enumerating_interface(this);
                        usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                        process_delayed_init();
                    }
                    if (event_type == 0x03)
                    {
                        // Commit pairing to dongle NVM
                        memset(m_ep_out_buf, 0, sizeof(m_ep_out_buf));
                        m_ep_out_buf[0] = 0xB3; // ID_DONGLE_COMMIT_DEVICE
                        set_report(0, HID_REPORT_TYPE_FEATURE, m_ep_out_buf, sizeof(m_ep_out_buf));
                    }
                    configure_controller();
                }
            }
            else
            {
                if (!m_connected && m_ep_in_buf[0] == 0x01 && m_ep_in_buf[1] == 0x00 && m_ep_in_buf[2] == 0x01)
                {
                    printf("steam: input received on itf %d without connect event, configuring\r\n", m_interface);
                    m_connected = true;
                    usb_host_remove_enumerating_interface(this);
                    usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                    process_delayed_init();
                    configure_controller();
                }
                steam_parse_usb_report(m_ep_in_buf, xferred_bytes, m_state);
            }
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

