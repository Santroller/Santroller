#pragma once

#include <stdint.h>
#include "instance.hpp"
extern "C" {
#include "wiimote.h"
#include "wiimote_btstack.h"
}

class WiiRemoteEmulationDeviceInstance : public Instance
{
public:
    WiiRemoteEmulationDeviceInstance();
    ~WiiRemoteEmulationDeviceInstance();

    void initialize() override;
    void process(bool full_poll, bool send_events) override;

private:
    WiimoteReport m_report = {};
    uint8_t m_extension_report[32] = {};
    uint8_t m_extension_size = 0;
    uint8_t m_extension_format = 3;
    bool m_initialized = false;
};
