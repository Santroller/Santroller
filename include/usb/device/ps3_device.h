
#pragma once
#include "usb/usb_descriptors.h"
#include "hid_device.h"

#include "tusb.h"

#define PS3_RUMBLE_ID 0x01
#define PS3_LED_ID 0x00
#define DJ_LED_ID 0x91
#define SANTROLLER_LED_ID 0x5A
#define SANTROLLER_LED_EXPANDED_ID 0x5B


class PS3GamepadDevice : public HIDDevice
{
public:
    PS3GamepadDevice(bool wiirb);
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);

private:
    bool m_wiirb;
    bool m_enabled = false;
    uint8_t m_pro_id = 4;
    uint8_t m_pg_id = 2;
};