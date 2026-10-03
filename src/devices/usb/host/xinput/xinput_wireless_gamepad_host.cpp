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
#include <cstring>
#include <stdio.h>

#if XINPUT_WIRELESS_DEBUG
#define XINPUT_WIRELESS_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define XINPUT_WIRELESS_DEBUG_PRINT(...)
#endif
static const uint8_t capabilitiesRequest[] = {0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t controllerHeader40[] = {0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t controllerHeader842[] = {0x00, 0x00, 0x08, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t controllerHeader840[] = {0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_prescence[] = {0x08, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint32_t connected_presence_interval_ms = 4000;
static const char xinput_wireless_gamepad_name[] = "X360 Wireless 0";
static const char xinput_wireless_gamepad_disconnected_name[] = "X360 Wireless Receiver Slot 0";

static void trace_wireless_report(uint8_t dev_addr, uint8_t interface, uint8_t ep_addr,
                                  bool found, uint32_t xferred_bytes, const uint8_t *data,
                                  const char *direction, const char *reason)
{
    constexpr size_t max_bytes = 32;
    char hex[max_bytes * 3 + 1] = {};
    const size_t count = std::min(static_cast<size_t>(xferred_bytes), max_bytes);
    for (size_t i = 0; i < count; ++i)
    {
        snprintf(hex + i * 3, sizeof(hex) - i * 3, "%02x ", data[i]);
    }
    XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless %s t=%lu dev=%u itf=%u ep=%02x found=%u len=%lu reason=%s data=%s\r\n",
           direction, static_cast<unsigned long>(millis()), dev_addr, interface, ep_addr,
           found, static_cast<unsigned long>(xferred_bytes), reason, hex);
}

XInputWirelessGamepadHost::XInputWirelessGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {
    m_delayed_init = true;
}

void XInputWirelessGamepadHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    flush_out_queue();
    const uint32_t now = millis();
    if (m_found && static_cast<int32_t>(now - m_next_input_stats_ms) >= 0)
    {
        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless input stats t=%lu dev=%u itf=%u inputs=%lu new_inputs=%lu ever_input=%u last_input_age_ms=%lu\r\n",
               static_cast<unsigned long>(now), m_dev_addr, m_interface,
               static_cast<unsigned long>(m_input_count),
               static_cast<unsigned long>(m_input_count - m_reported_input_count),
               m_last_input_ms != 0,
               static_cast<unsigned long>(m_last_input_ms ? now - m_last_input_ms : 0));
        m_reported_input_count = m_input_count;
        m_next_input_stats_ms = now + 4000;
    }
    if (m_found && static_cast<int32_t>(now - m_check_link) >= 0)
    {
        if (!send_out("presence", xbox360w_prescence, sizeof(xbox360w_prescence)))
        {
            XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless connected presence queue full dev=%u itf=%u\r\n", m_dev_addr, m_interface);
        }
        m_check_link = millis() + connected_presence_interval_ms;
    }
}

bool XInputWirelessGamepadHost::send_out(const char *reason, const uint8_t *packet, uint8_t len)
{
    if (len > sizeof(m_out_queue[0].data))
    {
        return false;
    }
    if (strcmp(reason, "rumble") == 0 || strcmp(reason, "player-led") == 0)
    {
        for (size_t i = 0; i < m_out_queue_count; ++i)
        {
            if (strcmp(m_out_queue[i].reason, reason) == 0)
            {
                m_out_queue[i].length = len;
                memcpy(m_out_queue[i].data, packet, len);
                return true;
            }
        }
    }
    if (m_out_queue_count == out_queue_capacity)
    {
        return false;
    }
    auto &command = m_out_queue[m_out_queue_count++];
    command.reason = reason;
    command.length = len;
    memcpy(command.data, packet, len);
    flush_out_queue();
    return true;
}

void XInputWirelessGamepadHost::flush_out_queue()
{
    if (m_out_pending || !m_out_queue_count)
    {
        return;
    }
    const auto &command = m_out_queue[0];
    memcpy(m_ep_out_buf, command.data, command.length);
    bool submitted = send_intr_xfer(m_ep_out, m_ep_out_buf, command.length);
    if (!submitted)
    {
        if (m_out_submit_failures++ == 0)
        {
            XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless OUT submit failed t=%lu dev=%u itf=%u ep=%02x found=%u reason=%s queued=%lu\r\n",
                   static_cast<unsigned long>(millis()), m_dev_addr, m_interface, m_ep_out,
                   m_found, command.reason, static_cast<unsigned long>(m_out_queue_count));
        }
        return;
    }
    if (m_out_submit_failures)
    {
        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless OUT submit recovered t=%lu dev=%u itf=%u ep=%02x failures=%lu\r\n",
               static_cast<unsigned long>(millis()), m_dev_addr, m_interface, m_ep_out,
               static_cast<unsigned long>(m_out_submit_failures));
        m_out_submit_failures = 0;
    }
    m_out_pending = true;
    trace_wireless_report(m_dev_addr, m_interface, m_ep_out, m_found,
                          command.length, m_ep_out_buf, "OUT submit", command.reason);
    --m_out_queue_count;
    for (size_t i = 0; i < m_out_queue_count; ++i)
    {
        m_out_queue[i] = m_out_queue[i + 1];
    }
}

void XInputWirelessGamepadHost::send_link_requests(bool newly_connected)
{
    if (!send_out("controller-header-0842", controllerHeader842, sizeof(controllerHeader842)))
    {
        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless controller header queue full dev=%u itf=%u\r\n", m_dev_addr, m_interface);
        return;
    }
    if (newly_connected)
    {
        m_check_caps = millis() + 1000;
        m_caps_retries = 0;
        if (!send_out("capabilities", capabilitiesRequest, sizeof(capabilitiesRequest)))
        {
            XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless capabilities queue full dev=%u itf=%u\r\n", m_dev_addr, m_interface);
        }
    }
    if (!send_out("controller-header-0040", controllerHeader40, sizeof(controllerHeader40)))
    {
        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless controller header queue full dev=%u itf=%u\r\n", m_dev_addr, m_interface);
    }
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
    if (!send_out("controller-header-0840", controllerHeader840, sizeof(controllerHeader840)))
    {
        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless initial controller header queue full dev=%u itf=%u\r\n", m_dev_addr, m_interface);
    }
    send_out("presence", xbox360w_prescence, sizeof(xbox360w_prescence));
    m_check_link = millis() + 1000;
    return true;
}

bool XInputWirelessGamepadHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (!(ep_addr & 0x80))
    {
        if (m_out_pending)
        {
            XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless OUT complete t=%lu dev=%u itf=%u ep=%02x result=%d len=%lu data=%02x %02x %02x %02x\r\n",
                   static_cast<unsigned long>(millis()), m_dev_addr, m_interface, ep_addr,
                   result, static_cast<unsigned long>(xferred_bytes),
                   m_ep_out_buf[0], m_ep_out_buf[1], m_ep_out_buf[2], m_ep_out_buf[3]);
        }
        if (m_out_pending && result != XFER_RESULT_SUCCESS &&
            m_ep_out_buf[0] == 0x00 && m_ep_out_buf[1] == 0x00 &&
            m_ep_out_buf[2] == 0x08)
        {
            m_led_set = false;
        }
        m_out_pending = false;
    }
    if (ep_addr & 0x80)
    {
        if (result != XFER_RESULT_SUCCESS)
        {
            XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless IN error dev=%u itf=%u ep=%02x result=%d len=%lu found=%u\r\n",
                   m_dev_addr, m_interface, ep_addr, result,
                   static_cast<unsigned long>(xferred_bytes), m_found);
        }
        if (result == XFER_RESULT_SUCCESS && xferred_bytes >= 2)
        {
            XBOX_WIRELESS_HEADER *header = (XBOX_WIRELESS_HEADER *)m_ep_in_buf;
            if (header->id == 0x08 || (header->id == 0x00 && header->type == 0x0f))
            {
                trace_wireless_report(m_dev_addr, m_interface, ep_addr, m_found,
                                      xferred_bytes, m_ep_in_buf, "IN", "status/link");
            }
            if (header->id == 0x08)
            {
                // Disconnected
                if (header->type == 0x00 || !(header->type & 0x80))
                {
                    if (m_found)
                    {
                        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless disconnect t=%lu dev=%u itf=%u ep=%02x out_pending=%u out_data=%02x %02x %02x %02x queued=%lu next=%s submit_failures=%lu inputs=%lu last_input_age=%lu\r\n",
                               static_cast<unsigned long>(millis()), m_dev_addr, m_interface, m_ep_out,
                               m_out_pending, m_ep_out_buf[0], m_ep_out_buf[1], m_ep_out_buf[2], m_ep_out_buf[3],
                               static_cast<unsigned long>(m_out_queue_count),
                               m_out_queue_count ? m_out_queue[0].reason : "none",
                               static_cast<unsigned long>(m_out_submit_failures),
                               static_cast<unsigned long>(m_input_count),
                               static_cast<unsigned long>(m_last_input_ms ? millis() - m_last_input_ms : 0));
                        m_input_count = 0;
                        m_last_input_ms = 0;
                        m_reported_input_count = 0;
                        m_next_input_stats_ms = 0;
                        usb_host_remove_assignable_interface(this);
                        usb_host_add_enumerating_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                        for (size_t i = 0; i < sizeof(xinput_wireless_gamepad_disconnected_name); i++)
                        {
                            // skip header
                            m_name[(i + 1) * 2] = xinput_wireless_gamepad_disconnected_name[i];
                        }
                        m_name[(sizeof(xinput_wireless_gamepad_disconnected_name) - 1) * 2] = '1' + (m_ep_out / 2);
                        m_found = false;
                        m_out_queue_count = 0;
                        m_out_submit_failures = 0;
                        m_check_caps = 0;
                        m_caps_retries = 0;
                        m_led_set = false;
                        m_check_link = millis() + 4000;
                        process_delayed_init();
                    }
                }
                else
                {
                    if (!m_found)
                    {
                        m_check_link = millis() + 1000;
                    }
                    if (!send_out("controller-header-0040", controllerHeader40, sizeof(controllerHeader40)))
                    {
                        XINPUT_WIRELESS_DEBUG_PRINT("X360 wireless controller header queue full dev=%u itf=%u\r\n", m_dev_addr, m_interface);
                    }
                }
            }
            else if (header->id == 0x00)
            {
                // Gamepad inputs
                // The embedded XInput report starts at byte 4 (03 13 in the
                // receiver's input packets), before the final header byte.
                constexpr size_t report_offset = 4;
                if (header->type == 0x01 || header->type == 0x03)
                {
                    m_input_count++;
                    m_last_input_ms = millis();
                }
                else if (header->type != 0x0f && header->type != 0x05)
                {
                    trace_wireless_report(m_dev_addr, m_interface, ep_addr, m_found,
                                          xferred_bytes, m_ep_in_buf, "IN", "other");
                }
                if ((header->type == 0x01 || header->type == 0x03) &&
                    xferred_bytes >= report_offset + sizeof(XInputGamepad_Data_t) &&
                    xferred_bytes - report_offset <= sizeof(m_report_buf))
                {
                    memcpy(m_report_buf, m_ep_in_buf + report_offset, xferred_bytes - report_offset);
                    if (!m_led_set)
                    {
                        set_player_led(m_last_player_led);
                    }
                }
                // Link report
                if (header->type == 0x0f && xferred_bytes >= sizeof(XBOX_WIRELESS_LINK_REPORT))
                {
                    XBOX_WIRELESS_LINK_REPORT *linkReport = (XBOX_WIRELESS_LINK_REPORT *)m_ep_in_buf;
                    if (linkReport->always_0xCC == 0xCC)
                    {
                        bool newly_connected = !m_found;
                        if (newly_connected)
                        {
                            m_subtype = get_subtype_from_xinput(linkReport->subtype & ~0x80);
                            for (size_t i = 0; i < sizeof(xinput_wireless_gamepad_name); i++)
                            {
                                // skip header
                                m_name[(i + 1) * 2] = xinput_wireless_gamepad_name[i];
                            }
                            m_name[(sizeof(xinput_wireless_gamepad_name) - 1) * 2] = '0' + m_subtype;
                            m_found = true;
                            m_wt = false;
                            m_led_set = false;
                            m_input_count = 0;
                            m_last_input_ms = 0;
                            m_reported_input_count = 0;
                            m_next_input_stats_ms = millis() + 4000;
                            m_check_link = millis() + connected_presence_interval_ms;
                        }
                        send_link_requests(newly_connected);
                        if (newly_connected)
                        {
                            usb_host_remove_enumerating_interface(this);
                            usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                            process_delayed_init();
                            set_player_led(m_last_player_led);
                        }
                    }
                }
                // Capabilities report
                if (header->type == 0x05 && xferred_bytes >= sizeof(XBOX_WIRELESS_CAPABILITIES))
                {
                    XBOX_WIRELESS_CAPABILITIES *caps = (XBOX_WIRELESS_CAPABILITIES *)m_ep_in_buf;
                    if (caps->always_0x12 == 0x12)
                    {
                        m_check_caps = 0;
                        m_caps_retries = 0;
                        if (caps->leftStickX == 0xFFC0 && caps->rightStickX == 0xFFC0)
                        {
                            m_wt = true;
                        }
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
            send_out("presence", xbox360w_prescence, sizeof(xbox360w_prescence));
            m_check_link = millis() + 1000;
        }
        if (m_check_caps && millis() > m_check_caps)
        {
            send_out("capabilities", capabilitiesRequest, sizeof(capabilitiesRequest));
            m_check_caps = millis() + 1000;
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
    send_out("rumble", buf, sizeof(buf));
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
    if (send_out("player-led", buf, sizeof(buf)))
    {
        m_led_set = true;
    }
}