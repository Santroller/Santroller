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

};