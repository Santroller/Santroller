#pragma once
#include "devices/usb/host/host.hpp"

// A chatpad plugged into a controller, which shows up as a keyboard of its own once it's found
class ChatpadHost : public UsbHostInterface, public std::enable_shared_from_this<ChatpadHost>
{
public:
    ChatpadHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> controller, const char *name);
    void update(bool full_poll, bool send_events) override;
    void disconnect() override;
    bool tick_digital(proto_Output &type) override;
    uint16_t tick_analog(proto_Output &type) override;
    std::optional<uint64_t> hardware_id() const override;
    // Shows or hides the chatpad once it's been found or unplugged, for whatever updates it before it's assignable
    void check_presence();
    // Lets go of every key and hides the chatpad again, e.g. when it's unplugged or its controller goes away
    void lost();

protected:
    // Lets go of every key
    virtual void reset_keys() = 0;
    // Shows up or goes away on the next update, as these can come from a transfer callback
    void found() { m_pending_found = true; }
    void unplugged() { m_pending_lost = true; }
    // Only standard controllers take a chatpad
    bool controller_takes_chatpad() const;
    std::weak_ptr<UsbHostInterface> m_controller;
    bool m_found = false;

private:
    volatile bool m_pending_found = false;
    volatile bool m_pending_lost = false;
};
