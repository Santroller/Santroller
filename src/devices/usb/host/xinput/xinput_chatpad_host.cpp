#include "tusb_option.h"
#include "devices/usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "protocols/xbox360_chatpad.hpp"
#include "host/usbh.h"
#include "devices/usb.hpp"
#include "utils.h"
#include <cstring>

// The LED commands, on then off
static const uint8_t chatpad_leds[][3] = {
    {chatpad_shift, 0x08, 0x00},
    {chatpad_green, 0x09, 0x01},
    {chatpad_orange, 0x0A, 0x02},
};

XInputChatpadHost::XInputChatpadHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> controller) : ChatpadHost(dev_addr, interface, id, controller, "Xbox 360 Chatpad")
{
}

void XInputChatpadHost::reset_keys()
{
    m_running = false;
    m_decoder.reset();
}

void XInputChatpadHost::restart(uint32_t delay_ms, uint8_t keepalives_first)
{
    m_running = true;
    m_enabled = false;
    m_keepalives_first = keepalives_first;
    m_keepalive_toggle = false;
    m_next_command_ms = millis() + delay_ms;
    // enabling it turns the LEDs off
    m_leds_sent = 0;
}

std::optional<uint8_t> XInputChatpadHost::next_command()
{
    uint32_t now = millis();
    // LEDs go out as soon as they change, between the keep-alives
    if (m_running && m_enabled)
    {
        uint8_t leds = m_decoder.sticky();
        for (const auto &led : chatpad_leds)
        {
            if ((leds ^ m_leds_sent) & led[0])
            {
                m_leds_sent ^= led[0];
                return (leds & led[0]) ? led[1] : led[2];
            }
        }
    }
    if (!m_running || static_cast<int32_t>(now - m_next_command_ms) < 0)
    {
        return std::nullopt;
    }
    m_next_command_ms = now + 1000;
    if (!m_enabled && !m_keepalives_first)
    {
        m_enabled = true;
        return chatpad_enable;
    }
    if (m_keepalives_first)
    {
        m_keepalives_first--;
    }
    // The keep-alive alternates between two commands
    m_keepalive_toggle = !m_keepalive_toggle;
    return m_keepalive_toggle ? 0x1F : 0x1E;
}


void XInputWirelessChatpadHost::handle_packet(const uint8_t *buf, uint32_t len)
{
    // Plug-in module packets carry the chatpad's report at the end
    constexpr uint32_t payload_offset = 24;
    if (len < payload_offset + 4 || buf[1] != 0x02)
    {
        return;
    }
    const uint8_t *payload = buf + payload_offset;
    if (payload[0] == 0xF0)
    {
        // The chatpad asking to be enabled again
        if (payload[1] == 0x03 && controller_takes_chatpad())
        {
            restart(0, 0);
        }
        return;
    }
    handle_keys(payload, 4);
}

void XInputWirelessChatpadHost::controller_linked()
{
    tuh_vid_pid_get(m_dev_addr, &m_vid, &m_pid);
    restart(100, 0);
    // for the updates that make it assignable once it's used
    usb_host_add_enumerating_interface(shared_from_this());
}

bool XInputWirelessChatpadHost::next_report(uint8_t report[12])
{
    if (!controller_takes_chatpad())
    {
        return false;
    }
    auto command = next_command();
    if (!command)
    {
        return false;
    }
    memset(report, 0, 12);
    report[2] = 0x0C;
    report[3] = *command;
    return true;
}
