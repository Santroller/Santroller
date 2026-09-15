#pragma once
#include <stdint.h>
#include <memory>
#include "i2c.hpp"
#include "devices/base.hpp"
#include "device.pb.h"

#include "tusb_config.h"
#include "tusb.h"
#include "device.pb.h"
#include "instance.hpp"
#include <vector>
#include <memory>

class BTGamepadDevice : public Instance
{
public:
    virtual ~BTGamepadDevice();
    BTGamepadDevice();
    void initialize() override;
    void deinitialize() override;
    bool is_bluetooth() const override { return true; }
    void process(bool full_poll, bool send_events) override;

private:
    bool m_initialized = false;
    uint8_t m_initial_report[CFG_TUD_XINPUT_TX_BUFSIZE];
    uint8_t m_last_report[CFG_TUD_XINPUT_TX_BUFSIZE];
    uint8_t m_epin_buffer[CFG_TUD_XINPUT_TX_BUFSIZE];
};