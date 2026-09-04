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
    // Double-buffered so the foreground loop can build a whole new frame in the
    // inactive buffer and publish it with a single atomic pointer swap, instead of
    // mutating the buffer the BT IRQ may be reading from mid-update.
    WiimoteReport m_report_buffers[2] = {};
    uint8_t m_active_buffer = 0;
    uint8_t m_extension_initial_report[32] = {};
    uint8_t m_extension_report[32] = {};
    uint8_t m_extension_size = 0;
    uint8_t m_extension_format = 1;
    uint8_t m_buttons_low_idx = 0;
    uint8_t m_buttons_high_idx = 0;
    bool m_initialized = false;
};
