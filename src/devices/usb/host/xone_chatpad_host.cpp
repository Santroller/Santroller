#include "tusb_option.h"
#include "devices/usb/host/xone_chatpad_host.h"
#include "xgip_protocol.h"
#include "devices/usb.hpp"
#include <cstring>

// The HID report message, which is a system message ([MS-GIPUSB] 2.2.2.4.7). Not GIP_HID_REPORT, which is the
// Guitar Hero Live guitar's own
static constexpr uint8_t gip_system_hid_report = 0x0B;
static constexpr uint8_t gip_power_on[] = {0x00};
// What a chatpad lists among its types in its metadata
static constexpr char chatpad_type[] = "Windows.Xbox.Input.Chatpad";

static bool contains(const uint8_t *data, uint16_t len, const char *text, size_t text_len)
{
    for (uint16_t i = 0; i + text_len <= len; i++)
    {
        if (memcmp(data + i, text, text_len) == 0)
        {
            return true;
        }
    }
    return false;
}

XboxOneChatpadHost::XboxOneChatpadHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> controller, Send send) : ChatpadHost(dev_addr, interface, id, controller, "Xbox One Chatpad"), m_send(send), m_incoming(new XGIPProtocol()), m_incoming_chunk(new XGIPProtocol()), m_outgoing(new XGIPProtocol())
{
    gip_sequence_pool_init(&m_sequences);
    m_keyboard.use_boot_layout();
}

XboxOneChatpadHost::~XboxOneChatpadHost()
{
    delete m_incoming;
    delete m_incoming_chunk;
    delete m_outgoing;
}

void XboxOneChatpadHost::reset_keys()
{
    m_client = 0;
    m_keyboard = HidKeyboardDecoder();
    m_keyboard.use_boot_layout();
}

void XboxOneChatpadHost::send(uint8_t command, uint8_t client, const uint8_t *data, uint16_t len)
{
    m_outgoing->reset();
    m_outgoing->setAttributes(command, gip_sequence_pool_next(&m_sequences, command), 1, 0, 0);
    m_outgoing->setClient(client);
    if (len)
    {
        m_outgoing->setData(data, len);
    }
    uint8_t *packet = m_outgoing->generatePacket();
    send_through_controller(packet, m_outgoing->getPacketLength(), false);
}

void XboxOneChatpadHost::send_through_controller(const uint8_t *data, uint16_t len, bool ack)
{
    if (auto controller = m_controller.lock())
    {
        m_send(*controller, data, len, ack);
    }
}

uint16_t XboxOneChatpadHost::process(const uint8_t *data, uint16_t len)
{
    uint8_t client = data[1] & 0x0F;
    XGIPProtocol *xgip = (data[1] & 0x80) ? m_incoming_chunk : m_incoming;
    if (!xgip->parse(data, len))
    {
        return 0;
    }
    uint16_t wire_len = xgip->getParsedWireLength();
    if (wire_len == 0 || wire_len > len)
    {
        return 0;
    }
    if (xgip->ackRequired())
    {
        uint8_t *ack = xgip->generateAckPacket();
        send_through_controller(ack, xgip->getPacketLength(), true);
    }
    if (xgip->getChunked() && !xgip->endOfChunk())
    {
        return wire_len;
    }
    switch (xgip->getCommand())
    {
    case GIP_ANNOUNCE:
        // Anything plugged in gets asked what it is
        send(GIP_DEVICE_DESCRIPTOR, client, nullptr, 0);
        break;
    case GIP_DEVICE_DESCRIPTOR:
        if (contains(xgip->getData(), xgip->getDataLength(), chatpad_type, sizeof(chatpad_type) - 1) &&
            controller_takes_chatpad())
        {
            m_client = client;
            send(GIP_SET_STATE, client, gip_power_on, sizeof(gip_power_on));
            // It said what it is, so unlike a 360 chatpad there's no waiting for a key
            found();
        }
        break;
    case gip_system_hid_report:
        if (client == m_client)
        {
            m_keyboard.handle_report(xgip->getData(), xgip->getDataLength());
        }
        break;
    case GIP_KEEPALIVE:
        // The status message, whose top bit is clear once the device is unplugged
        if (client == m_client && xgip->getDataLength() >= 1 && !(xgip->getData()[0] & 0x80))
        {
            unplugged();
        }
        break;
    }
    return wire_len;
}
