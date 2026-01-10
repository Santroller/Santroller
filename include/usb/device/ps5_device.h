
#pragma once
#include "usb/usb_descriptors.h"
#include "hid_device.h"

#include "tusb.h"

#define PS5_GAMEPAD 0
#define PS5_GUITAR 1
#define PS5_DRUMS 2
#define PS5_FIGHTSTICK 7 

class PS5GamepadDevice : public HIDDevice
{
public:
    PS5GamepadDevice();
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
};