#pragma once
#include "devices/usb/host/chatpad_host.h"
#include "protocols/hid_keyboard.hpp"
#include "gip_sequence.h"

class XGIPProtocol;

// An Xbox One chatpad. The controller passes it on as a device of its own, with its own client id in the GIP header,
// which announces and identifies itself like any other, then sends boot keyboard reports in HID report messages
// ([MS-GIPUSB] 2.2.1.4 and 2.2.2.4.7, and xone's chatpad driver). Shared by wired controllers and ones on the
// wireless adapter, which pass on anything from a client other than their own
class XboxOneChatpadHost : public ChatpadHost
{
public:
    // Sends a packet through the controller, before anything else waiting to go for an ACK
    using Send = void (*)(UsbHostInterface &controller, const uint8_t *data, uint16_t len, bool ack);
    XboxOneChatpadHost(uint8_t dev_addr, uint8_t interface, uint16_t id, std::weak_ptr<UsbHostInterface> controller, Send send);
    ~XboxOneChatpadHost();
    bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) override { return true; }
    bool key_pressed(uint8_t keycode) override { return m_keyboard.key_pressed(keycode); }
    // A packet from a device plugged into the controller. Returns how many bytes it took up, or 0 if it couldn't
    // make sense of it
    uint16_t process(const uint8_t *data, uint16_t len);

protected:
    void reset_keys() override;

private:
    void send(uint8_t command, uint8_t client, const uint8_t *data, uint16_t len);
    void send_through_controller(const uint8_t *data, uint16_t len, bool ack);
    Send m_send;
    XGIPProtocol *m_incoming;
    // Metadata comes in fragments, which get their own parser like the controller's
    XGIPProtocol *m_incoming_chunk;
    XGIPProtocol *m_outgoing;
    gip_sequence_pool_t m_sequences;
    HidKeyboardDecoder m_keyboard;
    // The chatpad's client id, 0 until one has identified itself
    uint8_t m_client = 0;
};
