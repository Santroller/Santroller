#include "tusb_option.h"
#include "devices/usb/host/chatpad_host.h"
#include "devices/usb.hpp"
#include <cstring>

ChatpadHost::ChatpadHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> controller, const char *name) : UsbHostInterface(dev_addr, interface, id), m_controller(controller)
{
    set_subtype(KeyboardMouse);
    m_delayed_init = true;
    m_has_name = true;
    for (size_t i = 0; name[i] && (i + 2) * 2 < sizeof(m_name); i++)
    {
        // skip header
        m_name[(i + 1) * 2] = name[i];
    }
}

void ChatpadHost::check_presence()
{
    if (m_pending_lost)
    {
        lost();
    }
    if (m_pending_found && !m_found)
    {
        m_found = true;
        usb_host_remove_enumerating_interface(this);
        usb_host_add_assignable_interface(shared_from_this());
        process_delayed_init();
        send_hotplug_event(true);
    }
}

void ChatpadHost::update(bool full_poll, bool send_events)
{
    check_presence();
    // There's nothing to tell the config tool about until a key is pressed
    if (m_found)
    {
        UsbHostInterface::update(full_poll, send_events);
    }
}

void ChatpadHost::disconnect()
{
    if (m_found)
    {
        UsbHostInterface::disconnect();
    }
}

void ChatpadHost::lost()
{
    m_pending_found = false;
    m_pending_lost = false;
    reset_keys();
    usb_host_remove_enumerating_interface(this);
    if (m_found)
    {
        m_found = false;
        usb_host_remove_assignable_interface(this);
        process_delayed_init();
        send_hotplug_event(false);
    }
}

bool ChatpadHost::controller_takes_chatpad() const
{
    auto controller = m_controller.lock();
    return controller && controller->subtype() == SubType_Gamepad;
}

bool ChatpadHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_keyboardPassthrough_tag)
    {
        return any_key_pressed(*this);
    }
    return type.which_mapping == proto_Output_keycode_tag && key_pressed(type.mapping.keycode);
}

uint16_t ChatpadHost::tick_analog(proto_Output &type)
{
    return tick_digital(type) ? UINT16_MAX : 0;
}

// The controller's own, with a bit that none of the ids use set (see hardware_id.hpp), so profiles can tell the
// chatpad apart from the controller it's on
std::optional<uint64_t> ChatpadHost::hardware_id() const
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
