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
    void initialize();
    void process();

private:
    uint8_t initialReport[CFG_TUD_XINPUT_TX_BUFSIZE];
    uint8_t lastReport[CFG_TUD_XINPUT_TX_BUFSIZE];
    uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
};