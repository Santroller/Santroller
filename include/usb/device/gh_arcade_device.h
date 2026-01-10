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
#ifdef __cplusplus
extern "C"
{
#endif
    void gh_arcaded_init(void);
    void gh_arcaded_reset(uint8_t rhport);
    uint16_t gh_arcaded_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                             uint16_t max_len);
    bool gh_arcaded_control_request(uint8_t rhport,
                                    tusb_control_request_t const *request);
    bool gh_arcaded_control_complete(uint8_t rhport,
                                     tusb_control_request_t const *request);
    bool gh_arcaded_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event,
                            uint32_t xferred_bytes);
    bool gh_arcaded_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
    void tud_gh_arcade_set_side_cb(uint8_t interface, uint8_t side);

#ifdef __cplusplus
}

#endif

class GHArcadeVendorDevice : public UsbDevice
{
public:
    GHArcadeVendorDevice();
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
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