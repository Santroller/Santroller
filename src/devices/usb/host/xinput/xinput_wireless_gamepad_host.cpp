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
#include "hardware/timer.h"
#define XINPUT_WIRELESS_DEBUG 0
#if XINPUT_WIRELESS_DEBUG
#define XINPUT_WIRELESS_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define XINPUT_WIRELESS_DEBUG_PRINT(...)
#endif
static const uint8_t capabilitiesRequest[] = {0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360w_prescence[] = {0x08, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const char xinput_wireless_gamepad_name[] = "X360 Wireless 0";
static const char xinput_wireless_gamepad_disconnected_name[] = "X360 Wireless Receiver Slot 0";

#ifndef XINPUT_WIRELESS_TRACE
#define XINPUT_WIRELESS_TRACE 0
#endif

#if XINPUT_WIRELESS_TRACE
// Timing-neutral trace: record into RAM and only print the history when a controller drops,
// since printing every event changes the timing enough to hide the drops
struct WirelessTraceEntry
{
    uint32_t ms;
    const char *reason;
    uint8_t itf;
    uint8_t dir;
    uint8_t b[4];
};
static constexpr size_t wireless_trace_size = 48;
static WirelessTraceEntry wireless_trace[wireless_trace_size];
static size_t wireless_trace_head = 0;
static size_t wireless_trace_count = 0;

void wireless_trace_record(uint8_t interface, bool out, const char *reason, const uint8_t *data, uint32_t len)
{
    auto &e = wireless_trace[wireless_trace_head];
    e.ms = millis();
    e.reason = reason;
    e.itf = interface;
    e.dir = out;
    for (size_t i = 0; i < 4; i++)
        e.b[i] = i < len ? data[i] : 0;
    wireless_trace_head = (wireless_trace_head + 1) % wireless_trace_size;
    if (wireless_trace_count < wireless_trace_size)
        wireless_trace_count++;
}

// Snapshot the trace on drop and print it a line at a time, so it doesn't overflow the console buffer
static WirelessTraceEntry wireless_dump[wireless_trace_size];
static size_t wireless_dump_count = 0;
static size_t wireless_dump_pos = 0;
static uint32_t wireless_dump_next_ms = 0;

static void wireless_trace_dump()
{
    if (wireless_dump_pos < wireless_dump_count)
        return;
    size_t start = (wireless_trace_head + wireless_trace_size - wireless_trace_count) % wireless_trace_size;
    for (size_t i = 0; i < wireless_trace_count; i++)
        wireless_dump[i] = wireless_trace[(start + i) % wireless_trace_size];
    wireless_dump_count = wireless_trace_count;
    wireless_dump_pos = 0;
    wireless_trace_count = 0;
}

static void wireless_trace_dump_tick()
{
    if (wireless_dump_pos >= wireless_dump_count)
        return;
    uint32_t now = millis();
    if (static_cast<int32_t>(now - wireless_dump_next_ms) < 0)
        return;
    const auto &e = wireless_dump[wireless_dump_pos++];
    printf("  %lu %u %s %s %02x%02x%02x%02x\r\n", static_cast<unsigned long>(e.ms), e.itf, e.dir ? "out" : "in",
           e.reason, e.b[0], e.b[1], e.b[2], e.b[3]);
    wireless_dump_next_ms = now + 20;
}

#else
void wireless_trace_record(uint8_t, bool, const char *, const uint8_t *, uint32_t) {}
static void wireless_trace_dump() {}
static void wireless_trace_dump_tick() {}
#endif

static void trace_wireless_report(uint8_t dev_addr, uint8_t interface, uint8_t ep_addr,
                                  bool found, uint32_t xferred_bytes, const uint8_t *data,
                                  const char *direction, const char *reason)
{
    if (strcmp(reason, "other") != 0)
        wireless_trace_record(interface, direction[0] == 'o', reason, data, xferred_bytes);
    constexpr size_t max_bytes = 16;
    char hex[max_bytes * 2 + 1] = {};
    size_t count = std::min(static_cast<size_t>(xferred_bytes), max_bytes);
    // Trailing zero padding carries no information
    while (count > 2 && data[count - 1] == 0)
    {
        --count;
    }
    for (size_t i = 0; i < count; ++i)
    {
        snprintf(hex + i * 2, sizeof(hex) - i * 2, "%02x", data[i]);
    }
    XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u %c %s %s\r\n",
                                static_cast<unsigned long>(millis()), interface, direction[0], reason, hex);
}

XInputWirelessGamepadHost::XInputWirelessGamepadHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id)
{
    m_delayed_init = true;
}

void XInputWirelessGamepadHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    const uint32_t now_us = time_us_32();
    if (m_last_update_us && now_us - m_last_update_us > m_max_update_gap_us)
    {
        m_max_update_gap_us = now_us - m_last_update_us;
    }
    m_last_update_us = now_us;
    process_events();
    flush_out_queue();
    wireless_trace_dump_tick();
    const uint32_t now = millis();
    if (m_found && m_link_lost && now - m_link_lost_ms >= link_loss_grace_ms)
    {
        XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u DISC after %lu\r\n",
                                    static_cast<unsigned long>(now), m_interface,
                                    static_cast<unsigned long>(now - m_link_lost_ms));
        disconnect();
    }
    if (m_found && static_cast<int32_t>(now - m_next_input_stats_ms) >= 0)
    {
        const unsigned long n = m_input_count - m_reported_input_count;
        // Idle slots with nothing to report just add noise
        if (n || m_empty_count)
        {
            XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u st n=%lu e=%lu age=%lu gap=%lu loop=%lu\r\n",
                                        static_cast<unsigned long>(now), m_interface, n,
                                        static_cast<unsigned long>(m_empty_count),
                                        static_cast<unsigned long>(m_last_input_ms ? now - m_last_input_ms : 0),
                                        static_cast<unsigned long>(m_max_in_gap_us / 1000),
                                        static_cast<unsigned long>(m_max_update_gap_us / 1000));
        }
        m_reported_input_count = m_input_count;
        m_empty_count = 0;
        m_max_in_gap_us = 0;
        m_max_update_gap_us = 0;
        m_next_input_stats_ms = now + 4000;
    }
}

void XInputWirelessGamepadHost::link_restored(const char *why)
{
    if (!m_link_lost)
    {
        return;
    }
    XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u UP %s after %lu\r\n",
                                static_cast<unsigned long>(millis()), m_interface, why,
                                static_cast<unsigned long>(millis() - m_link_lost_ms));
    m_link_lost = false;
    // The controller re-linked, so its LED state was reset
    m_led_set = false;
}

void XInputWirelessGamepadHost::disconnect()
{
    m_link_lost = false;
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
    memset(m_report_buf, 0, sizeof(m_report_buf));
    m_out_queue_count = 0;
    m_out_submit_failures = 0;
    m_request_caps_on_input = false;
    m_led_set = false;
    process_delayed_init();
}

bool XInputWirelessGamepadHost::send_out(const char *reason, const uint8_t *packet, uint8_t len)
{
    if (len > sizeof(m_out_queue[0].data))
    {
        return false;
    }
    if (strcmp(reason, "rmbl") == 0 || strcmp(reason, "led") == 0)
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
            XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u out fail %s q=%lu\r\n",
                                        static_cast<unsigned long>(millis()), m_interface,
                                        command.reason, static_cast<unsigned long>(m_out_queue_count));
        }
        return;
    }
    if (m_out_submit_failures)
    {
        XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u out ok after %lu\r\n",
                                    static_cast<unsigned long>(millis()), m_interface,
                                    static_cast<unsigned long>(m_out_submit_failures));
        m_out_submit_failures = 0;
    }
    m_out_pending = true;
    trace_wireless_report(m_dev_addr, m_interface, m_ep_out, m_found,
                          command.length, m_ep_out_buf, "out", command.reason);
    --m_out_queue_count;
    for (size_t i = 0; i < m_out_queue_count; ++i)
    {
        m_out_queue[i] = m_out_queue[i + 1];
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
    if (!send_out("pres", xbox360w_prescence, sizeof(xbox360w_prescence)))
    {
        XINPUT_WIRELESS_DEBUG_PRINT("w %u qfull presence\r\n", m_interface);
    }
    return true;
}

bool XInputWirelessGamepadHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    // Keep this path short: capture the transfer and let update() do the processing
    if (!(ep_addr & 0x80))
    {
        m_out_result = result;
        m_out_done.store(true, std::memory_order_release);
        return true;
    }
    const uint32_t now_us = time_us_32();
    if (m_last_in_us && now_us - m_last_in_us > m_max_in_gap_us)
    {
        m_max_in_gap_us = now_us - m_last_in_us;
    }
    m_last_in_us = now_us;
    uint8_t head = m_event_head.load(std::memory_order_relaxed);
    uint8_t next = (head + 1) % event_queue_capacity;
    if (next == m_event_tail.load(std::memory_order_acquire))
    {
        // Only xfer_cb writes this, so a plain increment avoids an RMW libcall on the M0+
        m_events_dropped.store(m_events_dropped.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
    else
    {
        auto &ev = m_events[head];
        ev.result = result;
        ev.length = static_cast<uint8_t>(std::min<uint32_t>(xferred_bytes, sizeof(ev.data)));
        memcpy(ev.data, m_ep_in_buf, ev.length);
        m_event_head.store(next, std::memory_order_release);
    }
    usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    return true;
}

void XInputWirelessGamepadHost::process_events()
{
    if (m_out_done.load(std::memory_order_acquire))
    {
        m_out_done.store(false, std::memory_order_relaxed);
        process_out(m_out_result);
    }
    uint32_t dropped_total = m_events_dropped.load(std::memory_order_relaxed);
    uint32_t dropped = dropped_total - m_events_dropped_reported;
    if (dropped)
    {
        m_events_dropped_reported = dropped_total;
        XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u in dropped %lu\r\n",
                                    static_cast<unsigned long>(millis()), m_interface,
                                    static_cast<unsigned long>(dropped));
    }
    uint8_t tail = m_event_tail.load(std::memory_order_relaxed);
    while (tail != m_event_head.load(std::memory_order_acquire))
    {
        const auto &ev = m_events[tail];
        if (ev.result != XFER_RESULT_SUCCESS)
        {
            XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u in err %d\r\n",
                                        static_cast<unsigned long>(millis()), m_interface, ev.result);
        }
        else
        {
            process_in(ev.data, ev.length);
        }
        tail = (tail + 1) % event_queue_capacity;
        m_event_tail.store(tail, std::memory_order_release);
    }
}

void XInputWirelessGamepadHost::process_out(xfer_result_t result)
{
    if (m_out_pending && result != XFER_RESULT_SUCCESS)
    {
        wireless_trace_record(m_interface, true, "err", m_ep_out_buf, 4);
        XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u out err %d %02x%02x%02x%02x\r\n",
                                    static_cast<unsigned long>(millis()), m_interface, result,
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

void XInputWirelessGamepadHost::process_in(const uint8_t *buf, uint32_t len)
{
    if (len < 2)
    {
        return;
    }
    XBOX_WIRELESS_HEADER *header = (XBOX_WIRELESS_HEADER *)buf;
    if (header->id == 0x08 || (header->id == 0x00 && header->type == 0x0f))
    {
        trace_wireless_report(m_dev_addr, m_interface, m_ep_in, m_found,
                              len, buf, "in", "status");
    }
    if (header->id == 0x08)
    {
        // bit 7 = gamepad data link, bit 6 = voice link (MS-XUSBI 3.5.5.1.3)
        if (!(header->type & 0x80))
        {
            if (m_found && !m_link_lost)
            {
#if XINPUT_WIRELESS_TRACE
                printf("w %lu %u drop n=%lu age=%lu\r\n", static_cast<unsigned long>(millis()), m_interface,
                       static_cast<unsigned long>(m_input_count),
                       static_cast<unsigned long>(m_last_input_ms ? millis() - m_last_input_ms : 0));
                wireless_trace_dump();
#endif
                XINPUT_WIRELESS_DEBUG_PRINT("w %lu %u DOWN %02x pend=%u q=%lu n=%lu age=%lu\r\n",
                                            static_cast<unsigned long>(millis()), m_interface, header->type,
                                            m_out_pending, static_cast<unsigned long>(m_out_queue_count),
                                            static_cast<unsigned long>(m_input_count),
                                            static_cast<unsigned long>(m_last_input_ms ? millis() - m_last_input_ms : 0));
                // Controllers frequently drop and re-link within a couple of seconds, so hold the
                // slot instead of tearing it down; update() disconnects if the grace period expires
                m_link_lost = true;
                m_link_lost_ms = millis();
                memset(m_report_buf, 0, sizeof(m_report_buf));
            }
        }
        else
        {
            link_restored("status");
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
            link_restored("input");
            m_input_count++;
            m_last_input_ms = millis();
        }
        else if (header->type == 0x00)
        {
            // Empty 00 00 00 f0 packets that genuine receivers stream constantly; count them
            // rather than printing, since the console output alone can stall the loop
            m_empty_count++;
        }
        else if (header->type != 0x0f && header->type != 0x05)
        {
            trace_wireless_report(m_dev_addr, m_interface, m_ep_in, m_found,
                                  len, buf, "in", "other");
        }
        // Controllers that were already on when the receiver was plugged in stream inputs
        // before their link report, when the subtype is still unknown
        if (m_found && (header->type == 0x01 || header->type == 0x03) &&
            len >= report_offset + sizeof(XInputGamepad_Data_t) &&
            len - report_offset <= sizeof(m_report_buf))
        {
            memcpy(m_report_buf, buf + report_offset, len - report_offset);
            if (m_request_caps_on_input)
            {
                if (send_out("caps", capabilitiesRequest, sizeof(capabilitiesRequest)))
                {
                    m_request_caps_on_input = false;
                }
                else
                {
                    XINPUT_WIRELESS_DEBUG_PRINT("w %u qfull caps\r\n", m_interface);
                }
            }
        }
        // Link report
        if (header->type == 0x0f && len >= sizeof(XBOX_WIRELESS_LINK_REPORT))
        {
            XBOX_WIRELESS_LINK_REPORT *linkReport = (XBOX_WIRELESS_LINK_REPORT *)buf;
            if (linkReport->always_0xCC == 0xCC)
            {
                const uint8_t raw_subtype = linkReport->subtype & ~0x80;
                m_request_caps_on_input = raw_subtype == XINPUT_GUITAR_ALTERNATE || raw_subtype == XINPUT_PRO_GUITAR;
                bool newly_connected = !m_found;
                if (newly_connected)
                {
                    m_subtype = get_subtype_from_xinput(raw_subtype);
                    for (size_t i = 0; i < sizeof(xinput_wireless_gamepad_name); i++)
                    {
                        // skip header
                        m_name[(i + 1) * 2] = xinput_wireless_gamepad_name[i];
                    }
                    m_name[(sizeof(xinput_wireless_gamepad_name) - 1) * 2] = '0' + m_subtype;
                    memset(m_report_buf, 0, sizeof(m_report_buf));
                    m_found = true;
                    m_wt = false;
                    m_led_set = false;
                    m_input_count = 0;
                    m_last_input_ms = 0;
                    m_reported_input_count = 0;
                    m_next_input_stats_ms = millis() + 4000;
                }
                if (newly_connected)
                {
                    usb_host_remove_enumerating_interface(this);
                    usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
                    process_delayed_init();
                    set_player_led(m_last_player_led);
                }
                else if (!m_led_set)
                {
                    // Re-linked after a loss: the controller drops again ~500ms later unless it gets an LED command
                    set_player_led(m_last_player_led);
                }
            }
        }
        // Capabilities report
        if (header->type == 0x05 && len >= sizeof(XBOX_WIRELESS_CAPABILITIES))
        {
            XBOX_WIRELESS_CAPABILITIES *caps = (XBOX_WIRELESS_CAPABILITIES *)buf;
            if (caps->always_0x12 == 0x12)
            {
                if (caps->leftStickX == 0xFFC0 && caps->rightStickX == 0xFFC0)
                {
                    m_wt = true;
                }
            }
        }
    }
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
    if (!m_ep_out || !m_found)
        return;
    uint8_t buf[12] = {0x00, 0x01, 0x0f, 0xc0, 0x00, left, right, 0x00, 0x00, 0x00, 0x00, 0x00};
    send_out("rmbl", buf, sizeof(buf));
}

void XInputWirelessGamepadHost::set_player_led(uint8_t player)
{
    m_last_player_led = player;
    if (!m_ep_out || !m_found)
        return;
    uint8_t target_player = player;
    if (target_player == 0)
    {
        target_player = (m_ep_out / 2) + 1;
    }
    uint8_t led_code = 0;
    if (target_player == 1)
        led_code = 6;
    else if (target_player == 2)
        led_code = 7;
    else if (target_player == 3)
        led_code = 8;
    else if (target_player == 4)
        led_code = 9;
    if (led_code == 0)
        return;
    uint8_t buf[12] = {0x00, 0x00, 0x08, (uint8_t)(0x40 + led_code), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (send_out("led", buf, sizeof(buf)))
    {
        m_led_set = true;
    }
}