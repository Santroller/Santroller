#pragma once
#include <stdint.h>
#include "base.hpp"
#include "device.pb.h"
#include "libxbox_one_auth.hpp"
#include "xgip_protocol.h"

// Xbox One console auth via a dedicated auth chip on I2C
class XboxOneAuthDevice : public Device
{
public:
    ~XboxOneAuthDevice() {}
    XboxOneAuthDevice(proto_XboxOneAuthDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    void handle_auth(XGIPProtocol *packet);

    XboxOneAuth m_chip;
    proto_XboxOneAuthDevice m_device;
    bool m_registered = false;
    uint8_t m_req_command = 0;
    uint8_t m_req_sequence = 0;
    uint8_t m_req_ack = 0;
};
