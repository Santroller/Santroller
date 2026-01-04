#include "tusb_config.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "device/usbd_pvt.h"
#include "usb/usb_descriptors.h"

class UsbDevice
{
public:
    virtual void initialize() = 0;
    virtual void process() = 0;
    virtual OS_COMPATIBLE_SECTION get_wcid_descriptor() = 0;

    const usbd_class_driver_t * get_class_driver() { return &class_driver; }

    virtual uint8_t* config_descriptor() = 0;
    virtual uint8_t* device_descriptor() = 0;
protected:
    usbd_class_driver_t class_driver;
    uint8_t interface;
};