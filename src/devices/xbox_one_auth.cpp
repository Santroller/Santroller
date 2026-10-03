#include "devices/xbox_one_auth.hpp"
#include "usb/auth_broker.h"

static constexpr uint16_t gip_max_chunk = 0x3A;

XboxOneAuthDevice::XboxOneAuthDevice(proto_XboxOneAuthDevice device, uint16_t id)
    : Device(id),
      m_chip(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock, device.has_resetPin ? device.resetPin : -1),
      m_device(device)
{
}

void XboxOneAuthDevice::begin()
{
    m_chip.begin();
}

void XboxOneAuthDevice::end(bool full)
{
    if (m_registered)
    {
        auth_broker.unregister_handler(ModeXboxOne);
        m_registered = false;
    }
    m_chip.end();
}

void XboxOneAuthDevice::handle_auth(XGIPProtocol *packet)
{
    // Control packets (data[0] == 1) and unsupported types are rejected by request()
    if (!m_chip.request(packet->getData(), packet->getDataLength()))
        return;
    m_req_command = packet->getCommand();
    m_req_sequence = packet->getSequence();
    m_req_ack = packet->getPacketAck();
}

void XboxOneAuthDevice::update(bool full_poll, bool send_events)
{
    m_chip.tick();
    // Only claim auth once the chip has answered, and never take over from another provider
    if (m_chip.is_ready() && !m_registered && !auth_broker.has_handler(ModeXboxOne))
    {
        auth_broker.register_handler(ModeXboxOne, [this](XGIPProtocol *packet)
                                     { handle_auth(packet); });
        m_registered = true;
    }
    if (m_chip.has_response())
    {
        m_chip.clear_response();
        XGIPProtocol reply;
        uint16_t len = m_chip.response_length();
        reply.setAttributes(m_req_command, m_req_sequence, 1, len > gip_max_chunk, m_req_ack);
        reply.setData(m_chip.response(), len);
        auth_broker.forward_auth_response(ModeXboxOne, &reply);
    }
}

bool XboxOneAuthDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda || (m_device.has_resetPin && pin == m_device.resetPin);
}
