#include "tusb_option.h"
#include "devices/usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "devices/usb.hpp"
#include "utils.h"
#include <cstring>

static const char chatpad_name[] = "Xbox 360 Chatpad";

// The chatpad's own scancodes, which aren't HID ones
static const uint8_t chatpad_keycodes[][2] = {
    {0x17, HID_KEY_1}, {0x16, HID_KEY_2}, {0x15, HID_KEY_3}, {0x14, HID_KEY_4}, {0x13, HID_KEY_5},
    {0x12, HID_KEY_6}, {0x11, HID_KEY_7}, {0x67, HID_KEY_8}, {0x66, HID_KEY_9}, {0x65, HID_KEY_0},
    {0x27, HID_KEY_Q}, {0x26, HID_KEY_W}, {0x25, HID_KEY_E}, {0x24, HID_KEY_R}, {0x23, HID_KEY_T},
    {0x22, HID_KEY_Y}, {0x21, HID_KEY_U}, {0x76, HID_KEY_I}, {0x75, HID_KEY_O}, {0x64, HID_KEY_P},
    {0x37, HID_KEY_A}, {0x36, HID_KEY_S}, {0x35, HID_KEY_D}, {0x34, HID_KEY_F}, {0x33, HID_KEY_G},
    {0x32, HID_KEY_H}, {0x31, HID_KEY_J}, {0x77, HID_KEY_K}, {0x72, HID_KEY_L}, {0x62, HID_KEY_COMMA},
    {0x46, HID_KEY_Z}, {0x45, HID_KEY_X}, {0x44, HID_KEY_C}, {0x43, HID_KEY_V}, {0x42, HID_KEY_B},
    {0x41, HID_KEY_N}, {0x52, HID_KEY_M}, {0x53, HID_KEY_PERIOD}, {0x63, HID_KEY_ENTER},
    {0x71, HID_KEY_BACKSPACE}, {0x55, HID_KEY_ARROW_LEFT}, {0x54, HID_KEY_SPACE}, {0x51, HID_KEY_ARROW_RIGHT},
};

// The modifier byte's bits, as the modifier keys xboxdrv gives them
static const uint8_t chatpad_modifiers[][2] = {
    {0x01, HID_KEY_SHIFT_LEFT},
    {0x02, HID_KEY_ALT_LEFT},     // green
    {0x04, HID_KEY_CONTROL_LEFT}, // orange
    {0x08, HID_KEY_GUI_LEFT},     // people
};

static void set_key(uint32_t *keys, uint8_t keycode)
{
    keys[keycode >> 5] |= 1u << (keycode & 31);
}

XInputChatpadHost::XInputChatpadHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> controller) : UsbHostInterface(dev_addr, interface, id), m_controller(controller)
{
    set_subtype(KeyboardMouse);
    m_delayed_init = true;
    m_has_name = true;
    for (size_t i = 0; i < sizeof(chatpad_name); i++)
    {
        // skip header
        m_name[(i + 1) * 2] = chatpad_name[i];
    }
}

void XInputChatpadHost::update(bool full_poll, bool send_events)
{
    if (m_pending_found && !m_found && host_devices[m_dev_addr])
    {
        m_found = true;
        usb_host_remove_enumerating_interface(this);
        usb_host_add_assignable_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
        process_delayed_init();
        send_hotplug_event(true);
    }
    // There's nothing to tell the config tool about until a key is pressed
    if (m_found)
    {
        UsbHostInterface::update(full_poll, send_events);
    }
}

void XInputChatpadHost::disconnect()
{
    if (m_found)
    {
        UsbHostInterface::disconnect();
    }
}

void XInputChatpadHost::lost()
{
    m_pending_found = false;
    m_running = false;
    memset((void *)m_keys, 0, sizeof(m_keys));
    usb_host_remove_enumerating_interface(this);
    if (m_found)
    {
        m_found = false;
        usb_host_remove_assignable_interface(this);
        process_delayed_init();
        send_hotplug_event(false);
    }
}

void XInputChatpadHost::restart(uint32_t delay_ms, uint8_t keepalives_first)
{
    m_running = true;
    m_enabled = false;
    m_keepalives_first = keepalives_first;
    m_keepalive_toggle = false;
    m_next_command_ms = millis() + delay_ms;
}

std::optional<uint8_t> XInputChatpadHost::next_command()
{
    uint32_t now = millis();
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

void XInputChatpadHost::handle_keys(const uint8_t *buf, uint32_t len)
{
    if (len < 4 || buf[0] != 0x00)
    {
        return;
    }
    uint32_t keys[8] = {};
    for (const auto &modifier : chatpad_modifiers)
    {
        if (buf[1] & modifier[0])
        {
            set_key(keys, modifier[1]);
        }
    }
    bool pressed = buf[1];
    for (uint8_t i = 2; i < 4; i++)
    {
        for (const auto &key : chatpad_keycodes)
        {
            if (buf[i] == key[0])
            {
                set_key(keys, key[1]);
                pressed = true;
            }
        }
    }
    memcpy((void *)m_keys, keys, sizeof(keys));
    if (pressed)
    {
        m_pending_found = true;
    }
}

bool XInputChatpadHost::controller_takes_chatpad() const
{
    auto controller = m_controller.lock();
    return controller && controller->subtype() == SubType_Gamepad;
}

bool XInputChatpadHost::key_pressed(uint8_t keycode)
{
    return m_keys[keycode >> 5] & (1u << (keycode & 31));
}

bool XInputChatpadHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_keyboardPassthrough_tag)
    {
        return any_key_pressed(*this);
    }
    return type.which_mapping == proto_Output_keycode_tag && key_pressed(type.mapping.keycode);
}

uint16_t XInputChatpadHost::tick_analog(proto_Output &type)
{
    return tick_digital(type) ? UINT16_MAX : 0;
}

// The controller's own, with a bit that none of the ids use set (see hardware_id.hpp), so profiles can tell the
// chatpad apart from the controller it's on
std::optional<uint64_t> XInputChatpadHost::hardware_id() const
{
    auto controller = m_controller.lock();
    if (!controller)
    {
        return std::nullopt;
    }
    auto id = controller->hardware_id();
    if (!id)
    {
        return std::nullopt;
    }
    return *id | (1ull << 40);
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
    if (host_devices[m_dev_addr])
    {
        usb_host_add_enumerating_interface(host_devices[m_dev_addr]->host_devices_by_itf[m_interface]);
    }
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
