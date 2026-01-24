#pragma once
#include "tusb_config.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "device/usbd_pvt.h"
#include "usb/usb_descriptors.h"
#include "instance.hpp"

class UsbHost: public Instance
{
public:

    virtual uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len) = 0;
};