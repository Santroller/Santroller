#pragma once

#include "common/tusb_common.h"
#include "device/usbd.h"
#include "protocols/og_xbox.hpp"
#include "device.hpp"

#ifndef CFG_TUD_OGXBOX_EPSIZE
#define CFG_TUD_OGXBOX_EPSIZE 64
#endif

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdXid;
    uint8_t bType;
    uint8_t bSubType;
    uint8_t bMaxInputReportSize;
    uint8_t bMaxOutputReportSize;
    uint16_t wAlternateProductIds[4];
} __attribute__((packed)) XID_DESCRIPTOR;



#ifdef __cplusplus
extern "C"
{
#endif
    void ogxboxd_init(void);
    void ogxboxd_reset(uint8_t rhport);

#ifdef __cplusplus
}
#endif

class OGXboxGamepadDevice : public UsbDevice
{
public:
    OGXboxGamepadDevice();
    void initialize();
    void process(bool full_poll);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    uint8_t m_epin;
    uint8_t m_epout;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
};