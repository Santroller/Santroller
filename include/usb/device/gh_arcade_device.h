#pragma once

#include "common/tusb_common.h"
#include "device/usbd.h"
#include "device.hpp"
#include "hid_device.h"

#ifndef CFG_TUD_GH_ARCADE_EPSIZE
#define CFG_TUD_GH_ARCADE_EPSIZE 64
#endif
#define GH_ARCADE_DESC_TYPE_RESERVED 0x21
#define GH_ARCADE_SECURITY_DESC_TYPE_RESERVED 0x41
class GHArcadeVendorDevice : public UsbDevice
{
public:
    GHArcadeVendorDevice();
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    uint8_t m_epin1;
    uint8_t m_epout;
    uint8_t m_epin2;
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_GH_ARCADE_RX_BUFSIZE];
};

class GHArcadeGamepadDevice : public HIDDevice
{
public:
    GHArcadeGamepadDevice();
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